include "hardware_defines.sh"
include "postfx_inc.sh"
include "shader_global.sh"
include "hdr/hdr_ps_output.sh"
include "hdr/hdr_render_defaults.sh"

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
    hdr_params@f4 = (paper_white_nits / 100, paper_white_nits / 10000, max(-0.5, 1 - hdr_shadows), 1 / (hdr_brightness + 0.01));
    hdr_source_offset@f2 = hdr_source_offset;
  }

  ENABLE_ASSERT(ps)
  POSTFX_VS(1)

  hlsl(ps) {

#include "pixelPacking/ColorSpaceUtility.hlsl"

    float3 to_linear(float3 srgbColor)
    {
      float3 linearColor = accurateSRGBToLinear(srgbColor);
      linearColor = pow(linearColor, hdr_params.w);

      const float shadows = 0.2;
      float lum = luminance(linearColor);
      linearColor *= lum < shadows ? pow(lum / shadows, hdr_params.z) : 1;
      return linearColor;
    }

    static const float3x3 from709to2020 =
    {
      { 0.6274040f, 0.3292820f, 0.0433136f },
      { 0.0690970f, 0.9195400f, 0.0113612f },
      { 0.0163916f, 0.0880132f, 0.8955950f }
    };

    float4 encode_hdr(float4 screenpos : VPOS) : SV_Target0
    {
##if shader == encode_hdr_from_linear
      float3 linearColor = texelFetch(hdr_fp_tex, screenpos.xy - hdr_source_offset, 0).xyz;
##else
      float3 srgbColor = texelFetch(hdr_fp_tex, screenpos.xy - hdr_source_offset, 0).xyz;
      float3 linearColor = to_linear(srgbColor);
##endif

##if hdr_ps_output_mode == hdr10_and_sdr || hdr_ps_output_mode == hdr10_only || shader == encode_hdr_to_int10
      float3 rec2020 = mul(from709to2020, linearColor);
      float3 normalizedLinearValue = rec2020 * hdr_params.y; // Normalize to max HDR nits.
      float3 hdrRresult = LinearToREC2084(normalizedLinearValue);
      return float4(hdrRresult, 1);
##else
      return float4(linearColor * hdr_params.x, 1);
##endif
    }
  }
  compile("target_ps", "encode_hdr");
}