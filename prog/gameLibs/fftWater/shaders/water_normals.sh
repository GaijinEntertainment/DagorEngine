include "shader_global.sh"
include "waveWorks.sh"

float4 water_normals_size = (512, 512, 1 / 512, 1 / 512);

int water_normals_cascade_no = 0;
interval water_normals_cascade_no: first<1, second<2, third<3, fourth<4, fifth;

shader water_normals
{
  cull_mode=none;
  z_test=false;
  z_write=false;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 tc : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()
  (ps) { water_normals_size@f4 = water_normals_size; }

  hlsl(vs) {
    VsOutput water_normals_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.x, pos.y, 1, 1);
      output.tc = pos * RT_SCALE_HALF + 0.5;
      return output;
    }
  }

  INIT_WATER_GRADIENTS()
  USE_WATER_GRADIENTS(0)

  (ps) { water_normals_cascade_no@f1 = float4(water_normals_cascade_no, 0, 0, 0); }

  hlsl(ps) {
    half4 water_normals_ps(VsOutput input): SV_Target
    {
##if support_texture_array == on
      #define GET_NORMAL(normal, offs) \
        normal = tex3D(water_gradients_tex0, float3(input.tc.xy + offs, water_normals_cascade_no)).xyz; \
        normal = normalize(float3(normal.xy, 1.0)); \
        normal = normal.xzy;
##else
      #define GET_NORMAL_NO(normal, offs, no) \
        normal = tex2D(water_gradients_tex##no, input.tc.xy + offs); \
        normal = normalize(float3(normal.xy, 1.0)); \
        normal = normal.xzy;
      ##if water_normals_cascade_no == first
          #define GET_NORMAL(normal, offs) GET_NORMAL_NO(normal, offs, 0)
      ##elif water_normals_cascade_no == second
          #define GET_NORMAL(normal, offs) GET_NORMAL_NO(normal, offs, 1)
      ##elif water_normals_cascade_no == third && water_cascades != two
          #define GET_NORMAL(normal, offs) GET_NORMAL_NO(normal, offs, 2)
      ##elif water_normals_cascade_no == fourth
          #define GET_NORMAL(normal, offs) GET_NORMAL_NO(normal, offs, 3)
      ##elif water_normals_cascade_no == fifth
          #define GET_NORMAL(normal, offs) GET_NORMAL_NO(normal, offs, 4)
      ##else
        #define GET_NORMAL(normal, offs)
      ##endif
##endif

      // Output
      float3 normal0 = float3(0, 0, 0), normal1 = float3(0, 0, 0), normal2 = float3(0, 0, 0), normal3 = float3(0, 0, 0);
      GET_NORMAL(normal0, float2(-0.5 * water_normals_size.x, -0.5 * water_normals_size.x));
      GET_NORMAL(normal1, float2(-0.5 * water_normals_size.x, 0.5 * water_normals_size.x));
      GET_NORMAL(normal2, float2(0.5 * water_normals_size.x, 0.5 * water_normals_size.x));
      GET_NORMAL(normal3, float2(0.5 * water_normals_size.x, -0.5 * water_normals_size.x));
      return half4((normal0 + normal1 + normal2 + normal3) * 0.25, 0);
    }
  }

  compile("target_vs", "water_normals_vs");
  compile("target_ps", "water_normals_ps");
}