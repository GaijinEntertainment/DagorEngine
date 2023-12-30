

float3 calcViewVec(float2 screen_tc)
{
  return lerp_view_vec(screen_tc);
}

float4 get_media_no_modifiers(float3 world_pos, float3 screenTcJittered, float wind_time)
{
  float3 depthAboveTcAndVignette = getDepthAboveTcAndVignette(world_pos);
  float2 depthAboveTc = depthAboveTcAndVignette.xy;
  float depthAboveVignette = depthAboveTcAndVignette.z;

  MRTOutput result = (MRTOutput)0;

  //[[shader_code]]

  return max(result.col0, 0);
}
