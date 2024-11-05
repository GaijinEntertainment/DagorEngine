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
  tci += gdata.depth_tci_offset.xy;
  uint2 limit = uint2(gdata.depth_size.xy) + gdata.depth_tci_offset.xy;
#endif
  return texelFetch(DAFX_DEPTH_TEX, min(tci, limit - uint2(1, 1)), 0).r; // tc _must_ be clamped, otherwise it causes huge spike on NV 10XX series
}

float dafx_sample_linear_depth(uint2 tci, GlobalData_cref gdata)
{
  return dafx_linearize_z(dafx_sample_depth(tci, gdata), gdata);
}

half dafx_get_depth_base(float2 tc, GlobalData gdata)
{
  return dafx_sample_linear_depth(tc * gdata.depth_size.xy, gdata);
}

half dafx_get_soft_depth_mask(float4 tc, float4 cloud_tc, float softness_depth_rcp, GlobalData_cref gdata)
{
#ifdef DAFXEX_SOFT_DEPTH_MASK_USE_REPROJECTION
  float4 tcOld = mul(dafxex_uv_to_prev_frame_uv_reprojection_mat, float4(tc.x, tc.y, tc.z, 1.0));
  tcOld /= tcOld.w;
  float depth = dafx_get_depth_base(tcOld.xy, gdata);
  float diff = depth - linearize_z(tcOld.z, gdata.zn_zfar.zw);
  // This reprojection method is imperfect, especially when VFX is occluded by something opaque and camera is translated.
  // In such cases tcOld will be calculated with wrong depth (vfx instead of occluder), mask will be wrongly set to 0,
  // which will look like ghosting of opaque occluders where VFX is missing in the "ghosting" pixels.
  // To combat this (and other similar issues) we set the mask to 1.0 if we see that the VFX was _likely_
  // occluded by something in the previous frame.
  half depthMask = (diff < -0.1) ? 1.0 : saturate(softness_depth_rcp * diff);
#else
  float depth = dafx_get_depth_base(tc.xy, gdata);
  float diff = depth - tc.w;
  half depthMask = saturate(softness_depth_rcp * diff);
#endif
#ifndef DAFXEX_DISABLE_NEARPLANE_FADE
  depthMask *= saturate(tc.w - tc.w*tc.z);
#endif
#ifdef DAFXEX_CLOUD_MASK_ENABLED
  depthMask *= dafx_get_screen_cloud_volume_mask(cloud_tc.xy, cloud_tc.w);
#endif
  return depthMask;
}

half dafx_get_hard_depth_mask(float4 tc, GlobalData gdata)
{
  float depth = dafx_get_depth_base(tc.xy, gdata);
  half depthMask = (depth >= tc.w) ? 1. : 0.;
#ifdef DAFXEX_CLOUD_MASK_ENABLED
  depthMask *= dafx_get_screen_cloud_volume_mask(tc.xy, tc.w);
#endif
  return depthMask;
}
#endif

#endif