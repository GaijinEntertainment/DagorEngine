include "sky_shader_global.sh"
include "viewVecVS.sh"
include "gpu_occlusion.sh"

include "volume_light_distant_common.sh"


hlsl{
  // experimental features:
  #define DISTANT_FOG_USE_CLAMP_FOR_PREV_VALUE 1
  #define TAA_NEW_FRAME_WEIGHT 0.06
}


hlsl{
  #include <pixelPacking/yCoCgSpace.hlsl>


#define USE_YCOCG_PACKING 1

    float4 unpackInscatter(float4 inscatter)
    {
#if USE_YCOCG_PACKING
      return PackToYCoCgAlpha(inscatter);
#else
      return inscatter;
#endif
    }
    float4 packInscatter(float4 inscatter)
    {
#if USE_YCOCG_PACKING
      return float4(UnpackFromYCoCg(inscatter.rgb), inscatter.a);
#else
      return inscatter;
#endif
    }

}




int distant_fog_disable_occlusion_check = 0; // for testing only
interval distant_fog_disable_occlusion_check: no<1, yes;

int distant_fog_disable_temporal_filtering = 0; // for testing only
interval distant_fog_disable_temporal_filtering: no<1, yes;

int distant_fog_disable_unfiltered_blurring = 0; // for testing only
interval distant_fog_disable_unfiltered_blurring: no<1, yes;

int distant_fog_reconstruct_current_frame_bilaterally = 0; // turn off for testing only
interval distant_fog_reconstruct_current_frame_bilaterally: no<1, yes;

int distant_fog_use_stable_filtering = 1; // turn off for testing only
interval distant_fog_use_stable_filtering: no<1, yes;

int distant_fog_reprojection_type = 0; // experimental
interval distant_fog_reprojection_type: default<1, block_based<2, bilateral;


float4 distant_fog_raymarch_resolution;
float4 distant_fog_reconstruction_resolution;

float4 distant_fog_reconstruction_params_0 = (0.00000001, 0.00001, 0.000001, 16.0); // {clear limit, fogged limit, occluded limit, pow factor}
float4 distant_fog_reconstruction_params_1 = (0, 0, 0, 0); // {max_view_space_dist, NULL, NULL, NULL}

float4 distant_fog_local_view_z;

texture distant_fog_raymarch_dist;
texture prev_distant_fog_inscatter;
texture distant_fog_raymarch_inscatter;
texture distant_fog_raymarch_extinction;

texture prev_distant_fog_reconstruction_weight;


