include "sky_shader_global.sh"
include "viewVecVS.sh"
include "csm.sh"
include "heightmap_common.sh"
include "static_shadow.sh"
include "fom_shadows.sh"
include "skies_shadows.sh"
include "depth_above.sh"
include "gpu_occlusion.sh"
include "clustered/lights_cb.sh"
include "dagi_volmap_gi.sh"
include "gi_demo.sh"
include "light_mask_helpers.sh"

include "volume_light_common.sh"
include "volume_light_hardcoded_media.sh"


// summarizing: current approach is direct implementation of frostbite paper
  //dice: http://advances.realtimerendering.com/s2015/Frostbite%20PB%20and%20unified%20volumetrics.pptx
// however it is currently NOT the fastest (that would be sun shadow + extinction (2 channel) 3d texture only, for one sun + ambient)
// not the best in terms of quality of implementation of phase functions (phase functions are not applied in initial pass, and instead averaged and applied in integration pass with some heurestics to determ sun/ambient)
// not the fastest/best for even local lights support with (important) limited number of 'good' shadow casting lights (with shadow reprojection).

// if we limit reprojection shadow casting light to one or 3, ideal solution would be reproject only shadows for sun + 3 local lights, and apply phase functions (and reprojected shadows) directly in initial scattering pass
// that seem to be ideal solution in real-case scenario.
// however, since in the moment we don't have _ANY_ local lights (and no local shadow casting lights at all!), but we plan to have them, I will currently keep generalized Frostbite solution (and return to it later).


//in the moment, the only scattering available is momochromatic, and only light sources are ambient and sun
//so, we can easily replace inscatterlightRGB with just (sun shadow, scatteringAmount/density) and inscatterlightRGB = (sun_phase*sun_color*sun_shadow + amb_phase*amb_color)*scatteringAmount
// which is more, in the moment there is no absorbtion, so muE (extinction) equals muS (scatteringAmount), so we already have it in volume texture, and can have only 2 channel initial_scattering 3d texture
// however, if we will use more complicated lighting solution (multiple lights) and/or colored scattering, current scheme is more generalized, as it is allowed multiple lights.
// unfortunately applying phase functions on initial scattering phase doesn't work well with temporal upsampling/reprojection.
// temporal can be replaced with multiple shadow samples per ray, but that is slightly slower (and generally worser quality)

// temporal can be also made temporal on shadows only (i.e. run shadows compute pass first, with temporal shadows, and then apply phase functions and everything without temporal reprojection)
// that will work perfectly (and expectedly even faster or the same speed, since lower bandwidth, 1channel even 8 bit texture will suffice), but all local lights will not get any temporal shadows upsample.
// local lights shadows require even more antiliasing due to it's local/aliased nature. However, currently we don't have local lights with shadows, so this will work
// it is also possible to store not only one separate shadow for temporal reprojection, but 2 or 4 shadows (in 8bits per channel), or even pack some shadows per channel (64 bit channel 3d texture with 8 8bit sshadows, or 6-8bit +1 16bit)
// all other shadows can be also applied, but without reprojection, directly in initial inscatter pass


macro USE_FROXEL_JITTERED_TC(code, frame_id, inv_resolution)
  hlsl(code){
    float3 calc_jittered_tc(uint3 dtId)
    {
      return calc_jittered_tc(dtId, (uint)frame_id, inv_resolution.xyz);
    }
  }
endmacro

hlsl {
  #define MAX_VOLUME_DEPTH_RANGE volume_resolution.w

  //currently there are no local lights. It is not clear if they should have separate phase at all
  #define HAS_LOCAL_LIGHTS_PHASE 0

  #include <phase_functions.hlsl>

  bool is_voxel_occluded(half extinction)
  {
    return !isfinite(extinction);
  }
  bool is_voxel_occluded(half4 packedData)
  {
    // technically only .w can be NaN, but this way we can safely avoid corrupted results
    return !isfinite(dot(packedData,1));
  }

  // to filter the final result too, justincase
  half4 nanfilter(half4 val)
  {
    return isfinite(dot(val, 1)) ? val : float4(0,0,0,1);
  }

  float3 get_swizzled_color(float3 color, uint frame_id)
  {
    return frame_id == 1 ? color.brg : (frame_id == 2 ? color.gbr : color.rgb);
  }
  float3 get_swizzled_color_inv(float3 color, uint frame_id)
  {
    return frame_id == 1 ? color.gbr : (frame_id == 2 ? color.brg : color.rgb);
  }

}


int froxel_fog_dispatch_mode = 0;
interval froxel_fog_dispatch_mode: separate<1, merged_parallel<2, merged_linear;


texture initial_inscatter;
texture initial_extinction;
texture initial_phase;

texture volfog_occlusion;
texture prev_volfog_occlusion;

texture skies_ms_texture;
texture skies_transmittance_texture;

float volfog_multi_scatter_ao_weight = 1.0;

float skies_scattering_effect = 1.; // it's only purpose is cuisine royale eclipse.

int4 channel_swizzle_indices = (1, 0, 0, 0);

int AccumulatedInscatter_view_result_no = 7;
int volfog_occlusion_no = 7;
int initial_media_no = 7;

int AccumulatedInscatter_no = 4;
int initial_phase_no = 5;
int initial_inscatter_no = 6;
int initial_extinction_no = 7;

