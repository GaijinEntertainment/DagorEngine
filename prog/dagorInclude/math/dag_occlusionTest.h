//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_adjpow2.h>
#include <util/dag_globDef.h>

#define OCCLUSION_INVWBUFFER 1
#define OCCLUSION_WBUFFER    2
#define OCCLUSION_Z_BUFFER   3
#define OCCLUSION_BUFFER \
  OCCLUSION_INVWBUFFER // OCCLUSION_WBUFFER////OCCLUSION_WBUFFER////OCCLUSION_WBUFFER//OCCLUSION_INVWBUFFER//OCCLUSION_Z_BUFFER//
#if OCCLUSION_BUFFER == OCCLUSION_Z_BUFFER
static __forceinline vec4f occlusion_depth_vmax(vec4f a, vec4f b) { return v_min(a, b); }
static __forceinline vec4f occlusion_depth_vmin(vec4f a, vec4f b) { return v_max(a, b); }
static __forceinline vec4f occlusion_depth_vcmp(vec4f val, vec4f zbuffer) { return v_cmp_ge(val, zbuffer); }
static __forceinline int occlusion_depth_vtest(vec4f val, vec4f zbuffer) { return v_test_vec_x_ge(val, zbuffer); }
#elif OCCLUSION_BUFFER == OCCLUSION_WBUFFER
static __forceinline vec4f occlusion_depth_vmax(vec4f a, vec4f b) { return v_max(a, b); }
static __forceinline vec4f occlusion_depth_vmin(vec4f a, vec4f b) { return v_min(a, b); }
static __forceinline vec4f occlusion_depth_vcmp(vec4f val, vec4f zbuffer) { return v_cmp_ge(zbuffer, val); }
static __forceinline int occlusion_depth_vtest(vec4f val, vec4f zbuffer) { return v_test_vec_x_ge(zbuffer, val); }
static __forceinline vec4f occlusion_convert_to_internal_zbuffer(vec4f minw) { return minw; }
static __forceinline vec4f occlusion_convert_from_internal_zbuffer(vec4f minw) { return minw; }
#elif OCCLUSION_BUFFER == OCCLUSION_INVWBUFFER
static __forceinline vec4f occlusion_depth_vmax(vec4f a, vec4f b) { return v_min(a, b); }
static __forceinline vec4f occlusion_depth_vmin(vec4f a, vec4f b) { return v_max(a, b); }
static __forceinline vec4f occlusion_depth_vcmp(vec4f val, vec4f zbuffer) { return v_cmp_ge(val, zbuffer); }
static __forceinline int occlusion_depth_vtest(vec4f val, vec4f zbuffer) { return v_test_vec_x_ge(val, zbuffer); }
static __forceinline vec4f occlusion_convert_to_internal_zbuffer(vec4f minw) { return v_rcp_x(minw); }
static __forceinline vec4f occlusion_convert_from_internal_zbuffer(vec4f minw) { return v_rcp(minw); }
#endif

template <int sizeX, int sizeY>
class OcclusionTest
{
public:
  static constexpr int RESOLUTION_X = sizeX;
  static constexpr int RESOLUTION_Y = sizeY;
  static constexpr int mip_count(int w, int h) { return 1 + ((w > 1 && h > 1) ? mip_count(w >> 1, h >> 1) : 0); }
  static constexpr int mip_chain_count = mip_count(RESOLUTION_X, RESOLUTION_Y);

  static constexpr int get_log2(int w) { return 1 + (w > 2 ? get_log2(w >> 1) : 0); }
  static constexpr int pitch_shift = get_log2(RESOLUTION_X);

  static float *getZbuffer(int mip) { return zBuffer + mip_chain_offsets[mip]; }
  static float *getZbuffer() { return zBuffer; }

