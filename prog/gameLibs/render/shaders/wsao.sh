include "dagi_volmap_gi.sh"
include "dagi_volmap_common_25d.sh"
include "dagi_quality.sh"

define_macro_if_not_defined USING_ECLIPSE(stage)
  hlsl(stage) {
    #define eclipse_progress 0
    #define eclipse_ambient_atten 1
    #define eclipse_sun_atten 1
    #define eclipse_lightprobe_weight_atten 1
    #define eclipse_gi_weight_atten 1
  }
endmacro

macro WSAO_BASE(code)
  hlsl {
    #define FIXED_LOOP_COUNT 1
    #if FIXED_LOOP_COUNT
      #define CASCADE_0_DIST 6
      #define CASCADE_1_DIST 8
      #define MAX_DIST 6
    #else
      #define CASCADE_0_DIST 3.0f
      #define CASCADE_1_DIST 15.f
      #define MAX_DIST 30.f
    #endif
  }

  USING_ECLIPSE(code)
  INIT_SKY_DIFFUSE_BASE(code)
  USE_SKY_DIFFUSE_BASE(code)
  INIT_HMAP_HOLES_ZONES(code)
  USE_VOXELS_HEIGHTMAP_HEIGHT_25D(code)
  SSGI_USE_VOLMAP_GI_AMBIENT_VOLMAP(code)

  SAMPLE_INIT_VOLMAP_25D(code)
  SAMPLE_VOLMAP_25D(code)

  (code) {
    gi_25d_volmap_size@f4 = gi_25d_volmap_size;
  }

  hlsl(code) {
    float getWsao(float3 worldPos, float3 normal)
    {
      half wsao = 1.0;

      half3 giAmbient = 1.0;
      half giAmount = get_ambient3d_unfiltered(worldPos, normal, 1, giAmbient, true);
      giAmount *= eclipse_gi_weight_atten;

      BRANCH
      if (giAmount < 1)
      {
        half3 ambient25d = 1.0;
        half ambient25dAmount = sample_25d_volmap(worldPos, normal, ambient25d);
        ambient25dAmount *= eclipse_gi_weight_atten;
        wsao = lerp(wsao, ambient25d.r, ambient25dAmount);
      }

      return lerp(wsao, giAmbient.r, giAmount);
    }
  }
endmacro

macro USING_WSAO()
  WSAO_BASE(ps)
endmacro