// TODO: deprecate it if the merged version has no problems in practice
shader view_result_inscatter_cs
{
  supports global_const_block;
  hlsl{
    // needed for finalShadowFromShadowTerm
    // (other shadow functions are not needed here)
    #define SHADOWMAP_ENABLED 1
    #define getShadow(a, b, c, d) 1 // unused
  }
  (cs) {
    inv_resolution@f4 = (inv_volfog_froxel_volume_res.x, inv_volfog_froxel_volume_res.y, inv_volfog_froxel_volume_res.z, volfog_froxel_range_params.x);
    volfog_froxel_volume_res@f3 = (volfog_froxel_volume_res);
    local_view_z@f3 = local_view_z;
    initial_inscatter@tex = initial_inscatter hlsl { Texture3D<float3>initial_inscatter@tex; }
    initial_extinction@tex = initial_extinction hlsl { Texture3D<float3>initial_extinction@tex; }
    initial_phase@tex = initial_phase hlsl { Texture3D<float>initial_phase@tex; }
    volfog_occlusion@tex2d = volfog_occlusion;
    prev_volfog_occlusion@tex2d = prev_volfog_occlusion;
    volfog_multi_scatter_ao_weight@f1 = (max(min(volfog_multi_scatter_ao_weight, 1), 0));

    channel_swizzle_indices@i2 = (channel_swizzle_indices.x, channel_swizzle_indices.y, 0, 0);
    volfog_blended_slice_cnt@i1 = (volfog_blended_slice_cnt);
  }

  // for scattering sample:
  (cs) {
    world_view_pos@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, time_phase(0, 0));
    skies_primary_sun_light_dir@f3 = skies_primary_sun_light_dir;
    skies_primary_sun_color@f3 = (skies_primary_sun_color.x * skies_scattering_effect,
                                  skies_primary_sun_color.y * skies_scattering_effect,
                                  skies_primary_sun_color.z * skies_scattering_effect, 0);
    skies_transmittance_texture@smp2d = skies_transmittance_texture;
    skies_ms_texture@smp2d = skies_ms_texture;

    // last layer is volfog-only (no scattering)
    AccumulatedInscatter@uav: register(AccumulatedInscatter_view_result_no) hlsl {
      RWTexture3D<float4> AccumulatedInscatter@uav;
    }
  }
  ATMO(cs)
  GET_ATMO(cs)

  VIEW_VEC_OPTIMIZED(cs)

  VOLUME_DEPTH_CONVERSION(cs)
  ENABLE_ASSERT(cs)

  hlsl(cs) {

    //dice: http://advances.realtimerendering.com/s2015/Frostbite%20PB%20and%20unified%20volumetrics.pptx
    //which is based on https://bartwronski.files.wordpress.com/2014/08/bwronski_volumetric_fog_siggraph2014.pdf but with a lot of changes

    float calcPhase(float viewDotSun, float3 inscattering, float3 muE)
    {
       #if USE_SEPARATE_PHASE
         //input phase for sun and ambient are actually straightforward
         float sunPhase = calc_sun_phase(viewDotSun, from_sun_direction);
         float ambPhase = phaseFunctionConst();
         float sunPhaseAmount = saturate(dot(inscattering/muE - get_base_ambient_color(), 0.333) );//fixme: actually it will go to sunPhase even if localLights are on
         float phaseSunAmb = lerp(ambPhase, sunPhase, sunPhaseAmount);
         float phaseFunction = phaseSunAmb;
         #if HAS_LOCAL_LIGHTS_PHASE
         float localPhase = texelFetch(initial_phase, int3(dtId.xy, z), 0).r;
         phaseFunction = localPhase>0 ? (phaseFunction+localPhase)*0.5 : phaseFunction;//replace with logarithmic
         #endif
       #else
         float phaseFunction = 1;//premultiplied by phase
       #endif
       return phaseFunction;
    }

    float4 sampleVolfog(int3 dtId, out float2 out_atmo_scattering_shadow2)
    {
      float3 packedExtinction = texelFetch(initial_extinction, dtId, 0).xyz;
      float extinction = packedExtinction.x;
      out_atmo_scattering_shadow2 = packedExtinction.yz;
      BRANCH
      if (extinction > 0.0001) // improves perf assuming mostly empty spaces
        return float4(get_swizzled_color(texelFetch(initial_inscatter, dtId, 0).rgb, channel_swizzle_indices.x), extinction);
      return 0;
    }

    [numthreads( RESULT_WARP_SIZE, RESULT_WARP_SIZE, 1 )]
    void RayMarchThroughVolume( uint2 dtId : SV_DispatchThreadID )
    {
      float2 screenTc = (dtId.xy+0.5)*inv_resolution.xy;
      float3 viewVect = getViewVecOptimized(screenTc.xy);
      float3 view = normalize(viewVect);
      float normalizedFactor = rcp(dot(local_view_z, view));
      float viewDotSun = -dot(from_sun_direction, view);

      int sliceCnt = (int)volfog_froxel_volume_res.z;
      int curActiveLayerEnd = min((int)calc_last_active_froxel_layer(volfog_occlusion, dtId.xy) + 1, sliceCnt);
      int prevActiveLayerEnd = min((int)calc_last_active_froxel_layer(prev_volfog_occlusion, dtId.xy) + 1, sliceCnt);
      int maxActiveLayerEnd = max(curActiveLayerEnd, prevActiveLayerEnd);

      float4 accumulatedScatteringVolfogOnly = float4(0,0,0,1);
      float cLen = 0;

      float3 accumulatedScattering = float3(0,0,0);
      float3 accumulatedScatteringExt = float3(1,1,1);

      Length r = world_view_pos.y/1000 + theAtmosphere.bottom_radius;
      Number mu = view.y;
      Number primaryNu = dot(view.xzy, skies_primary_sun_light_dir);
      Number primaryMu_s = skies_primary_sun_light_dir.z;
      Number primaryRayleighPhaseValue = RayleighPhaseFunction(primaryNu);
      Number primaryMiePhaseValue = primaryRayleighPhaseValue*MiePhaseFunctionDivideByRayleighOptimized(theAtmosphere.mie_phase_consts, primaryNu);
      MultipleScatteringTexture multiple_scattering_approx = SamplerTexture2DFromName(skies_ms_texture);
      TransmittanceTexture transmittance_texture = SamplerTexture2DFromName(skies_transmittance_texture);
      float3 scatteringMul = skies_primary_sun_color.rgb * theAtmosphere.solar_irradiance / 1000;

      float4 accumulatedCombinedFog = float4(0,0,0,1);
      int blendingStartSliceId = sliceCnt - volfog_blended_slice_cnt - 1;
      float4 accumulatedVolfogAtBlending = float4(0,0,0,1);

      for (int z = 0; z < curActiveLayerEnd; ++z)
      {
        float2 atmoScatteringShadows;
        float4 volfogvalues = sampleVolfog(int3(dtId.xy, z), atmoScatteringShadows);

        float nextDepth = volume_pos_to_depth(max(float(z), 0.5)*inv_resolution.z);
        float nextLen = nextDepth * normalizedFactor;
        float stepLen = nextLen - cLen;
        cLen = nextLen;

        float4 result;
        FLATTEN // special case, loop range already filters occluded voxels
        if (!is_voxel_occluded(volfogvalues))
        {
          float3 viewToPoint = nextDepth*viewVect;
          float3 worldPos = viewToPoint + world_view_pos.xyz;

          float3 scatteringPrimaryS, scatteringExtinction;
          {
            Length d = nextLen/1000;
            Length r_d = ClampRadius(theAtmosphere, SafeSqrt(d * d + 2.0 * r * mu * d + r * r));
            Number primaryMu_s_d = ClampCosine((r * primaryMu_s + d * primaryNu) / r_d);

            MediumSampleRGB medium = SampleMediumFull(theAtmosphere, r_d-theAtmosphere.bottom_radius, worldPos);
            float3 primaryTransmittanceToSun = GetTransmittanceToSun( theAtmosphere, transmittance_texture, r_d, primaryMu_s_d);

            float primaryScatterShadow = atmoScatteringShadows.x;
            float temporalBlurredAO = atmoScatteringShadows.y;
            primaryTransmittanceToSun *= finalShadowFromShadowTerm(primaryScatterShadow);

            float3 primaryPhaseTimesScattering = medium.scatteringMie * primaryMiePhaseValue + medium.scatteringRay * primaryRayleighPhaseValue;
            float3 primaryMultiScatteredLuminance = GetMultipleScattering(theAtmosphere, multiple_scattering_approx, r_d, primaryMu_s_d) * medium.scattering;

            primaryMultiScatteredLuminance *= lerp(1, temporalBlurredAO, volfog_multi_scatter_ao_weight);

            scatteringPrimaryS = (primaryTransmittanceToSun * primaryPhaseTimesScattering + primaryMultiScatteredLuminance) * scatteringMul;
            scatteringExtinction = medium.extinction.rgb / 1000;
            // no secondary values to keep it realatively cheap
          }

          float4 combinedTransmittance = exp(-float4(scatteringExtinction.xyz, volfogvalues.a) * stepLen);

          { // scattering-only
            float3 muE = scatteringExtinction;
            float phaseFunction = calcPhase(viewDotSun, scatteringPrimaryS, muE);
            float3 currentTransmittance = combinedTransmittance.xyz; //exp(-muE * stepLen);
            float3 S = scatteringPrimaryS*phaseFunction;
            //https://www.shadertoy.com/view/XlBSRz
            float3 Sint = (S - S * currentTransmittance) / max(VOLFOG_MEDIA_DENSITY_EPS, muE); // integrate along the current step segment
            accumulatedScattering += accumulatedScatteringExt * Sint; // accumulate and also take into account the transmittance from previous steps
            accumulatedScatteringExt *= currentTransmittance;
            // Evaluate transmittance to view independentely
          }
          float volfogTransmittance = combinedTransmittance.w;
#ifdef DEBUG_VOLFOG_BLEND_SCREEN_RATIO
          if (screenTc.x < (1.0 + VOLFOG_TEST_BLEND_SCREEN_RATIO) / 2)
#endif
          { // volfog-only
            float muE = volfogvalues.a;
            float3 scatteredLight = accumulatedScatteringVolfogOnly.rgb;
            float transmittance = accumulatedScatteringVolfogOnly.a;

            float phaseFunction = calcPhase(viewDotSun, volfogvalues.rgb, muE);

            float3 S = volfogvalues.rgb*phaseFunction;//float3 S = evaluateLight(p) * muS * phaseFunction()* volumetricShadow(p,lightPos);// incoming light

            //https://www.shadertoy.com/view/XlBSRz
            float3 Sint = (S - S * volfogTransmittance) / max(VOLFOG_MEDIA_DENSITY_EPS, muE); // integrate along the current step segment
            scatteredLight += transmittance * Sint; // accumulate and also take into account the transmittance from previous steps
            // Evaluate transmittance to view independentely
            transmittance *= volfogTransmittance;

            accumulatedScatteringVolfogOnly.rgb = scatteredLight;
            accumulatedScatteringVolfogOnly.a = transmittance;

            FLATTEN if (z <= blendingStartSliceId)
              accumulatedVolfogAtBlending = accumulatedScatteringVolfogOnly;
          }

          accumulatedCombinedFog = nanfilter(float4(
            accumulatedScattering * accumulatedScatteringVolfogOnly.a + accumulatedScatteringVolfogOnly.rgb,
            dot(accumulatedScatteringExt, 1.0/3) * accumulatedScatteringVolfogOnly.a)); // volfog+scattering
        }
        AccumulatedInscatter[int3(dtId.xy, z)] = accumulatedCombinedFog;
      }

      // clear layers which became occluded since last frame
      for (int2 z2 = curActiveLayerEnd; z2.x < maxActiveLayerEnd; ++z2) // int2 due to a spri-v (parser) bug
        AccumulatedInscatter[int4(dtId.xy, z2).xyw] = accumulatedCombinedFog; // clearing occluded froxels with last valid value in a ray (this hides some weird HW dependent bugs)

      AccumulatedInscatter[int3(dtId.xy, sliceCnt)] = nanfilter(accumulatedVolfogAtBlending);
    }
  }
  compile("target_cs", "RayMarchThroughVolume");
}


