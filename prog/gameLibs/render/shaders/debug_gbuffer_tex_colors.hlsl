#ifndef DEBUG_GBUFFER_TEX_COLORS
#define DEBUG_GBUFFER_TEX_COLORS 1
half4 texel_density_overlay_color(float debug_tex_value, half3 albedo)
{
  float texDensity = debug_tex_value * (MAX_TEXEL_DENSITY_DISPLAYED + 1);
  half3 texDensityColor;
  if (texDensity > 9.5)
  {
    texDensityColor = half3(1, 1, 0);
  }
  else if (texDensity > 1)
  {
    texDensityColor = lerp(half3(1,1,1), half3(1,0,0), (texDensity - 1.0) / 5.0);
  }
  else if(texDensity > 0.1)
  {
    texDensityColor = lerp(half3(0,0,1), half3(1,1,1), texDensity);
  }
  else
  {
    texDensityColor = half3(1, 0, 1);
  }
  return half4(lerp(texDensityColor, albedo, texDensity <= MAX_TEXEL_DENSITY_DISPLAYED + 0.99 ? 0.2 : 1), 1);
}

half4 mip_overlay_color(float debug_tex_value, half3 albedo)
{
  static const half3 DBG_MIP_COLOR[MAX_DEBUG_MIP_LEVELS_DISPLAYED + 1] = {
    half3(1,1,1),      // 0 : White
    half3(1,0,0),      // 1 : Red
    half3(1, 0.5, 0),  // 2 : Orange
    half3(1, 1, 0),    // 3 : Yellow
    half3(0.2, 1, 0),  // 4 : Neon Green
    half3(0, 0.5, 0),  // 5 : Dark Green
    half3(0, 0.5, 0.5),// 6 : Dark Cyan
    half3(0, 1, 1),    // 7 : Bright Cyan
    half3(0, 0, 1),    // 8 : Bright Blue
    half3(0, 0, 0.5),  // 9 : Dark Blue
    half3(0.5, 0, 0.5),//10 : Purple
    half3(1, 0, 1),    //11+: Magenta
  };

  float mip_level = debug_tex_value * (MAX_DEBUG_MIP_LEVELS_DISPLAYED + 1);

  int idxFloor = clamp((int)floor(mip_level), 0, MAX_DEBUG_MIP_LEVELS_DISPLAYED);
  int idxCeil = clamp((int)ceil(mip_level), 0, MAX_DEBUG_MIP_LEVELS_DISPLAYED);

  half3 mipColor = lerp(DBG_MIP_COLOR[idxFloor], DBG_MIP_COLOR[idxCeil], frac(mip_level));

  return half4(lerp(mipColor, albedo, mip_level <= MAX_DEBUG_MIP_LEVELS_DISPLAYED ? 0.2 : 1.0), 1);
}

#endif