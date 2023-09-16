include "clouds_shadow.sh"

macro SQ_INIT_CLOUDS_SHADOW()
  INIT_CLOUDS_SHADOW_BASE(-from_sun_direction.y, ps)
endmacro

macro SQ_CLOUDS_SHADOW()
  USE_CLOUDS_SHADOW_BASE(ps)
  hlsl(ps) {
    #define to_sun_direction (-from_sun_direction)
    #define CLOUDS_SUN_LIGHT_DIR 1
  }
endmacro