shader distant_fog_reconstruct_cs
{
  VIEW_VEC_OPTIMIZED(cs)
  INIT_ZNZFAR_STAGE(cs)
  BASE_GPU_OCCLUSION(cs)

  USE_PREV_TC(cs)

  INIT_HALF_RES_DEPTH(cs)
  USE_RAYMARCH_FRAME_INDEX(cs)

  INIT_AND_USE_VOLFOG_MODIFIERS(cs)
  VOLUME_DEPTH_CONVERSION(cs)

  (cs) {
    distant_fog_reconstruction_resolution@f4 = distant_fog_reconstruction_resolution;
    distant_fog_raymarch_resolution@f4 = distant_fog_raymarch_resolution;
    distant_fog_raymarch_inscatter@smp2d = distant_fog_raymarch_inscatter;
    distant_fog_raymarch_extinction@smp2d = distant_fog_raymarch_extinction;
    prev_distant_fog_inscatter@smp2d = prev_distant_fog_inscatter;
    distant_fog_raymarch_dist@smp2d = distant_fog_raymarch_dist;
    prev_distant_fog_reconstruction_weight@smp2d = prev_distant_fog_reconstruction_weight;

    world_view_pos@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, time_phase(0, 0));
    distant_fog_local_view_z@f3 = distant_fog_local_view_z;
    distant_fog_reconstruction_params_0@f4 = distant_fog_reconstruction_params_0;
    distant_fog_reconstruction_params_1@f4 = distant_fog_reconstruction_params_1;
  }

  hlsl(cs)
  {
    #define df_param_filtering_min_threshold (distant_fog_reconstruction_params_0.x)
    #define df_param_filtering_max_threshold (distant_fog_reconstruction_params_0.y)
    #define df_param_occluded_threshold (distant_fog_reconstruction_params_0.z)
    #define df_param_pow_factor (distant_fog_reconstruction_params_0.w)

    #define max_view_space_dist (distant_fog_reconstruction_params_1.x)
  }

  hlsl(cs) {
    #include <interleavedGradientNoise.hlsl>
    #include <hammersley.hlsl>

    #include <fp16_aware_lerp.hlsl>

    RWTexture2D<float4>  OutputInscatter : register(u7);
    RWTexture2D<float>  OutputReconstructionWeight : register(u6);

    RWTexture2D<float>  OutputReprojectionDist : register(u5);

#if DEBUG_DISTANT_FOG_RECONSTRUCT
    RWTexture2D<float4>  DebugTexture : register(u4);
#endif


    float getDistSingleRaw(float2 tc)
    {
      return tex2Dlod(distant_fog_half_res_depth_tex, float4(tc,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
    }

    float linearize_z(float raw_depth)
    {
      return linearize_z(raw_depth, zn_zfar.zw);
    }

    float getReprojectedFogDist(float2 tc)
    {
      static const int kernel_size = 1;
      float closestFogDistRaw = NaN;
      UNROLL for (int y = -kernel_size; y <= kernel_size; y++)
      {
        UNROLL for (int x = -kernel_size; x <= kernel_size; x++)
        {
          float2 offset = float2(x,y)*distant_fog_raymarch_resolution.zw;
          float dist = tex2Dlod(distant_fog_raymarch_dist, float4(tc + offset,0,0)).x;
          closestFogDistRaw = min(closestFogDistRaw, dist);
        }
      }
      return closestFogDistRaw;
    }

    struct RaymarchedData
    {
      uint2 reconstructionId;
      float4 raymarchedBlockColor;
      float fogDistRaw;
    };

    RaymarchedData getRaymarchedData(uint2 raymarch_id)
    {
      RaymarchedData raymarchedData;
      float2 raymarchUV = (raymarch_id + 0.5) * distant_fog_raymarch_resolution.zw;
      raymarchedData.fogDistRaw = tex2Dlod(distant_fog_raymarch_dist, float4(raymarchUV,0,0)).x;
      float4 inscatter4 = tex2Dlod(distant_fog_raymarch_inscatter, float4(raymarchUV,0,0));
      float extinction = tex2Dlod(distant_fog_raymarch_extinction, float4(raymarchUV,0,0)).x;
      inscatter4.rgb = pow2(inscatter4.rgb); // gamma correction to avoid color banding
      raymarchedData.raymarchedBlockColor = packInscatter(float4(inscatter4.rgb, extinction));
      #if DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION
        raymarchedData.reconstructionId = raymarch_id;
      #else
        uint2 offset = decodeRaymarchingOffset(inscatter4.a);
        raymarchedData.reconstructionId = raymarch_id * 2 + offset;
      #endif
      return raymarchedData;
    }


    static const int kernelSize1D = DISTANT_FOG_NEIGHBOR_KERNEL_SIZE*2+1;
    struct ProcessedData
    {
      float4 blurredColor; // weighted current frame color in case of bilateral reconstruction
      float4 raymarchedColors[kernelSize1D*kernelSize1D];
      float depthDiffs[kernelSize1D*kernelSize1D];
      bool hasBlurredData;
      float fogDist;
    };

    ProcessedData processNeigborData(int2 raymarch_id, float2 screen_tc, float reconstruction_depth_raw, float water_depth_raw)
    {
      ProcessedData result;

      raymarch_id = clamp(raymarch_id, (int2)DISTANT_FOG_NEIGHBOR_KERNEL_SIZE, (int2)(distant_fog_raymarch_resolution.xy)-1 - DISTANT_FOG_NEIGHBOR_KERNEL_SIZE);

      result.blurredColor = 0;
      result.fogDist = 0;
      float blurredCnt = 0;
      UNROLL for (int y = -DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; y <= DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; y++)
      {
        UNROLL for (int x = -DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; x <= DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; x++)
        {
          int flatId = (y+DISTANT_FOG_NEIGHBOR_KERNEL_SIZE)*kernelSize1D + (x+DISTANT_FOG_NEIGHBOR_KERNEL_SIZE);
          int2 blockId = raymarch_id + int2(x, y);
          RaymarchedData raymarchedData = getRaymarchedData(blockId);
          float2 neighborUV = (raymarchedData.reconstructionId + 0.5) * distant_fog_reconstruction_resolution.zw;
          result.raymarchedColors[flatId] = raymarchedData.raymarchedBlockColor;
          float neighborDepthRaw = getDistSingleRaw(neighborUV);
          neighborDepthRaw = max(neighborDepthRaw, water_depth_raw);

          float depthDiff = abs(neighborDepthRaw - reconstruction_depth_raw);
          result.depthDiffs[flatId] = depthDiff;

          FLATTEN if (df_param_filtering_max_threshold > depthDiff)
          {
##if (distant_fog_reconstruct_current_frame_bilaterally == yes)
            float w = pow2(rcp(0.0000001 + depthDiff));
##else
            float w = 1;
##endif
            blurredCnt += w;
            result.blurredColor += raymarchedData.raymarchedBlockColor * w;
##if distant_fog_reprojection_type == bilateral
            result.fogDist += raymarchedData.fogDistRaw * w;
##endif
          }
        }
      }

      result.hasBlurredData = blurredCnt > 0.00000001;

      BRANCH
      if (result.hasBlurredData) // pretty much always true
      {
        float blurredCntInv = 1.0 / blurredCnt;
        result.blurredColor *= blurredCntInv;
##if distant_fog_reprojection_type == bilateral
        result.fogDist *= blurredCntInv;
##endif
      }
##if distant_fog_reprojection_type == bilateral
      else
      {
        result.fogDist = reconstruction_depth_raw;
      }
##else
      result.fogDist = getReprojectedFogDist(screen_tc);
##endif

      return result;
    }


    struct NeighborData
    {
      float4 colorMin;
      float4 colorMax;
      bool hasFilteringData;
    };

    NeighborData calcNeighborAABB(ProcessedData processed_data, float transmittance, bool is_occluded)
    {
      NeighborData neighborData;
      neighborData.hasFilteringData = false;
      neighborData.colorMin = NaN;
      neighborData.colorMax = NaN;

##if (distant_fog_use_stable_filtering == yes && distant_fog_reconstruct_current_frame_bilaterally == yes)
      FLATTEN if (processed_data.hasBlurredData)
        transmittance = processed_data.blurredColor.a;
##endif

      float3 colorMoment1 = 0;
      float3 colorMoment2 = 0;

      UNROLL for (int y = -DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; y <= DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; y++)
      {
        UNROLL for (int x = -DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; x <= DISTANT_FOG_NEIGHBOR_KERNEL_SIZE; x++)
        {
          int flatId = (y+DISTANT_FOG_NEIGHBOR_KERNEL_SIZE)*kernelSize1D + (x+DISTANT_FOG_NEIGHBOR_KERNEL_SIZE);
          float4 neighborVal = processed_data.raymarchedColors[flatId];
          float depthDiff = processed_data.depthDiffs[flatId];

          // TODO: optimize it, maybe replace pow with muls after finding decent params
          float transmittanceDiff = abs(neighborVal.a - transmittance);
          float depthDiffLimit = lerp(df_param_filtering_min_threshold, df_param_filtering_max_threshold, pow(1-transmittanceDiff, df_param_pow_factor));

          FLATTEN if (is_occluded)
            depthDiffLimit = max(df_param_occluded_threshold, depthDiffLimit);

          FLATTEN if (depthDiffLimit > depthDiff)
          {
            float4 color = neighborVal;
            colorMoment1.rgb += color.rgb;
            colorMoment2.rgb += color.rgb * color.rgb;
            neighborData.colorMin = min(neighborData.colorMin, color);
            neighborData.colorMax = max(neighborData.colorMax, color);
            neighborData.hasFilteringData = true;
          }

        }
      }

      const float neighborhoodSize = (DISTANT_FOG_NEIGHBOR_KERNEL_SIZE*2+1)*(DISTANT_FOG_NEIGHBOR_KERNEL_SIZE*2+1);
      colorMoment1 /= (float)neighborhoodSize;
      colorMoment2 /= (float)neighborhoodSize;

// TODO: disabled for now, test it after smart raymarching pattern
#define TAA_VARIANCE_CLIP_WITH_MINMAX 0
#define TAA_VARIANCE_CLIPPING 0

#define TAA_CLAMPING_GAMMA 1.2
#ifndef TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN
#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN 1.125
#endif
#define TAA_IN_HDR_SPACE 0

      #if TAA_VARIANCE_CLIPPING
        const float3 YCOCG_MIN = float3(0.0, -0.5, -0.5);
        const float3 YCOCG_MAX = float3(2.0,  0.5,  0.5);

        float3 colorVariance = colorMoment2 - colorMoment1 * colorMoment1;
        float3 colorSigma = sqrt(max(0, colorVariance)) * TAA_CLAMPING_GAMMA;
        float3 colorMin2 = colorMoment1 - colorSigma;
        float3 colorMax2 = colorMoment1 + colorSigma;

          // When we tonemap, it's important to keep the min / max bounds within limits, we keep it within previous aabb (instead of some predefined)
        #if TAA_VARIANCE_CLIP_WITH_MINMAX || defined(TAA_VARIANCE_CLIP_WITH_MINMAX_INSET)
          #if defined(TAA_VARIANCE_CLIP_WITH_MINMAX_INSET)
            float3 colorBoxCenter = 0.5*(neighborData.colorMin + neighborData.colorMax);
            float3 colorBoxExt = colorMax - colorBoxCenter;
            float3 colorMinInset = clamp(colorBoxCenter - TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN*colorBoxExt, YCOCG_MIN, YCOCG_MAX);
            float3 colorMaxInset = clamp(colorBoxCenter + TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX*colorBoxExt, YCOCG_MIN, YCOCG_MAX);
            colorMin2 = clamp(colorMin2, colorMinInset, colorMaxInset);
            colorMax2 = clamp(colorMax2, colorMinInset, colorMaxInset);
          #else
            colorMin2 = clamp(colorMin2, neighborData.colorMin.rgb, neighborData.colorMax.rgb);
            colorMax2 = clamp(colorMax2, neighborData.colorMin.rgb, neighborData.colorMax.rgb);
          #endif
        #elif TAA_IN_HDR_SPACE
          colorMin2 = clamp(colorMin2, YCOCG_MIN, YCOCG_MAX);
          colorMax2 = clamp(colorMax2, YCOCG_MIN, YCOCG_MAX);
        #endif

        #if TAA_VARIANCE_CLIPPING
          neighborData.colorMin.rgb = colorMin2.rgb;
          neighborData.colorMax.rgb = colorMax2.rgb;
        #endif
      #endif

      return neighborData;
    }

    float getReprojectedOcclusionDepth(float2 screenTc)
    {
      int kernel_size = 1;
      float bestDepth = NaN;
      UNROLL for (int y = -kernel_size; y <= kernel_size; y++)
      {
        UNROLL for (int x = -kernel_size; x <= kernel_size; x++)
        {
          float2 offset = float2(x,y)*distant_fog_reconstruction_resolution.zw;
          float rawDepth = tex2Dlod(distant_fog_half_res_depth_tex_far, float4(screenTc + offset,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
          bestDepth = min(rawDepth, bestDepth);
        }
      }
      return linearize_z(bestDepth);
    }

    float2 getPrevTc(float3 reprojectedWorldPos)
    {
      float3 prev_clipSpace;
      return getPrevTc(reprojectedWorldPos, prev_clipSpace);
    }

    float2 calcHistoryUV(float reprojectDist, float3 viewVec)
    {
      float3 reprojectedWorldPos = reprojectDist*viewVec + world_view_pos.xyz;
      return getPrevTc(reprojectedWorldPos);
    }

    bool checkOcclusion(float2 screenTc, float3 viewVec)
    {
      float2 historyUV = calcHistoryUV(getReprojectedOcclusionDepth(screenTc), viewVec);
      float curRawDepth = tex2Dlod(distant_fog_half_res_depth_tex_far, float4(screenTc,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
      float prevRawDepth = tex2Dlod(distant_fog_half_res_depth_tex_far_prev, float4(historyUV,0,DISTANT_FOG_DEPTH_SAMPLE_LOD)).x;
      float diff = prevRawDepth - curRawDepth;
      return diff > df_param_occluded_threshold;
    }

    [numthreads( RECONSTRUCT_WARP_SIZE, RECONSTRUCT_WARP_SIZE, 1 )]
    void ReconstructDistantFog( uint2 dtId : SV_DispatchThreadID)
    {
      if (any(dtId >= uint2(distant_fog_reconstruction_resolution.xy)))
        return;

      uint2 raymarchId = DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION ? dtId : (dtId / 2);

      RaymarchedData raymarchedBlockData = getRaymarchedData(raymarchId);
      uint2 reconstructionId = raymarchedBlockData.reconstructionId;

      bool isRaymarched = all(dtId == reconstructionId);

      float2 screenTc = (dtId.xy+0.5)*distant_fog_reconstruction_resolution.zw;
      float3 viewVec = getViewVecOptimized(screenTc);

      float downsampledDistRaw = getDistSingleRaw(screenTc);

      float waterDepthRaw = inv_linearizeZ(calc_ray_dist_restriction(viewVec), zn_zfar);
      downsampledDistRaw = max(downsampledDistRaw, waterDepthRaw);

      ProcessedData processedData = processNeigborData(raymarchId, screenTc, downsampledDistRaw, waterDepthRaw);

      float reprojectDist = processedData.fogDist;
//      float downsampledDist = linearize_z(downsampledDistRaw);
//      reprojectDist = min(reprojectDist, downsampledDist); // can't get further than real depth -> BUT: it breaks occlusion check -> TODO: test it more

##if (distant_fog_reprojection_type == block_based)
      float2 raymarchedScreenTc = (reconstructionId.xy+0.5)*distant_fog_reconstruction_resolution.zw;
      float reprojectedBlockDist = getReprojectedFogDist(raymarchedScreenTc);
      float3 blockViewVec = getViewVecOptimized(raymarchedScreenTc);
      float2 raymarchedHistoryUV = calcHistoryUV(reprojectedBlockDist, blockViewVec);
      float2 historyUV = raymarchedHistoryUV + screenTc - raymarchedScreenTc;
##else
      float2 historyUV = calcHistoryUV(reprojectDist, viewVec);
##endif

      bool bOffscreen = any(abs(historyUV.xy - 0.5) >= 0.5);

      float4 prevValue = packInscatter(tex2Dlod(prev_distant_fog_inscatter, float4(historyUV,0,0)));

      float4 result = isRaymarched ? raymarchedBlockData.raymarchedBlockColor : prevValue;

##if (distant_fog_reconstruct_current_frame_bilaterally == yes)
      FLATTEN if (processedData.hasBlurredData)
        result = processedData.blurredColor;
##endif

#if DEBUG_DISTANT_FOG_RECONSTRUCT
      float4 debugVal = 0;
#endif

##if (distant_fog_disable_occlusion_check == no)
      bool bOccluded = bOffscreen;
      BRANCH
      if (!bOccluded)
        bOccluded = checkOcclusion(screenTc, viewVec);

#if DEBUG_DISTANT_FOG_RECONSTRUCT
      if (bOccluded)
        debugVal.ba = 1;
#endif
##else
      bool bOccluded = false;
##endif

      NeighborData neighborData = calcNeighborAABB(processedData, result.a, bOccluded);

      FLATTEN
      if (neighborData.hasFilteringData)
      {
##if (distant_fog_reconstruct_current_frame_bilaterally == no)
  #if (!DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION)
        FLATTEN if (!isRaymarched)
          result = clamp(result, neighborData.colorMin, neighborData.colorMax);
  #endif
##endif
        // AABB clamp/clip
        {
        #if DISTANT_FOG_USE_CLAMP_FOR_PREV_VALUE
          prevValue = clamp(prevValue, neighborData.colorMin, neighborData.colorMax);
        #else
          const float bias = 0;

          // Clip color difference against neighborhood min/max AABB
          float3 boxCenter = (neighborData.colorMax.rgb + neighborData.colorMin.rgb) * 0.5;
          float3 boxExtents = neighborData.colorMax.rgb - boxCenter + bias;

          float3 rayDir = result.rgb - prevValue.rgb;
          float3 rayOrg = prevValue.rgb - boxCenter;

          float clipLength = 1.0;

          if (length(rayDir) > 1e-6)
          {
            // Intersection using slabs
            float3 rcpDir = rcp(rayDir);
            float3 tNeg = ( boxExtents - rayOrg) * rcpDir;
            float3 tPos = (-boxExtents - rayOrg) * rcpDir;
            clipLength = saturate(max(max(min(tNeg.x, tPos.x), min(tNeg.y, tPos.y)), min(tNeg.z, tPos.z)));
          }

          prevValue = half4(lerp(prevValue.rgb, result.rgb, clipLength), clamp(prevValue.a, neighborData.colorMin.a, neighborData.colorMax.a));
        #endif
        }
      }
      else // can only happen with non-raymarched texels
      {
##if (distant_fog_disable_unfiltered_blurring == no)
        FLATTEN if (processedData.hasBlurredData)
          result = prevValue = processedData.blurredColor; // without new sample, errors can propagate (can even cause full black output)
##endif

#if DEBUG_DISTANT_FOG_RECONSTRUCT
        debugVal.ra = 1;
#endif
      }

##if (distant_fog_reconstruct_current_frame_bilaterally == no)
      FLATTEN if (processedData.hasBlurredData && bOffscreen)
        result = processedData.blurredColor;
##endif

      bool bResetTemporalWeights = bOffscreen || bOccluded;

      float prevWeight = tex2Dlod(prev_distant_fog_reconstruction_weight, float4(historyUV, 0,0)).x;
      float clampEventFrame = prevWeight > SAMPLE_NUM/255. ? 255.0 : ceil(255*prevWeight);
      half newFrameWeight = bResetTemporalWeights ? 1.0 : max(TAA_NEW_FRAME_WEIGHT, rcp(2+clampEventFrame));
      const float taaEventFrameStep = 1.75/255; // TODO: experiment with different values
      float taaWeight = bResetTemporalWeights ? 0.0 : saturate(clampEventFrame*(1./255)+taaEventFrameStep);
      OutputReconstructionWeight[dtId] = isRaymarched ? taaWeight : min(taaWeight, prevWeight);

##if (distant_fog_disable_temporal_filtering == no)
      result = lerp(prevValue, result, newFrameWeight);
##endif

      FLATTEN if (!isfinite(dot(result,1))) // justincase
        result = float4(0,0,0,1);

      OutputInscatter[dtId] = saturate(unpackInscatter(result));

      float downsampledDist = linearize_z(downsampledDistRaw, zn_zfar.zw);
      float3 viewVecN = normalize(viewVec);
      float normalizedFactorInv = dot(distant_fog_local_view_z, viewVecN);
      float maxRange = normalizedFactorInv * max_view_space_dist;

      FLATTEN
      if (processedData.fogDist >= maxRange)
        processedData.fogDist = downsampledDist; // optimization, to skip far away DF samples entirely

      OutputReprojectionDist[dtId] = processedData.fogDist;

#if DEBUG_DISTANT_FOG_RECONSTRUCT
      //debugVal.b = reprojectDist / MAX_RAYMARCH_FOG_DIST;
      uint2 reconstructionId0 = calc_reconstruction_id(raymarchId);
      if (any(reconstructionId0 != reconstructionId))
        debugVal.ga = 1;
      DebugTexture[dtId] = debugVal;
#endif


    }
  }
  compile("target_cs", "ReconstructDistantFog");
}



