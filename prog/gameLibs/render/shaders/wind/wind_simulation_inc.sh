int fluid_wind_status = 0;
interval fluid_wind_status: fluid_wind_off < 1, fluid_wind_on;

texture fluid_wind_tex;
float4 fluid_wind_origin_delta = float4(0, 0, 0, 0);
float4 fluid_wind_origin;
float4 fluid_wind_world_size;
float fluid_wind_multiplier = 20; // FIXME: after tuning this parameter, it should be replaced with code constant

texture ambient_wind_tex;
texture wind_perlin_tex;
float4 ambient_wind__speed__current_time__previous_time = (1.811, 0, 0, 0);
float4 ambient_wind_noise__speed__scale__perpendicular__strength = (-0.640, -0.537, 0.014, 1.182);
float4 ambient_wind_grass_noise__speed__scale__perpendicular__strength = (-0.640, -0.537, 0.014, 1.182);
float4 ambient_wind_map_scale__offset = (1. / 4096, 1. / 4096, 0.5, 0.5);

macro INIT_FLUID_WIND(stage)
  (stage) {
      fluid_wind_tex@smp3d = fluid_wind_tex;
      fluid_wind_origin@f4 = fluid_wind_origin;
      fluid_wind_world_size@f4 = fluid_wind_world_size;
      fluid_wind_multiplier@f1 = (fluid_wind_multiplier);
  }
endmacro

macro INIT_AMBIENT_WIND_BASE(stage)
  (stage) {
    ambient_wind__speed__current_time__previous_time@f4 = ambient_wind__speed__current_time__previous_time;
    ambient_wind_noise__speed__scale__perpendicular__strength@f4 = ambient_wind_noise__speed__scale__perpendicular__strength;
    ambient_wind_grass_noise__speed__scale__perpendicular__strength@f4 = ambient_wind_grass_noise__speed__scale__perpendicular__strength;
    ambient_wind_map_scale__offset@f4 = ambient_wind_map_scale__offset;
  }
endmacro

macro INIT_AMBIENT_WIND(stage)
  INIT_AMBIENT_WIND_BASE(stage)
  (stage) {
    ambient_wind_tex@smp2d = ambient_wind_tex;
  }
endmacro

macro USE_FLUID_WIND(stage)
  if (fluid_wind_status == fluid_wind_on)
  {
    hlsl(stage) {
      #define FLUID_WIND_ON 1
      float3 sampleFluidWind(float3 pos)
      {
        float3 tcSpeed = (pos.xyz - fluid_wind_origin.xyz) / fluid_wind_world_size.xyz + 0.5;
        float4 speed = tex3Dlod(fluid_wind_tex, float4(tcSpeed.xzy, 0));
        return speed.xzy;
      }
    }
  }
endmacro

macro USE_AMBIENT_WIND(stage)
  hlsl(stage) {
    #include "sample_wind_common.hlsl"

    float3 sampleAmbientWind(float3 world_pos, float4 noise_params, float current_time)
    {
      float2 ambient_dir = sample_ambient_wind_dir_base(world_pos);
#if USE_SIMPLE_NOISE_TEX //noise strength is already increasing with wind strength
      float2 ambient_wind = ambient_dir;
#else
      float2 ambient_wind = ambient_dir * get_ambient_wind_speed();
#endif

      float2 noise_speed = ambient_dir * noise_params.x;

      float2 ambient_windT = float2(ambient_wind.y, -ambient_wind.x);
      float noise_scale = noise_params.y;
      float noise_perpendicular = noise_params.z;

      float3 noisePos = float3(0, 0, 0);
      noisePos.xz = (world_pos.xz + noise_speed * current_time) * noise_scale;
#if USE_SIMPLE_NOISE_TEX
      float2 noise = float2(tex2Dlod(wind_perlin_tex, float4(noisePos.xz, 0, 0)).r, tex2Dlod(wind_perlin_tex, float4(noisePos.xz + 0.5f, 0, 0)).r);
#else
      float2 noise = 2 * (tex3Dlod(perlin_noise3d, float4(noisePos, 0)).xz - 0.5);
#endif

      float3 res = 0;
      res.xz = ambient_wind * (1 + noise_params.w * noise.x);
      res.xz += ambient_windT * noise_perpendicular * noise.y;

      return res;
    }
  }
endmacro

macro INIT_USE_COMBINED_WIND_SIMPLE_NOISE_TEX(stage)
  INIT_COMBINED_WIND(stage)
  INIT_WIND_SIMPLE_NOISE_TEX(stage)
  USE_COMBINED_WIND(stage)
endmacro

macro INIT_WIND_SIMPLE_NOISE_TEX(stage)
  hlsl(stage)
  {
    #define USE_SIMPLE_NOISE_TEX 1
  }
  (stage)
  {
    wind_perlin_tex@smp2d = wind_perlin_tex;
  }
endmacro

macro INIT_USE_COMBINED_WIND(stage)
  INIT_COMBINED_WIND(stage)
  (stage)
  {
    perlin_noise3d@smp3d = perlin_noise3d;
  }
  USE_COMBINED_WIND(stage)
endmacro

macro INIT_COMBINED_WIND(stage)
  if (fluid_wind_status == fluid_wind_on)
  {
    INIT_FLUID_WIND(stage)
  }
  INIT_AMBIENT_WIND(stage)
endmacro

macro USE_COMBINED_WIND(stage)
  USE_FLUID_WIND(stage)
  USE_AMBIENT_WIND(stage)

  hlsl(stage) {

    float3 sampleWind(float3 pos, float4 noise_params, float3 velocity, float noise_speed_mult, float time)
    {
      noise_params.x *= noise_speed_mult;
      float3 wind = sampleAmbientWind(pos, noise_params, time) - velocity;
    #if FLUID_WIND_ON
      wind += fluid_wind_multiplier * sampleFluidWind(pos);
    #endif
      return wind;
    }

    float3 sampleWindCurrentTime(float3 pos, float noise_speed_mult, float3 velocity)
    {
      return sampleWind(pos, ambient_wind_noise__speed__scale__perpendicular__strength,
        velocity, noise_speed_mult, ambient_wind__speed__current_time__previous_time.y);
    }

    float3 sampleWindPreviousTime(float3 pos, float noise_speed_mult, float3 velocity)
    {
      return sampleWind(pos, ambient_wind_noise__speed__scale__perpendicular__strength,
        velocity, noise_speed_mult, ambient_wind__speed__current_time__previous_time.z);
    }

    float3 sampleWindGrassCurrentTime(float3 pos, float noise_speed_mult, float3 velocity, float time_override = 0)
    {
      return sampleWind(pos, ambient_wind_grass_noise__speed__scale__perpendicular__strength,
        velocity, noise_speed_mult, ambient_wind__speed__current_time__previous_time.y);
    }

    float3 sampleWindPreviousGrassTime(float3 pos, float noise_speed_mult, float3 velocity)
    {
      return sampleWind(pos, ambient_wind_grass_noise__speed__scale__perpendicular__strength,
        velocity, noise_speed_mult, ambient_wind__speed__current_time__previous_time.z);
    }
  }
endmacro
