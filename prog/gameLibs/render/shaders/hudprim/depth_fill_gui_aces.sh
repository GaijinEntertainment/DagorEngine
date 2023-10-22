include "shader_global.sh"
include "gui_aces_common.sh"

shader depth_fill_gui_aces
{
  supports gui_aces_object;

  cull_mode = none;
  z_write = true;
  color_write = false;
  no_ablend;
  USE_ATEST_255()

  channel float4 pos = pos;
  channel color8 vcol = vcol;
  channel float2 tc = tc;

  DECL_POSTFX_TC_VS_DEP()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord     : TEXCOORD0;
    };
  }

  hlsl(vs) {
    struct VsInput
    {
      float4 pos: POSITION;
      float2 texcoord : TEXCOORD0;
      float4 color    : COLOR0;
    };

    VsOutput depth_fill_gui_aces_vs(VsInput input)
    {
      VsOutput output;

      output.pos = input.pos;
      output.texcoord = input.texcoord;

      return output;
    }
  }

  hlsl(ps) {
    fixed4 depth_fill_gui_aces_ps(VsOutput input) : SV_Target
    {
      fixed4 result = h4tex2D(tex, input.texcoord);
      clip_alpha(result.a);
      return result;
    }
  }
  compile("target_vs", "depth_fill_gui_aces_vs");
  compile("target_ps", "depth_fill_gui_aces_ps");
}
