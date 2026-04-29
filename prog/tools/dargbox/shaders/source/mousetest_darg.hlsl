// Cursor-interactive shader for ROBJ_SHADER
// Built-in globals: darg_time, darg_opacity, darg_resolution, darg_cursor, darg_params0..3
// darg_cursor: xy = normalized cursor pos (0..1), -1 if outside

float4 darg_ps(float2 uv)
{
  float2 cpos = darg_cursor.xy;

  // Background: dark grid
  float2 grid = step(frac(uv * 10.0), 0.05);
  float3 bg = float3(0.08, 0.1, 0.12) + max(grid.x, grid.y) * 0.04;

  float3 col = bg;

  // Draw cursor glow when cursor is over the element
  if (cpos.x >= 0.0)
  {
    float dist = length(uv - cpos);
    float glow = 0.06 / (dist + 0.01);
    float3 glowColor = float3(0.2, 0.6, 1.0);
    col += glow * glowColor;

    // Pulsing ring at cursor
    float ring = abs(dist - 0.08 - sin(darg_time * 3.0) * 0.02);
    ring = smoothstep(0.005, 0.0, ring);
    col += ring * glowColor * 0.5;
  }

  return float4(col, darg_opacity);
}
