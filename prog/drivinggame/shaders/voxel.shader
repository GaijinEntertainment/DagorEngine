// voxel.shader - Voxel world rendering shader

shader voxel
{
  supports deferred;

  cbuffer voxel_globals (auto)
  {
    float4x4 globtm;
    float4x4 globtm_no_ofs;
    float4x4 view_itm;
    float4 view_vec;
    float4 view_vec2;
    float4 globtm_psf_0;
    float4 globtm_psf_1;
    float4 view_pos;
    float time_game;
    float4 color0;
    float4 color1;
    float4 sun_color0;
    float4 sun_dir0;
    float time_of_day;
    int voxel_atlas_size;
    float4 voxel_atlas_uv_scale;
  };

  cbuffer chunk_transform (auto)
  {
    float4x4 chunk_tm;
  };

  texture voxel_atlas;

  hlsl {
    struct VS_IN
    {
      float3 pos : POSITION;
      uint normal : NORMAL;
      uint texCoord : TEXCOORD;
      uint8_t voxelType : TEXCOORD1;
      uint8_t ao : TEXCOORD2;
      uint16_t padding;
    };

    struct VS_OUT
    {
      float4 pos : SV_POSITION;
      float3 worldPos : TEXCOORD0;
      float2 texCoord : TEXCOORD1;
      float3 normal : TEXCOORD2;
      uint voxelType : TEXCOORD3;
      float ao : TEXCOORD4;
    };
  }

  channel float3 pos = POSITION;
  channel uint normal = NORMAL;
  channel uint texCoord = TEXCOORD;
  channel uint8_t voxelType = TEXCOORD1;
  channel uint8_t ao = TEXCOORD2;

  vertex voxel_vs
  {
    VS_OUT main(VS_IN input)
    {
      VS_OUT output;
      float3 pos = input.pos;
      float3 worldPos = mul(chunk_tm, float4(pos, 1.0)).xyz;
      output.pos = mul(globtm, float4(worldPos, 1.0));
      output.worldPos = worldPos;
      
      // Decode octahedral normal
      uint packed = input.normal;
      float2 oct = float2((packed >> 8) & 0xFF, packed & 0xFF);
      float2 f = (oct / 127.5) - 1.0;
      float norm = 1.0 / (abs(f.x) + abs(f.y) + 1e-6);
      output.normal = float3(f.x * norm, f.y * norm, 1.0 - norm);
      
      // Decode texture coordinates
      uint packedTC = input.texCoord;
      output.texCoord = float2((packedTC >> 16) & 0xFFFF, packedTC & 0xFFFF);
      output.texCoord = output.texCoord / float2(voxel_atlas_size, voxel_atlas_size);
      
      output.voxelType = input.voxelType;
      output.ao = input.ao / 255.0;
      return output;
    }
  }

  fragment voxel_ps
  {
    float4 main(VS_OUT input) : SV_TARGET
    {
      // Sample voxel atlas
      float2 uv = input.texCoord;
      // Voxel type determines atlas region
      float atlasU = (input.voxelType % voxel_atlas_size) / float(voxel_atlas_size);
      float atlasV = floor(input.voxelType / voxel_atlas_size) / float(voxel_atlas_size);
      uv = uv / float(voxel_atlas_size) + float2(atlasU, atlasV);
      
      float4 color = voxel_atlas.Sample(sampler_voxel_atlas, uv);
      
      // Apply AO
      color.rgb *= input.ao;
      
      return color;
    }
  }

  compile "vs_5_0" "voxel_vs";
  compile "ps_5_0" "voxel_ps";
}