shader volfog_occlusion_cs
{
  VIEW_VEC_OPTIMIZED(cs)

  // we increase +-1 froxel to prevent filtering/blending with jittered occluded
  local float4 froxel_offset = (-0.5*inv_volfog_froxel_volume_res.x, -0.5*inv_volfog_froxel_volume_res.y, 0.5*inv_volfog_froxel_volume_res.x, 0.5*inv_volfog_froxel_volume_res.y);
  (cs)
  {
    inv_volfog_froxel_volume_res@f2 = (inv_volfog_froxel_volume_res);
    occlusion_resolution@f3 = (1.0 / inv_volfog_froxel_volume_res.x, 1.0 / inv_volfog_froxel_volume_res.y,1.0 / inv_volfog_froxel_volume_res.z,0);
    bbox_offset@f4 = (froxel_offset.x, froxel_offset.y, froxel_offset.z + inv_volfog_froxel_volume_res.x, froxel_offset.w + inv_volfog_froxel_volume_res.y);
    volume_z_offset@f1 = (inv_volfog_froxel_volume_res.z,0,0,0); // adds one slice, so reprojection work better
    inv_occlusion_resolution@f4 = (inv_volfog_froxel_volume_res.x, inv_volfog_froxel_volume_res.y, inv_volfog_froxel_volume_res.z, volfog_froxel_range_params.x);

    volfog_occlusion@uav: register(volfog_occlusion_no) hlsl {
      RWTexture2D<float> volfog_occlusion@uav;
    }
  }

  INIT_ZNZFAR_STAGE(cs)
  BASE_GPU_OCCLUSION(cs)
  VOLUME_DEPTH_CONVERSION(cs)
  ENABLE_ASSERT(cs)
  hlsl(cs) {
    float checkVolFogBox(float2 coord)
    {
      float lod = 0;
      float closestRawDepth = check_box_occl_visible_tc_base(saturate(coord.xyxy*inv_occlusion_resolution.xyxy + bbox_offset), lod);
      return linearize_z(closestRawDepth, zn_zfar.zw);
    }

    [numthreads( RESULT_WARP_SIZE, RESULT_WARP_SIZE, 1)]
    void occlusion_cs( uint2 dtId : SV_DispatchThreadID )
    {
      static const float MIN_DIST = 4.0; // first few meters are too close, we'd better always shade them even if occluded
      float volumePosZ = depth_to_volume_pos(max(MIN_DIST, checkVolFogBox(float2(dtId)))) + volume_z_offset;
      uint sliceId = ceil(volumePosZ * occlusion_resolution.z);
      volfog_occlusion[dtId] = (sliceId + 0.5) / 255.f;
    }
  }
  compile("target_cs", "occlusion_cs");
}



int froxel_fog_use_debug_media = 0;
interval froxel_fog_use_debug_media: no<1, yes;

