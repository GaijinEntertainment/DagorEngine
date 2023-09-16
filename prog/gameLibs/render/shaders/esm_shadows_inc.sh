float4 esm_params = (1, 1, 20, 1048576 /*2^20*/);

macro INIE_ESM_RENDER_DEPTH_PS()
  (ps) { esm_depth_encode_params@f3 = (esm_params.z, esm_params.w, 1.0 / (esm_params.w - 1.0), 0); }
endmacro

macro ESM_RENDER_DEPTH_PS()
  hlsl(ps) {
    float2 esm_render_depth_ps(float4 screen_pos : VPOS) : SV_Target
    {
      float esmDepth = esm_depth_encode_params.x * (1.0 - screen_pos.z);
      float2 esmDepthExp = exp2(float2(esmDepth, -esmDepth));
      float expKExp = esm_depth_encode_params.y;
      return float2(esmDepthExp.x - 1.0, expKExp * esmDepthExp.y - 1.0) * esm_depth_encode_params.z;
    }
  }
  compile("target_ps", "esm_render_depth_ps");
endmacro