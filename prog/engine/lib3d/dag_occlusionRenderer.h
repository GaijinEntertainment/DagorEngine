// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_occlusionZBuffer.h>

#define OCCLUSION_OPTIMIZE_OUTDOOR_EARLYEXIT 1
// allows early exit in reprojection if 4 points are all too far (beyond zfar).
// this cost 10% (0.01msec) in fully indor and can save up to 75% time in fully outdoor. In typical outdoor case (sky is about 30% of
// scene), results in optimization 15%. the help is not clear. On one side, sky is very fast to render, so we optimize optimized case.
// On the other side, full occlusion is best scenario to occlude, so we optimize average case

class OcclusionRenderer
{
private:
  static vec4f computeViewCorner(mat44f_cref inv_vp, vec4f clip_pos, vec4f view_pos)
  {
    vec4f v = v_mat44_mul_vec3p(inv_vp, clip_pos);
    return v_sub(v_div(v, v_splat_w(v)), view_pos);
  }

  static void setupScanlineViewVecs(vec4f LT, vec4f RT, vec4f LB, vec4f RB, vec4f scr_y, vec4f scr1_y, vec4f &vec_x, vec4f &vec_y,
    vec4f &vec_z, vec4f &vec_inc4)
  {
    vec4f left = v_madd(LT, scr1_y, v_mul(LB, scr_y)), right = v_madd(RT, scr1_y, v_mul(RB, scr_y));
    vec4f inc = v_mul(v_sub(right, left), v_splats(0.5f / OcclusionZBuffer::WIDTH));
    vec4f v0 = v_add(left, inc);
    inc = v_add(inc, inc);
    vec4f v1 = v_add(v0, inc);
    vec_x = v_perm_xaxa(v0, v1);
    vec_y = v_perm_xaxa(v_rot_1(v0), v_rot_1(v1));
    vec_z = v_perm_xaxa(v_rot_2(v0), v_rot_2(v1));
    v1 = v_add(v1, inc);
    v0 = v_add(v1, inc);
    vec_x = v_perm_xyab(vec_x, v_perm_xaxa(v1, v0));
    vec_y = v_perm_xyab(vec_y, v_perm_xaxa(v_rot_1(v1), v_rot_1(v0)));
    vec_z = v_perm_xyab(vec_z, v_perm_xaxa(v_rot_2(v1), v_rot_2(v0)));
    vec_inc4 = v_add(inc, inc);
    vec_inc4 = v_add(vec_inc4, vec_inc4);
  }

  OcclusionZBuffer &occlusionZBuffer;
  alignas(32) float zBufferReprojected[OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT]; //-V730_NOINIT

public:
#if DAGOR_DBGLEVEL > 0
  alignas(32) float reprojectionDebug[OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT]; //-V730_NOINIT
#endif

  float *getReprojected() { return zBufferReprojected; }

  OcclusionRenderer(OcclusionZBuffer &occ_zbuf) : occlusionZBuffer(occ_zbuf) {}

  ~OcclusionRenderer() {}

