// Animated radial gradient for ROBJ_SHADER
// Built-in globals: darg_time, darg_opacity, darg_resolution, darg_cursor, darg_params0..3

float4 darg_ps(float2 uv)
{
  float t = darg_time;

  // Pulsing radial gradient
  float2 center = float2(0.5, 0.5);
  float dist = length(uv - center) * 2.0;
  float pulse = sin(t * 2.0) * 0.1 + 0.9;
  float ring = frac(dist * 3.0 - t * 0.5);
  ring = smoothstep(0.0, 0.05, ring) * smoothstep(0.5, 0.45, ring);

  float3 col1 = float3(0.1, 0.4, 0.8);
  float3 col2 = float3(0.9, 0.3, 0.1);
  float3 bg = lerp(col1, col2, dist * pulse);

  float3 col = bg + ring * 0.4;
  float alpha = saturate(1.0 - dist * 0.8) * darg_opacity;

  return float4(col, alpha);
}
