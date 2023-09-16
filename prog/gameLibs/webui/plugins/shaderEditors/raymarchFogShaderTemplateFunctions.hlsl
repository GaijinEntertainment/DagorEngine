



// TODO: until node basd shaders have precompile support
float4 get_transformed_zn_zfar()
{
  return float4(zn_zfar.x, zn_zfar.y, 1.0 / zn_zfar.y, (zn_zfar.y-zn_zfar.x)/(zn_zfar.x * zn_zfar.y));
}
float3 get_base_ambient_color()
{
  return float3(enviSPH0.y + enviSPH0.w, enviSPH1.y + enviSPH1.w, enviSPH2.y + enviSPH2.w) * eclipse_params.y;
}

float3 calcViewVec(float2 screen_tc)
{
  return lerp_view_vec(screen_tc);
}

float2 getPrevTc(float3 world_pos, out float3 prev_clipSpace)
{
  // TODO: very ugly and subpotimal
  float4x4 prev_globtm_psf;
  prev_globtm_psf._m00_m10_m20_m30 = prev_globtm_psf_0;
  prev_globtm_psf._m01_m11_m21_m31 = prev_globtm_psf_1;
  prev_globtm_psf._m02_m12_m22_m32 = prev_globtm_psf_2;
  prev_globtm_psf._m03_m13_m23_m33 = prev_globtm_psf_3;
  float4 lastClipSpacePos = mul(float4(world_pos, 1), prev_globtm_psf);

  prev_clipSpace = lastClipSpacePos.w<=0.01 ? float3(2,2,2) : lastClipSpacePos.xyz/lastClipSpacePos.w;
  return prev_clipSpace.xy*float2(0.5,-0.5) + float2(0.5,0.5);
}

uint2 calc_raymarch_offset(uint2 raymarch_id)
{
  return calc_raymarch_offset(raymarch_id, fog_raymarch_frame_id);
}

uint2 calc_reconstruction_id(uint2 raymarch_id)
{
  return calc_reconstruction_id(raymarch_id, fog_raymarch_frame_id);
}

uint calc_raymarch_noise_index(uint2 dtId)
{
  return calc_raymarch_noise_index(dtId, (uint)jitter_ray_offset.w);
}

float4 get_media(float3 world_pos, float3 screenTcJittered, float wind_time)
{
  float3 depthAboveTcAndVignette = getDepthAboveTcAndVignette(world_pos); // TODO: not sure if needed
  float2 depthAboveTc = depthAboveTcAndVignette.xy;
  float depthAboveVignette = depthAboveTcAndVignette.z;

  MRTOutput result = (MRTOutput)0;

  //[[shader_code]]

  apply_volfog_modifiers(result.col0, screenTcJittered, world_pos);

#if DEBUG_DISTANT_FOG_MEDIA_EXTRA_MUL
  result.col0 *= volfog_media_fog_input_mul;
#endif

  return max(result.col0, 0); // (result.col0 < 1.0e10 && result.col0 >= 0.0f) ? result.col0 : 0; // TODO: maybe this is the cause of black spots
}
