#ifndef DAFX_MODFX_RENDER_FUNCS_HLSL
#define DAFX_MODFX_RENDER_FUNCS_HLSL

float dafx_linearize_z(float depth, GlobalData_cref gdata)
{
  return linearize_z(depth, gdata.zn_zfar.zw);
}

#ifdef DAFX_DEPTH_TEX
float dafx_sample_depth(uint2 tci, GlobalData_cref gdata)
{
#ifdef DAFX_DEPTH_FOR_COLLISION
  uint2 limit = uint2(gdata.depth_size_for_collision.xy);
#else
  uint2 limit = uint2(gdata.depth_size.xy);
#endif
  return texelFetch(DAFX_DEPTH_TEX, min(tci, limit - uint2(1, 1)), 0).r; // tc _must_ be clamped, otherwise it causes huge spike on NV 10XX series
}

float dafx_sample_linear_depth(uint2 tci, GlobalData_cref gdata)
{
  return dafx_linearize_z(dafx_sample_depth(tci, gdata), gdata);
}

half dafx_get_depth_base(float4 screen_tc, GlobalData gdata)
{
  float2 ftc = screen_tc.xy / screen_tc.w;
  return dafx_sample_linear_depth(ftc * gdata.depth_size.xy, gdata);
}

half dafx_get_soft_depth_mask(float4 tc, float softness_depth_rcp, GlobalData_cref gdata)
{
  float depth = dafx_get_depth_base(tc, gdata);
  float diff = depth - tc.w;
  half depthMask = saturate(softness_depth_rcp * diff);
#ifndef DAFXEX_DISABLE_NEARPLANE_FADE
  depthMask *= saturate(tc.w - tc.z);
#endif
#ifdef DAFXEX_CLOUD_MASK_ENABLED
  depthMask *= dafx_get_cloud_volume_mask(tc);
#endif
  return depthMask;
}

half dafx_get_hard_depth_mask(float4 tc, GlobalData gdata)
{
  float depth = dafx_get_depth_base(tc, gdata);
  half depthMask = (depth >= tc.w) ? 1. : 0.;
#ifdef DAFXEX_CLOUD_MASK_ENABLED
  depthMask *= dafx_get_cloud_volume_mask(tc);
#endif
  return depthMask;
}
#endif

#endif