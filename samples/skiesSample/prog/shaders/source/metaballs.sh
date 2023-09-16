include "shader_global.sh"
include "gbuffer.sh"

shader metaballs_simple
{
  cull_mode = cw;

  (vs) { globtm@f44 = globtm; }

  channel float3 pos = pos;
  channel float3 norm = norm;

  WRITE_GBUFFER()

  hlsl{
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 normal : TEXCOORD0;
    };
  }

  hlsl(vs) {
    struct VsInput
    {
      float3 pos : POSITION;
      float4 normal : NORMAL;
    };

    VsOutput simple_vs(VsInput input)
    {
      VsOutput output;

      output.pos = INVARIANT(mul(float4(input.pos, 1), globtm));
      output.normal.xyz = normalize(input.normal.xyz);

      return output;
    }
  }

  hlsl(ps) {
    GBUFFER_OUTPUT simple_ps(VsOutput input)
    {
      UnpackedGbuffer gbuffer;
      init_gbuffer(gbuffer);
      init_material(gbuffer, SHADING_NORMAL);
      init_smoothness(gbuffer, 0.1);
      init_metalness(gbuffer, 0.1);
      init_normal(gbuffer, input.normal);
      init_albedo(gbuffer, float3(0.5, 0, 0));
      return encode_gbuffer(gbuffer, input.pointToEye);
    }
  }
  compile("target_ps", "simple_ps");
  compile("target_vs", "simple_vs");
}