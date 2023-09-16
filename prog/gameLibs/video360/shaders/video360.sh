include "shader_global.sh"
include "viewVecVS.sh"

texture video360_screen_env;

shader copy_texture
{
  supports global_frame;

  z_write=false;
  z_test=false;
  cull_mode  = none;

  (ps) { source_tex@smp2d = video360_screen_env; }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 uv: TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl(vs) {
    VsOutput main_vs_copy(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.xy, 0.0, 1.0);
      output.uv = (pos.xy * 0.5 + 0.5);
      return output;
    }
  }

  hlsl(ps) {
    void main_ps_copy(VsOutput input, out half4 res0 : SV_Target0 )
    {
        res0 = tex2D(source_tex, input.uv);
    }
  }

  compile("target_vs", "main_vs_copy");
  compile("target_ps", "main_ps_copy");
}

shader cubemap_to_spherical_projection
{
  supports global_frame;

  blend_src = sa; blend_dst = isa;
  cull_mode  = none;
  z_write=false;
  z_test=false;

  (ps) { source_tex@smpCube = video360_screen_env; }

  USE_AND_INIT_VIEW_VEC_VS()
  USE_POSTFX_VERTEX_POSITIONS()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 uv  : TEXCOORD0;
    };
  }

  hlsl(vs) {
    VsOutput main_vs_spherical_projection(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.xy, 0.0, 1.0);
      output.uv = pos.xy * 0.5 + 0.5;
      return output;
    }
  }

  hlsl(ps) {
    half4 main_ps_spherical_projection(VsOutput input ):SV_Target
    {
      float theta = -(2.0 * input.uv.x - 1.0) * PI;
      float phi = -(2.0 * input.uv.y - 1.0) * PI / 2.0 ;

      float x = cos(phi) * cos(theta) ;
      float y = sin(phi);
      float z = cos(phi) * sin(theta);

      float3 view = normalize(float3(x,y,z));
      return texCUBElod(source_tex, float4(view,0));
    }
  }

  compile("target_vs", "main_vs_spherical_projection");
  compile("target_ps", "main_ps_spherical_projection");
}
