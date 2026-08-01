#include "renderLights.hlsli"

  half getAngleClip ( half3 normalizedLightVector, half3 lightDir, half lightAngleScale , half lightAngleOffset)
  {
    half cd = dot ( lightDir , normalizedLightVector );
    half attenuation = saturate (cd * lightAngleScale + lightAngleOffset );
    // smooth the transition
    return attenuation * attenuation ;
  }

half getOmniRestrictionBox(RenderOmniLight ol, float3 worldPos)
{
  #if NO_OMNI_LIGHT_FADE
    return 1;
  #else
    float3 boxPos = float3(ol.boxR0.w, ol.boxR1.w, ol.boxR2.w);
    float3 boxDiff = worldPos - boxPos;
    half3 box = 2 * half3((ol.boxR0.xyz * boxDiff.x + ol.boxR1.xyz * boxDiff.y + ol.boxR2.xyz * boxDiff.z));
    box = saturate(abs(box));
    const half FADEOUT_DIST = 0.05; // 5% on both sides
    box = 1 - box;
    half fadeout = min3(box.x, box.y, box.z);
    half fadelimit = FADEOUT_DIST; // linear function
    return fadeout <= fadelimit ? fadeout / fadelimit : 1;
  #endif
}

// counts.y - lights in the tile, counts.x - lights actually reaching the surface
void count_omni_light(uint omni_light_index, float3 worldPos, inout int2 omniCount)
{
  RenderOmniLight ol = omni_lights_cb[omni_light_index];
  float4 pos_and_radius = ol.posRadius;
  omniCount.y++;
  if (length(worldPos - pos_and_radius.xyz) < pos_and_radius.w && getOmniRestrictionBox(ol, worldPos) > 0)
    omniCount.x++;
}

void count_spot_light(uint spot_light_index, float3 worldPos, inout int2 spotCount)
{
  spotCount.y++;
  RenderSpotLight sl = spot_lights_cb[spot_light_index];
  float4 lightPosRadius = decode_spot_light_pos_radius(sl);
  float4 lightColorScale = sl.lightColorAngleScale;
  float4 lightDirection = sl.lightDirectionAngleOffset;
  float geomAttenuation = getAngleClip(normalize(worldPos - lightPosRadius.xyz),
                                       lightDirection.xyz, lightColorScale.w, lightDirection.w);
  if (length(worldPos - lightPosRadius.xyz) < lightPosRadius.w && geomAttenuation > 0)
    spotCount.x++;
}

half4 get_dynamic_lights_count(float3 worldPos, float w, float2 screenpos, float2 screen_size, float lights_debug_mode)
{
  half3 result = 0;
  uint2 tiledGridSize = (screen_size + TILE_EDGE - 1) / TILE_EDGE;

  uint2 tileIdx = screen_uv_to_tile_idx(screenpos);
  uint tileOffset = (tileIdx.x * tiledGridSize.y + tileIdx.y) * DWORDS_PER_TILE;

  int2 spotCount = 0; // y - tile count, x - actual lit surface
  int2 omniCount = 0; // y - tile count, x - actual lit surface

  #define TILED_LIGHTS_WALK_SPOT 0
  #define TILED_LIGHTS_WALK_DEPTH w
  #define TILED_LIGHTS_WALK_TILE_OFFSET tileOffset
  #define TILED_LIGHTS_WALK_BUFFER_AT(buffer, index) structuredBufferAt(buffer, index)
  #define TILED_LIGHTS_WALK_LIGHT_BODY(omni_light_index) count_omni_light(omni_light_index, worldPos, omniCount);
  #include <tiled_lights_walk.hlsli>

  #define TILED_LIGHTS_WALK_SPOT 1
  #define TILED_LIGHTS_WALK_DEPTH w
  #define TILED_LIGHTS_WALK_TILE_OFFSET tileOffset
  #define TILED_LIGHTS_WALK_BUFFER_AT(buffer, index) structuredBufferAt(buffer, index)
  #define TILED_LIGHTS_WALK_LIGHT_BODY(spot_light_index) count_spot_light(spot_light_index, worldPos, spotCount);
  #include <tiled_lights_walk.hlsli>

  float3 dynamicLighting = float3(omniCount.x, spotCount.x, omniCount.x + spotCount.x);

  if (lights_debug_mode > 0) // debug tiles
    dynamicLighting = float3(omniCount.y, spotCount.y, omniCount.y + spotCount.y);

  #define COLORS_COUNT 8
  #define COLORS_STEP 4
  #define MAX_LIGHTS_DEBUG (COLORS_COUNT * COLORS_STEP)

  float3 debugColor[COLORS_COUNT + 1] = {float3(0.1, 0.1, 0.1),
                          float3(0.4, 0.0, 0.4),   // violet
                          float3(0, 0.0, 0.7),     // blue
                          float3(0, 0.6, 0.6),     // cyan
                          float3(0.0, 0.8, 0.0),   // green
                          float3(0.7, 0.7, 0.0),   // yellow
                          float3(0.7, 0.4, 0.0),   // orange
                          float3(1, 0., 0.0),      // red
                          float3(1, 1, 1)};        // white

  int colorIdx = min(dynamicLighting.z/4, COLORS_COUNT - 1);
  float colorLerp = frac(dynamicLighting.z/4);

  if (dynamicLighting.z < MAX_LIGHTS_DEBUG)
    return float4(lerp(debugColor[colorIdx], debugColor[colorIdx + 1], colorLerp), 1);
  else
    return float4(debugColor[8], 1);
}