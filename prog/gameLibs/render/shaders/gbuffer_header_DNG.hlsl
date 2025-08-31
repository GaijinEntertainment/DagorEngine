// This is for DNG only, but it is required for NBS, so it needs to stay in gameLibs

#define GBUFFER_ORDER_STATIC 0
#define GBUFFER_ORDER_DYNAMIC 1

#define GBUFFER_HAS_DYNAMIC 1

#ifndef DEFERRED_MOBILE
  #error Need to define DEFERRED_MOBILE based on interval: mobile_render == deferred
#endif

#if DEFERRED_MOBILE
#define SHADING_NORMAL 0
#define SHADING_FOLIAGE 1
#define SUPPORTED_MATERIALS_IN_RESOLVE_MASK 1
//reserve 2-nd bit
#define SHADING_SUBSURFACE 2
#define SHADING_SELFILLUM 3
#else
#define SHADING_NORMAL 0
#define SHADING_SUBSURFACE 1//translucent/blood
#define SHADING_FOLIAGE 2
#define SHADING_SELFILLUM 3
#endif
#define MATERIAL_COUNT 4

#define MAX_EMISSION 16.0h

#define SSS_PROFILE_MASK 0xF8
#define SSS_PROFILE_EYES 0xF0
#define SSS_PROFILE_CLOTH 0x08
#define SSS_PROFILE_NEUTRAL_TRANSLUCENT 0x10

bool isSubSurfaceShader(int material) {
#if DEFERRED_MOBILE
  return material == SHADING_FOLIAGE;
#else
  return material == SHADING_FOLIAGE || material == SHADING_SUBSURFACE;
#endif
}
bool isMetallicShader(uint material) {return material == SHADING_NORMAL;}
bool isEmissiveShader(float material) {return material == SHADING_SELFILLUM;}
bool isFoliageShader(uint material) {return material == SHADING_FOLIAGE;}

bool isSubSurfaceProfileValid(uint sss_profile) {
  return sss_profile != SSS_PROFILE_EYES && sss_profile != SSS_PROFILE_CLOTH && sss_profile != SSS_PROFILE_NEUTRAL_TRANSLUCENT;
}

bool isTranslucentProfileValid(uint sss_profile) {
  return sss_profile != SSS_PROFILE_EYES && sss_profile != SSS_PROFILE_CLOTH;
}
