//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_adjpow2.h>
#include <util/dag_globDef.h>
#include <string.h>

static __forceinline vec4f occlusion_depth_vmax(vec4f a, vec4f b) { return v_min(a, b); }
static __forceinline vec4f occlusion_depth_vmin(vec4f a, vec4f b) { return v_max(a, b); }
static __forceinline vec4f occlusion_depth_vcmp(vec4f val, vec4f zbuffer) { return v_cmp_ge(val, zbuffer); }
static __forceinline int occlusion_depth_vtest(vec4f val, vec4f zbuffer) { return v_test_vec_x_ge(val, zbuffer); }
static __forceinline vec4f occlusion_convert_to_internal_zbuffer(vec4f minw) { return v_rcp_x(minw); }
static __forceinline vec4f occlusion_convert_from_internal_zbuffer(vec4f minw) { return v_rcp(minw); }

namespace occlusion_details
{
constexpr int mip_count(int w, int h) { return 1 + ((w > 1 && h > 1) ? mip_count(w >> 1, h >> 1) : 0); }
constexpr int mip_sum(int w, int h) { return w * h + ((w > 1 && h > 1) ? mip_sum(w >> 1, h >> 1) : 0); }
constexpr int get_log2(int w) { return 1 + (w > 2 ? get_log2(w >> 1) : 0); }
} // namespace occlusion_details

// NOTE: Occlusion uses inverse W-buffer (1/w) depth encoding
class OcclusionZBuffer
{
public:
  static constexpr int WIDTH = 256;
  static constexpr int HEIGHT = 128;
  static constexpr int mip_chain_count = occlusion_details::mip_count(WIDTH, HEIGHT);
  static constexpr int pitch_shift = occlusion_details::get_log2(WIDTH);

  float *getZbuffer(int mip) { return zBuffer + mip_chain_offsets[mip]; }
  const float *getZbuffer(const int mip) const { return zBuffer + mip_chain_offsets[mip]; }
  float *getZbuffer() { return zBuffer; }
  const float *getZbuffer() const { return zBuffer; }

  OcclusionZBuffer()
  {
    mip_chain_offsets[0] = 0;
    for (int mip = 1; mip < mip_chain_count; mip++)
      mip_chain_offsets[mip] = mip_chain_offsets[mip - 1] + (WIDTH >> (mip - 1)) * (HEIGHT >> (mip - 1));
  }
  void clear() { memset(zBuffer, 0, mip_chain_size * sizeof(zBuffer[0])); }
  void buildMips()
  {
    for (int mip = 1; mip < mip_chain_count; mip++)
      downsample4x_simda_max(getZbuffer(mip), getZbuffer(mip - 1), WIDTH >> mip, HEIGHT >> mip);
  }

  static constexpr int mip_chain_size = occlusion_details::mip_sum(WIDTH, HEIGHT);
  static constexpr unsigned bitShiftX = get_const_log2(WIDTH);
  static constexpr unsigned bitMaskX = (1 << bitShiftX) - 1;
  static constexpr unsigned bitMaskY = ~bitMaskX;

protected:
  void downsample4x_simda_max(float *__restrict destData, float *__restrict srcData, int destW, int destH)
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
  alignas(32) float zBuffer[mip_chain_size] = {};
  int mip_chain_offsets[mip_chain_count] = {};
};
