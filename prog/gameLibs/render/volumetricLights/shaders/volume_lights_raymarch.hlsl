#ifndef VOLUME_LIGHTS_RAYMARCH_INCLUDED
#define VOLUME_LIGHTS_RAYMARCH_INCLUDED 1


#ifndef DEBUG_DISTANT_FOG_RAYMARCH
#define DEBUG_DISTANT_FOG_RAYMARCH 0
#endif

#ifndef DISTANT_FOG_DISABLE_SMART_RAYMARCHING_PATTERN
#define DISTANT_FOG_DISABLE_SMART_RAYMARCHING_PATTERN 0
#endif

#ifndef DISTANT_FOG_SUBSTEP_CNT
#define DISTANT_FOG_SUBSTEP_CNT 0
#endif

#define USE_SUBSTEPPING (DISTANT_FOG_SUBSTEP_CNT > 1)

#define enable_raymarch_opt_empty_ray distant_fog_raymarch_params_0.x // float comparison (fine in this case)
#define enable_raymarch_opt_ray_start distant_fog_raymarch_params_0.y // float comparison (fine in this case)
#define raymarch_substep_start distant_fog_raymarch_params_0.z
#define raymarch_substep_mul_inv distant_fog_raymarch_params_0.w

#define raymarch_start_volfog_slice_offset distant_fog_raymarch_params_1.x
#define raymarch_step_size distant_fog_raymarch_params_1.y
#define smart_raymarching_pattern_depth_bias distant_fog_raymarch_params_1.z
#define raymarch_occlusion_threshold distant_fog_raymarch_params_1.w

#define raymarch_max_range distant_fog_raymarch_params_2.x
#define raymarch_fading_dist distant_fog_raymarch_params_2.y
#define occlusion_weight_mip_level distant_fog_raymarch_params_2.z
#define occlusion_weight_sample_offset_scale distant_fog_raymarch_params_2.w


groupshared uint sharedFogStart;



// empty check sample num (32) must be multiple of small step (8)
// too high values introduce slower convergence speed, causing artefacts
// "sample mul" improves performance (semi-independent from "sample num"), but multiples of ("sample num"/8) reduces quality
#define EMPTY_CHECK_SAMPLE_MUL 16



float4 sample_media(float dist, float2 screen_tc, float3 view_vec, out float3 world_pos)
{
  world_pos = dist*view_vec + world_view_pos.xyz;

  float screenTcZ = saturate(depth_to_volume_pos(dist));
  float3 screenTcMedia = float3(screen_tc.xy, screenTcZ);

  return get_media(world_pos, screenTcMedia, world_view_pos.w);
}
float4 sample_media(float dist, float2 screen_tc, float3 view_vec)
{
  float3 worldPos;
  return sample_media(dist, screen_tc, view_vec, worldPos);
}

struct AccumulationConstParams
{
  // TODO: these should be shader consts (no preshader for NBS yet)
  float4 clouds_weather_inv_size__alt_scale;
  float4 world_pos_to_clouds_alt;

  float2 screenTc;
  float3 view_vec;
  float3 sun_color;
  float3 ambient_color;
  float stepLen;
  float stepSize;
#if USE_SUBSTEPPING
  float subStepLen;
  float subStepSize;
  float subStepDistOffset;
#endif
};


// copy-paste from clouds_shadow.sh
float clouds_shadow(float3 worldPos, AccumulationConstParams cp)
{
  float alt = cp.world_pos_to_clouds_alt.x*worldPos.y + cp.world_pos_to_clouds_alt.y;
  worldPos.xz = cp.clouds_weather_inv_size__alt_scale.yz*alt + worldPos.xz;
  float shadow = tex2Dlod(clouds_shadows_2d,
    float4(worldPos.xz*cp.clouds_weather_inv_size__alt_scale.x + cp.world_pos_to_clouds_alt.zw + clouds_hole_pos.zw, 0, 0)).x;
  return lerp(shadow,1, max(alt,0));
}


