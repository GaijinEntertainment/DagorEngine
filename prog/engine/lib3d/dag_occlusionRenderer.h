// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_occlusionTest.h>
#include <3d/dag_occlusionSystem.h>

#define OCCLUSION_OPTIMIZE_OUTDOOR_EARLYEXIT 1
// allows early exit in reprojection if 4 points are all too far (beyond zfar).
// this cost 10% (0.01msec) in fully indor and can save up to 75% time in fully outdoor. In typical outdoor case (sky is about 30% of
// scene), results in optimization 15%. the help is not clear. On one side, sky is very fast to render, so we optimize optimized case.
// On the other side, full occlusion is best scenario to occlude, so we optimize average case

template <int sizeX, int sizeY>
class OcclusionRenderer : public OcclusionTest<sizeX, sizeY>
{
private:
  static float zBufferReprojected[sizeX * sizeY];

public:
#if DAGOR_DBGLEVEL > 0
  static float reprojectionDebug[sizeX * sizeY];
#endif

  static float *getReprojected() { return zBufferReprojected; }

  OcclusionRenderer() {}

  ~OcclusionRenderer() {}

  static void clear()
  {
#if OCCLUSION_BUFFER == OCCLUSION_WBUFFER
    vec4f *dst = (vec4f *)zBufferReprojected;
    vec4f zfar = v_splats(1e6);
    for (int i = 0; i < sizeX * sizeY / 4; ++i, dst++)
      *dst = zfar;
#else
    memset(zBufferReprojected, 0, sizeX * sizeY * sizeof(zBufferReprojected[0]));
#endif
    OcclusionTest<sizeX, sizeY>::clear();
  }
  // toWorldHW
  // requires nonlinear depth buffer
  static void reprojectHWDepthBuffer(mat44f_cref toWorldHW, const mat44f &viewproj, int nStartLine, int nNumLines, float *hwSrc)
  {
    const float fWidth = (float)sizeX;
    const float fHeight = (float)sizeY;
    {
      const vec4f vXOffsets = v_make_vec4f(((0.0f + 0.5f) / fWidth) * 2.0f - 1.0f, ((1.0f + 0.5f) / fWidth) * 2.0f - 1.0f,
        ((2.0f + 0.5f) / fWidth) * 2.0f - 1.0f, ((3.0f + 0.5f) / fWidth) * 2.0f - 1.0f);
      const vec4f vXIncrement = v_splats(4.0f / fWidth * 2.0f);

      const vec4f *pSrcZ = (const vec4f *)hwSrc + (nStartLine * sizeX);
      float fY = -(float(nStartLine + 0.5f) / fHeight) * 2.0f + 1.0f;
      mat44f clipToScreen;
      clipToScreen.col0 = v_make_vec4f(float(sizeX / 2), 0, 0, 0);
      clipToScreen.col1 = v_make_vec4f(0, -float(sizeY / 2), 0, 0);
      clipToScreen.col2 = v_make_vec4f(0, 0, 1.0f, 0);
      clipToScreen.col3 = v_make_vec4f(float(sizeX / 2), float(sizeY / 2), 0.0f, 1.0f);
      mat44f worldToScreen;
      v_mat44_mul(worldToScreen, clipToScreen, viewproj);

      for (int y = nStartLine; y < nStartLine + nNumLines; y++, fY -= 1.0f / sizeY * 2.0f)
      {
        const vec4f vYYYY = v_splats(fY);

        vec4f vXCoords = vXOffsets;

        for (int x = 0; x < sizeX; x += 4)
        {
          const vec4f vNonLinearDepth = *pSrcZ;


          vec4f vXXXX[4];
          vXXXX[0] = v_splat_x(vXCoords);
          vXXXX[1] = v_splat_y(vXCoords);
          vXXXX[2] = v_splat_z(vXCoords);
          vXXXX[3] = v_splat_w(vXCoords);

          vec4f vZZZZ[4];
          vZZZZ[0] = v_splat_x(vNonLinearDepth);
          vZZZZ[1] = v_splat_y(vNonLinearDepth);
          vZZZZ[2] = v_splat_z(vNonLinearDepth);
          vZZZZ[3] = v_splat_w(vNonLinearDepth);

          for (int i = 0; i < 4; i++)
          {
            vec4f vWorldPos =
              v_madd(toWorldHW.col0, vXXXX[i], v_madd(toWorldHW.col1, vYYYY, v_madd(toWorldHW.col2, vZZZZ[i], toWorldHW.col3)));

            vec4f vWorldPosH = v_div(vWorldPos, v_splat_w(vWorldPos));

            vec4f vScreenPos = v_mat44_mul_vec3p(worldToScreen, vWorldPosH);

            vec4f vScreenPosH = v_div(vScreenPos, v_splat_w(vScreenPos));

            // float newDepth = v_extract_w(vScreenPos);
            float newDepth = v_extract_z(vScreenPosH);

            if (v_test_vec_x_gt(v_splat_w(v_min(vScreenPos, vWorldPos)), V_C_EPS_VAL))
            {
              int xy[2];
              v_stui_half(xy, v_cvt_vec4i(vScreenPosH));

              if (xy[0] >= 0 && xy[1] >= 0 && xy[0] < sizeX && xy[1] < sizeY)
              {
                float *pDstZ = &zBufferReprojected[xy[0] + (xy[1] * sizeX)];
                *pDstZ = max(*pDstZ, newDepth);
              }
            }
          }
          vXCoords = v_add(vXIncrement, vXCoords);
          pSrcZ++;
        }
      }
    }
  }
#define DEPTH_VMAX  occlusion_depth_vmax
#define DEPTH_VMIN  occlusion_depth_vmin
#define DEPTH_VTEST occlusion_depth_vtest
#define DEPTH_VCMP  occlusion_depth_vcmp

#if OCCLUSION_BUFFER == OCCLUSION_Z_BUFFER
#define DEPTH_MIN            max
#define DEPTH_MAX            min
#define DEPTH_VGE_SRCINF(a)  v_cmp_ge(zero, a)
#define DEPTH_VGE_PROCINF(a) v_cmp_ge(a, inf)
#error convert_to_internal_zbuffer not implemented yet, we need encoding params, such as zn/zf
#elif OCCLUSION_BUFFER == OCCLUSION_WBUFFER
#define DEPTH_MIN            min
#define DEPTH_MAX            max
#define DEPTH_VGE_SRCINF(a)  v_cmp_ge(a, srcinf)
#define DEPTH_VGE_PROCINF(a) v_cmp_ge(zero, a)
#elif OCCLUSION_BUFFER == OCCLUSION_INVWBUFFER
#define DEPTH_MIN            max
#define DEPTH_MAX            min
#define DEPTH_VGE_SRCINF(a)  v_cmp_ge(zero, a)
#define DEPTH_VGE_PROCINF(a) v_cmp_ge(a, inf)
#else
#error unselected occlusion type
#endif


