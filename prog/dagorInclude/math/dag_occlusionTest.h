//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_occlusionZBuffer.h>

class OcclusionTest
{
public:
  enum
  {
    CULL_FRUSTUM = 0,
    VISIBLE = 1,
    CULL_OCCLUSION = 2
  };

  OcclusionTest(const OcclusionTest &) = delete;
  OcclusionTest &operator=(const OcclusionTest &) = delete;

  OcclusionTest(const OcclusionZBuffer &zb) : zBuf(zb)
  {
    clipToScreenVec = v_make_vec4f(0.5 * OcclusionZBuffer::WIDTH, -0.5 * OcclusionZBuffer::HEIGHT, 0.5 * OcclusionZBuffer::WIDTH,
      0.5 * OcclusionZBuffer::HEIGHT);
    screenMax = v_make_vec4f(OcclusionZBuffer::WIDTH - 1, OcclusionZBuffer::WIDTH - 1, OcclusionZBuffer::HEIGHT - 1,
      OcclusionZBuffer::HEIGHT - 1);
  }

  // max_test_mip define coarsest mip to test. The bigger number, the rougher cull tests are (but each test is faster)
  // return 0 if frustum culled
  // return 1 if visible
  // return 2 if occlusion culled
  VECTORCALL int testVisibility(vec3f bmin, vec3f bmax, vec3f threshold, mat44f_cref clip, int max_test_mip) const
  {
    vec4f minmax_w, clipScreenBox;
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

    int sz[2] = {screenCrd[1] - screenCrd[0], screenCrd[2] - screenCrd[3]};
    int mip = min(max(get_log2w(min(sz[0], sz[1])) - 1, 0), min(max_test_mip, OcclusionZBuffer::mip_chain_count));
    return CULL_OCCLUSION - testCulledMip(screenCrd[0], screenCrd[1], screenCrd[3], screenCrd[2], mip, minmax_w);
  }

  __forceinline int testCulledMip(int xMin, int xMax, int yMin, int yMax, int mip, vec4f minw) const
  {
    xMin >>= mip;
    yMin >>= mip;
    xMax = xMax >> mip;
    yMax = yMax >> mip;
    return testCulledZbuffer(xMin, xMax, yMin, yMax, minw, zBuf.getZbuffer(mip), mip);
  }

protected:
  const OcclusionZBuffer &zBuf;
  vec4f clipToScreenVec = {};
  vec4f screenMax = {};

  // return 0 if occluded
  __forceinline int testCulledZbuffer(int xMin, int xMax, int yMin, int yMax, vec4f minw, const float *zbuffer, int mip) const
  {
    const float *zbufferRow = zbuffer + (yMin << (OcclusionZBuffer::pitch_shift - mip));
    const int pitch = (1 << (OcclusionZBuffer::pitch_shift - mip));
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

    {
      const int xEnd4 = xMin + ((xMax - xMin + 1) & (~3));
      const uint32_t endXMask = xEnd4 < xMax + 1 ? ((1 << ((xMax - xEnd4 + 1))) - 1) : 0;
      zbufferRow += xMin;
      if (endXMask)
      {
        for (int y = yMin; y <= yMax; ++y, zbufferRow += pitch)
        {
          const float *zbufferP = (float *)zbufferRow;
          int x = xMin;
          for (; x < xEnd4; x += 4, zbufferP += 4)
          {
            vec4f passTest = occlusion_depth_vcmp(closestPoint, v_ldu(zbufferP));
            if (v_check_xyzw_any_true(passTest))
              return 1;
          }
          vec4f passTest = occlusion_depth_vcmp(closestPoint, v_ldu(zbufferP));
          if (v_signmask(passTest) & endXMask)
            return 1;
        }
      }
      else
        for (int y = yMin; y <= yMax; ++y, zbufferRow += pitch)
        {
          const float *zbufferP = (float *)zbufferRow;
          for (int x = xMin; x < xEnd4; x += 4, zbufferP += 4)
            if (v_check_xyzw_any_true(occlusion_depth_vcmp(closestPoint, v_ldu(zbufferP))))
              return 1;
        }
    }
    return 0;
  }
};
