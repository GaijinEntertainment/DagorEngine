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

half4 get_dynamic_lights_count(float3 worldPos, float w, float2 screenpos, float2 screen_size, float lights_debug_mode)
{
  half3 result = 0;
  uint2 tiledGridSize = (screen_size + TILE_EDGE - 1) / TILE_EDGE;

  uint2 tileIdx = screen_uv_to_tile_idx(screenpos);
  uint tileOffset = (tileIdx.x * tiledGridSize.y + tileIdx.y) * DWORDS_PER_TILE;

  uint zbinsOmni = structuredBufferAt(z_binning_lookup, depth_to_z_bin(w));
  uint omniBinsBegin = zbinsOmni >> 16;
  uint omniBinsEnd = zbinsOmni & 0xFFFF;
  uint mergedOmniBinsBegin = WAVE_MIN(omniBinsBegin);
  uint mergedOmniBinsEnd = WAVE_MAX(omniBinsEnd);
  uint omniLightsBegin = mergedOmniBinsBegin >> 5;
  uint omniLightsEnd = mergedOmniBinsEnd >> 5;
  uint omniMaskWidth = clamp((int)omniBinsEnd - (int)omniBinsBegin + 1, 0, 32);
  uint omniWord = 0;

  int2 spotCount = 0; // y - tile count, x - actual lit surface
  int2 omniCount = 0; // y - tile count, x - actual lit surface

  for (omniWord = omniLightsBegin; omniWord <= omniLightsEnd; ++omniWord)
  {
    uint mask = structuredBufferAt(lights_list, tileOffset + omniWord);
    //return float(mask)*0.1;
    // Mask by ZBin mask
    uint localMin = clamp((int)omniBinsBegin - (int)(omniWord << 5), 0, 31);
    // BitFieldMask op needs manual 32 size wrap support
    uint zbinMask = omniMaskWidth == 32 ? (uint)(0xFFFFFFFF) : BitFieldMask(omniMaskWidth, localMin);
    mask &= zbinMask;
    uint mergedMask = WAVE_OR(mask);

    LOOP
    while (mergedMask)
    {
      uint bitIdx = firstbitlow(mergedMask);
      uint omni_light_index = omniWord * BITS_IN_UINT + bitIdx;
      mergedMask ^= (1U << bitIdx);

      RenderOmniLight ol = omni_lights_cb[omni_light_index];
      float4 pos_and_radius = ol.posRadius;

      omniCount.y++;

      if (length(worldPos - pos_and_radius.xyz) < pos_and_radius.w && getOmniRestrictionBox(ol, worldPos) > 0)
        omniCount.x++;
    }
  }

  uint zbinsSpot = structuredBufferAt(z_binning_lookup, depth_to_z_bin(w) + Z_BINS_COUNT);
  uint spotBinsBegin = zbinsSpot >> 16;
  uint spotBinsEnd = zbinsSpot & 0xFFFF;
  uint mergedSpotBinsBegin = WAVE_MIN(spotBinsBegin);
  uint mergedSpotBinsEnd = WAVE_MAX(spotBinsEnd);
  uint spotLightsBegin = (mergedSpotBinsBegin >> 5) + DWORDS_PER_TILE / 2;
  uint spotLightsEnd = (mergedSpotBinsEnd >> 5) + DWORDS_PER_TILE / 2;
  uint spotMaskWidth = clamp((int)spotBinsEnd - (int)spotBinsBegin + 1, 0, 32);
  uint spotWord = DWORDS_PER_TILE / 2;

  for (spotWord = spotLightsBegin; spotWord <= spotLightsEnd; ++spotWord)
  {
    uint mask = structuredBufferAt(lights_list, tileOffset + spotWord);
    // Mask by ZBin mask
    uint localMin = clamp((int)spotBinsBegin - (int)((spotWord - DWORDS_PER_TILE / 2) << 5), 0, 31);
    // BitFieldMask op needs manual 32 size wrap support
    uint zbinMask = spotMaskWidth == 32 ? (uint)(0xFFFFFFFF) : BitFieldMask(spotMaskWidth, localMin);
    mask &= zbinMask;
    uint mergedMask = WAVE_OR(mask);

    LOOP
    while (mergedMask)
    {
      uint bitIdx = firstbitlow(mergedMask);
      uint spot_light_index = (spotWord - DWORDS_PER_TILE / 2) * BITS_IN_UINT + bitIdx;
      mergedMask ^= (1U << bitIdx);

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
  }

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