  OcclusionTest()
  {
    clipToScreenVec = v_make_vec4f(0.5 * sizeX, -0.5 * sizeY, 0.5 * sizeX, 0.5 * sizeY);
    screenMax = v_make_vec4f(sizeX - 1, sizeX - 1, sizeY - 1, sizeY - 1);
    mip_chain_offsets[0] = 0;
    for (int mip = 1; mip < mip_chain_count; mip++)
      mip_chain_offsets[mip] = mip_chain_offsets[mip - 1] + (sizeX >> (mip - 1)) * (sizeY >> (mip - 1));
  }
  static void clear()
  {
#if OCCLUSION_BUFFER == OCCLUSION_WBUFFER
    vec4f *dst = (vec4f *)zBuffer;
    vec4f zfar = v_splats(1e6);
    for (int i = 0; i < mip_chain_size / 4; ++i, dst++)
      *dst = zfar;
#else
    memset(zBuffer, 0, mip_chain_size * sizeof(zBuffer[0]));
#endif
  }
  static void buildMips()
  {
    for (int mip = 1; mip < mip_chain_count; mip++)
      downsample4x_simda_max(getZbuffer(mip), getZbuffer(mip - 1), sizeX >> mip, sizeY >> mip);
  }
  enum
  {
    CULL_FRUSTUM = 0,
    VISIBLE = 1,
    CULL_OCCLUSION = 2
  };
  // max_test_mip define coarsest mip to test. The bigger number, the rougher cull tests are (but each test is faster)
  // return 0 if frustum culled
  // return 1 if visible
  // return 2 if occlusion culled
  static __forceinline int VECTORCALL testVisibility(vec3f bmin, vec3f bmax, vec3f threshold, mat44f_cref clip, int max_test_mip)
  {
    vec4f minmax_w, clipScreenBox;
    // return v_screen_size_b(bmin, bmax, threshold, clipScreenBox, minmax_w, clip);
    int visibility = v_screen_size_b(bmin, bmax, threshold, clipScreenBox, minmax_w, clip);
    if (!visibility)
      return CULL_FRUSTUM;
    if (visibility == -1)
      return VISIBLE; // todo: better check using rasterization?
    vec4f screenBox = v_madd(clipScreenBox, v_perm_xxyy(clipToScreenVec), v_perm_zzww(clipToScreenVec));
    screenBox = v_max(screenBox, v_zero());    // on min part only
    screenBox = v_min(screenBox, screenMax);   // on max part only
    vec4i screenBoxi = v_cvt_vec4i(screenBox); // todo: ceil max xy

    alignas(16) int screenCrd[4];
    v_sti(screenCrd, screenBoxi);
    // debug("%d %d .. %d %d", screenCrd[0], screenCrd[1], screenCrd[3], screenCrd[2]);
    // return testCulledFull(screenCrd[0], screenCrd[1], screenCrd[3], screenCrd[2], minmax_w);

    int sz[2] = {screenCrd[1] - screenCrd[0], screenCrd[2] - screenCrd[3]};
    // G_ASSERT(sz[1]>=0 && sz[0]>=0);//should not ever happen!
    int mip = min(max(get_log2w(min(sz[0], sz[1])) - 1, 0), min(max_test_mip, mip_chain_count));
    return CULL_OCCLUSION - testCulledMip(screenCrd[0], screenCrd[1], screenCrd[3], screenCrd[2], mip, minmax_w);
  }

  static int testCulledFull(int xMin, int xMax, int yMin, int yMax, vec4f minw)
  {
    G_ASSERT(xMin >= 0 && yMin >= 0 && xMax < sizeX && yMax < sizeX);
    return testCulledZbuffer(xMin, xMax, yMin, yMax, minw, zBuffer, 0);
  }

  static __forceinline int testCulledMip(int xMin, int xMax, int yMin, int yMax, int mip, vec4f minw)
  {
    // G_ASSERT(xMin>=0 && yMin>=0 && xMax<sizeX && yMax<sizeX && mip < mip_chain_count-1);
    xMin >>= mip;
    yMin >>= mip;
    xMax = xMax >> mip;
    yMax = yMax >> mip;
    // debug("%d: sz - %dx%d %dx%d", mip, xMax-xMin+1, yMax-yMin+1, xMin, yMin);
    return testCulledZbuffer(xMin, xMax, yMin, yMax, minw, getZbuffer(mip), mip);
  }

protected:
  static constexpr int mip_sum(int w, int h) { return w * h + ((w > 1 && h > 1) ? mip_sum(w >> 1, h >> 1) : 0); }
  static constexpr int mip_chain_size = mip_sum(RESOLUTION_X, RESOLUTION_Y);
  static constexpr unsigned bitShiftX = get_const_log2(RESOLUTION_X);
  static constexpr unsigned bitMaskX = (1 << bitShiftX) - 1;
  static constexpr unsigned bitMaskY = ~bitMaskX;

