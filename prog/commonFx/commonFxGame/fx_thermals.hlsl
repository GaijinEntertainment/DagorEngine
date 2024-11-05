#define FX_TEMPERATURE float4(20.0/255.0, 100.0/255.0, 30.0/255.0, 500.0/2550.0)

#define MAX_THERMAL_VALUE_FOR_ALBEDO 0.5 // [0, MAX_THERMAL_VALUE_FOR_ALBEDO] will bind color to thermal color [MAX_THERMAL_VALUE_FOR_ALBEDO , 1] will bind to glow color

float4 fx_thermals_apply_additive( float4 c )
{
  c.g = FX_TEMPERATURE.w * 0.02f * dot(c.rgb, float3(1, 1, 1));
  c.rb = 0;
  return c;
}

float4 fx_thermals_apply_ablend( float4 c )
{
  c.r = lerp(dot(c.rgb, float3(0.3, 0.3, 0.3))*FX_TEMPERATURE.x,
                  dot(1-c.rgb, float3(0.3, 0.3, 0.3))*FX_TEMPERATURE.y,
                  pow3(c.a));
  c.gb = 0;
  return c;
}

float4 fx_thermals_apply_premult( float4 c )
{
  c.r = lerp(dot(c.rgb, float3(0.3, 0.3, 0.3))*FX_TEMPERATURE.z,
                  dot(1 - c.rgb, float3(0.3, 0.3, 0.3))*FX_TEMPERATURE.y,
                  pow3(c.a));
  c.gb = 0;
  return c;
}

float3 fx_thermals_apply( float value, float3 color, float3 emission, float3 normal, float alpha)
{
  float forColor = max(min(value, MAX_THERMAL_VALUE_FOR_ALBEDO), 0.0) * (1.0 /MAX_THERMAL_VALUE_FOR_ALBEDO);
  float forGlow = value < MAX_THERMAL_VALUE_FOR_ALBEDO ? 0.0 : (value -MAX_THERMAL_VALUE_FOR_ALBEDO) * (1.0 / (1.0-MAX_THERMAL_VALUE_FOR_ALBEDO));
  float3 frac1 = frac(color.rgb * 4.0);
  frac1 *= 1.0 - round(frac(color.rgb * 2.0));
  float3 frac2 = frac((1.0 - color.rgb) * 4.0);
  frac2 *= round(frac(color.rgb * 2.0));
  float finalColorFrac = dot(((frac1 + frac2) *0.5 + 0.5) * normal, float3(0.5, 0.2, 0.2));
  color.g = (dot(emission.rgb, 0.05)) * forGlow * pow2(alpha);
  color.r = finalColorFrac * forColor * pow2(alpha);
  color.b = 0;
  return color;
}
