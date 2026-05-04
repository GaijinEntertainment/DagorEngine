// Test pixel shader for ROBJ_SHADER: animated plasma effect
// User defines: float4 darg_ps(float2 uv)
// Built-in globals: darg_time, darg_opacity, darg_resolution, darg_cursor, darg_params0..3

float4 darg_ps(float2 uv)
{
  float t = darg_time;
  float2 p = uv * 6.0 - 3.0;

  float v = sin(p.x + t);
  v += sin((p.y + t) * 0.5);
  v += sin((p.x + p.y + t) * 0.5);
  v += sin(length(p) + t);
  v *= 0.5;

  float3 col;
  col.r = sin(v * 3.14159) * 0.5 + 0.5;
  col.g = sin(v * 3.14159 + 2.094) * 0.5 + 0.5;
  col.b = sin(v * 3.14159 + 4.189) * 0.5 + 0.5;

  // Mix with user color from params0 (if non-zero)
  float3 tint = darg_params0.rgb;
  if (dot(tint, tint) > 0.001)
    col = lerp(col, col * tint, 0.5);

  return float4(col, darg_opacity);
}
