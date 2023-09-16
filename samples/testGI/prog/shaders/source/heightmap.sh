include "shader_global.sh"
include "gbuffer.sh"
include "psh_derivate.sh"
include "edge_tesselation.sh"
include "normaldetail.sh"

texture hmap_ldetail;


texture heightmap;
float4 world_to_hmap_low = (1/2048, 1/2048, 0, 0);
float4 tex_hmap_inv_sizes = (1/2048,1/2048,1/2048,1/2048);
float4 heightmap_scale = (1,0,1,0);

macro USE_HEIGHTMAP_INSTANCING()
  HEIGHTMAP_DECODE_EDGE_TESSELATION()
  hlsl(vs) {
    float2 decodeWorldPosXZ(int2 inPos input_used_instance_id)
    {
      float4 instance_const = heightmap_scale_offset[instance_id.x];
      int4 border = decode_edge_tesselation(instance_const);

      inPos.y = adapt_edge_tesselation(inPos.y, inPos.x == 0 ? border.x : (inPos.x == patchDim ? border.y : 1));
      inPos.x = adapt_edge_tesselation(inPos.x, inPos.y == 0 ? border.z : (inPos.y == patchDim ? border.w : 1));
      return decodeWorldPosXZConst(instance_const, inPos);
    }
    float3 getWorldPos(int2 inPos input_used_instance_id)
    {
      float3 worldPos;
      worldPos.xz = decodeWorldPosXZ(inPos used_instance_id);
      worldPos.y = getHeight(worldPos.xz);
      return worldPos;
    }
  }
endmacro

shader heightmap
{
  no_ablend;

  channel short2 pos[0]=pos[0];
  (vs) {
    hmap_ldetail@smp2d = hmap_ldetail;
    globtm@f44 = globtm;
    heightmap_scale@f2 = heightmap_scale;
    world_to_hmap_low@f4 = world_to_hmap_low;
    world_view_pos@f3 = world_view_pos;
  }


  //cull_mode = none;

  hlsl(vs) {
    #define decode_height(a) ((a)*heightmap_scale.x+heightmap_scale.y)
    float getHeight(float2 worldPosXZ)
    {
      float2 tc_low = worldPosXZ*world_to_hmap_low.xy + world_to_hmap_low.zw;
      float height = tex2Dlod(hmap_ldetail, float4(tc_low,0,0)).r;
      float decodedHeight = decode_height(height);
      return decodedHeight;
    }
  }
  USE_HEIGHTMAP_INSTANCING()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 p2e:  TEXCOORD1;
    };
  }

  hlsl(vs) {
    VsOutput test_vs(INPUT_VERTEXID_POSXZ USE_INSTANCE_ID)
    {
      DECODE_VERTEXID_POSXZ
      VsOutput output;
      float3 worldPos = getWorldPos(posXZ USED_INSTANCE_ID);
      output.pos = mul(float4(worldPos, 1), globtm);
      output.p2e = world_view_pos-worldPos;
      return output;
    }
  }
  //cull_mode = none;
  (ps) {
    heightmap@smp2d = heightmap;
    sizes@f4 = (tex_hmap_inv_sizes.x/world_to_hmap_low.x/heightmap_scale.x, 0, 0, 0);
    tex_hmap_inv_sizes@f4 = (tex_hmap_inv_sizes.x, tex_hmap_inv_sizes.y, 1/tex_hmap_inv_sizes.x, 1/tex_hmap_inv_sizes.y);
    world_to_hmap_low@f4 = world_to_hmap_low;
    world_view_pos@f3 = world_view_pos;
  }

  USE_PIXEL_TANGENT_SPACE()
  WRITE_GBUFFER()
  USE_NORMAL_DETAIL()

  hlsl(ps) {
    #define sizeInMeters (sizes.x)
    float getTexel(float2 p)
    {
      return h1tex2D(heightmap, p);
    }

    half3 getNormalLow(float2 pos)
    {
      float3 offset = float3(tex_hmap_inv_sizes.x, 0, tex_hmap_inv_sizes.y);
      half W = getTexel(float2(pos.xy - offset.xy));
      half E = getTexel(float2(pos.xy + offset.xy));
      half N = getTexel(float2(pos.xy - offset.yz));
      half S = getTexel(float2(pos.xy + offset.yz));
      return normalize(half3(W-E, sizeInMeters, N-S));
    }
    half3 getWorldNormal(float3 worldPos)
    {
      half3 normal;
      float2 worldPosXZ = worldPos.xz;
      float2 tc_low = worldPosXZ*world_to_hmap_low.xy + world_to_hmap_low.zw;
      normal = getNormalLow(tc_low);
      return normal;
    }

    GBUFFER_OUTPUT test_ps(VsOutput input HW_USE_SCREEN_POS INPUT_VFACE)
    {
      float4 screenpos = GET_SCREEN_POS(input.pos);
      float3 worldPos = world_view_pos.xyz-input.p2e.xyz;
      float3 worldNormal = getWorldNormal(worldPos.xyz).xyz;

      UnpackedGbuffer result;
      init_gbuffer(result);
      float2 worldPosXZ = worldPos.xz;
      float2 tc_low = worldPosXZ*world_to_hmap_low.xy + world_to_hmap_low.zw;

      //texCoord = input.tc;
      half3 albedo = half3(0.15,0.2, 0.05);

      //init_albedo_roughness(result, albedo_roughness);
      init_ao(result, 1);
      init_albedo(result, albedo.xyz);
      init_smoothness(result, 0.01);
      init_normal(result, worldNormal);
      return encode_gbuffer(result, screenpos);
    }
  }
  compile("target_vs", "test_vs");
  compile("target_ps", "test_ps");
}