shader fill_media_cs
{
  if (froxel_fog_use_debug_media == yes)
  {
    VIEW_VEC_OPTIMIZED(cs)
    INIT_JITTER_PARAMS(cs)

    (cs) {
      local_view_z@f3 = local_view_z;
      inv_resolution@f4 = (inv_volfog_froxel_volume_res.x, inv_volfog_froxel_volume_res.y, inv_volfog_froxel_volume_res.z, volfog_froxel_range_params.x);
      volfog_occlusion@tex2d = volfog_occlusion;
      world_view_pos@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, time_phase(0,0));
    }
    INIT_ZNZFAR_STAGE(cs)

    INIT_AND_USE_VOLFOG_MODIFIERS(cs)
    GET_MEDIA_HARDCODED(cs)

    VOLUME_DEPTH_CONVERSION(cs)
    USE_FROXEL_JITTERED_TC(cs, jitter_ray_offset.w, inv_resolution)
    ENABLE_ASSERT(cs)

    hlsl(cs) {
      #include <hammersley.hlsl>
    }
  }
  (cs) {
    initial_media@uav: register(initial_media_no) hlsl {
      RWTexture3D<float4> initial_media@uav;
    }
  }

  hlsl(cs) {
    [numthreads( MEDIA_WARP_SIZE_X, MEDIA_WARP_SIZE_Y, MEDIA_WARP_SIZE_Z)]
    void media_cs( uint3 dtId : SV_DispatchThreadID )
    {
##if froxel_fog_use_debug_media == no
      initial_media[dtId] = 0;
##else
      BRANCH
      if (is_voxel_id_occluded(dtId))
      {
        initial_media[dtId] = 0;
        return;
      }
      float3 screenTcJittered = calc_jittered_tc(dtId);
      float3 view = getViewVecOptimized(screenTcJittered.xy);
      float cdepth = volume_pos_to_depth(screenTcJittered.z);
      float3 worldPos = cdepth*view + world_view_pos.xyz;
      initial_media[dtId] = get_media(worldPos, screenTcJittered, world_view_pos.w);
##endif
    }
  }
  compile("target_cs", "media_cs");
}

texture prev_initial_inscatter;
texture prev_initial_extinction;
texture initial_media;
float volfog_prev_range_ratio;

int shadows_quality = 1;
interval shadows_quality: low<1, high;

int dynamic_lights_count = 0;
interval dynamic_lights_count: lights_off<1, lights_omni_1<2, lights_spot_1<3, lights_omnispot_1<4, lights_clustered;

float4 volumelight_reprojection_factors = (0.97, 0.9, 0, 0);


