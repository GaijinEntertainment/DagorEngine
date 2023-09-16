include "clouds2/cloudsShadowVolume.sh"
include "clouds2/clouds_alt_fraction.sh"

int skies_use_2d_shadows;

texture clouds_shadows_2d;

macro INIT_CLOUDS_SHADOW_BASE_COMMON(light_dir_x, light_dir_y, light_dir_z, code)
  (code) {
    world_pos_to_clouds_alt@f4 = (0.001/clouds_thickness2, -clouds_start_altitude2/clouds_thickness2,
      0.5 + clouds_origin_offset.x/clouds_weather_size, 0.5 + clouds_origin_offset.y/clouds_weather_size);
    clouds_weather_inv_size__alt_scale@f4 = (1/clouds_weather_size,
      light_dir_x*clouds_thickness2*-1000/max(0.02, light_dir_y),
      light_dir_z*clouds_thickness2*-1000/max(0.02, light_dir_y),
      0//1000*skies_planet_radius
      );
      //clouds_shadow_light_dir@f4 = (light_dir_x, light_dir_y, light_dir_z, 1000*1000*(clouds_start_altitude2 + skies_planet_radius)*(clouds_start_altitude2 + skies_planet_radius));

    clouds_hole_pos@tex = clouds_hole_pos_tex hlsl {Texture2D<float4> clouds_hole_pos@tex;};
  }
endmacro

//reference shadow implementation uses volume texture for shadowing, so it is correct for planes, mountains, etc
//it is teeny tiny bit slower, and just a bit less beatiful though (we can actually improve visuals, by additionally blurring just the lowest slice of volume texture)
macro INIT_CLOUDS_SHADOW_BASE_REF(light_dir_x, light_dir_y, light_dir_z, code)
  INIT_CLOUDS_SHADOWS_VOLUME(code)
  INIT_CLOUDS_SHADOW_BASE_COMMON(light_dir_x, light_dir_y, light_dir_z, code)
endmacro

macro USE_CLOUDS_SHADOW_BASE_REF(code)
  hlsl(code) {
    #define INV_WEATHER_SIZE (clouds_weather_inv_size__alt_scale.x)
  }
  USE_CLOUDS_SHADOWS_VOLUME(code)
  hlsl(code) {
    float base_clouds_transmittance(float3 worldPos)
    {
      #if MOBILE_DEVICE
        return 1.0;
      #else
        float alt = world_pos_to_clouds_alt.x*worldPos.y + world_pos_to_clouds_alt.y;
        //#define ACCURATE_SHADOW_INTERSECTION 1//correct is not making much better quality
        #if ACCURATE_SHADOW_INTERSECTION
          if (alt < 0)
          {
            float planet_radius_meters = clouds_weather_inv_size__alt_scale.w;
            float clouds_start_radius_meters_sq = clouds_shadow_light_dir.w;
            //float r = length(worldPos.xyz-float3(0,planet_radius_meters,0));
            float r = worldPos.y+planet_radius_meters;
            Area discriminant = r * r * (clouds_shadow_light_dir.y * clouds_shadow_light_dir.y - 1.0) + clouds_start_radius_meters_sq;
            worldPos += clouds_shadow_light_dir.xyz*max(0, -r * clouds_shadow_light_dir.y + sqrt(max(0, discriminant)));
          }
        #else
          worldPos.xz = clouds_weather_inv_size__alt_scale.yz*min(0, alt) + worldPos.xz;
        #endif
        //that should be actually calculated with correct lighting attenuation and then blurred.
        //we can do that for 2d
        return getCloudsShadows3d(float3(worldPos.xz*clouds_weather_inv_size__alt_scale.x + world_pos_to_clouds_alt.zw + clouds_hole_pos[uint2(0, 0)].zw, max(0, alt))).x;
      #endif
    }
    float clouds_shadow_from_extinction(float extinction)
    {
      #if MOBILE_DEVICE
        return 1.0;
      #else
        //to avoid 2 pow, we use hardcoded Multiple Scattering attenuation and contribution
        extinction*=extinction;
        float extinction3 = extinction*extinction*extinction;
        return extinction*(0.4 + extinction3*0.6);
      #endif
    }
    float clouds_shadow(float3 worldPos)
    {
      #if MOBILE_DEVICE
        return 1.0;
      #else
        return clouds_shadow_from_extinction(base_clouds_transmittance(worldPos));
      #endif
    }
  }
