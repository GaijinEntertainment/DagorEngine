include "shader_global.sh"

float4 simple_outline_color;
texture simple_outline_color_rt;

shader simple_outline_final_render
{
  z_test = false;
  z_write = false;
  supports none;
  supports global_frame;
  cull_mode = none;
  blend_src = 1; blend_dst = isa;

  POSTFX_VS_TEXCOORD(1, texcoord)

  (ps) {
    simple_outline_color@f4 = simple_outline_color;
    color_rt@smp2d = simple_outline_color_rt;
  }

  hlsl(ps) {
    half4 simple_outline_final_render_ps(VsOutput input HW_USE_SCREEN_POS) : SV_Target
    {
      float4 pos = GET_SCREEN_POS(input.pos);
      int2 ipos = int2(pos.xy);

      float4 color = texelFetchOffset(color_rt, ipos, 0, int2(0, 0));
      if (color.r == 0.0)
      {
        if (texelFetchOffset(color_rt, ipos, 0, int2(-1, 0)).r != 0.0
          || texelFetchOffset(color_rt, ipos, 0, int2(1, 0)).r != 0.0
          || texelFetchOffset(color_rt, ipos, 0, int2(0, -1)).r != 0.0
          || texelFetchOffset(color_rt, ipos, 0, int2(0, 1)).r != 0.0)
        {
          return simple_outline_color;
        }
      }

      return float4(0.0, 0.0, 0.0, 0.0);
    }
  }
  compile("target_ps", "simple_outline_final_render_ps");
}