macro VOLFOG_INITIAL_COMMON(stage, warp_size_z)

  supports global_const_block;

  if (froxel_fog_dispatch_mode != separate)
  {
    hlsl(stage)
    {
      #define FROXEL_FOG_MERGED_PASSES 1
    }
    (stage)
    {
      prev_volfog_occlusion@tex2d = prev_volfog_occlusion;
    }
  }

  VIEW_VEC_OPTIMIZED(stage)
  INIT_ESM_SHADOW(stage)
  hlsl(stage)
  {
    #define FASTEST_STATIC_SHADOW_PCF 1
    #define DO_NOT_FILTER_GI_AMBIENT_BY_NORMAL_WEIGHT 1
  }

  INIT_BLURRED_DEPTH_ABOVE(stage)
  USE_BLURRED_DEPTH_ABOVE(stage)
  SSGI_USE_VOLMAP_GI_AUTO(stage)

  USE_ESM_SHADOW(stage)

  INIT_FOM_SHADOWS(stage)
  USE_FOM_SHADOWS(stage)

  GI_DEMO(stage)

  USE_PREV_TC(stage)

  ENABLE_ASSERT(stage)

  //we can use csm shadows, but performance is worser. downsample and blur is 0.32, additional csm cost is 0.7msec.
  //however, we can use csm with min filter (more shadowing) and save ESM blur
  //we can use static cascade for other cascades than first 2 as well (will probably be sufficient as well)
  //all in all we can get up to ~ -0.15msec on blur as optimization here (and less memory)

  (stage)
  {
    inv_resolution@f4 = (inv_volfog_froxel_volume_res.x, inv_volfog_froxel_volume_res.y, inv_volfog_froxel_volume_res.z, volfog_froxel_range_params.x);
    jitter_ray_offset@f4 = jitter_ray_offset;
    local_view_z@f3 = local_view_z;
    prev_initial_inscatter@smp3d = prev_initial_inscatter;
    prev_initial_extinction@smp3d = prev_initial_extinction;
    initial_media@tex3d = initial_media;
    volfog_occlusion@tex2d = volfog_occlusion;

    channel_swizzle_indices__volfog_blended_slice_cnt@i3 = (channel_swizzle_indices.x, channel_swizzle_indices.y, volfog_blended_slice_cnt, 0);
    volumelight_reprojection_factors__volfog_froxel_volume_res_z__volfog_multi_scatter_ao_weight@f4 =
      (volumelight_reprojection_factors.x, volumelight_reprojection_factors.y, volfog_froxel_volume_res.z,
      max(min(volfog_multi_scatter_ao_weight, 1), 0));

    skies_primary_sun_light_dir__volfog_prev_range_ratio@f4 =
      (skies_primary_sun_light_dir.x, skies_primary_sun_light_dir.y, skies_primary_sun_light_dir.z, volfog_prev_range_ratio);
  }
  hlsl(stage)
  {
    #define volfog_froxel_range (inv_resolution.w)
    #define channel_swizzle_indices (channel_swizzle_indices__volfog_blended_slice_cnt.xy)
    #define volfog_blended_slice_cnt (channel_swizzle_indices__volfog_blended_slice_cnt.z)
    #define volumelight_reprojection_factors (volumelight_reprojection_factors__volfog_froxel_volume_res_z__volfog_multi_scatter_ao_weight.xy)
    #define volfog_froxel_volume_res_z (volumelight_reprojection_factors__volfog_froxel_volume_res_z__volfog_multi_scatter_ao_weight.z)
    #define volfog_multi_scatter_ao_weight (volumelight_reprojection_factors__volfog_froxel_volume_res_z__volfog_multi_scatter_ao_weight.w)
    #define skies_primary_sun_light_dir (skies_primary_sun_light_dir__volfog_prev_range_ratio.xyz)
    #define volfog_prev_range_ratio (skies_primary_sun_light_dir__volfog_prev_range_ratio.w)
  }
  INIT_ZNZFAR_STAGE(stage)
  INIT_ECLIPSE(stage)
  USE_ECLIPSE(stage)
  INIT_SIMPLE_AMBIENT(stage)
  USE_FROXEL_JITTERED_TC(stage, jitter_ray_offset.w, inv_resolution.xyz)

  SKY_CLOUDS_SHADOWS(stage, skies_world_view_pos, skies_primary_sun_light_dir.x, skies_primary_sun_light_dir.z, skies_primary_sun_light_dir.y)

  // for scattering sample:
  (stage)
  {
    skies_primary_sun_color@f3 = (skies_primary_sun_color.x * skies_scattering_effect,
                                  skies_primary_sun_color.y * skies_scattering_effect,
                                  skies_primary_sun_color.z * skies_scattering_effect, 0);
    skies_transmittance_texture@smp2d = skies_transmittance_texture;
    skies_ms_texture@smp2d = skies_ms_texture;
  }

  ATMO(stage)
  GET_ATMO(stage)

  hlsl(stage)
  {
    #define DYNAMIC_LIGHTS_SPECULAR 0
  }
  if (dynamic_lights_count != lights_off)
  {
    hlsl(stage)
    {
      #define LAMBERT_LIGHT 1
      #define DYNAMIC_LIGHTS_EARLY_EXIT 1
    }
    INIT_AND_USE_PHOTOMETRY_TEXTURES(stage)
    INIT_AND_USE_CLUSTERED_LIGHTS(stage)
    INIT_AND_USE_OMNI_LIGHTS_CB(stage)
    INIT_SPOT_LIGHTS_CB(stage)
    INIT_AND_USE_LIGHT_SHADOWS(stage)
    INIT_AND_USE_COMMON_LIGHTS_SHADOWS_CB(stage)
    INIT_AND_USE_LIGHTS_CLUSTERED_CB(stage)
  }
  hlsl(stage)
  {
    #include "clustered/punctualLightsMath.hlsl"
  }

  hlsl(stage) {
    #define NUM_CASCADES 3
  }

  INIT_CSM_SHADOW_STCODE(stage)
  VOLUME_DEPTH_CONVERSION(stage)
  USE_OMNI_LIGHT_MASK(stage)
  USE_SPOT_LIGHT_MASK(stage)

  (stage) {
    AccumulatedInscatter@uav: register(AccumulatedInscatter_no) hlsl {
      RWTexture3D<float4> AccumulatedInscatter@uav;
    }
    initial_phase@uav: register(initial_phase_no) hlsl {
      RWTexture3D<float> initial_phase@uav;
    }
    initial_inscatter@uav: register(initial_inscatter_no) hlsl {
      RWTexture3D<float3> initial_inscatter@uav;
    }
    initial_extinction@uav: register(initial_extinction_no) hlsl {
      RWTexture3D<float3> initial_extinction@uav;
    }
  }

  hlsl(stage)
  {
    #include <hammersley.hlsl>

    #define MIN_SHADOW_SIZE 128
    #include "csm_shadow_tc.hlsl"
    float getWorldBlurredAO(float3 worldPos)
    {
      float vignetteEffect;
      float depthHt = getWorldBlurredDepth(worldPos, vignetteEffect);
      const float height_bias = 0.05;
      const float height_scale = 0.5f;
      float occlusion = rcp((max(0.01, (depthHt - height_bias - worldPos.y)*(height_scale)+1)));
      float ao = saturate(occlusion)*0.85 + 0.15;
      return lerp(ao, 1, vignetteEffect);
    }

    bool samplePrevValue(float3 tc, out float4 out_prev_inscatter, out float2 prev_atmo_scattering_shadows)
    {
      out_prev_inscatter = 0;
      float3 packedExtinction = tex3Dlod(prev_initial_extinction, float4(tc,0)).xyz;
      float extinction = packedExtinction.x;
      prev_atmo_scattering_shadows = packedExtinction.yz;
      if (is_voxel_occluded(extinction))
        return false;
      BRANCH
      if (extinction > 0.0001) // improves perf assuming mostly empty spaces
        out_prev_inscatter = float4(get_swizzled_color(tex3Dlod(prev_initial_inscatter, float4(tc,0)).rgb, channel_swizzle_indices.y), extinction);
      return true;
    }

    #define __XBOX_REGALLOC_VGPR_LIMIT 48
    #define __XBOX_ENABLE_LIFETIME_SHORTENING 1


    void calc_impl(uint3 dtId, uint3 tid, uint baseFlatId)
    {
      float2 screenTc2d = (dtId.xy+0.5f)*inv_resolution.xy;
      float3 viewVect = getViewVecOptimized(screenTc2d.xy);
      float3 view = normalize(viewVect);

#if FROXEL_FOG_MERGED_PASSES
      Length r = world_view_pos.y/1000 + theAtmosphere.bottom_radius;
      Number mu = view.y;
      Number primaryNu = dot(view.xzy, skies_primary_sun_light_dir);
      Number primaryMu_s = skies_primary_sun_light_dir.z;
      Number primaryRayleighPhaseValue = RayleighPhaseFunction(primaryNu);
      Number primaryMiePhaseValue = primaryRayleighPhaseValue*MiePhaseFunctionDivideByRayleighOptimized(theAtmosphere.mie_phase_consts, primaryNu);
      MultipleScatteringTexture multiple_scattering_approx = SamplerTexture2DFromName(skies_ms_texture);
      TransmittanceTexture transmittance_texture = SamplerTexture2DFromName(skies_transmittance_texture);
      float3 scatteringMul = skies_primary_sun_color.rgb * theAtmosphere.solar_irradiance / 1000;


      uint sliceCnt = (uint)volfog_froxel_volume_res_z;

      uint flatId = baseFlatId + tid.z;

      uint blendingStartSliceId = sliceCnt - volfog_blended_slice_cnt - 1;

      uint curActiveLayerEnd = calc_last_active_froxel_layer(volfog_occlusion, dtId.xy);
      uint curActiveBlockEnd = (min(curActiveLayerEnd + 1, sliceCnt) + warp_size_z - 1) / warp_size_z;


      // {volfog only, atmo scattering only}
      float2x3 accumulatedInscatter = 0;
      float2 accumulatedExtinction = 1;

      float4 accumulatedCombinedFog = float4(0,0,0,1);
      float4 accumulatedVolfogAtBlending = float4(0,0,0,1);

      float normalizedFactor = rcp(dot(local_view_z, view));

      // semi-parallel accumulation
      LOOP
      for (uint zblock = 0; zblock < curActiveBlockEnd; ++zblock)
      {
      dtId.z = zblock*warp_size_z + tid.z; // dispatch group Z count is fixed 1, group index is fixed 0, we only care about thread index
      bool isOccluded = is_voxel_id_occluded_cached(curActiveLayerEnd, dtId); // otherwise it can clear last blocks as empty, no occluded
#else

      BRANCH
      if (is_voxel_id_occluded(dtId))
      {
        // initial_inscatter (and atmo scattering shadows) can be anything
        initial_extinction[dtId] = NaN;
        return;
      }
      bool isOccluded = false;
#endif

      float3 screenTc = float3(screenTc2d, (dtId.z+0.5f)*inv_resolution.z);
      float3 screenTcJittered = calc_jittered_tc(dtId);

      // technically, this is not consistent with the jittered froxel media sample,
      // but it is with daskies atmo scattering, and that interpolation is much more sensitive to errors than distant fog (where there is practically no visible change)
      // TODO: maybe change daskies atmo scattering sampling, or separate froxel media and atmo scattering sample, or even DF
      float startDepth = dtId.z == 0 ? 0 : volume_pos_to_depth(max(float(dtId.z) - 1, 0.5)*inv_resolution.z);
      float nextDepth = volume_pos_to_depth(max(float(dtId.z), 0.5)*inv_resolution.z);

      float cdepth = volume_pos_to_depth(screenTc.z);
      float3 viewToPoint = cdepth*viewVect;
      float3 worldPos = viewToPoint + world_view_pos.xyz;

#if FROXEL_FOG_MERGED_PASSES
      float nextLen = nextDepth * normalizedFactor;
      float stepLen = nextLen - startDepth * normalizedFactor;
#endif

      float3 jitteredViewVect = getViewVecOptimized(screenTcJittered.xy);
      float3 jitteredView = normalize(jitteredViewVect);
      float jitteredW = max(volume_pos_to_depth(screenTcJittered.z), 0.05);
      float3 jitteredPointToEye = -jitteredViewVect*jitteredW;
      float3 jitteredWorldPos = world_view_pos.xyz - jitteredPointToEye.xyz;

      float shadowBiasTowardsSun = nextDepth - startDepth;
      float3 staticShadowWorldPos = jitteredWorldPos + from_sun_direction * shadowBiasTowardsSun;
      float3 staticShadowPointToEye = world_view_pos.xyz - staticShadowWorldPos;

      float shadowEffect = 0;
      uint cascade_id;
      float3 shadowTc = get_csm_shadow_tc(staticShadowPointToEye, cascade_id, shadowEffect);

      float shadow = 1;
      ##if shadows_quality == high
        shadow = get_downsampled_shadows(shadowTc);
        BRANCH
        if (cascade_id > 2)
          shadow *= getStaticShadow(staticShadowWorldPos);
      ##else
        shadow = getStaticShadow(staticShadowWorldPos);
        #if HAS_DOWNSAMPLED_SHADOWS
        FLATTEN
        if (shadowTc.z < 1)
          shadow = shadow*get_downsampled_shadows(shadowTc);
        #endif
      ##endif

      shadow *= getFOMShadow(jitteredWorldPos);

      float cloudExtinction = base_clouds_transmittance(jitteredWorldPos);
      float shadowForScattering = shadow * cloudExtinction;

      shadow *= clouds_shadow_from_extinction(cloudExtinction);


      float4 media = texelFetch(initial_media, dtId, 0);
      float3 color = media.rgb;
      float density = media.a;

      //==

      //for high mie we need self shadow from media. Instead we change mie extrincity
      float mieG0 = volfog_get_mieG0(), mieG1 = volfog_get_mieG1();
      float ambPhase = phaseFunctionConst();
      float sunPhase = calc_sun_phase(jitteredView, from_sun_direction);

      float blurredAO = getWorldBlurredAO(jitteredWorldPos);
      float3 inScatterAmbLight = amb_color*blurredAO;//fixme: replace with SH ambient
      float3 giAmbient = inScatterAmbLight;
      float giAmount = get_ambient3d(jitteredWorldPos, -jitteredView, amb_color, giAmbient);
      giAmount *= get_gi_param(screenTc.x) * eclipse_gi_weight_atten;
      inScatterAmbLight = lerp(inScatterAmbLight, giAmbient, giAmount);

      float3 sunLightingNoShadow = sun_color_0;
      float3 ambientLighting = inScatterAmbLight;
      float3 dynamicLighting = 0;
      #if USE_SEPARATE_PHASE
        #if HAS_LOCAL_LIGHTS_PHASE
          initial_phase[dtId] = 0;//(sunPhase+ambPhase)*0.5;
        #endif
      #else
        sunLightingNoShadow *= sunPhase;
        ambientLighting *= ambPhase;
        ##if (dynamic_lights_count != lights_off)
          #define SPOT_SHADOWS 1
          #define OMNI_SHADOWS 1
          float lightsEffect = saturate(1 + 40./20 - (1./20)*length(viewToPoint));//start decay at 40, totally darken all lights at next 20meters
          BRANCH
          if (lightsEffect>0)
          {
            uint sliceId = min(getSliceAtDepth(nextDepth, depthSliceScale, depthSliceBias), CLUSTERS_D);
            uint clusterIndex = getClusterIndex(screenTcJittered.xy, sliceId);
            uint wordsPerOmni = omniLightsWordCount;
            uint wordsPerSpot = spotLightsWordCount;
            uint omniAddress = clusterIndex*wordsPerOmni;
            ##if dynamic_lights_count == lights_omnispot_1
              wordsPerOmni = 1;
              wordsPerSpot = 1;
            ##endif
            ##if dynamic_lights_count == lights_omni_1
              wordsPerOmni = 1;
              wordsPerSpot = 0;
            ##endif
            ##if dynamic_lights_count == lights_spot_1
              wordsPerOmni = 0;
              wordsPerSpot = 1;
            ##endif
            ##if (dynamic_lights_count == lights_omnispot_1 || dynamic_lights_count == lights_omni_1 || dynamic_lights_count == lights_spot_1)
              UNROLL
            ##endif
            for ( uint omniWordIndex = 0; omniWordIndex < wordsPerOmni; omniWordIndex++ )
            {
              // Load bit mask data per lane
              uint mask = flatBitArray[omniAddress + omniWordIndex];
              uint mergedMask = MERGE_MASK(mask);
              while ( mergedMask != 0 ) // processed per lane
              {
                uint bitIndex = firstbitlow( mergedMask );
                mergedMask ^= ( 1U << bitIndex );
                uint omni_light_index = ((omniWordIndex<<5) + bitIndex);
                #include "volume_light_omni.hlsl"
              }
            }

            uint spotAddress = clusterIndex*wordsPerSpot + ((CLUSTERS_D+1)*CLUSTERS_H*CLUSTERS_W*wordsPerOmni);
            ##if (dynamic_lights_count == lights_omnispot_1 || dynamic_lights_count == lights_omni_1 || dynamic_lights_count == lights_spot_1)
              UNROLL
            ##endif
            for ( uint spotWordIndex = 0; spotWordIndex < wordsPerSpot; spotWordIndex++ )
            {
              // Load bit mask data per lane
              uint mask = flatBitArray[spotAddress + spotWordIndex];
              uint mergedMask = MERGE_MASK(mask);
              while ( mergedMask != 0 ) // processed per lane
              {
                uint bitIndex = firstbitlow( mergedMask );
                mergedMask ^= ( 1U << bitIndex );
                uint spot_light_index = ((spotWordIndex<<5) + bitIndex);
                #include "volume_light_spot.hlsl"
              }
            }
          }
        ##endif
      #endif

      float3 prev_clipSpace;
      float2 prev_tc_2d = getPrevTc(worldPos, prev_clipSpace);
      float4 prev_zn_zfar = zn_zfar;//fixme:check if it is correct assumption
      float prevDepth = volfog_prev_range_ratio*linearize_z(prev_clipSpace.z, prev_zn_zfar.zw);
      float3 prev_tc = float3(prev_tc_2d, depth_to_volume_pos(prevDepth));

      float weight = volumelight_reprojection_factors.x;
      FLATTEN if (any(abs(prev_tc - 0.5) >= 0.4999))
        weight = 0;

      float4 prevValue;

      float2 prevTemporalShadows;
      if (!samplePrevValue(prev_tc, prevValue, prevTemporalShadows))
      {
        weight = 0;
        prevValue = 0;
        prevTemporalShadows = 0;
      }
      else
      {
        //try reject history of fast moving froxels
        float dt = saturate(abs(prev_tc.z - screenTcJittered.z)-inv_resolution.z);
        float velocityFactor = exp2(-dt*64.0f);
        float velWeight = volumelight_reprojection_factors.y;
        weight *= lerp(1.0, velocityFactor, velWeight);
      }

      float2 temporalShadows = lerp(float2(shadowForScattering, blurredAO), prevTemporalShadows, weight);

      float4 atmoScatteringResult = 0;
#if FROXEL_FOG_MERGED_PASSES
      {
        Length d = nextLen/1000;
        Length r_d = ClampRadius(theAtmosphere, SafeSqrt(d * d + 2.0 * r * mu * d + r * r));
        Number primaryMu_s_d = ClampCosine((r * primaryMu_s + d * primaryNu) / r_d);

        MediumSampleRGB medium = SampleMediumFull(theAtmosphere, r_d-theAtmosphere.bottom_radius, worldPos);
        float3 primaryTransmittanceToSun = GetTransmittanceToSun( theAtmosphere, transmittance_texture, r_d, primaryMu_s_d);

        float primaryScatterShadow = temporalShadows.x;
        float temporalBlurredAO = temporalShadows.y;
        primaryTransmittanceToSun *= finalShadowFromShadowTerm(primaryScatterShadow);

        float3 primaryPhaseTimesScattering = medium.scatteringMie * primaryMiePhaseValue + medium.scatteringRay * primaryRayleighPhaseValue;
        float3 primaryMultiScatteredLuminance = GetMultipleScattering(theAtmosphere, multiple_scattering_approx, r_d, primaryMu_s_d) * medium.scattering;

        primaryMultiScatteredLuminance *= lerp(1, temporalBlurredAO, volfog_multi_scatter_ao_weight);

        atmoScatteringResult = float4((
          primaryTransmittanceToSun * primaryPhaseTimesScattering + primaryMultiScatteredLuminance) * scatteringMul,
          dot(medium.extinction.rgb, 1.0/3 / 1000));
      }
#endif

      float3 inScatterLight = sunLightingNoShadow*shadow + ambientLighting + dynamicLighting;
      float4 result = float4(color*inScatterLight, density);

      float4 result_inscatter = lerp(result, prevValue, weight);

      FLATTEN if (is_voxel_occluded(result_inscatter)) // justincase to avoid any NaNs
        result_inscatter = 0;

      initial_inscatter[dtId] = get_swizzled_color_inv(result_inscatter.rgb, channel_swizzle_indices.x);
      initial_extinction[dtId] = isOccluded ? NaN : float3(result_inscatter.a, temporalShadows);

#if FROXEL_FOG_MERGED_PASSES

        // integrate NBS volfog + atmo scattering separately
        float2x3 combinedInscatter = float2x3(result_inscatter.rgb, atmoScatteringResult.rgb);
        float2 combinedMuE = float2(result_inscatter.a, atmoScatteringResult.a);

        float2 volfogTransmittance = exp(-combinedMuE * stepLen);
        float2 combinedInvMuE = rcp(max(VOLFOG_MEDIA_DENSITY_EPS, combinedMuE));

        float phaseFunction = 1; // premultiplied by phase
        float3 S0 = combinedInscatter[0] * phaseFunction;
        float3 S1 = combinedInscatter[1] * phaseFunction;
        // integrate along the current step segment
        float3 Sint0 = (S0 - S0 * volfogTransmittance[0]) * combinedInvMuE[0];
        float3 Sint1 = (S1 - S1 * volfogTransmittance[1]) * combinedInvMuE[1];

  #if warp_size_z == 1 // special case: linear accumulation along Z slices
        // accumulate and also take into account the transmittance from previous steps
        accumulatedInscatter += float2x3(accumulatedExtinction[0] * Sint0, accumulatedExtinction[1] * Sint1);
        // Evaluate transmittance to view independentely
        accumulatedExtinction *= volfogTransmittance;

        // apply volfog-only on atmo scattering to keep consistency with daskies apply
        accumulatedCombinedFog = nanfilter(float4(
          accumulatedInscatter[1] * accumulatedExtinction[0] + accumulatedInscatter[0],
          accumulatedExtinction[1] * accumulatedExtinction[0]));

        // volfog-only at blending slice
        FLATTEN if (dtId.z == blendingStartSliceId)
          accumulatedVolfogAtBlending = float4(accumulatedInscatter[0], accumulatedExtinction[0]);

  #else // semi-parallel accumulation, both volfog and atmo scattering separately

        // no need for a sync point as we are processing Z first

        // evaluate transmittance first
        {
          float2 baseTransmittance = volfogTransmittance;

          if (tid.z == 0)
            baseTransmittance *= accumulatedExtinction; // apply the result of parallel blocks
          sharedTmpAccumulatedExtinction[flatId] = baseTransmittance;

          GroupMemoryBarrier();

          UNROLL
          for (uint offset = 1; offset < warp_size_z; offset = offset << 1)
          {
            if (tid.z >= offset)
              sharedTmpAccumulatedExtinction[flatId] *= sharedTmpAccumulatedExtinction[flatId - offset];
            GroupMemoryBarrier();
          }
        }

        float2 prevExtinction = tid.z == 0 ? accumulatedExtinction : sharedTmpAccumulatedExtinction[flatId - 1];
        float2 finalExtinction = prevExtinction * volfogTransmittance;

        // then evaluate transmittance-dependent inscatter
        {
          float2x3 baseInscatter = float2x3(prevExtinction[0] * Sint0, prevExtinction[1] * Sint1); // not inclusive transmittance (N-1 is applied to inscatter)

          if (tid.z == 0)
            baseInscatter += accumulatedInscatter; // apply the result of parallel blocks
          sharedTmpAccumulatedInscatter[flatId] = inscatter_to_struct(baseInscatter);

          GroupMemoryBarrier();

          UNROLL
          for (uint offset = 1; offset < warp_size_z; offset = offset << 1)
          {
            if (tid.z >= offset)
              sharedTmpAccumulatedInscatter[flatId] = inscatter_to_struct(
                inscatter_from_struct(sharedTmpAccumulatedInscatter[flatId]) +
                inscatter_from_struct(sharedTmpAccumulatedInscatter[flatId - offset])
              );
            GroupMemoryBarrier();
          }
        }

        float2x3 finalInscatter = inscatter_from_struct(sharedTmpAccumulatedInscatter[flatId]);

        // store the result of parallel blocks
        accumulatedInscatter = inscatter_from_struct(sharedTmpAccumulatedInscatter[baseFlatId + warp_size_z - 1]);
        accumulatedExtinction = sharedTmpAccumulatedExtinction[baseFlatId + warp_size_z - 1];

        // apply volfog-only on atmo scattering to keep consistency with daskies apply
        accumulatedCombinedFog = nanfilter(float4(
          finalInscatter[1] * finalExtinction[0] + finalInscatter[0],
          finalExtinction[1] * finalExtinction[0]));

        FLATTEN if (dtId.z == blendingStartSliceId)
          accumulatedVolfogAtBlending = float4(finalInscatter[0], finalExtinction[0]);
  #endif
        AccumulatedInscatter[dtId] = accumulatedCombinedFog;
      }

      uint blendingStartThreadId = blendingStartSliceId % warp_size_z;
      uint prevActiveLayerEnd = calc_last_active_froxel_layer(prev_volfog_occlusion, dtId.xy);
      uint prevActiveBlockEnd = (min(prevActiveLayerEnd + 1, sliceCnt) + warp_size_z - 1) / warp_size_z;
      uint maxActiveBlockEnd = max(curActiveBlockEnd, prevActiveBlockEnd);
      uint sliceBlockCnt = (sliceCnt + warp_size_z - 1) / warp_size_z;

  #if warp_size_z != 1
      if (curActiveBlockEnd > 0 && curActiveBlockEnd < maxActiveBlockEnd)
      {
        // the very last accumulated data must be used for all parallel threads along Z
        accumulatedInscatter = inscatter_from_struct(sharedTmpAccumulatedInscatter[baseFlatId + warp_size_z - 1]);
        accumulatedExtinction = sharedTmpAccumulatedExtinction[baseFlatId + warp_size_z - 1];
        accumulatedCombinedFog = nanfilter(float4(
          accumulatedInscatter[1] * accumulatedExtinction[0] + accumulatedInscatter[0],
          accumulatedExtinction[1] * accumulatedExtinction[0]));
      }
  #endif

      // clear layers which became occluded since last frame
      {
        for (uint z = curActiveBlockEnd; z < maxActiveBlockEnd; ++z)
        {
          dtId.z = z*warp_size_z + tid.z;
          initial_extinction[dtId] = NaN;
          AccumulatedInscatter[dtId] = accumulatedCombinedFog; // clearing occluded froxels with last valid value in a ray (this hides some weird HW dependent bugs)
        }
      }

      // also mark the rest of temporal froxels occluded, so reprojection can detect them next frame
      // this can differ from prev-current frame diff, as reprojection uses (3d) jittering on prev samples
      {
        // TODO: instead of fully cleaning it, transform the frustum edges, find furthest point, clean it until that froxel
        for (uint z = maxActiveBlockEnd; z < sliceBlockCnt; ++z)
        {
          dtId.z = z*warp_size_z + tid.z;
          initial_extinction[dtId] = NaN; // occluded
        }
      }

      if (tid.z == blendingStartThreadId)
        AccumulatedInscatter[int3(dtId.xy, sliceCnt)] = nanfilter(accumulatedVolfogAtBlending);
#endif

    }
  }