  void clear()
  {
    memset(zBufferReprojected, 0, OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT * sizeof(zBufferReprojected[0]));
    occlusionZBuffer.clear();
  }
  // toWorldHW
  // requires nonlinear depth buffer
  void reprojectHWDepthBuffer(mat44f_cref toWorldHW, const mat44f &viewproj, int nStartLine, int nNumLines, float *hwSrc)
  {
    const float fWidth = (float)OcclusionZBuffer::WIDTH;
    const float fHeight = (float)OcclusionZBuffer::HEIGHT;
    {
      const vec4f vXOffsets = v_make_vec4f(((0.0f + 0.5f) / fWidth) * 2.0f - 1.0f, ((1.0f + 0.5f) / fWidth) * 2.0f - 1.0f,
        ((2.0f + 0.5f) / fWidth) * 2.0f - 1.0f, ((3.0f + 0.5f) / fWidth) * 2.0f - 1.0f);
      const vec4f vXIncrement = v_splats(4.0f / fWidth * 2.0f);

      const vec4f *pSrcZ = (const vec4f *)hwSrc + (nStartLine * OcclusionZBuffer::WIDTH);
      float fY = -(float(nStartLine + 0.5f) / fHeight) * 2.0f + 1.0f;
      mat44f clipToScreen;
      clipToScreen.col0 = v_make_vec4f(float(OcclusionZBuffer::WIDTH / 2), 0, 0, 0);
      clipToScreen.col1 = v_make_vec4f(0, -float(OcclusionZBuffer::HEIGHT / 2), 0, 0);
      clipToScreen.col2 = v_make_vec4f(0, 0, 1.0f, 0);
      clipToScreen.col3 = v_make_vec4f(float(OcclusionZBuffer::WIDTH / 2), float(OcclusionZBuffer::HEIGHT / 2), 0.0f, 1.0f);
      mat44f worldToScreen;
      v_mat44_mul(worldToScreen, clipToScreen, viewproj);

      for (int y = nStartLine; y < nStartLine + nNumLines; y++, fY -= 1.0f / OcclusionZBuffer::HEIGHT * 2.0f)
      {
        const vec4f vYYYY = v_splats(fY);

        vec4f vXCoords = vXOffsets;

        for (int x = 0; x < OcclusionZBuffer::WIDTH; x += 4)
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

              if (xy[0] >= 0 && xy[1] >= 0 && xy[0] < OcclusionZBuffer::WIDTH && xy[1] < OcclusionZBuffer::HEIGHT)
              {
                float *pDstZ = &zBufferReprojected[xy[0] + (xy[1] * OcclusionZBuffer::WIDTH)];
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

#define DEPTH_MIN            max
#define DEPTH_MAX            min
#define DEPTH_VGE_SRCINF(a)  v_cmp_ge(zero, a)
#define DEPTH_VGE_PROCINF(a) v_cmp_ge(a, inf)


  // todo: replace with cameraspace (view separated from proj)
  void reprojectHWDepthBuffer(mat44f_cref toWorldHW, mat44f_cref cockpitToWorldHW, vec4f viewPos, float zn, float zf,
    const mat44f &viewproj, int nStartLine, int nNumLines, float *hwSrc, float cockpit_distance, CockpitReprojectionMode cockpitMode,
    const mat44f &cockpitAnim, mat44f_cref cockpit_view_proj, bool has_separate_cockpit_proj)
  {
    const float fHeight = (float)OcclusionZBuffer::HEIGHT;
    {
      float zAt1 = zn * (zf - 1.f) / (zf - zn);
      vec4f viewLT = computeViewCorner(toWorldHW, v_make_vec4f(-1, +1, zAt1, 1), viewPos);
      vec4f viewRT = computeViewCorner(toWorldHW, v_make_vec4f(+1, +1, zAt1, 1), viewPos);
      vec4f viewLB = computeViewCorner(toWorldHW, v_make_vec4f(-1, -1, zAt1, 1), viewPos);
      vec4f viewRB = computeViewCorner(toWorldHW, v_make_vec4f(+1, -1, zAt1, 1), viewPos);
      const bool hasCockpitReproject = cockpitMode == COCKPIT_REPROJECT_ANIMATED;
      const bool hasSeparateCockpitReproject = hasCockpitReproject && has_separate_cockpit_proj;
      vec4f cpViewLT = v_zero(), cpViewRT = v_zero(), cpViewLB = v_zero(), cpViewRB = v_zero();
      if (hasSeparateCockpitReproject)
      {
        cpViewLT = computeViewCorner(cockpitToWorldHW, v_make_vec4f(-1, +1, zAt1, 1), viewPos);
        cpViewRT = computeViewCorner(cockpitToWorldHW, v_make_vec4f(+1, +1, zAt1, 1), viewPos);
        cpViewLB = computeViewCorner(cockpitToWorldHW, v_make_vec4f(-1, -1, zAt1, 1), viewPos);
        cpViewRB = computeViewCorner(cockpitToWorldHW, v_make_vec4f(+1, -1, zAt1, 1), viewPos);
      }
      vec4i bounds = v_make_vec4i(OcclusionZBuffer::WIDTH - 1, OcclusionZBuffer::HEIGHT - 1, 0, 0);
      vec4f znear = v_splats(zn);
      // debug("%f %f %f %f", v_extract_x(viewLT), v_extract_y(viewLT), v_extract_z(viewLT), v_extract_w(viewLT));
      // debug("%f %f %f", v_extract_x(viewRB), v_extract_y(viewRB), v_extract_z(viewRB));
      // vec4f viewPos = v_mat44_mul_vec3p(toWorldHW, v_make_vec4f(0, 0, 0, 1));
      // viewPos = v_div(viewPos, v_splat_w(viewPos))

      const vec4f *pSrcZ = (const vec4f *)hwSrc + (nStartLine * OcclusionZBuffer::WIDTH);
      float fY = (float(nStartLine + 0.5f) / fHeight);
      mat44f clipToScreen;
      clipToScreen.col0 = v_make_vec4f(float(OcclusionZBuffer::WIDTH / 2), 0, 0, 0);
      clipToScreen.col1 = v_make_vec4f(0, -float(OcclusionZBuffer::HEIGHT / 2), 0, 0);
      clipToScreen.col2 = v_make_vec4f(0, 0, 1.0f, 0);
      clipToScreen.col3 = v_make_vec4f(float(OcclusionZBuffer::WIDTH / 2), float(OcclusionZBuffer::HEIGHT / 2), 0.0f, 1.0f);
      mat44f worldToScreen;
      v_mat44_mul(worldToScreen, clipToScreen, viewproj);
      mat44f worldToScreenCockpit = {};
      if (hasCockpitReproject)
      {
        v_mat44_mul(worldToScreenCockpit, clipToScreen, cockpit_view_proj);
        v_mat44_mul43(worldToScreenCockpit, worldToScreenCockpit, cockpitAnim);
      }
      constexpr int widthBits = OcclusionZBuffer::bitShiftX;

      vec4f zfar = v_splats(zf * 0.9f);
      vec4f rcp_zf = v_splats(1 / zf);
      vec4f decode_depth = v_splats((zf - zn) / (zn * zf));

      vec4f viewPosX = v_splat_x(viewPos);
      vec4f viewPosY = v_splat_y(viewPos);
      vec4f viewPosZ = v_splat_z(viewPos);
      alignas(16) float lineZ[OcclusionZBuffer::WIDTH];
      alignas(16) int lineAddress[OcclusionZBuffer::WIDTH];
      alignas(16) int lineAddress2[OcclusionZBuffer::WIDTH];
      vec4i nonProjAddr = v_make_vec4i(nStartLine * OcclusionZBuffer::WIDTH, nStartLine * OcclusionZBuffer::WIDTH + 1,
        nStartLine * OcclusionZBuffer::WIDTH + 2, nStartLine * OcclusionZBuffer::WIDTH + 3);
      if (cockpitMode == COCKPIT_REPROJECT_OUT_OF_SCREEN)
        nonProjAddr = v_splatsi(OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT);
      vec4f cockpitDistance = v_splats(cockpit_distance);
      const float occlusion_z_scale = 1. / 1.01f;
      vec4f scale_result_z = v_splats(occlusion_z_scale);

      for (int y = nStartLine; y < nStartLine + nNumLines; y++, fY += 1.0f / OcclusionZBuffer::HEIGHT)
      {
        vec4f scrY = v_splats(fY);
        vec4f scr1Y = v_splats(1.0f - fY);

        vec4f viewVecX, viewVecY, viewVecZ, viewVecInc4;
        setupScanlineViewVecs(viewLT, viewRT, viewLB, viewRB, scrY, scr1Y, viewVecX, viewVecY, viewVecZ, viewVecInc4);

        vec4f cpViewVecX = v_zero(), cpViewVecY = v_zero(), cpViewVecZ = v_zero(), cpViewVecInc4 = v_zero();
        if (hasSeparateCockpitReproject)
          setupScanlineViewVecs(cpViewLT, cpViewRT, cpViewLB, cpViewRB, scrY, scr1Y, cpViewVecX, cpViewVecY, cpViewVecZ,
            cpViewVecInc4);

        vec4f *z = (vec4f *)lineZ;
        vec4i *address = (vec4i *)lineAddress;
        memset(address, 0xFF, sizeof(int) * OcclusionZBuffer::WIDTH);
        vec4i *address2 = (vec4i *)(lineAddress2);
        memset(address2, 0xFF, sizeof(int) * OcclusionZBuffer::WIDTH);
        for (int x = 0; x < OcclusionZBuffer::WIDTH; x += 4, address2++, address++, z++, pSrcZ++,
                 viewVecX = v_add(viewVecX, v_splat_x(viewVecInc4)), viewVecY = v_add(viewVecY, v_splat_y(viewVecInc4)),
                 viewVecZ = v_add(viewVecZ, v_splat_z(viewVecInc4)), nonProjAddr = v_addi(nonProjAddr, v_splatsi(4)))
        {
          const vec4f rawDepth = *pSrcZ;
          const vec4f isWithinCockpit = v_is_neg(rawDepth);
          const vec4f depth = v_rcp(v_madd(decode_depth, v_abs(rawDepth), rcp_zf));
#if DAGOR_DBGLEVEL > 0
          *(vec4f *)&reprojectionDebug[y * OcclusionZBuffer::WIDTH + x] = v_and(isWithinCockpit, v_splats(1)); // 0 or 1 when cockpit
#endif
          vec4f vWorldPosX = v_madd(viewVecX, depth, viewPosX);
          vec4f vWorldPosY = v_madd(viewVecY, depth, viewPosY);
          vec4f vWorldPosZ = v_madd(viewVecZ, depth, viewPosZ);
          bool shouldReprojectAnimated = v_check_xyzw_any_true(isWithinCockpit) & hasCockpitReproject;
          if (shouldReprojectAnimated && hasSeparateCockpitReproject)
          {
            vWorldPosX = v_sel(vWorldPosX, v_madd(cpViewVecX, depth, viewPosX), isWithinCockpit);
            vWorldPosY = v_sel(vWorldPosY, v_madd(cpViewVecY, depth, viewPosY), isWithinCockpit);
            vWorldPosZ = v_sel(vWorldPosZ, v_madd(cpViewVecZ, depth, viewPosZ), isWithinCockpit);
          }
          if (hasSeparateCockpitReproject)
          {
            cpViewVecX = v_add(cpViewVecX, v_splat_x(cpViewVecInc4));
            cpViewVecY = v_add(cpViewVecY, v_splat_y(cpViewVecInc4));
            cpViewVecZ = v_add(cpViewVecZ, v_splat_z(cpViewVecInc4));
          }

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
#if OCCLUSION_OPTIMIZE_OUTDOOR_EARLYEXIT
          if (v_check_xyzw_all_true(notInBounds)) // early exit
            continue;
#endif
          vec4f invW = v_rcp(screenPosW);

          *z = v_mul(invW, scale_result_z);

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
            // #define DEBUG_ADDRESS(a) G_ASSERT(uint32_t(a)<OcclusionZBuffer::WIDTH*OcclusionZBuffer::HEIGHT)

#undef CALC_SCREEN_POS
        }

        for (int i = 0; i < OcclusionZBuffer::WIDTH; ++i)
        {
          if ((uint32_t)lineAddress[i] >= (uint32_t)(OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT)) // Filter both notInBounds
                                                                                                          // (0xFFFFFFFF) and NaNs
                                                                                                          // (0x80000000).
            continue;
          uint32_t addr = lineAddress[i];
          float *pDstZ = &occlusionZBuffer.getZbuffer()[addr];
          float z = lineZ[i];
          // G_ASSERTF(!check_nan(*pDstZ), "%f", *pDstZ);
          // G_ASSERTF(!check_nan(lineZ[i]), "%f", lineZ[i]);
          *pDstZ = DEPTH_MIN(*pDstZ, z);
          // unsigned addr2 = lineAddress2[i].x + (lineAddress2[i].y<<8);
          unsigned addr2 = lineAddress2[i];
          if (addr2 >= OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT || addr2 == lineAddress[i])
            continue;

          uint32_t addr3 = (addr2 & OcclusionZBuffer::bitMaskY) | (addr & OcclusionZBuffer::bitMaskX);
          uint32_t addr4 = (addr2 & OcclusionZBuffer::bitMaskX) | (addr & OcclusionZBuffer::bitMaskY);
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
    vec4f *zbuf2 = (vec4f *)occlusionZBuffer.getZbuffer();
    vec4f *zbuf1 = (vec4f *)zBufferReprojected;
    vec4f zero = v_zero();
    for (int i = 0; i < OcclusionZBuffer::WIDTH * OcclusionZBuffer::HEIGHT / 4; ++i, zbuf1++, zbuf2++)
    {
      vec4f z1 = *zbuf1, z2 = *zbuf2;
      *zbuf2 = v_sel(z2, z1, DEPTH_VGE_SRCINF(z2));
    }
  }

  void fixup(float /*bias*/)
  {
    vec4f zero = v_zero();
    vec4f inf = v_splats(1e6f);
    vec4f srcinf = zero;

    vec4f *pSrc = reinterpret_cast<vec4f *>(&zBufferReprojected);
    vec4f *pDst = reinterpret_cast<vec4f *>(occlusionZBuffer.getZbuffer());
    const int pitchX = OcclusionZBuffer::WIDTH / 4;

    for (int y = 0; y < OcclusionZBuffer::HEIGHT; y++)
    {
      int vecX = 0;

      int minY = max(0, y - 1);
      int maxY = min<int>(OcclusionZBuffer::HEIGHT - 1, y + 1);
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
        if (v_check_xyzw_any_true(vSrcIsInf))
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
