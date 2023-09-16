include "hardware_defines.sh"
include "sky_shader_global.sh"
include "skyLight.sh"
include "moon.sh"
include "brunetonSkies.sh"
include "atmosphere.sh"
include_optional "sky_ground_color.sh"

//float4 skies_world_view_pos;
//float render_sun = 1;
float render_sun = 1;
float min_ground_offset;

define_macro_if_not_defined INIT_SKY_GROUND_COLOR()
endmacro

define_macro_if_not_defined GET_SKY_GROUND_COLOR()
  hlsl(ps) {
    #ifndef HAVE_GROUND_COLOR_FUNCTION
    #define HAVE_GROUND_COLOR_FUNCTION

    #define PreparedGroundData float
    #define NO_GROUND 1
    void prepare_ground_data(float3 v, float distToGround, PreparedGroundData data) {}
    #endif
  }
endmacro
texture skies_ms_texture;

float4 real_skies_sun_color;

macro USE_SKIES_SUN_COLOR(code)
  (code) {
    real_skies_sun_light_dir@f3 = real_skies_sun_light_dir;
    real_skies_sun_color@f4 = (real_skies_sun_color.x*render_sun*2, real_skies_sun_color.y*render_sun*2, real_skies_sun_color.z*render_sun*2, render_sun*2);
   }
  hlsl(code) {
    //float pow2(float a){return a*a;}
    #define SUN_MAX_VISIBLE_NU cos(PI * 8.1 / 180.0)
    float3 calcSunColorInt(float nu, float3 transmittance, half3 sun_color)
    {
      float sunMaxRadius = SUN_MAX_VISIBLE_NU;
      float untransmittance = saturate(1-transmittance.r);
      float radiusScale = 2+2*untransmittance;
      float sunOuterRadius = cos(PI * radiusScale*0.3 / 180.0);//should be smaller on mars
      float sunInnerRadius = cos(PI * radiusScale*0.1 / 180.0);//should be smaller on mars
      float val = saturate(nu*(1.0/(sunInnerRadius-sunOuterRadius))-sunOuterRadius/(sunInnerRadius-sunOuterRadius));
      float sun = lerp(pow4(val), val, untransmittance);
      #define SUN_MIE 0.99
      #define PHASE_CONST ((1.0 / (8.0 * PI) * (1.0 - SUN_MIE*SUN_MIE)))
      float h = (1.0 + (SUN_MIE*SUN_MIE)) + (-2.0*SUN_MIE)*saturate(nu);
      float phaseM = PHASE_CONST / (h * sqrt(h));
      float hidePhase = saturate(nu*(1.0/((sunInnerRadius+sunMaxRadius)*0.5-sunMaxRadius))-sunMaxRadius/((sunInnerRadius+sunMaxRadius)*0.5-sunMaxRadius));

      //float sun = saturate(clamp(pow2(inscA),0.002, 0.00675) * phaseM);
      sun += saturate(0.0015 * phaseM)*(untransmittance+0.005)*hidePhase;
      //if (nu > sunOuterRadius)
      return sqrt(transmittance)*sun_color.rgb*sun;
    }
    bool visibleSun(float nu)
    {
      return nu > SUN_MAX_VISIBLE_NU;
    }

    float3 calcSunColor(float nu, float3 transmittance, half3 sun_color)
    {
      BRANCH
      if (!visibleSun(nu))
        return float3(0,0,0);
      return calcSunColorInt(nu, transmittance, sun_color);
    }
  }
endmacro
