macro INIT_PAINT_DETAILS()
  if (in_editor_assume == yes)
  {
    (vs){custom_color_paint@f3 = custom_color_paint;}
  }
endmacro

macro USE_PAINT_DETAILS()
  hlsl(vs) {
    float3 getColorMul(uint hashVal)
    {
      float correction = 4.59479341998814;
      ##if in_editor_assume == yes && use_custom_color_paint == yes
      float3 colorMul = custom_color_paint.xyz;
      colorMul = pow(colorMul, 2.2); //Need Gamma correction, because color from pallet in another color space
      ##else
      uint paint_palette_col = hashVal;
      uint2 dim;
      paint_details_tex.GetDimensions(dim.x, dim.y);
      dim.x >>= 1;
      uint paint_palette_row = get_paint_palette_row_index().x;
      uint palette_index = get_paint_palette_row_index().y;
      float3 colorMul = paint_details_tex[uint2(paint_palette_col%dim.x + dim.x * palette_index, paint_palette_row % dim.y)].rgb;
      ##endif
      colorMul *= correction;
      return colorMul;
    }
  }
endmacro
