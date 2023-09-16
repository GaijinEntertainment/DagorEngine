include "postfx_inc.sh"
include "shader_global.sh"
include "tonemapHelpers/use_full_tonemap_lut_inc.sh"

float4 tonemap_render_size;
float4 tonemap_debug_color;
float tonemap_max_intensity;

shader debug_tonemap_overlay
{
  cull_mode  = none;
  z_test = false;
  z_write = false;

  blend_src = sa; blend_dst = isa;

  (vs)
  {
    tonemap_render_size@f4 = tonemap_render_size;
    tonemap_max_intensity@f1 = (tonemap_max_intensity);
  }

  hlsl
  {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl(vs)
  {
    VsOutput draw_tonemap(uint vertexId : SV_VertexID)
    {
      float2 pos = getPostfxVertexPositionById(vertexId);
      VsOutput output;
      output.pos = float4(pos.x, pos.y, 1, 1);
      output.pos.xy *= tonemap_render_size.xy;
      output.pos.xy += tonemap_render_size.zw;
      float2 tc = screen_to_texcoords(pos);
      tc.y = 1 - tc.y;
      output.texcoord = tc * tonemap_max_intensity;

      return output;
    }
  }

  FULL_TONEMAP_LUT_APPLY(ps)

  (ps)
  {
    tonemap_debug_color@f4 = tonemap_debug_color;
  }

  hlsl(ps)
  {
    #include "pixelPacking/ColorSpaceUtility.hlsl"
    float4 draw_tonemap(VsOutput input) : SV_Target
    {
      float3 sourceColor = tonemap_debug_color.rgb * input.texcoord.x;
      float3 mappedColor = accurateSRGBToLinear(performLUTTonemap(sourceColor));
      float distance = sqrt(length(mappedColor - tonemap_debug_color.rgb * input.texcoord.y));
      float resultIntensity = 1 - distance;
      float3 diff = abs(mappedColor - sourceColor);
      float maxComponent = max(max(max(diff.r, diff.g), diff.b), 0.001);
      diff /= maxComponent;
      return float4(diff * resultIntensity, 1);
    }
  }
  compile("target_vs", "draw_tonemap");
  compile("target_ps", "draw_tonemap");
}