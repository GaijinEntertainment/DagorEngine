include "vertex_density_overlay_inc.sh"
include "shader_global.sh"

shader vertexDensityOverlay
{
  if (vertex_density_allowed == no)
  {
    dont_render;
  }

  cull_mode=none;
  z_write=false;
  z_test=false;
  blend_src = sa; blend_dst = isa;
  blend_asrc = zero; blend_adst = one;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 tc : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl(vs) {

    VsOutput main_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 inpos = getPostfxVertexPositionById(vertexId);

      output.pos = float4(inpos,1,1);
      output.tc = screen_to_texcoords(inpos);

      return output;
    }
  }

  VERTEX_DENSITY_INIT_PS()
  VERTEX_DENSITY_READ()

  hlsl(ps) {
    half4 main_ps(VsOutput input HW_USE_SCREEN_POS) : SV_Target0
    {
      half3 heatColor = readVertexDensityPos(input.tc);
      if (any(heatColor))
        return half4(heatColor, vertexDensityOverlayBlendFactor);
      //special case for zero density, show original color if blending is enabled, otherwise render black
      else if (vertexDensityOverlayBlendFactor < 1.0)
        return half4(0,0,0,0);
      else
        return half4(heatColor, 1);
    }
  }

  compile("target_vs", "main_vs");
  compile("target_ps", "main_ps");
}
