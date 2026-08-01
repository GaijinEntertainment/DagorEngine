// Full-featured per-light evaluation shared by the screen-space light walkers:
// decodes the packed light, evaluates the BRDF with dynamic shadows,
// photometry and optional contact shadows / SSSS.

#ifndef OMNI_CONTACT_SHADOWS_CALC
#define OMNI_CONTACT_SHADOWS_CALC
#endif

// per-pixel shading context, invariant across the light walk; built once by
// the resolve before iterating lights
struct FullLightContext
{
  ProcessedGbuffer gbuffer;
  float3 worldPos;
  half3 view;
  half NoV;
  half3 specularColor;
  half dynamicLightsSpecularStrength;
  half dynamicAO;
  half2 screenpos;
  float2 tc;
  float3 ssssWorldPos;
  float3 contactStartCameraToPoint;
  float dither;
  float w;
};

half3 perform_omni_light_full(uint omni_light_index, RenderOmniLight ol, FullLightContext ctx,
  out bool hasShadow, out float3 point2light_out, out float shadowZnNear)
{
  ProcessedGbuffer gbuffer = ctx.gbuffer;
  float3 worldPos = ctx.worldPos;
  float3 contactStartCameraToPoint = ctx.contactStartCameraToPoint;
  float dither = ctx.dither;
  float w = ctx.w;

  float4 pos_and_radius = ol.posRadius;
  float4 color_and_specular = getFinalColor(ol, worldPos);
  float2 shadowZnZf = ol.shadowZnZf_pad.xy;
  hasShadow = any(ol.colorFlags.rgb > 0);
  #if OMNI_SHADOWS
    float4 shadowTcToAtlas = getOmniLightShadowData(omni_light_index);
  #else
    float4 shadowTcToAtlas = float4(0, 0, 0, 0);
  #endif
  half3 omniLight = perform_point_light(worldPos.xyz, ctx.view, ctx.NoV, gbuffer, ctx.specularColor, ctx.dynamicLightsSpecularStrength, ctx.dynamicAO, pos_and_radius, color_and_specular, shadowTcToAtlas, shadowZnZf, ctx.screenpos);//use gbuffer.specularColor for equality with point_lights.dshl

  OMNI_CONTACT_SHADOWS_CALC

  point2light_out = pos_and_radius.xyz - worldPos.xyz;
  shadowZnNear = shadowZnZf.x;
  return omniLight;
}

// ssss_result: SSSS transmittance term, callers add it to their total
// unscaled by the dynamic lights AO
half3 perform_spot_light_full(uint spot_light_index, RenderSpotLight sl, FullLightContext ctx,
  out bool hasShadow, out half3 point2light_out, out half3 ssss_result)
{
  ProcessedGbuffer gbuffer = ctx.gbuffer;
  float3 worldPos = ctx.worldPos;
  half3 view = ctx.view;
  half NoV = ctx.NoV;
  half3 specularColor = ctx.specularColor;
  half dynamicLightsSpecularStrength = ctx.dynamicLightsSpecularStrength;
  half2 screenpos = ctx.screenpos;
  float2 tc = ctx.tc;
  float3 ssssWorldPos = ctx.ssssWorldPos;
  float3 contactStartCameraToPoint = ctx.contactStartCameraToPoint;
  float dither = ctx.dither;
  float w = ctx.w;

  half3 result = 0;
  ssss_result = 0;
  float4 lightPosRadius = decode_spot_light_pos_radius(sl);
  float4 lightColor = sl.lightColorAngleScale;
  float4 lightDirection = sl.lightDirectionAngleOffset;
  float2 texId_scale = sl.texId_scale_illuminatingplane_packedDataBits.xy;
  float ao = ctx.dynamicAO;
  float illuminatingPlaneOffset = sl.texId_scale_illuminatingplane_packedDataBits.z;
  float lightRollAngle = decode_light_roll_angle(sl);
  uint shadowDataBits = get_spot_light_shadow_data_bits(sl);
  hasShadow = spot_light_has_shadow(shadowDataBits);
  bool needsContactShadows = spot_light_needs_contact_shadows(shadowDataBits);
  point2light_out = 0;
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  #define EXIT_STATEMENT { hasShadow = false; return 0; }
  #endif
  #include <clustered/oneSpotLight.hlsl>
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  #undef EXIT_STATEMENT
  #endif
  point2light_out = point2light;
  ssss_result = result;
  return lightBRDF;
}
