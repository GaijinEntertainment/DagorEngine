include "shader_global.sh"
include "clustered/lights_cb.sh"
include "viewVecVS.sh"

texture downsampled_close_depth_tex;

float pixel_scale;
int has_conservative_rasterization = 0;
interval has_conservative_rasterization : no < 1, yes;

shader spot_lights_tiles, omni_lights_tiles
{
  supports global_frame;
  cull_mode = none;
  z_test = false;
  z_write = false;

  (vs) {
    globtm@f44 = globtm;
    tile_scale@f1 = (pixel_scale * 16, 0, 0, 0);
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)

      nointerpolation uint index : TEXCOORD0;
      nointerpolation float4 sourceToEye : TEXCOORD1;
      ##if shader == spot_lights_tiles
      nointerpolation float4 cone : TEXCOORD2;
      ##endif
    };
  }

  INIT_ZNZFAR()
  ENABLE_ASSERT(ps)

  if (shader == spot_lights_tiles)
  {
    INIT_SPOT_LIGHTS_CB(vs)
    hlsl(vs) {
      VsOutput spot_lights_tiles_vs(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
      {
        RenderSpotLight light = spot_lights_cb[instanceId];
        float3 e1 = abs(light.lightDirectionAngleOffset.z) < 0.9 ? float3(0, 0, 1) : float3(1, 0, 0);
        float3 e2 = normalize(cross(light.lightDirectionAngleOffset.xyz, e1));
        e1 = normalize(cross(light.lightDirectionAngleOffset.xyz, e2));
        float halfCos = -light.lightDirectionAngleOffset.w / abs(light.lightColorAngleScale.w);
        float halfTan = sqrt(1 / pow2(halfCos) - 1);
        float halfWidth = light.lightPosRadius.w * halfTan;

        ##if has_conservative_rasterization == no
          float worstZ = mul(float4(light.lightPosRadius.xyz, 1), globtm).z + light.lightPosRadius.w;
          const float ENLARGE_GEOMETRY = 2;
          halfWidth += ENLARGE_GEOMETRY * worstZ * tile_scale;
        ##endif

        e1 *= halfWidth;
        e2 *= halfWidth;
        float3 worldPos = light.lightPosRadius.xyz;
        uint triangleId = vertexId / 3;
        uint vertId = vertexId % 3;
        bool bottomTriangles = triangleId > 3;
        if (vertId != 0 || bottomTriangles)
          worldPos += light.lightDirectionAngleOffset.xyz * light.lightPosRadius.w;
        triangleId &= 3;
        if (vertId != 0) {
          if (triangleId == 0)
            worldPos += e1;
          else if (triangleId == 1)
            worldPos += e2;
          else if (triangleId == 2)
            worldPos -= e1;
          else if (triangleId == 3)
            worldPos -= e2;
        }
        if ((vertId == 2 && !bottomTriangles) || (vertId == 1 && bottomTriangles)) {
          if (triangleId == 0)
            worldPos += e2;
          else if (triangleId == 1)
            worldPos -= e1;
          else if (triangleId == 2)
            worldPos -= e2;
          else if (triangleId == 3)
            worldPos += e1;
        }
        if ((vertId == 1 && !bottomTriangles) || (vertId == 2 && bottomTriangles)) {
          if (triangleId == 0)
            worldPos -= e2;
          else if (triangleId == 1)
            worldPos += e1;
          else if (triangleId == 2)
            worldPos += e2;
          else if (triangleId == 3)
            worldPos -= e1;
        }
        VsOutput output;
        output.pos = mul(float4(worldPos, 1), globtm);
        output.pos.z = max(0.0000001, output.pos.z);
        output.index = instanceId + 256;
        float3 sourceToEye = world_view_pos - light.lightPosRadius.xyz;
        output.cone = float4(light.lightDirectionAngleOffset.xyz, pow2(halfCos));
        output.sourceToEye.xyz = sourceToEye;
        output.sourceToEye.w = pow2(light.lightPosRadius.w) - dot(sourceToEye, sourceToEye);
        return output;
      }
    }
    compile("target_vs", "spot_lights_tiles_vs");
  }
  else
  {
    INIT_AND_USE_PHOTOMETRY_TEXTURES(vs)
    INIT_AND_USE_OMNI_LIGHTS_CB(vs)
    hlsl(vs)
    {
      #include "spheres_vertices.hlsl"
      VsOutput omni_lights_tiles_vs(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
      {
        RenderOmniLight light = omni_lights_cb[instanceId];
        light.posRadius.w = light.posRelToOrigin_cullRadius.w > 0.0f ? light.posRelToOrigin_cullRadius.w : light.posRadius.w;

        VsOutput output;
        float radius = light.posRadius.w;
        ##if has_conservative_rasterization == no
          float worstZ = mul(float4(light.posRadius.xyz, 1), globtm).z + light.posRadius.w;
          const float ENLARGE_GEOMETRY = 2;
          radius += ENLARGE_GEOMETRY * worstZ * tile_scale;
        ##endif
        float3 worldPos = get_sphere_vertex_pos(vertexId) * radius + light.posRadius.xyz;

        output.pos = mul(float4(worldPos, 1), globtm);
        output.pos.z = max(0.0000001, output.pos.z);
        output.index = instanceId;
        float3 sourceToEye = world_view_pos - light.posRadius.xyz;
        output.sourceToEye.xyz = sourceToEye;
        output.sourceToEye.w = pow2(light.posRadius.w) - dot(sourceToEye, sourceToEye);
        return output;
      }
    }
    compile("target_vs", "omni_lights_tiles_vs");
  }

  (ps) {
    downsampled_far_depth_tex@smp2d = downsampled_far_depth_tex;
    close_depth_tex@smp2d = downsampled_close_depth_tex;
    screen_size@f4 = (1. / screen_pos_to_texcoord.x, 1. / screen_pos_to_texcoord.y, screen_pos_to_texcoord.x, screen_pos_to_texcoord.y);
  }
  USE_AND_INIT_VIEW_VEC(ps)

  hlsl(ps) {
    #include <tiled_light_consts.hlsli>

    RWStructuredBuffer<uint> perTileLights : register(u1);

    bool check_ray_light_intersection(VsOutput input, float2 depth_min_max, float3 view_vec)
    {
      float sphereD2 = pow2(dot(view_vec, input.sourceToEye.xyz)) + input.sourceToEye.w * dot(view_vec, view_vec);
      BRANCH
      if (sphereD2 < 0)
        return false;
      float sphereD = sqrt(sphereD2);
      float t1 = (-dot(view_vec, input.sourceToEye.xyz) - sphereD) / dot(view_vec, view_vec);
      float t2 = (-dot(view_vec, input.sourceToEye.xyz) + sphereD) / dot(view_vec, view_vec);

##if shader == spot_lights_tiles
      float a = pow2(dot(view_vec, input.cone.xyz)) - input.cone.w * dot(view_vec, view_vec);
      float b = (dot(input.sourceToEye.xyz, input.cone.xyz) * dot(view_vec, input.cone.xyz) - dot(view_vec, input.sourceToEye.xyz) * input.cone.w) / a;
      float c = (pow2(dot(input.sourceToEye.xyz, input.cone.xyz)) - dot(input.sourceToEye.xyz, input.sourceToEye.xyz) * input.cone.w) / a;
      float coneD2 = pow2(b) - c;
      if (coneD2 < 0)
        return false;
      float coneD = sqrt(coneD2);
      float coneT1 = (-b - coneD);
      float coneT2 = (-b + coneD);
      if (dot(input.cone.xyz, normalize(input.sourceToEye.xyz + view_vec * coneT1)) < 0)
      {
        t1 = max(t1, coneT2);
      }
      else if (dot(input.cone.xyz, normalize(input.sourceToEye.xyz + view_vec * coneT2)) < 0)
      {
        t2 = min(t2, coneT1);
      }
      else
      {
        t1 = max(t1, coneT1);
        t2 = min(t2, coneT2);
      }
##endif

      const float EPS = 1e-5;
      return depth_min_max.x < t2 + EPS && depth_min_max.y > t1 - EPS;
    }

    float light_tiles_ps(VsOutput input INPUT_VFACE HW_USE_SCREEN_POS) : SV_Target0
    {
      float4 screenpos = GET_SCREEN_POS(input.pos);
      uint2 tileIdx = screenpos.xy;
      uint2 depthTile = tileIdx;
      uint2 tiledGridSize = (screen_size.xy + TILE_EDGE - 1) / TILE_EDGE;
      depthTile = tileIdx * TILE_EDGE + TILE_EDGE - 1 < screen_size.xy ? depthTile : depthTile - 1;
      uint index = input.index;
      uint finalIdx = (tileIdx.x * tiledGridSize.y + (tileIdx.y)) * DWORDS_PER_TILE + (index / BITS_IN_UINT);
      float closeDepth = linearize_z(texelFetch(close_depth_tex, depthTile, DIVIDE_RESOLUTION_BITS - 1).x, zn_zfar.zw);
      float farDepth = linearize_z(texelFetch(downsampled_far_depth_tex, depthTile, DIVIDE_RESOLUTION_BITS - 1).x, zn_zfar.zw);
      float3 viewVecTileLT = lerp_view_vec(screenpos.xy * screen_size.zw * TILE_EDGE);
      float3 viewVecTileLB = lerp_view_vec((screenpos.xy + float2(0, 1)) * screen_size.zw * TILE_EDGE);
      float3 viewVecTileRT = lerp_view_vec((screenpos.xy + float2(1, 0)) * screen_size.zw * TILE_EDGE);
      float3 viewVecTileRB = lerp_view_vec((screenpos.xy + float2(1, 1)) * screen_size.zw * TILE_EDGE);
      #if DISABLE_RAY_LIGHT_INTERSECTION
        InterlockedOr(structuredBufferAt(perTileLights, finalIdx), 1u << (index % BITS_IN_UINT));
      #else
        if (
          check_ray_light_intersection(input, float2(closeDepth, farDepth), viewVecTileLT)
          || check_ray_light_intersection(input, float2(closeDepth, farDepth), viewVecTileLB)
          || check_ray_light_intersection(input, float2(closeDepth, farDepth), viewVecTileRT)
          || check_ray_light_intersection(input, float2(closeDepth, farDepth), viewVecTileRB))
          InterlockedOr(structuredBufferAt(perTileLights, finalIdx), 1u << (index % BITS_IN_UINT));
      #endif
      return 0;
    }
  }
  compile("target_ps", "light_tiles_ps");
}