void accumulatePreparedFogStep(AccumulationConstParams const_params,
  float4 media, float dist, float3 worldPos, float step_len, float shadow,
  inout float4 accumulated_scattering, out float transmittance)
{
  float muE = media.a;
  transmittance = exp(-muE * step_len);
  float3 S = media.rgb * (const_params.sun_color * shadow + const_params.ambient_color);
  float3 Sint = (S - S * transmittance) / max(VOLFOG_MEDIA_DENSITY_EPS, muE);
  accumulated_scattering.rgb += accumulated_scattering.a * Sint;
  accumulated_scattering.a *= transmittance;
}

void accumulateFogStep(
  bool use_substeps, // should use it as compile time constant
  AccumulationConstParams const_params, float dist, float sampleWeight,
  inout float4 accumulated_scattering, out float transmittance, inout float fog_start_dist, inout int sampleCnt)
{
  float3 worldPos;
  float4 media = sample_media(dist, const_params.screenTc, const_params.view_vec, worldPos) * sampleWeight;
  ++sampleCnt;

  float muE = media.a;

  transmittance = 1;

  BRANCH // assuming mostly empty spaces
  if (muE > VOLFOG_MEDIA_DENSITY_EPS)
  {
    FLATTEN
    if (fog_start_dist <= 0.0)
      fog_start_dist = dist; // store the distance of the very first non-empty sample

    float subTransmittance;
    float stepLen = const_params.stepLen;

    float shadow = clouds_shadow(worldPos, const_params);
    FLATTEN // not having a branch for static shadows is faster when it is enabled (and it is an edge case to have it disabled, but not DF)
    if (distant_fog_use_static_shadows)
      shadow *= getStaticShadow(worldPos);

#if USE_SUBSTEPPING
    if (use_substeps)
    {
      stepLen = const_params.subStepLen;
      dist += const_params.subStepDistOffset;
      UNROLL for (int i = 1; i < DISTANT_FOG_SUBSTEP_CNT; ++i)
      {
        float4 subMedia = sample_media(dist, const_params.screenTc, const_params.view_vec, worldPos) * sampleWeight;
        ++sampleCnt;
        accumulatePreparedFogStep(const_params, subMedia, dist, worldPos, stepLen, shadow, accumulated_scattering, subTransmittance);
        dist += const_params.subStepSize;
        transmittance *= subTransmittance;
      }
    }
#endif
    // accumulate last substep using the already sampled media
    accumulatePreparedFogStep(const_params, media, dist, worldPos, stepLen, shadow, accumulated_scattering, subTransmittance);
    transmittance *= subTransmittance;
  }
}

bool isEmpty(float2 screen_tc, float start_dist, float end_dist, float3 view_vec, float step_size, inout int sampleCnt)
{
  LOOP
  for (float dist = start_dist; dist < end_dist; dist += step_size)
  {
    ++sampleCnt;
    float4 media = sample_media(dist, screen_tc, view_vec);
    if (media.w > 0.000001)
      return false;
  }
  return true;
}


static const int FRAME_CNT_UNTIL_GUESSING_EMPTY = SAMPLE_NUM;

