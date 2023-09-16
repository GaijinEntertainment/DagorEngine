include "clouds_vars.sh"
include "clouds_shadow.sh"
include "static_shadow.sh"
include "depth_above.sh"

macro INIT_LIGHT_25D_HELPERS(code)
  supports global_const_block;
  hlsl(code) {
    #define HAS_STATIC_SHADOW 1
    #define FASTEST_STATIC_SHADOW_PCF 1
    #define STATIC_SHADOW_NO_VIGNETTE 1
    #define to_sun_direction (-from_sun_direction)
    #define CLOUDS_SUN_LIGHT_DIR 1
  }

  INIT_CLOUDS_SHADOW_BASE(-from_sun_direction.y, code)
  USE_CLOUDS_SHADOW_BASE(code)
  INIT_STATIC_SHADOW_BASE(code)
  USE_STATIC_SHADOW_BASE(code)

  INIT_SKY_DIFFUSE_BASE(code)//todo: remove
  USE_SKY_DIFFUSE_BASE(code)//todo: remove
  hlsl(code) {
    half3 light_voxel_ambient(float3 worldPos, half3 normal, half3 albedo, float3 emission, out half3 ambient) {
      half shadow = clouds_shadow(worldPos);
      BRANCH
      if (shadow > 0.001)
        shadow *= getStaticShadow(worldPos);
      ambient = albedo.rgb * GetSkySHDiffuseSimple(normal);
      return albedo.rgb * sun_color_0 * (saturate(dot(normal,-from_sun_direction))*shadow) + emission;
    }
  }
endmacro

