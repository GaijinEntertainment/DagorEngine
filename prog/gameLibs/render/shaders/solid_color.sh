include "postfx_inc.sh"
include "shader_global.sh"

float4 solid_color;

shader solid_color_shader
{
  supports global_frame;
  supports none;

  cull_mode  = none;
  z_test = false;
  z_write = false;

  blend_src = sa; blend_dst = isa;

  POSTFX_VS(0)

  (ps)
  {
    solid_color@f4 = solid_color;
  }

  hlsl(ps)
  {
    float4 postfx_ps(VsOutput input) : SV_Target
    {
      return solid_color;
    }
  }

  compile("target_ps", "postfx_ps");
}