float getRawDepth(float2 uv)
{
  return tex2Dlod(distant_fog_half_res_depth_tex, float4(uv,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
}

void getRaymarchedNeighborDepthStats(uint2 raymarchId, float4 transformed_znzfar, out float close_depth, out float far_depth)
{
  close_depth = NaN;
  far_depth = NaN;
  UNROLL for (int y = -DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; y <= DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; y++)
  {
    UNROLL for (int x = -DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; x <= DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; x++)
    {
      int2 blockId = (int2)raymarchId + int2(x, y);
      uint2 neighborId = calc_reconstruction_id(blockId);
      float2 neighborUV = (neighborId + 0.5) * distant_fog_reconstruction_resolution.zw;
      float neighborDepth = getRawDepth(neighborUV);
      close_depth = max(close_depth, neighborDepth);
      far_depth = min(far_depth, neighborDepth);
    }
  }
  close_depth = linearize_z(close_depth, transformed_znzfar.zw);
  far_depth = linearize_z(far_depth, transformed_znzfar.zw);
}

struct BlockStats
{
  uint2 index[4];
  float linearDepth[4];
};
BlockStats getBlockNeighborDepthStats(uint2 raymarchId, float4 transformed_znzfar)
{
  BlockStats result;
  UNROLL for (int y = 0; y <= 1; y++)
  {
    UNROLL for (int x = 0; x <= 1; x++)
    {
      float2 neighborUV = (2*raymarchId + int2(x, y) + 0.5) * distant_fog_reconstruction_resolution.zw;
      result.index[y*2+x] = uint2(x, y);
      result.linearDepth[y*2+x] = linearize_z(getRawDepth(neighborUV), transformed_znzfar.zw);
    }
  }
  return result;
}

uint2 get_raymarch_offset(uint2 dtId, float4 transformed_znzfar)
{
  uint2 raymarchIndexOffset = calc_raymarch_offset(dtId, fog_raymarch_frame_id);

#if !DISTANT_FOG_DISABLE_SMART_RAYMARCHING_PATTERN
  float outerMin, outerMax;
  getRaymarchedNeighborDepthStats(dtId, transformed_znzfar, outerMin, outerMax);

  BlockStats blockStats2x2 = getBlockNeighborDepthStats(dtId, transformed_znzfar);
  float innerMin = min(min(blockStats2x2.linearDepth[0], blockStats2x2.linearDepth[1]), min(blockStats2x2.linearDepth[2], blockStats2x2.linearDepth[3]));
  float innerMax = max(max(blockStats2x2.linearDepth[0], blockStats2x2.linearDepth[1]), max(blockStats2x2.linearDepth[2], blockStats2x2.linearDepth[3]));

  float bias = smart_raymarching_pattern_depth_bias;
  float extendedOuterMin = outerMin - bias;
  float extendedOuterMax = outerMax + bias;

  BRANCH if (!(extendedOuterMin <= innerMin && innerMax <= extendedOuterMax))
  {
    int bestId = 0;
    float bestDiff = 0;
    UNROLL for (int i = 0; i < 4; ++i)
    {
      float depth = blockStats2x2.linearDepth[i];
      float diff = max(depth - outerMax, outerMin - depth);
      FLATTEN if (diff > bestDiff)
      {
        bestId = i;
        bestDiff = diff;
      }
    }
    raymarchIndexOffset = blockStats2x2.index[bestId];
  }
#endif

  return raymarchIndexOffset;
}


// TODO: optimize these
void getDepthStatistics(float2 tc, float4 transformed_znzfar, out float current_depth, out float close_depth, out float far_depth)
{
  static const int kernel_size = 1;
  close_depth = NaN;
  far_depth = NaN;
  UNROLL for (int y = -kernel_size; y <= kernel_size; y++)
  {
    UNROLL for (int x = -kernel_size; x <= kernel_size; x++)
    {
      float2 offset = float2(x,y)*distant_fog_reconstruction_resolution.zw;
      float rawDepth = getRawDepth(tc + offset);
      close_depth = max(close_depth, rawDepth);
      far_depth = min(far_depth, rawDepth);
      if (x == 0 && y == 0)
        current_depth = linearize_z(rawDepth, transformed_znzfar.zw);
    }
  }
  close_depth = linearize_z(close_depth, transformed_znzfar.zw);
  far_depth = linearize_z(far_depth, transformed_znzfar.zw);
}
void getPrevDepthStatistics(float2 prev_tc, float4 transformed_znzfar, out float prev_far_depth)
{
  static const int kernel_size = 1;
  prev_far_depth = NaN;
  UNROLL for (int y = -kernel_size; y <= kernel_size; y++)
  {
    UNROLL for (int x = -kernel_size; x <= kernel_size; x++)
    {
      float2 offset = float2(x,y)*distant_fog_reconstruction_resolution.zw;
      float rawPrevDepth = tex2Dlod(distant_fog_half_res_depth_tex_far_prev, float4(prev_tc + offset,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
      prev_far_depth = min(prev_far_depth, rawPrevDepth);
    }
  }
  prev_far_depth = linearize_z(prev_far_depth, transformed_znzfar.zw);
}

// TODO: optimize these
float getReprojectedOcclusionDepth(float2 screen_tc, float4 transformed_znzfar)
{
  int kernel_size = 1;
  float bestDepth = NaN;
  UNROLL for (int y = -kernel_size; y <= kernel_size; y++)
  {
    UNROLL for (int x = -kernel_size; x <= kernel_size; x++)
    {
      float2 offset = float2(x,y)*distant_fog_reconstruction_resolution.zw;
      float rawDepth = tex2Dlod(distant_fog_half_res_depth_tex_far, float4(screen_tc + offset,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
      bestDepth = min(rawDepth, bestDepth);
    }
  }
  return linearize_z(bestDepth, transformed_znzfar.zw);
}
float2 calcHistoryUV(float reprojectDist, float3 viewVec)
{
  float3 reprojectedWorldPos = reprojectDist*viewVec + world_view_pos.xyz;
  return getPrevTc(reprojectedWorldPos);
}
bool checkOcclusion(float2 screen_tc, float3 view_vec, float2 history_tc)
{
  float curRawDepth = tex2Dlod(distant_fog_half_res_depth_tex_close, float4(screen_tc,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
  float prevRawDepth = tex2Dlod(distant_fog_half_res_depth_tex_far_prev, float4(history_tc,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
  float depth_diff = prevRawDepth - curRawDepth;
  return depth_diff > raymarch_occlusion_threshold;
}

float4 accumulateFog_impl(uint2 dtId, float4 transformed_znzfar, float2 screen_tc, float2 prev_tc, float start_dist, float3 view_vec,
  inout float fog_start, inout float raymarch_step_weight,
  out float out_weighted_end_dist, out float4 out_debug)
{
  out_weighted_end_dist = 0;
  out_debug = 0;

  #if !DEBUG_DISTANT_FOG_BOTH_SIDES_RAYMARCH
    if (screen_tc.x > 0.5)
      return float4(0,0,0,1);
  #endif

  if (any(dtId >= (uint2)distant_fog_raymarch_resolution.xy))
    return float4(0,0,0,1);

  const uint MAX_SAFETY_STEP_CNT = 256;
  const float EXTINCTION_EPS = 0.005; // MUST BE a compile time const, otherwise it is much slower
  const float SMALL_STEPS_START_DEPTH = max(raymarch_substep_start, 0);
  const float SMALL_STEPS_RATIO = clamp(raymarch_substep_mul_inv, 1.0/16, 1.0);

  const float smallStepSize = max(raymarch_step_size, 8);
  const float emptyCheckStepSize = smallStepSize * EMPTY_CHECK_SAMPLE_MUL;

  float3 viewVecN = normalize(view_vec);
  float normalizedFactorInv = dot(distant_fog_local_view_z.xyz, viewVecN);
  float normalizedFactor = rcp(normalizedFactorInv);

  float maxDist = normalizedFactorInv * min(min(raymarch_max_range, MAX_RAYMARCH_FOG_DIST), MAX_SAFETY_STEP_CNT * smallStepSize);
  float fading_dist = clamp(raymarch_fading_dist, 0.001f, maxDist);

  float distRestriction = calc_ray_dist_restriction(view_vec);

  float currentDepth, closeDepth, farDepth;
  getDepthStatistics(screen_tc, transformed_znzfar, currentDepth, closeDepth, farDepth);
  currentDepth = min(currentDepth, distRestriction);
  closeDepth = min(closeDepth, distRestriction);
  farDepth = min(farDepth, distRestriction);

  // in froxel fog range we can dilate depth as much as we want, as the blurry edges are cut off in resolve pass
  // this extra dilation makes the result much more stable and eliminates half res depth artefacts in close range
  // we interpolate between dilated and non-dilated range in a very small distance, so the effect doesn't pop-up immediately
  // note: the half res depth edge artefacts are still present in far range, but they are not that noticeable
  const float CUTOUT_TRANSITION_SCALE = 0.1;
  currentDepth = lerp(currentDepth, farDepth, saturate((start_dist - currentDepth)*CUTOUT_TRANSITION_SCALE));

  float prevFarDepth;
  getPrevDepthStatistics(prev_tc, transformed_znzfar, prevFarDepth);
  prevFarDepth = min(prevFarDepth, distRestriction); // can use the current frame water distance, as it changes relatively slowly

  // reduce aggressiveness of optimizations using occlusion check
  // it uses dilated depth, so jittering does not affect it, and optimization does not suffer from it
  bool bOccluded = checkOcclusion(screen_tc, view_vec, prev_tc);
  if (bOccluded)
    raymarch_step_weight += 1.0 / FRAME_CNT_UNTIL_GUESSING_EMPTY; // short time penalty for next frames too

#if DEBUG_DISTANT_FOG_RAYMARCH
  if (bOccluded)
    out_debug.ra = 1;
#endif

  const float DF_NIGHT_SUN_COS = 0.1;
  float3 sunColor = sun_color_0.rgb;
  sunColor *= saturate(-from_sun_direction.y/DF_NIGHT_SUN_COS); // fix low angle (underground) sun color
  sunColor *= calc_sun_phase(viewVecN, from_sun_direction.xyz);

  float3 ambientColor = get_base_ambient_color() * phaseFunctionConst();

  uint noiseIndex = calc_raymarch_noise_index(dtId); // TODO: maybe use reconstructionId as index (needs more testing)
  float blueNoiseOffsetZ = calc_raymarch_noise_offset_8(noiseIndex);

  float endDist = min(currentDepth, maxDist);

  int sampleCnt = 0; // for debugging


  float prevStartDist = start_dist; // TODO: not necessarily true, but froxel and distant fog is more stable with fixed froxel range anyway

  bool useHugeSteps =
    enable_raymarch_opt_empty_ray
    && raymarch_step_weight < 0.0001 // very conservative check after several consecutive empty rays
    && !(prevFarDepth < prevStartDist && closeDepth > start_dist); // special case, if depth is suddenly outside of froxel range, calc everything

  BRANCH
  if (useHugeSteps)
  {
    // check if it is an empty ray (no fog samples along the entire ray) using huge steps

    float blueNoiseOffsetZ_32 = calc_raymarch_noise_offset_32(noiseIndex);
    float startD = start_dist + blueNoiseOffsetZ_32 * emptyCheckStepSize;

    float maxD = endDist;
    if (isEmpty(screen_tc, startD, endDist, view_vec, emptyCheckStepSize, sampleCnt))
    {
      out_weighted_end_dist = maxD;
      fog_start = 0;
#if DEBUG_DISTANT_FOG_RAYMARCH
      out_debug.ga = 1;
      out_debug.b = (saturate((float)sampleCnt / MAX_SAFETY_STEP_CNT));
#endif
      return float4(0,0,0,1); // skip fog calc as it is (probably) an empty ray
    }
    // wrong guess, calc everything
  }


  float4 accumulatedScattering = float4(0,0,0,1);
  float dist0 = start_dist + blueNoiseOffsetZ * smallStepSize;
  float dist = dist0;

  BRANCH
  if (enable_raymarch_opt_ray_start)
  {
    const float RAY_START_OPT_RATIO = 2.0;
    // use bigger steps until the potential first sample dist
    float bigStepSize = smallStepSize * RAY_START_OPT_RATIO;

    // TODO: use a different set of blue noise offsets, sample size scaled by RAY_START_OPT_RATIO
    dist = start_dist + blueNoiseOffsetZ * bigStepSize;

    float bigStepEndDist = min(endDist, smallStepSize * fog_start);

    LOOP
    while (dist < bigStepEndDist)
    {
      float4 media = sample_media(dist, screen_tc, view_vec);
      ++sampleCnt;
      if (media.w > 0.000001)
        break;
      dist += bigStepSize;
    }
    // step back 2 times: 1st to correct the guess, 2nd to offset the noise (!)
    dist = max(dist - blueNoiseOffsetZ * bigStepSize - bigStepSize + blueNoiseOffsetZ * smallStepSize, dist0);
  }

  float fogStartDist = 0;

  // TODO: extra samples in dead zone (no prev data), especially near froxel range

  uint safetyI = 0;

  float weightSum = 0;

  const float smallStepStartDepth = max(smallStepSize, SMALL_STEPS_START_DEPTH);
  float offsetStepNegate = 0;

  AccumulationConstParams cp;
  float3 lightDir = skies_primary_sun_light_dir.xzy;
  cp.clouds_weather_inv_size__alt_scale = float4(
    nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_m.z,
    lightDir.xz / max(0.02, lightDir.y) * nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_m.w,
    0);
  cp.world_pos_to_clouds_alt = float4(
    nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_m.xy,
    clouds_origin_offset.xy * cp.clouds_weather_inv_size__alt_scale.x + 0.5);
  cp.screenTc = screen_tc;
  cp.view_vec = view_vec;
  cp.sun_color = sunColor;
  cp.ambient_color = ambientColor;
  cp.stepSize = smallStepSize;
  cp.stepLen = smallStepSize * normalizedFactor;
#if USE_SUBSTEPPING
  cp.subStepSize = cp.stepSize / DISTANT_FOG_SUBSTEP_CNT;
  cp.subStepLen = cp.stepLen / DISTANT_FOG_SUBSTEP_CNT;
  cp.subStepDistOffset = cp.subStepSize * (1 - DISTANT_FOG_SUBSTEP_CNT);
#endif

  float endSampleWeight = saturate((maxDist - endDist) / fading_dist);

  // big steps until close to depth, then small ones
  UNROLL
  for (int stepType = 0; stepType < 2; ++stepType)
  {
    float sampleWeight = 1;
    if (stepType == 0)
    {
      float prevEndDist = endDist;
      endDist = max(endDist - smallStepStartDepth, 0);
      offsetStepNegate = prevEndDist - endDist;
    }
    else
    {
      sampleWeight = endSampleWeight;
      endDist += offsetStepNegate;
      dist += blueNoiseOffsetZ*cp.stepSize*(SMALL_STEPS_RATIO - 1);
      cp.stepSize *= SMALL_STEPS_RATIO;
      cp.stepLen = cp.stepSize * normalizedFactor;
      // no internal substepping in 2nd phase, as it is already doing that
    }

    LOOP
    while (dist < endDist && accumulatedScattering.w > EXTINCTION_EPS && safetyI < MAX_SAFETY_STEP_CNT)
    {
      float transmittance;
      bool useSubsteps = stepType == 0; // known at compile time
      accumulateFogStep(useSubsteps, cp, dist, sampleWeight, accumulatedScattering, transmittance, fogStartDist, sampleCnt);

      float weight = 1 - transmittance;
      out_weighted_end_dist += dist*weight;
      weightSum += weight;
      dist += cp.stepSize;
    }
  }

  cp.stepSize = endDist + cp.stepSize - dist;

  // final step correction with exact depth difference
  FLATTEN
  if (safetyI < MAX_SAFETY_STEP_CNT && cp.stepSize > 0) // always true with proper input
  {
    cp.stepLen = cp.stepSize * normalizedFactor;
    // no substepping here, no need to update its params
    float transmittance;
    accumulateFogStep(false, cp, endDist, endSampleWeight, accumulatedScattering, transmittance, fogStartDist, sampleCnt);
  }

#if DEBUG_DISTANT_FOG_SHOW_RAYMARCH_END
    if (safetyI > MAX_SAFETY_STEP_CNT && all(abs(accumulatedScattering - float4(0,0,0,1)) < 0.000001))
      accumulatedScattering = float4(1,0,1,0);
#endif

  out_weighted_end_dist = weightSum > 1e-9 ? out_weighted_end_dist/weightSum : endDist;

#if DEBUG_DISTANT_FOG_RAYMARCH
  out_debug.b = (saturate((float)sampleCnt / MAX_SAFETY_STEP_CNT));
#endif

  if (weightSum <= 1e-9)
    fogStartDist = endDist;

  // fog start is only a guess, used for optimization, quantized to the number of small steps, stored in 8 bits
  fog_start = floor(fogStartDist / smallStepSize);

  return accumulatedScattering;
}

struct DistantFogRaymarchResult
{
  float4 inscatter; // .a is 2 bits == raymarching pattern offset
  float extinction;
  float weightedEndDist;
  float fogStart;
  float raymarchStepWeight;
  float4 debug;
};

float unpack_fog_start(float packed_start_step)
{
  return floor((1.0 - packed_start_step) * 255.0);
}
float pack_fog_start(float start_step)
{
  return 1.0 - saturate(floor(start_step) / 255.0);
}

void get_occlusion_weights(float2 prev_tc, out float start_step, out float step_weight)
{
  BRANCH
  if (any(abs(prev_tc - 0.5) >= 0.5)) // was offscreen, just calc everything
  {
    step_weight = 2.0 / FRAME_CNT_UNTIL_GUESSING_EMPTY; // fixed, but short penalty for offscreen texels
    start_step = 0;
  }
  else
  {
    // "raymarch step weight" determines if it is a potentially empty ray or not
    // we dilate it over frames, so nearby non empty ones can suck in the rest, making the empty ray optimization very conservative, minimizing artefacts
    // to avoid bloating the nearby texels unnecessarily (which can get stuck in next frames), a falloff penalty is used, this controls the reach of the actually empty texels
    // "ray start step" is packed as inverse, so the same max operation can be used for both
    // it is NOT dilated over frames, calculated only from the prev frame
    const int WEIGHT_KERNEL_SIZE = 1;
    float weightFalloff = 0.5 / FRAME_CNT_UNTIL_GUESSING_EMPTY;
    float2 maxWeights = 0;
    float2 occlusionWeightSampleOffset = distant_fog_raymarch_resolution.zw * occlusion_weight_sample_offset_scale;
    UNROLL for (int y = -WEIGHT_KERNEL_SIZE; y <= WEIGHT_KERNEL_SIZE; y++)
    {
      UNROLL for (int x = -WEIGHT_KERNEL_SIZE; x <= WEIGHT_KERNEL_SIZE; x++)
      {
        float2 weights = tex2Dlod(prev_distant_fog_raymarch_start_weights,
          float4(prev_tc + float2(x, y) * occlusionWeightSampleOffset, 0, occlusion_weight_mip_level)).xy;
        maxWeights = max(maxWeights, weights);
      }
    }
    step_weight = max(maxWeights.y - weightFalloff, 0);
    start_step = unpack_fog_start(maxWeights.x);
  }

  // same start step per tile to avoid unnecessary divergence of threads
  uint fogStartI = (uint)start_step;
  InterlockedMin(sharedFogStart, fogStartI);
  GroupMemoryBarrierWithGroupSync();
  start_step = sharedFogStart;
}

DistantFogRaymarchResult accumulateFog(uint2 dtId, uint2 tid)
{
  if (all(tid == 0))
    sharedFogStart = 255;

  DistantFogRaymarchResult result;

  float4 transformed_znzfar = get_transformed_zn_zfar();
  uint2 raymarch_index_offset = get_raymarch_offset(dtId, transformed_znzfar);
  uint2 reconstructionId = dtId*2 + raymarch_index_offset;

  #if DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION
    raymarch_index_offset = 0;
    reconstructionId = dtId;
  #endif

  float2 screenTc = (reconstructionId+0.5)*distant_fog_reconstruction_resolution.zw;
  float3 viewVec = calcViewVec(screenTc);

  float2 prevTc = calcHistoryUV(getReprojectedOcclusionDepth(screenTc, transformed_znzfar), viewVec);

  get_occlusion_weights(prevTc, result.fogStart, result.raymarchStepWeight);

  float4 inscatterExtinction = accumulateFog_impl(dtId, transformed_znzfar, screenTc, prevTc, volfog_blended_slice_start_depth, viewVec,
    result.fogStart, result.raymarchStepWeight, result.weightedEndDist, result.debug);

  result.fogStart = pack_fog_start(result.fogStart);

  result.raymarchStepWeight = all(abs(inscatterExtinction - float4(0,0,0,1)) < 0.000001) ? (result.raymarchStepWeight - 1.0/FRAME_CNT_UNTIL_GUESSING_EMPTY) : 1;

  inscatterExtinction.rgb = sqrt(inscatterExtinction.rgb); // gamma correction to avoid color banding
  result.inscatter = float4(inscatterExtinction.rgb, encodeRaymarchingOffset(raymarch_index_offset));
  result.extinction = inscatterExtinction.a;

  return result;
}


#endif