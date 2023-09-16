include "sky_shader_global.sh"
include "viewVecVS.sh"
include "frustum.sh"
include "gbuffer.sh"
include "dagi_volmap_common_25d.sh"
include "dagi_scene_voxels_common_25d.sh"
include "dagi_raycast_voxels_25d.sh"
//include "sample_voxels.sh"

hlsl {
  #include "dagi_volmap_consts_25d.hlsli"
}


//
int debug_volmap_type = 0;
//interval debug_volmap_type: frustum<4, temporal_age<5, random_point_selected<6, selected<8, all;

shader debug_render_volmap_25d
{
  no_ablend;

  (vs) {
    globtm@f44 = globtm;
    world_view_pos@f3 = world_view_pos;
    debug_volmap_type@f1 = (debug_volmap_type, 0, 0, 0);
  }

  hlsl(vs) {
    #define NO_GRADIENTS_IN_SHADER 1
  }
  SAMPLE_INIT_VOLMAP_25D(ps)
  USE_VOLMAP_25D(vs)
  USE_VOLMAP_25D(ps)
  GI_INIT_25D_BUFFER(vs)
  VOLMAP_INTERSECTION_25D(vs)
  //INIT_VOXELS_HEIGHTMAP_HEIGHT_25D(vs)
  RAY_CAST_VOXELS_25D(ps)
  SAMPLE_VOLMAP_25D(ps)
  ENABLE_ASSERT(ps)

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 norm:  TEXCOORD1;
      float3 centerPos:  TEXCOORD2;
      uint3 coord: TEXCOORD3;
      float2 minMaxHt: TEXCOORD4;
    };
  }
  hlsl(vs) {
    VsOutput test_vs(uint iId : SV_InstanceID, uint vertId : SV_VertexID)
    {
      VsOutput output;
      uint3 coord = uint3(iId%GI_25D_RESOLUTION_X, (iId/GI_25D_RESOLUTION_X)%GI_25D_RESOLUTION_X, iId/(GI_25D_RESOLUTION_X*GI_25D_RESOLUTION_X));
      output.coord = uint3(wrapGI25dCoord(coord.xy), coord.z);
      float2 centerXZ = gi25dCoordToWorldPos(coord.xy);
      //float floorHt = ssgi_get_heightmap_2d_height_25d_volmap(centerXZ);//todo: should be heavily optimized
      float floorHt = get_volmap_25d_floor(output.coord.xy);
      float3 center = float3(centerXZ, floorHt + (pow2(coord.z+0.5f)/GI_25D_RESOLUTION_Y)*getGI25DVolmapSizeY()).xzy;
      output.minMaxHt = floorHt + (float2(pow2(coord.z), pow2(coord.z+1))/GI_25D_RESOLUTION_Y)*getGI25DVolmapSizeY();

      float3 probeSize = 0.15;
      float3 cornerPos = float3(vertId&1 ? 1 : 0, vertId&2 ? 1 : 0, vertId&4 ? 1 : 0);//generate 
      //float3 cornerPos = float3(vertId&1 ? 1 : 0, vertId&2 ? 1 : 0, vertId&4 ? (1-inside) : inside);//generate 
      output.centerPos.xyz = center;

      float3 worldPos = center + (cornerPos*2-float3(1,1,1))*probeSize;
      output.pos = mul(float4(worldPos, 1), globtm);
      output.norm = normalize(cornerPos*float3(2,2,2)-float3(1,1,1));
      return output;
    }
  }

  (ps) { debug_volmap_type@f1 = (debug_volmap_type,0,0,0); }

  hlsl(ps) {
    half3 test_ps(VsOutput input HW_USE_SCREEN_POS):SV_Target0
    {
      float4 screenpos = GET_SCREEN_POS(input.pos);
      //float3 normal = normalize(cross(ddx_fine(input.pointToEye), ddy_fine(input.pointToEye)));
      float3 absnorm = abs(input.norm);
      float maxn = max(max(absnorm.x, absnorm.y), absnorm.z);
      float3 w = pow2_vec3(1-saturate(maxn.xxx-absnorm));
      w = pow2_vec3(w); w = pow2_vec3(w); w = pow2_vec3(w);w = pow2_vec3(w);

      float3 dirNormal = normalize(input.norm * w);
      //float3 dirNormalSq = dirNormal*dirNormal;
      //float3 color = dirNormalSq.x*input.cx + dirNormalSq.y*input.cy + dirNormalSq.z*input.cz;
      float3 nSquared = dirNormal * dirNormal;
      uint z_ofs = GI_25D_RESOLUTION_Y;
      //float3 ambient;
      //sample_25d_volmap(input.centerPos, dirNormal, ambient);
      //return ambient;
      if (debug_volmap_type < 2)
      {
        bool intersected = is_intersected_25d_unsafe(input.coord);
        if (debug_volmap_type == 1)
          return intersected;
        if (debug_volmap_type == 0 && !intersected)
          discard;
      }
      return nSquared.x * FETCH_25D_AMBIENT_VOLMAP(input.coord+uint3(0,0,dirNormal.x<0 ? z_ofs:0)) +
             nSquared.y * FETCH_25D_AMBIENT_VOLMAP(input.coord+uint3(0,0,dirNormal.y<0 ? z_ofs*3:z_ofs*2)) +
             nSquared.z * FETCH_25D_AMBIENT_VOLMAP(input.coord+uint3(0,0,dirNormal.z<0 ? z_ofs*5:z_ofs*4));
    }
  }
  //*/
  compile("target_vs", "test_vs");
  compile("target_ps", "test_ps");
}

