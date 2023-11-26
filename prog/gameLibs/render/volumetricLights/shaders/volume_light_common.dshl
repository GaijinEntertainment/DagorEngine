
hlsl {
  #include "volume_lights_common_def.hlsli"
  #include "heightFogNode.hlsli"
  #include "pcg_hash.hlsl"
  #include "phase_functions.hlsl"
  #include "volume_lights_common.hlsl"

  // #define DEBUG_VOLFOG_BLEND_SCREEN_RATIO 0.5 // use it for debugging

  #define transformed_zn_zfar zn_zfar
}



macro VOLUME_DEPTH_CONVERSION(code)
  (code){volfog_froxel_range_params@f2 = (volfog_froxel_range_params.x, volfog_froxel_range_params.y, 0, 0);}
  hlsl(code) {
    float depth_to_volume_pos(float v)
    {
      return depth_to_volume_pos(v, volfog_froxel_range_params.y);
    }
    float volume_pos_to_depth(float v)
    {
      return volume_pos_to_depth(v, volfog_froxel_range_params.x);
    }
  }
endmacro


float nbs_clouds_start_altitude2_meters = 100000; // high default value to not restrict volfog, justincase
float4 froxel_fog_fading_params = (0,1,0,0);

macro INIT_AND_USE_VOLFOG_MODIFIERS(code)
  (code)
  {
    water_level__clouds_start_alt@f2 = (water_level, nbs_clouds_start_altitude2_meters, 0, 0);
    froxel_fog_fading_params@f4 = froxel_fog_fading_params;
  }
  hlsl(code)
  {
    #define water_level (water_level__clouds_start_alt.x)
    #define nbs_clouds_start_altitude2_meters (water_level__clouds_start_alt.y)
  }
endmacro


float4 prev_globtm_psf_0;
float4 prev_globtm_psf_1;
float4 prev_globtm_psf_2;
float4 prev_globtm_psf_3;

macro USE_PREV_TC(code)
  (code) {
    prev_globtm_psf@f44 = { prev_globtm_psf_0, prev_globtm_psf_1, prev_globtm_psf_2, prev_globtm_psf_3 };
  }
  hlsl(code) {
    float2 getPrevTc(float3 world_pos, out float3 prev_clipSpace)
    {
      float4 lastClipSpacePos = mulPointTm(world_pos, prev_globtm_psf);
      prev_clipSpace = lastClipSpacePos.w<=0.01 ? float3(2,2,2) : lastClipSpacePos.xyz/lastClipSpacePos.w;
      return prev_clipSpace.xy*float2(0.5,-0.5) + float2(0.5,0.5);
    }
  }
endmacro


float4 jitter_ray_offset;

texture volfog_poisson_samples;

macro INIT_JITTER_PARAMS(code)
  (code) {
    // currently, only .w is used, which is frame index {0, 1}
    jitter_ray_offset@f4 = jitter_ray_offset;
    volfog_poisson_samples@tex2d = volfog_poisson_samples;
  }
endmacro
