float3 get_reprojected_history_uvz1(float3 eye_to_point, float4x4 prev_viewproj_tm, out float2 screen_pos)
{
  float4 prevClip = mul(float4(eye_to_point, 1), prev_viewproj_tm);
  float3 spz = prevClip.w > 0 ? prevClip.xyz / prevClip.w : float3(2, 2, 0);
  screen_pos = spz.xy;
  return float3(screen_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5), linearize_z(spz.z, prev_zn_zfar.zw));
}
float3 get_reprojected_history_uvz1(float3 eye_to_point, float4x4 prev_viewproj_tm)
{
  float2 screenPosDummy;
  return get_reprojected_history_uvz1(eye_to_point, prev_viewproj_tm, screenPosDummy);
}
float3 get_reprojected_motion_vector1(float3 screen_uvz, float3 eye_to_point, float4x4 prev_viewproj_tm)
{
  float3 historyUvZ = get_reprojected_history_uvz1(eye_to_point, prev_viewproj_tm);
  return historyUvZ - screen_uvz;
}