  static void downsample4x_simda_max(float *__restrict destData, float *__restrict srcData, int destW, int destH)
  {
    unsigned int srcPitch = destW * 2;
    G_ASSERT(destW > 1); // we can implement last mip, if needed, later

    if (destW >= 4)
    {
      for (int y = 0; y < destH; y++, srcData += srcPitch)
      {
        for (int x = 0; x < destW; x += 4, srcData += 8, destData += 4)
        {
          vec4f up0 = v_ld(srcData);
          vec4f up1 = v_ld(srcData + 4);
          vec4f down0 = v_ld(srcData + srcPitch);
          vec4f down1 = v_ld(srcData + srcPitch + 4);
          vec4f left = occlusion_depth_vmax(up0, down0);
          vec4f right = occlusion_depth_vmax(up1, down1);
          left = occlusion_depth_vmax(left, v_perm_yzwx(left));
          right = occlusion_depth_vmax(right, v_perm_yzwx(right));
          v_st(destData, v_perm_xzac(left, right));
        }
      }
    }
    else
    {
      for (int y = 0; y < destH; y++, srcData += srcPitch)
      {
        for (int x = 0; x < destW; x += 2, srcData += 4, destData += 2)
        {
          vec4f up = v_ld(srcData);
          vec4f down = v_ld(srcData + srcPitch);
          vec4f left = occlusion_depth_vmax(up, down);
          left = occlusion_depth_vmax(left, v_perm_yzwx(left));
          v_stu_half(destData, v_perm_xzxz(left));
        }
      }
    }
  }
  static float zBuffer[mip_chain_size];
  static vec4f clipToScreenVec;
  static vec4f screenMax;
  static int mip_chain_offsets[mip_chain_count];