endmacro



shader view_initial_inscatter_cs
{
  hlsl(cs)
  {
  ##if (froxel_fog_dispatch_mode != separate)
    ##if froxel_fog_dispatch_mode == merged_linear
        #define WARP_SIZE_X 8
        #define WARP_SIZE_Y 8
        #define WARP_SIZE_Z 1
    ##else
        #define WARP_SIZE_X 4
        #define WARP_SIZE_Y 4
        #define WARP_SIZE_Z 4

##if hardware.metal // metal can't handle matrix types in groupshared memory
        struct InscatterAcc
        {
          float3 v0, v1;
        };
        InscatterAcc inscatter_to_struct(float2x3 v)
        {
          InscatterAcc insc;
          insc.v0 = v[0];
          insc.v1 = v[1];
          return insc;
        }
        float2x3 inscatter_from_struct(InscatterAcc v)
        {
          return float2x3(v.v0, v.v1);
        }

        groupshared InscatterAcc sharedTmpAccumulatedInscatter[WARP_SIZE_X*WARP_SIZE_Y*WARP_SIZE_Z];
##else
        #define inscatter_to_struct(v) v
        #define inscatter_from_struct(v) v

        groupshared float2x3 sharedTmpAccumulatedInscatter[WARP_SIZE_X*WARP_SIZE_Y*WARP_SIZE_Z];
##endif

        groupshared float2 sharedTmpAccumulatedExtinction[WARP_SIZE_X*WARP_SIZE_Y*WARP_SIZE_Z];
    ##endif
  ##else
      #define WARP_SIZE_X INITIAL_WARP_SIZE_X
      #define WARP_SIZE_Y INITIAL_WARP_SIZE_Y
      #define WARP_SIZE_Z INITIAL_WARP_SIZE_Z
  ##endif
  }

  if (froxel_fog_dispatch_mode == merged_parallel)
  {
    VOLFOG_INITIAL_COMMON(cs, 4)
  }
  else
  {
    VOLFOG_INITIAL_COMMON(cs, 1)
  }

  (cs)
  {
    world_view_pos@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, time_phase(0, 0));
  }

  hlsl(cs) {

##if (froxel_fog_dispatch_mode != separate)
    [numthreads( WARP_SIZE_Z, WARP_SIZE_X, WARP_SIZE_Y)]
    void initial_cs( uint3 tid : SV_GroupThreadID, uint3 gId : SV_GroupID )
##else
    [numthreads( WARP_SIZE_X, WARP_SIZE_Y, WARP_SIZE_Z)]
    void initial_cs( uint3 dtId : SV_DispatchThreadID )
##endif
    {

##if (froxel_fog_dispatch_mode != separate)
      tid.zxy = tid.xyz;
      gId.zxy = gId.xyz;

      uint3 threadGroupNum = uint3(WARP_SIZE_X, WARP_SIZE_Y, WARP_SIZE_Z);
      uint3 dtId = gId * threadGroupNum + tid;
      uint baseFlatId = WARP_SIZE_Z * (tid.y * WARP_SIZE_X + tid.x);
      calc_impl(dtId, tid, baseFlatId);
##else
      calc_impl(dtId, 0, 0);
##endif
    }
  }

  compile("target_cs", "initial_cs");
}



// merged pass with linear accumulation, for compatiblity mode only, optimized for integrated GPUs
shader view_initial_inscatter_ps
{
  z_test = false;
  z_write = false;
  cull_mode = none;

  assume froxel_fog_dispatch_mode = merged_linear;

  supports global_frame; // needed for world_view_pos

  VOLFOG_INITIAL_COMMON(ps, 1)

  hlsl
  {
    #define USE_TEXCOORD tc
  }
  POSTFX_VS(0)

  (ps)
  {
    volfog_froxel_volume_res@f3 = (volfog_froxel_volume_res);
  }

  hlsl(ps) {
    void render_ps(VsOutput input HW_USE_SCREEN_POS)
    {
      uint3 dtId = uint3(volfog_froxel_volume_res.xy * input.tc, 0);
      calc_impl(dtId, 0, 0);
    }
  }

  compile("target_ps", "render_ps");
}
