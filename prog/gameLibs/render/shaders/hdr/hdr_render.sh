include "hardware_defines.sh"
include "postfx_inc.sh"
include "shader_global.sh"
include "hdr/hdr_ps_output.sh"
include "hdr/hdr_render_defaults.sh"
include "hdr/hdr_render_logic.sh"

texture hdr_fp_tex;
float4 hdr_source_offset = (0, 0, 0, 0);

shader encode_hdr_to_int10, encode_hdr_to_float, encode_hdr_from_linear
{
  cull_mode = none;
  z_test = false;
  z_write = false;
  no_ablend;

  (ps) {
    hdr_fp_tex@tex2d = hdr_fp_tex;
    hdr_source_offset@f2 = hdr_source_offset;
  }

  ENABLE_ASSERT(ps)
  POSTFX_VS(1)

  HDR_LOGIC(ps)

  hlsl(ps) {
    float4 encode_hdr(float4 screenpos : VPOS) : SV_Target0
    {
      ##if shader == encode_hdr_from_linear
        float3 linearColor = texelFetch(hdr_fp_tex, screenpos.xy - hdr_source_offset, 0).xyz;
      ##else
        float3 srgbColor = texelFetch(hdr_fp_tex, screenpos.xy - hdr_source_offset, 0).xyz;
        float3 linearColor = to_linear(srgbColor);
      ##endif

      ##if hdr_ps_output_mode == hdr10_and_sdr || hdr_ps_output_mode == hdr10_only || shader == encode_hdr_to_int10
        bool hdr10 = true;
      ##else
        bool hdr10 = false;
      ##endif

      return float4(encode_hdr_value(linearColor, hdr10), 1);
    }
  }
  compile("target_ps", "encode_hdr");
}