  // return 0 if occluded
  static __forceinline int testCulledZbuffer(int xMin, int xMax, int yMin, int yMax, vec4f minw, float *zbuffer, int mip)
  {
    const float *zbufferRow = zbuffer + (yMin << (pitch_shift - mip));
    const int pitch = (1 << (pitch_shift - mip));
    vec4f closestPoint = occlusion_convert_to_internal_zbuffer(minw);

    if (xMax - xMin <= 1) // very simple up to 2xN pixel test
    {
      zbufferRow += xMin;
      vec4f farDepth;
      farDepth = v_ldu_half(zbufferRow);
      zbufferRow += pitch;
      for (int y = yMin + 1; y <= yMax; ++y, zbufferRow += pitch)
        farDepth = occlusion_depth_vmax(farDepth, v_ldu_half(zbufferRow));

      if (xMax - xMin == 0)
        return occlusion_depth_vtest(closestPoint, farDepth);

      farDepth = occlusion_depth_vmax(farDepth, v_rot_1(farDepth));
      return occlusion_depth_vtest(closestPoint, farDepth);
    }
    closestPoint = v_splat_x(closestPoint);

    ///*
    {
      const int xEnd4 = xMin + ((xMax - xMin + 1) & (~3));
      const uint32_t endXMask = xEnd4 < xMax + 1 ? ((1 << ((xMax - xEnd4 + 1))) - 1) : 0;
      // debug("%dx%d %dx%d %f xEnd4=%d", xMin,yMin, xMax, yMax, v_extract_x(minw), xEnd4);
      zbufferRow += xMin;
      if (endXMask)
      {
        for (int y = yMin; y <= yMax; ++y, zbufferRow += pitch)
        {
          const float *zbufferP = (float *)zbufferRow;
          // int mask = 0;
          int x = xMin;
          for (; x < xEnd4; x += 4, zbufferP += 4)
          {
            vec4f passTest = occlusion_depth_vcmp(closestPoint, v_ldu(zbufferP));
            if (v_signmask(passTest))
            {
              // debug("entry: %dx%d 0x%X mask (%f %f %f %f)", x, y, _mm_movemask_ps(passTest), V4D(v_ldu(zbuffer)));
              return 1;
            }
          }
          vec4f passTest = occlusion_depth_vcmp(closestPoint, v_ldu(zbufferP));
          // mask |= _mm_movemask_ps(passTest)&endXMask;
          if (v_signmask(passTest) & endXMask)
          {
            // debug("end %dx%d mas=%d&endXMask = %x", x, y, _mm_movemask_ps(passTest), endXMask);
            return 1;
          }
          // if (mask)
          //   return 1;
        }
      }
      else
        for (int y = yMin; y <= yMax; ++y, zbufferRow += pitch)
        {
          const float *zbufferP = (float *)zbufferRow;
          // int mask = 0;
          for (int x = xMin; x < xEnd4; x += 4, zbufferP += 4)
            if (v_signmask(occlusion_depth_vcmp(closestPoint, v_ldu(zbufferP))))
              return 1;
        }
    }
    /*/
    //this one works slower (generally)
    zbufferRow = zbuffer + pitch*yMin;
    const int xMin4 = (xMin+3)&~3, xEnd4 = (xMax+1)&(~3);
    const uint32_t startXMask = 0xF&~( (1<<(4-(xMin4-xMin))) - 1);
    const uint32_t endXMask = xMax>=xEnd4 ? ((1<<((xMax-xEnd4+1)))-1) : 0;

    zbufferRow += (xMin&(~3));
    //debug("%dx%d %dx%d %f", xMin,yMin, xMax, yMax, v_extract_x(minw));
    for (int y = yMin; y <= yMax; ++y, zbufferRow+=pitch)
    {
      const vec4f *zbuffer = (vec4f*)zbufferRow;
      if (startXMask)
      {
        vec4f passTest = occlusion_depth_vcmp(closestPoint, *zbuffer);
        if (_mm_movemask_ps(passTest)&startXMask)
        {
          //debug("entry: %dx%d 0x%X &0x%X mask (%f %f %f %f)", xMin&(~3), y, _mm_movemask_ps(passTest), startXMask, V4D(*zbuffer));
          return 1;
        }
        zbuffer++;
      }
      for (int x = xMin4; x < xEnd4; x+=4, zbuffer++)
      {
        vec4f passTest = occlusion_depth_vcmp(closestPoint, *zbuffer);
        if (_mm_movemask_ps(passTest))
        {
          //debug("entry: %dx%d 0x%X mask (%f %f %f %f)", x, y, _mm_movemask_ps(passTest), V4D(*zbuffer));
          return 1;
        }
      }
      if (endXMask)
      {
        vec4f passTest = occlusion_depth_vcmp(closestPoint, *zbuffer);
        if (_mm_movemask_ps(passTest)&endXMask)
        {
          //debug("trail: %dx%d 0x%X &0x%X mask (%f %f %f %f)", xMax, y, _mm_movemask_ps(passTest), endXMask, V4D(*zbuffer));
          return 1;
        }
      }
    }
    //*/
    // debug(deb.str());
    // debug("%dx%d %dx%d %f xMin4=%d, xEnd4=%d, smask=0x%X emask=0x%X",
    //   xMin,yMin, xMax, yMax, v_extract_x(minw), xMin4, xEnd4, startXMask, endXMask);
    return 0;
  }
};

// default sizes
enum
{
  OCCLUSION_W = 256,
  OCCLUSION_H = 128
};

template <int sizeX, int sizeY>
alignas(32) float OcclusionTest<sizeX, sizeY>::zBuffer[] = {};

template <int sizeX, int sizeY>
vec4f OcclusionTest<sizeX, sizeY>::clipToScreenVec = {};

template <int sizeX, int sizeY>
vec4f OcclusionTest<sizeX, sizeY>::screenMax = {};

template <int sizeX, int sizeY>
int OcclusionTest<sizeX, sizeY>::mip_chain_offsets[] = {};
