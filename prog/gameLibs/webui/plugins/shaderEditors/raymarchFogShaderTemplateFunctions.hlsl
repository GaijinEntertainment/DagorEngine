



// TODO: until node basd shaders have precompile support
float3 get_base_ambient_color()
{
  return float3(enviSPH0.y + enviSPH0.w, enviSPH1.y + enviSPH1.w, enviSPH2.y + enviSPH2.w) * skylight_params.y;
}

float3 calcViewVec(float2 screen_tc)
{
  return lerp_view_vec(screen_tc);
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
  MRTOutput result = (MRTOutput)0;

  //[[shader_code]]

  apply_volfog_modifiers(result.col0, screenTcJittered, world_pos);

#if DEBUG_DISTANT_FOG_MEDIA_EXTRA_MUL
  result.col0 *= volfog_media_fog_input_mul;
#endif

  return max(result.col0, 0); // (result.col0 < 1.0e10 && result.col0 >= 0.0f) ? result.col0 : 0; // TODO: maybe this is the cause of black spots
}
