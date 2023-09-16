include "shader_global.sh"
include "gbuffer.sh"
include "psh_derivate.sh"
include "psh_tangent.sh"

shader testMaterialAO
{
  no_ablend;

  channel float3 pos = pos;
  channel float3 norm = norm;
  channel float2 tc[0] = tc[0];

  texture tex = material.texture[0];
  texture normalmap = material.texture[2];
  texture ao = material.texture[3];

  cull_mode = none;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 tc:  TEXCOORD0;
      float3 norm:  NORMAL;
      float3 p2e:  TEXCOORD1;
    };
  }
  (vs)
  {
    globtm@f44 = globtm;
    world_view_pos@f3 = world_view_pos;
  }
  hlsl(vs) {
    struct VsInput
    {
      float3 pos: POSITION;
      float3 norm: NORMAL;
      float2 tc: TEXCOORD0;
    };


    VsOutput test_vs(VsInput input)
    {
      VsOutput output;
      float3 pos = input.pos;
      output.pos = mul(float4(pos, 1), globtm);
      output.p2e = world_view_pos-pos;
      output.norm = input.norm;
      output.tc  = input.tc;
      return output;
    }
  }
  //cull_mode = none;

  USE_PIXEL_TANGENT_SPACE()
  WRITE_GBUFFER()

  (ps)
  {
    tex@static = tex;
    normalmap@static = normalmap;
    aotex@static = ao;
  }
  hlsl(ps) {
    #define PARALLAX_TEX get_tex()
    #define parallax_tex2dlod(a,b) tex2DLodBindless(a, b)
    #define parallax_tex2d(a,b) tex2DBindless(a, b)
    #define PARALLAX_ATTR a
    #include "parallax.hlsl"

    GBUFFER_OUTPUT test_ps(VsOutput input HW_USE_SCREEN_POS INPUT_VFACE)
    {
      float4 screenpos = GET_SCREEN_POS(input.pos);
      UnpackedGbuffer result;
      init_gbuffer(result);
      half4 albedo;
      float3 viewDir;
      float3 vertexNormal = normalize(input.norm);
      vertexNormal = MUL_VFACE(vertexNormal);
      float2 texCoord;

      half3x3 tangent = cotangent_frame( input.norm, input.p2e, input.tc );
      viewDir.x = dot(input.p2e, tangent[0]);
      viewDir.y = dot(input.p2e, tangent[1]);
      viewDir.z = dot(input.p2e, tangent[2]);
      viewDir = normalize(viewDir);
      /*texCoord = 
        ParallaxOcclusionMap(
          half4( input.tc, 0, 
            ComputeTextureLOD(float2(512,512),ddx(input.tc),ddy(input.tc))),
          viewDir.xyz,
          6,
          5,
          -0.02,
          albedo
        );*/
      texCoord = get_parallax(viewDir.xy, input.tc, 0.02);
      //texCoord = input.tc;
      albedo = tex2DBindless(get_tex(), texCoord);
      //float2 texCoord = input.tc;
      half4 normal_roughness = tex2DBindless(get_normalmap(), texCoord);
      //albedo = normal_roughness;
      half ao = tex2DBindless(get_aotex(), texCoord).g;
      float3 normal;
      normal.xy = (normal_roughness.ag*2-1);
      normal.z = sqrt(saturate(1-dot(normal.xy, normal.xy)));
      //init_albedo_roughness(result, albedo_roughness);
      init_albedo(result, albedo.xyz);
      init_smoothness(result, normal_roughness.r);
      //vertexNormal = MUL_VFACE(vertexNormal);
      init_normal(result, perturb_normal( normal, vertexNormal, input.p2e, input.tc));
      //init_normal(result, normalize(input.norm));
      init_metalness(result, normal_roughness.b);
      init_ao(result, ao);
      return encode_gbuffer(result, screenpos);
    }
  }
  compile("target_vs", "test_vs");
  compile("target_ps", "test_ps");
}

shader cpuFogRender
{
  no_ablend;

  USE_POSTFX_VERTEX_POSITIONS()

  cull_mode = none;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 insc:  TEXCOORD0;
      float3 loss:  TEXCOORD1;
    };
  }
  (vs)
  {
    globtm@f44 = globtm;
  }
  hlsl(vs) {
    float3 pos:register(c60);
    float3 insc:register(c61);
    float3 loss:register(c62);

    VsOutput test_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float3 ofs = float3(0,0,0);
      ofs.xz = getPostfxVertexPositionById(vertexId).xy;
      float3 cpos = pos+ofs*float3(2,3,1);
      output.pos = mul(float4(cpos, 1), globtm);
      output.insc = insc;
      output.loss = loss;
      return output;
    }
  }
  hlsl(ps) {
    half4 test_ps(VsOutput input): SV_Target
    {
      return half4(input.loss*float3(0.5,0.5,0.5) + input.insc, 1);
    }
  }
  compile("target_vs", "test_vs");
  compile("target_ps", "test_ps");
}
