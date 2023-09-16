include "postfx_inc.sh"
include "shader_global.sh"
include "rgbm_inc.sh"

texture rgb_to_rgbm_src_tex;
float4 rgbm_stretch_rect = (0, 0, 0, 0);

shader rgb_to_rgbm
{
  cull_mode=none;
  z_write=false;
  z_test=false;

  (vs) {
    rgbm_stretch_rect@f4 = (1/rgbm_stretch_rect.x, 1/rgbm_stretch_rect.y, rgbm_stretch_rect.z, rgbm_stretch_rect.w);
  }

  (ps) {
    srcTex@smp2d = rgb_to_rgbm_src_tex;
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord        : TEXCOORD0;
    };
  }
  USE_POSTFX_VERTEX_POSITIONS()
  USE_DEF_RGBM_SH()
  hlsl(vs) {
    VsOutput rgb_converter_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.x, pos.y, 0, 1);
      output.texcoord = pos*RT_SCALE_HALF+float2(0.5, 0.5);
      output.texcoord *= rgbm_stretch_rect.xy;
      output.texcoord += rgbm_stretch_rect.zw;
      return output;
    }
  }

  hlsl(ps) {
  float4 convert_to_rgbm(float2 texcoord)
  {
    return encode_rgbm(tex2Dlod(srcTex, float4(texcoord,0,0)).rgb, rgbm_conversion_scale);
  }

  float4 rgb_converter_ps(VsOutput IN): SV_Target
  {
    return convert_to_rgbm(IN.texcoord);
  }
  }
  compile("target_vs", "rgb_converter_vs");
  compile("target_ps", "rgb_converter_ps");
}