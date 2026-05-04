#define GBUFFER_ORDER_STATIC 0
#define GBUFFER_ORDER_DYNAMIC 1

#define GBUFFER_HAS_DYNAMIC 1

#define SHADING_NORMAL 0
#define SHADING_SUBSURFACE 1//translucent/blood
#define SHADING_FOLIAGE 2
#define SHADING_SELFILLUM 3
#define BVH_GLASS 1 << 0
#define BVH_GRASS 1 << 1
#define BVH_FOLIAGE 1 << 2
#define BVH_FACING_SUN 1 << 3

#define BASE_MAT_BIT_CNT 2
#define BASE_MAT_CNT (1 << BASE_MAT_BIT_CNT)
#define BASE_MAT_MASK (((1 << BASE_MAT_BIT_CNT)-1))

#define ADV_MAT_BIT_CNT 1u
#define ADV_MAT_BIT_POS_IN_MAT 3u
#define ADV_MAT_BIT_POS_IN_EXTRA 2u

#define ADV_MAT_VAL (ADV_MAT_BIT_CNT << (ADV_MAT_BIT_POS_IN_MAT - 1))

#define ADV_MAT_MASK_IN_MAT (ADV_MAT_BIT_CNT << (ADV_MAT_BIT_POS_IN_MAT - 1))
#define ADV_MAT_MASK_IN_EXTRA (ADV_MAT_BIT_CNT << (ADV_MAT_BIT_POS_IN_EXTRA - 1))

// Shift from either extra -> mat(<<) or mat -> extra (>>)
#define ADV_MAT_CONVERT_SHIFT (ADV_MAT_BIT_POS_IN_MAT - ADV_MAT_BIT_POS_IN_EXTRA)

// Advanced Materials
#define SHADING_NO_LIGHT_EMISSIVE (ADV_MAT_VAL + SHADING_SELFILLUM)


#define MAX_EMISSION_LEGACY 4.0h
#define MAX_EMISSION 16.0h

#define SSS_PROFILE_MASK 0xF8
#define SSS_PROFILE_EYES 0xF0
#define SSS_PROFILE_CLOTH 0x08
#define SSS_PROFILE_NEUTRAL_TRANSLUCENT 0x10

bool isSubSurfaceShader(int material) {
  return material == SHADING_FOLIAGE || material == SHADING_SUBSURFACE;
}
bool isMetallicShader(uint material) {return material == SHADING_NORMAL;}
bool isEmissiveShader(uint material) {return material == SHADING_SELFILLUM || material == SHADING_NO_LIGHT_EMISSIVE;}
bool isNoLightEmissiveShader(uint material) {return material == SHADING_NO_LIGHT_EMISSIVE;}
bool isFoliageShader(uint material) {return material == SHADING_FOLIAGE;}

bool isSubSurfaceProfileValid(uint sss_profile) {
  return sss_profile != SSS_PROFILE_EYES && sss_profile != SSS_PROFILE_CLOTH && sss_profile != SSS_PROFILE_NEUTRAL_TRANSLUCENT;
}

bool isTranslucentProfileValid(uint sss_profile) {
  return sss_profile != SSS_PROFILE_EYES && sss_profile != SSS_PROFILE_CLOTH;
}
