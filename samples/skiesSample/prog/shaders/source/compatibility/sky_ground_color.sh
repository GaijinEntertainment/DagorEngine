int multiple_scattering_not_supported = 1;
macro INIT_SKY_GROUND_COLOR()
endmacro

macro GET_SKY_GROUND_COLOR_COMPATIBILITY()
  (ps) {
    sun_light_dir_sky_ground@f3 = from_sun_direction;
    sun_color_sky_ground@f3 = sun_color_0;
  }

  hlsl(ps) {
    half3 get_ground_lit_color_compatibility()
    {
      half absNoL = max(0, -sun_light_dir_sky_ground.y);
      half3 averageGroundColor = half3(0.09, 0.223, 0.239);    // The average color of the earth, as seen from a satellite.
      return averageGroundColor * (absNoL * sun_color_sky_ground + enviUp);
    }
  }
endmacro


macro GET_SKY_GROUND_COLOR()
  INIT_SKY_UP_DIFFUSE()
  USE_SKY_UP_DIFFUSE()
  GET_SKY_GROUND_COLOR_COMPATIBILITY()

  hlsl(ps) {
    #define HAVE_GROUND_COLOR_FUNCTION 1
    #define PreparedGroundData float

    void prepare_ground_data(float3 v, float distToGround, PreparedGroundData data)
    {
      data = 0;
    }

    float3 get_ground_lit_color(float3 x_, float3 s_, float3 v, float3 sunColor, PreparedGroundData data)
    {
      return get_ground_lit_color_compatibility();
    }
  }

endmacro
