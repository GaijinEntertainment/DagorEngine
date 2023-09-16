float4 reprojected_world_view_pos;

macro INIT_REPROJECTED_MOTION_VECTORS(code)
  (code) {
    reprojected_world_view_pos@f4 = reprojected_world_view_pos;
  }
endmacro

macro USE_REPROJECTED_MOTION_VECTORS(code)
  hlsl(code) {
    float2 get_reprojected_history_uv(float3 eye_to_point, float4x4 prev_viewproj_tm, out float2 screen_pos)
    {
      float4 reprojectedViewVec = mul(float4(eye_to_point, 0), prev_viewproj_tm);
      float4 prevClip = reprojected_world_view_pos + reprojectedViewVec;
      screen_pos = prevClip.w > 0 ? prevClip.xy * rcp(prevClip.w) : float2(2, 2);
      return float2(screen_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5));
    }
    float2 get_reprojected_history_uv(float3 eye_to_point, float4x4 prev_viewproj_tm)
    {
      float2 screenPosDummy;
      return get_reprojected_history_uv(eye_to_point, prev_viewproj_tm, screenPosDummy);
    }
    float2 get_reprojected_motion_vector(float2 screen_uv, float3 eye_to_point, float4x4 prev_viewproj_tm)
    {
      float2 historyUv = get_reprojected_history_uv(eye_to_point, prev_viewproj_tm);
      return historyUv - screen_uv;
    }
  }
endmacro