  // todo: replace with cameraspace (view separated from proj)
  static void reprojectHWDepthBuffer(mat44f_cref toWorldHW, vec4f viewPos, float zn, float zf, const mat44f &viewproj, int nStartLine,
    int nNumLines, float *hwSrc, float cockpit_distance, CockpitReprojectionMode cockpitMode, const mat44f &cockpitAnim)
  {
    const float fHeight = (float)sizeY;
    {
      float zAt1 = zn * (zf - 1.f) / (zf - zn);
      // todo: pass viewLT vectors
      vec4f viewLT = v_mat44_mul_vec3p(toWorldHW, v_make_vec4f(-1, +1, zAt1, 1));
      viewLT = v_sub(v_div(viewLT, v_splat_w(viewLT)), viewPos);
      vec4f viewRT = v_mat44_mul_vec3p(toWorldHW, v_make_vec4f(+1, +1, zAt1, 1));
      viewRT = v_sub(v_div(viewRT, v_splat_w(viewRT)), viewPos);
      vec4f viewLB = v_mat44_mul_vec3p(toWorldHW, v_make_vec4f(-1, -1, zAt1, 1));
      viewLB = v_sub(v_div(viewLB, v_splat_w(viewLB)), viewPos);
      vec4f viewRB = v_mat44_mul_vec3p(toWorldHW, v_make_vec4f(+1, -1, zAt1, 1));
      viewRB = v_sub(v_div(viewRB, v_splat_w(viewRB)), viewPos);
      vec4i bounds = v_make_vec4i(sizeX - 1, sizeY - 1, 0, 0);
      vec4f znear = v_splats(zn);
      // debug("%f %f %f %f", v_extract_x(viewLT), v_extract_y(viewLT), v_extract_z(viewLT), v_extract_w(viewLT));
      // debug("%f %f %f", v_extract_x(viewRB), v_extract_y(viewRB), v_extract_z(viewRB));
      // vec4f viewPos = v_mat44_mul_vec3p(toWorldHW, v_make_vec4f(0, 0, 0, 1));
      // viewPos = v_div(viewPos, v_splat_w(viewPos))

      const vec4f *pSrcZ = (const vec4f *)hwSrc + (nStartLine * sizeX);
      float fY = (float(nStartLine + 0.5f) / fHeight);
      mat44f clipToScreen;
      clipToScreen.col0 = v_make_vec4f(float(sizeX / 2), 0, 0, 0);
      clipToScreen.col1 = v_make_vec4f(0, -float(sizeY / 2), 0, 0);
      clipToScreen.col2 = v_make_vec4f(0, 0, 1.0f, 0);
      clipToScreen.col3 = v_make_vec4f(float(sizeX / 2), float(sizeY / 2), 0.0f, 1.0f);
      mat44f worldToScreen;
      v_mat44_mul(worldToScreen, clipToScreen, viewproj);
      mat44f worldToScreenCockpit;
      v_mat44_mul43(worldToScreenCockpit, worldToScreen, cockpitAnim);
      const int widthBits = OcclusionTest<sizeX, sizeY>::bitShiftX;

      vec4f zfar = v_splats(zf * 0.9f);
      vec4f rcp_zf = v_splats(1 / zf);
      vec4f decode_depth = v_splats((zf - zn) / (zn * zf));

      vec4f viewPosX = v_splat_x(viewPos);
      vec4f viewPosY = v_splat_y(viewPos);
      vec4f viewPosZ = v_splat_z(viewPos);
      alignas(16) float lineZ[sizeX];
      alignas(16) int lineAddress[sizeX];
      alignas(16) int lineAddress2[sizeX];
      vec4i nonProjAddr = v_make_vec4i(nStartLine * sizeX, nStartLine * sizeX + 1, nStartLine * sizeX + 2, nStartLine * sizeX + 3);
      if (cockpitMode == COCKPIT_REPROJECT_OUT_OF_SCREEN)
        nonProjAddr = v_splatsi(sizeX * sizeY);
      vec4f cockpitDistance = v_splats(cockpit_distance);
#if OCCLUSION_BUFFER == OCCLUSION_Z_BUFFER
      const float occlusion_z_scale = 1. / 1.01f; //, occlusion_z_reprojection_scale = 1./1.04f;
#elif OCCLUSION_BUFFER == OCCLUSION_WBUFFER
      const float occlusion_z_scale = 1.01f; //, occlusion_z_reprojection_scale = 1.04f;
#elif OCCLUSION_BUFFER == OCCLUSION_INVWBUFFER
      const float occlusion_z_scale = 1. / 1.01f; //, occlusion_z_reprojection_scale = 1./1.0f;
#endif
      vec4f scale_result_z = v_splats(occlusion_z_scale);

      for (int y = nStartLine; y < nStartLine + nNumLines; y++, fY += 1.0f / sizeY)
      {
        vec4f scrY = v_splats(fY);
        vec4f scr1Y = v_splats(1.0f - fY);
        vec4f left = v_madd(viewLT, scr1Y, v_mul(viewLB, scrY)), right = v_madd(viewRT, scr1Y, v_mul(viewRB, scrY));
        vec4f viewVecInc = v_mul(v_sub(right, left), v_splats(0.5f / sizeX));

        vec4f viewVec = v_add(left, viewVecInc);
        viewVecInc = v_add(viewVecInc, viewVecInc);

        vec4f viewVec2 = v_add(viewVec, viewVecInc);
        vec4f viewVecX = v_perm_xaxa(viewVec, viewVec2);
        vec4f viewVecY = v_perm_xaxa(v_rot_1(viewVec), v_rot_1(viewVec2));
        vec4f viewVecZ = v_perm_xaxa(v_rot_2(viewVec), v_rot_2(viewVec2));

        viewVec2 = v_add(viewVec2, viewVecInc);
        viewVec = v_add(viewVec2, viewVecInc);
        viewVecX = v_perm_xyab(viewVecX, v_perm_xaxa(viewVec2, viewVec));
        viewVecY = v_perm_xyab(viewVecY, v_perm_xaxa(v_rot_1(viewVec2), v_rot_1(viewVec)));
        viewVecZ = v_perm_xyab(viewVecZ, v_perm_xaxa(v_rot_2(viewVec2), v_rot_2(viewVec)));
        vec4f viewVecInc4 = v_add(viewVecInc, viewVecInc);
        viewVecInc4 = v_add(viewVecInc4, viewVecInc4);
        vec4f *z = (vec4f *)lineZ;
        vec4i *address = (vec4i *)lineAddress;
        memset(address, 0xFF, sizeof(int) * sizeX);
        vec4i *address2 = (vec4i *)(lineAddress2);
        memset(address2, 0xFF, sizeof(int) * sizeX);
        for (int x = 0; x < sizeX; x += 4, address2++, address++, z++, pSrcZ++, viewVecX = v_add(viewVecX, v_splat_x(viewVecInc4)),
                 viewVecY = v_add(viewVecY, v_splat_y(viewVecInc4)), viewVecZ = v_add(viewVecZ, v_splat_z(viewVecInc4)),
                 nonProjAddr = v_addi(nonProjAddr, v_splatsi(4)))
        {
          const vec4f rawDepth = *pSrcZ;
          const vec4f isWithinCockpit = v_is_neg(rawDepth);
          const vec4f depth = v_rcp(v_madd(decode_depth, v_abs(rawDepth), rcp_zf));
#if DAGOR_DBGLEVEL > 0
          *(vec4f *)&reprojectionDebug[y * sizeX + x] = v_and(isWithinCockpit, v_splats(1)); // 0 or 1 when cockpit
#endif
          bool shouldReprojectAnimated = v_check_xyzw_any_true(isWithinCockpit) & (cockpitMode == COCKPIT_REPROJECT_ANIMATED);
          vec4f vWorldPosX = v_madd(viewVecX, depth, viewPosX);
          vec4f vWorldPosY = v_madd(viewVecY, depth, viewPosY);
          vec4f vWorldPosZ = v_madd(viewVecZ, depth, viewPosZ);

#define CALC_SCREEN_POS(component, tm)             \
  v_madd(v_splat_##component(tm.col0), vWorldPosX, \
    v_madd(v_splat_##component(tm.col1), vWorldPosY, v_madd(v_splat_##component(tm.col2), vWorldPosZ, v_splat_##component(tm.col3))))

          vec4f screenPosW = CALC_SCREEN_POS(w, worldToScreen);
          if (shouldReprojectAnimated)
          {
            vec4f screenPosWCockpit = CALC_SCREEN_POS(w, worldToScreenCockpit);
            screenPosW = v_sel(screenPosW, screenPosWCockpit, isWithinCockpit);
          }
          else
            screenPosW = v_sel(screenPosW, cockpitDistance, isWithinCockpit);

          vec4f notInBounds;
          notInBounds = v_cmp_gt(znear, screenPosW);                   // replace with eps?
          notInBounds = v_or(notInBounds, v_cmp_ge(screenPosW, zfar)); // replace with eps?
#if _TARGET_SIMD_SSE && OCCLUSION_OPTIMIZE_OUTDOOR_EARLYEXIT
          int movemask = v_signmask(notInBounds); // v_cmp_ge(depth, zfar));
          if (movemask == 15)                     // early exit
            continue;
#endif
          vec4f invW = v_rcp(screenPosW);

#if OCCLUSION_BUFFER == OCCLUSION_Z_BUFFER
          vec4f screenPosZ = CALC_SCREEN_POS(z, worldToScreen);
          if (shouldReprojectAnimated)
          {
            vec4f screenPosZCockpit = CALC_SCREEN_POS(z, worldToScreenCockpit);
            screenPosZ = v_sel(screenPosZ, screenPosZCockpit, isWithinCockpit);
          }
          screenPosZ = v_mul(screenPosZ, invW);
          *z = v_mul(screenPosZ, scale_result_z);
#elif OCCLUSION_BUFFER == OCCLUSION_WBUFFER
          *z = v_mul(screenPosW, scale_result_z);
#elif OCCLUSION_BUFFER == OCCLUSION_INVWBUFFER
          *z = v_mul(invW, scale_result_z);
#else
#error unselected occlusion type
#endif

          vec4f screenPosX = CALC_SCREEN_POS(x, worldToScreen);
          if (shouldReprojectAnimated)
          {
            vec4f screenPosXCockpit = CALC_SCREEN_POS(x, worldToScreenCockpit);
            screenPosX = v_sel(screenPosX, screenPosXCockpit, isWithinCockpit);
          }
          screenPosX = v_mul(screenPosX, invW);

          vec4f screenPosY = CALC_SCREEN_POS(y, worldToScreen);
          if (shouldReprojectAnimated)
          {
            vec4f screenPosYCockpit = CALC_SCREEN_POS(y, worldToScreenCockpit);
            screenPosY = v_sel(screenPosY, screenPosYCockpit, isWithinCockpit);
          }
          screenPosY = v_mul(screenPosY, invW);

          vec4i addrX = v_cvt_vec4i(screenPosX);
          vec4i addrY = v_cvt_vec4i(screenPosY);

          notInBounds =
            v_or(notInBounds, v_cast_vec4f(v_ori(v_cmp_gti(addrX, v_splat_xi(bounds)), v_cmp_gti(v_splat_zi(bounds), addrX))));
          notInBounds =
            v_or(notInBounds, v_cast_vec4f(v_ori(v_cmp_gti(addrY, v_splat_yi(bounds)), v_cmp_gti(v_splat_zi(bounds), addrY))));
          vec4i index = v_addi(v_slli(addrY, widthBits), addrX);
          *address = v_ori(v_cast_vec4i(notInBounds), index);
          if (cockpitMode != COCKPIT_REPROJECT_ANIMATED)
            *address = v_seli(*address, nonProjAddr, v_cast_vec4i(isWithinCockpit));

          vec4i addr2X = v_cvt_roundi(screenPosX);
          vec4i addr2Y = v_cvt_roundi(screenPosY);
          vec4i index2 = v_ori(v_slli(addr2Y, widthBits), addr2X);

          notInBounds =
            v_or(notInBounds, v_cast_vec4f(v_ori(v_cmp_gti(addr2X, v_splat_xi(bounds)), v_cmp_gti(v_splat_zi(bounds), addr2X))));
          notInBounds =
            v_or(notInBounds, v_cast_vec4f(v_ori(v_cmp_gti(addr2Y, v_splat_yi(bounds)), v_cmp_gti(v_splat_zi(bounds), addr2Y))));

          *address2 = v_ori(v_cast_vec4i(notInBounds), index2);
          if (cockpitMode != COCKPIT_REPROJECT_ANIMATED)
            *address2 = v_seli(*address2, nonProjAddr, v_cast_vec4i(isWithinCockpit));
            // #define DEBUG_ADDRESS(a) G_ASSERT(uint32_t(a)<sizeX*sizeY)

#undef CALC_SCREEN_POS
        }

        for (int i = 0; i < sizeX; ++i)
        {
          if ((uint32_t)lineAddress[i] >= (uint32_t)(sizeX * sizeY)) // Filter both notInBounds (0xFFFFFFFF) and NaNs (0x80000000).
            continue;
          uint32_t addr = lineAddress[i];
          float *pDstZ = &OcclusionTest<sizeX, sizeY>::zBuffer[addr];
          float z = lineZ[i];
          // G_ASSERTF(!check_nan(*pDstZ), "%f", *pDstZ);
          // G_ASSERTF(!check_nan(lineZ[i]), "%f", lineZ[i]);
          *pDstZ = DEPTH_MIN(*pDstZ, z);
          // unsigned addr2 = lineAddress2[i].x + (lineAddress2[i].y<<8);
          unsigned addr2 = lineAddress2[i];
          if (addr2 >= sizeX * sizeY || addr2 == lineAddress[i])
            continue;

          uint32_t addr3 = (addr2 & OcclusionTest<sizeX, sizeY>::bitMaskY) | (addr & OcclusionTest<sizeX, sizeY>::bitMaskX);
          uint32_t addr4 = (addr2 & OcclusionTest<sizeX, sizeY>::bitMaskX) | (addr & OcclusionTest<sizeX, sizeY>::bitMaskY);
          // it is actually better to rely on farthest possible 'other' reprojected pixels, than on just random one
          // however, it is significantly slower
          // inplace_min(zBufferReprojected[addr2], z);
          // inplace_min(zBufferReprojected[addr3], z);
          // inplace_min(zBufferReprojected[addr4], z);
          zBufferReprojected[addr2] = zBufferReprojected[addr3] = zBufferReprojected[addr4] = z; //*occlusion_z_reprojection_scale;
        }

#undef DEBUG_ADDRESS
      }
    }
    vec4f *zbuf2 = (vec4f *)OcclusionTest<sizeX, sizeY>::zBuffer, *zbuf1 = (vec4f *)zBufferReprojected;
    vec4f zero = v_zero();
    for (int i = 0; i < sizeX * sizeY / 4; ++i, zbuf1++, zbuf2++)
    {
      vec4f z1 = *zbuf1, z2 = *zbuf2;
      // z1 = v_sel(z1, zero, DEPTH_VGE_PROCINF(z1));
      *zbuf2 = v_sel(z2, z1, DEPTH_VGE_SRCINF(z2));
      //*zbuf2 = DEPTH_VMIN(*zbuf1, *zbuf2);
    }
  }

  /*#if _TARGET_SIMD_SSE
  void fixupScalar(float)//reference implementation
  {
    vec4f zero = v_zero();
    vec4f inf = v_splats(1e6f);
    float *dst = zBuffer;
    const float *src = zBufferReprojected;
    memscpy(dst, src, sizeX*sizeY*sizeof(zBufferReprojected[0]));

    vec4f *vSrc = (vec4f *)zBufferReprojected;
    carray<uint8_t, sizeY*sizeX/8> fixUpMask;
    uint8_t* mask = fixUpMask.data();
    for (int i = 0; i < sizeY*sizeX/8; i++, vSrc+=2, mask++)
    {
      vec4f src1 = vSrc[0], src2 = vSrc[1];
      vec4f mask1 = v_cmp_ge(zero, src1);
      vSrc[0] = v_sel(src1, inf, mask1);
      vec4f mask2 = v_cmp_ge(zero, src2);
      vSrc[1] = v_sel(src2, inf, mask2);
      *mask = v_signmask(mask1) | (v_signmask(mask2)<<4);
    }
    mask = fixUpMask.data();

    for (int y = 0; y < sizeY; y++)
    {
      int minYOfs = y ? sizeX : 0, maxYOfs = y < sizeY-1 ? sizeX : 0;
      for (int x8 = 0; x8 < sizeX; x8+=8, src+=8, dst+=8, mask++)
      {
        uint8_t mask1 = *mask;
        int xl = x8 ? 1 : 0;
        for (int x = 0; x < 8 && mask1; ++x, mask1>>=1)
        {
          if (mask1&1)
          {
            int xr = x+x8<sizeX-1 ? 1 : 0;
            float maxAround = min(min(min(min(src[x-xl], src[x+xr]),
                                      min(src[x-xl-minYOfs], src[x+xr-minYOfs])),
                                      min(src[x-xl+maxYOfs], src[x+xr+maxYOfs])),
                                      min(src[x-minYOfs], src[x+maxYOfs]));
            if (maxAround < 1e6f)
              dst[x] = maxAround;
          }
          xl = 1;
        }
      }
    }
  }
  #endif*/

  void fixup(float /*bias*/)
  {
    vec4f zero = v_zero();
#if OCCLUSION_BUFFER == OCCLUSION_WBUFFER
    vec4f inf = zero;
    vec4f srcinf = v_splats(1e6f);
#else
    vec4f inf = v_splats(1e6f);
    vec4f srcinf = zero;
#endif

    vec4f *pSrc = reinterpret_cast<vec4f *>(&zBufferReprojected);
    vec4f *pDst = reinterpret_cast<vec4f *>(&OcclusionTest<sizeX, sizeY>::zBuffer);
    const int pitchX = sizeX / 4;

    for (int y = 0; y < sizeY; y++)
    {
      int vecX = 0;

      int minY = max(0, y - 1);
      int maxY = min<int>(sizeY - 1, y + 1);
      int maxX = min(pitchX - 1, vecX + 1);

      vec4f src[3];
      vec4f srcMax[3];
      vec4f srcCenter;

      // left, no data available yet
      srcMax[0] = inf;
#define FIXUP_INF(a) a = v_sel(a, inf, DEPTH_VGE_SRCINF(a))
#define FIXUP_ALL_INF  \
  {                    \
    FIXUP_INF(src[0]); \
    FIXUP_INF(src[1]); \
    FIXUP_INF(src[2]); \
  }

      // center
      src[0] = pSrc[vecX + minY * pitchX];
      src[1] = pSrc[vecX + y * pitchX];
      src[2] = pSrc[vecX + maxY * pitchX];
      FIXUP_ALL_INF
      srcMax[1] = DEPTH_VMAX(DEPTH_VMAX(src[0], src[1]), src[2]);
      srcCenter = src[1];

      // right
      src[0] = pSrc[maxX + minY * pitchX];
      src[1] = pSrc[maxX + y * pitchX];
      src[2] = pSrc[maxX + maxY * pitchX];
      FIXUP_ALL_INF
      srcMax[2] = DEPTH_VMAX(DEPTH_VMAX(src[0], src[1]), src[2]);

      for (; vecX < pitchX;) // todo, fix edge cases
      {
        vec4f vSrcIsInf = DEPTH_VGE_PROCINF(srcCenter);
        if (v_signmask(vSrcIsInf) != 0)
        {
          vec4f vDst;
          // 0
          {
            vec4f vLeft, vCenter;
            vLeft = v_sel(inf, srcMax[0], v_cast_vec4f(V_CI_MASK0001));
            vCenter = v_sel(inf, srcMax[1], v_cast_vec4f(V_CI_MASK1100));

            vec4f _vMax;
            _vMax = DEPTH_VMAX(vLeft, vCenter);
            _vMax = DEPTH_VMAX(_vMax, v_rot_1(_vMax));
            _vMax = DEPTH_VMAX(_vMax, v_rot_2(_vMax));

            vDst = _vMax;
          }

          // 1
          {
            vec4f vCenter;

            vCenter = v_sel(inf, srcMax[1], v_cast_vec4f(V_CI_MASK1110));

            vec4f _vMax;
            _vMax = DEPTH_VMAX(vCenter, v_rot_1(vCenter));
            _vMax = DEPTH_VMAX(_vMax, v_rot_2(_vMax));

            vDst = v_sel(vDst, _vMax, v_cast_vec4f(V_CI_MASK0100));
          }

          // 2
          {
            vec4f vCenter;

            vCenter = v_sel(inf, srcMax[1], v_cast_vec4f(V_CI_MASK0111));

            vec4f _vMax;
            _vMax = DEPTH_VMAX(vCenter, v_rot_1(vCenter));
            _vMax = DEPTH_VMAX(_vMax, v_rot_2(_vMax));

            vDst = v_sel(vDst, _vMax, v_cast_vec4f(V_CI_MASK0010));
          }

          // 3
          {
            vec4f vRight, vCenter;

            vRight = v_sel(inf, srcMax[2], v_cast_vec4f(V_CI_MASK1000));
            vCenter = v_sel(inf, srcMax[1], v_cast_vec4f(V_CI_MASK0011));

            vec4f _vMax;
            _vMax = DEPTH_VMAX(vRight, vCenter);

            _vMax = DEPTH_VMAX(_vMax, v_rot_1(_vMax));
            _vMax = DEPTH_VMAX(_vMax, v_rot_2(_vMax));

            vDst = v_sel(vDst, _vMax, v_cast_vec4f(V_CI_MASK0001));
          }

          vec4f vDstIsInf = DEPTH_VGE_PROCINF(vDst);
          vDst = v_sel(vDst, srcinf, vDstIsInf);

          vDst = v_sel(srcCenter, vDst, vSrcIsInf);

          // vDst = v_add(vDst, vBiasAdd);  //linear bias
          // vDst = v_add(vDst, v_madd(vBiasMul, vDst, vBiasMul));// none-linear bias
          *pDst = vDst;
        }
        else
        {
          *pDst = srcCenter;
        }

        // next loop
        ++pDst;
        ++vecX;

        // shift to the left
        srcMax[0] = srcMax[1];
        srcMax[1] = srcMax[2];
        srcCenter = src[1];

        // load right data
        maxX = min(pitchX - 1, vecX + 1);
        src[0] = pSrc[maxX + minY * pitchX];
        src[1] = pSrc[maxX + y * pitchX];
        src[2] = pSrc[maxX + maxY * pitchX];
        FIXUP_ALL_INF
        srcMax[2] = DEPTH_VMAX(DEPTH_VMAX(src[0], src[1]), src[2]);
      }
    }
  }
  void fixupEmpty(float /*bias*/) {}

#undef DEPTH_VTEST
#undef DEPTH_VCMP
#undef DEPTH_VMAX
#undef DEPTH_VMIN
#undef DEPTH_MIN
#undef DEPTH_MAX
};

template <int sizeX, int sizeY>
alignas(32) float OcclusionRenderer<sizeX, sizeY>::zBufferReprojected[] = {};

#if DAGOR_DBGLEVEL > 0
template <int sizeX, int sizeY>
alignas(32) float OcclusionRenderer<sizeX, sizeY>::reprojectionDebug[] = {};
#endif