endmacro

//2d clouds shadow is a fraction faster and since we blur shadows it is looking better
//however, if something is ABOVE lower level clouds, then it is incorrect - it can't know if we are within shadows or not
// while it is possible to implement ESM to try to address it, actually we have reference implementation above
// if your project is flightsim - use reference implementation
macro INIT_CLOUDS_SHADOW_BASE_2D(light_dir_x, light_dir_y, light_dir_z, code)
  INIT_CLOUDS_SHADOW_BASE_COMMON(light_dir_x, light_dir_y, light_dir_z, code)
  (code) {
    clouds_shadows_2d@smp2d = clouds_shadows_2d;
  }
endmacro

macro USE_CLOUDS_SHADOW_BASE_2D(code)
  hlsl(code) {
    half clouds_shadow(float3 worldPos)
    {
      #if MOBILE_DEVICE
        return 1.0;
      #else
        float alt = world_pos_to_clouds_alt.x*worldPos.y + world_pos_to_clouds_alt.y;
        worldPos.xz = clouds_weather_inv_size__alt_scale.yz*alt + worldPos.xz;
        //that should be actually calculated with correct lighting attenuation and then blurred.
        //we can do that for 2d
        float shadow = tex2Dlod(clouds_shadows_2d, float4(worldPos.xz*clouds_weather_inv_size__alt_scale.x + world_pos_to_clouds_alt.zw + clouds_hole_pos[uint2(0, 0)].zw, 0, 0)).x;
        return lerp(shadow,1, max(alt,0));
      #endif
    }
  }
endmacro

//this auto is for SAMPLES ONLY!!
//in your project choose either ref or 2d!
macro INIT_CLOUDS_SHADOW_BASE_AUTO(light_dir_x, light_dir_y, light_dir_z, code)
  INIT_CLOUDS_SHADOW_BASE_2D(light_dir_x, light_dir_y, light_dir_z, code)
  INIT_CLOUDS_SHADOWS_VOLUME(code)
  (code) {skies_use_2d_shadows@f1 = (skies_use_2d_shadows);}
endmacro

macro USE_CLOUDS_SHADOW_BASE_AUTO(code)
  hlsl(code) {
    #define clouds_shadow clouds_shadow_ref
  }
  USE_CLOUDS_SHADOW_BASE_REF(code)
  hlsl(code) {
    #undef clouds_shadow
    #define clouds_shadow clouds_shadow_2d
  }
  USE_CLOUDS_SHADOW_BASE_2D(code)
  hlsl(code) {
    #undef clouds_shadow
    half clouds_shadow(float3 worldPos)
    {
      if (skies_use_2d_shadows)
        return clouds_shadow_2d(worldPos);
      return clouds_shadow_ref(worldPos);
    }
  }
endmacro

//by default use volume shadows, as they don't need any flags
float4 light_shadow_dir;

macro INIT_CLOUDS_SHADOW_BASE(legacy_light_dir_y, code)
  INIT_CLOUDS_SHADOW_BASE_REF(light_shadow_dir.x, light_shadow_dir.y, light_shadow_dir.z, code)
endmacro
macro USE_CLOUDS_SHADOW_BASE(code)
  USE_CLOUDS_SHADOW_BASE_REF(code)
endmacro
macro INIT_CLOUDS_SHADOW(legacy_light_dir_y)
  INIT_CLOUDS_SHADOW_BASE(legacy_light_dir_y, ps)
endmacro
macro USE_CLOUDS_SHADOW()
  USE_CLOUDS_SHADOW_BASE(ps)
endmacro
