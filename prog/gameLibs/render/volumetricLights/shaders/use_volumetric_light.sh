texture view_result_inscatter;
texture distant_fog_result_inscatter;

int volfog_blended_slice_cnt = 4;
float volfog_blended_slice_start_depth = 0.001;

int initial_media_no = 7;

float4 volfog_froxel_volume_res = (128,128,64,0);
float4 inv_volfog_froxel_volume_res = (1/128.0,1/128.0,1/64.0,0);
float4 volfog_froxel_range_params = (1,1,0,0); // (range, 1/range, froxel_fog_use, distant_fog_use)
// int distant_fog_use_3x3_bilateral_kernel = 0; // TODO: temporarily disabled due to a Vulkan compiler bug

float4 distant_fog_test_params = (5000.0, 0.0, 0.01, 0); // {DISTANT_FOG_MAX_LERP_RANGE, DISTANT_FOG_CUTOUT_BIAS, DISTANT_FOG_CUTOUT_SCALE, NULL}

texture distant_fog_downsampled_depth_tex;
texture distant_fog_reprojection_dist;

int volumetric_light_assumed_off;
interval volumetric_light_assumed_off:no<1, yes;

macro INIT_VOLUMETRIC_LIGHT_COMMON()
  if (volumetric_light_assumed_off == no)
  {
    // precalced before calling dispatch due to the lack of preshader support in node based shaders
    // local float volfog_blending_start_slice = 1.0 - volfog_blended_slice_cnt * inv_volfog_froxel_volume_res.z;
    // local float volfog_blended_slice_start_depth = (volfog_blending_start_slice * volfog_blending_start_slice) * volfog_froxel_range_params.x;

    local float volfog_inv_transition_ratio = (1.0 / max(volfog_froxel_range_params.x - volfog_blended_slice_start_depth, 0.0001));
    local float volfog_inv_transition_offset = 1.0 - volfog_froxel_range_params.x * volfog_inv_transition_ratio;
    local float volfog_inv_res_z_plus_1 = 1.0 / (volfog_froxel_volume_res.z + 1);
    local float volfog_froxel_slice_transform_scale = volfog_froxel_volume_res.z * volfog_inv_res_z_plus_1;
    local float volfog_froxel_slice_end_offset = 1.0 - 1.5 * volfog_inv_res_z_plus_1;
    local float4 volfog_use_params_base = (volfog_froxel_range_params.z, 1 - volfog_froxel_range_params.z, volfog_froxel_range_params.w, 1 - volfog_froxel_range_params.w);
  }
endmacro

macro INIT_VOLUMETRIC_LIGHT_BASE_ASSUMED(code)
  if (volumetric_light_assumed_off == no)
  {
    (code) {
      view_result_inscatter@smp3d = view_result_inscatter;
      volume_fog_use_params@f4 = (volfog_use_params_base.x, volfog_use_params_base.y, volfog_froxel_range_params.y, volfog_use_params_base.z);

      //distant_fog_use_params@f1 = (distant_fog_use_3x3_bilateral_kernel, 0, 0, 0);
      distant_fog_result_inscatter@tex2d = distant_fog_result_inscatter;
      distant_fog_lowres_depth@tex2d = distant_fog_downsampled_depth_tex;

      volume_fog_use_params_depth_transform@f4 = (
        volfog_froxel_slice_transform_scale, volfog_froxel_slice_end_offset,
        volfog_inv_transition_offset, volfog_inv_transition_ratio);
    }
  }
endmacro

macro INIT_VOLUMETRIC_LIGHT_BASE_UNASSUMED(code)
  (code) {
    lowres_tex_size@f4 = lowres_tex_size;
  }
endmacro

macro INIT_VOLUMETRIC_LIGHT_BASE(code)
  INIT_VOLUMETRIC_LIGHT_BASE_ASSUMED(code)
  INIT_VOLUMETRIC_LIGHT_BASE_UNASSUMED(code)
endmacro

macro INIT_VOLUMETRIC_LIGHT(code)
  INIT_VOLUMETRIC_LIGHT_COMMON()
  INIT_VOLUMETRIC_LIGHT_BASE(code)
endmacro

macro INIT_VOLUMETRIC_LIGHT_TRANSPARENT_ONLY_INC(code)
    (code) {
      distant_fog_reprojection_dist@tex2d = distant_fog_reprojection_dist;
      volfog_froxel_range__volfog_blended_slice_start_depth@f2 = (volfog_froxel_range_params.x, volfog_blended_slice_start_depth, 0, 0);
    }
endmacro

macro INIT_VOLUMETRIC_LIGHT_TRANSPARENT(code)
  INIT_VOLUMETRIC_LIGHT_TRANSPARENT_ONLY_INC(code)
  INIT_VOLUMETRIC_LIGHT_COMMON()
  INIT_VOLUMETRIC_LIGHT_BASE(code)
endmacro

// init both vs and ps, so apply type can be changed runtime for FX
macro INIT_VOLUMETRIC_LIGHT_TRANSPARENT_VS_PS()
  INIT_VOLUMETRIC_LIGHT_TRANSPARENT_ONLY_INC(vs)
  INIT_VOLUMETRIC_LIGHT_TRANSPARENT_ONLY_INC(ps)
  INIT_VOLUMETRIC_LIGHT_COMMON()
  INIT_VOLUMETRIC_LIGHT_BASE(vs)
  INIT_VOLUMETRIC_LIGHT_BASE(ps)
endmacro


macro USE_VOLUMETRIC_LIGHT_ONLY_INC(code)
  //(code) {
  //  distant_fog_test_params@f4 = distant_fog_test_params; // for testing
  //}
  hlsl(code) {
    #define APPLY_VOLUMETRIC_FOG 1

    #ifndef DISTANT_FOG_USE_BILINEAR_SAMPLING
      #define DISTANT_FOG_USE_BILINEAR_SAMPLING 0
    #endif
    #ifndef VOLFOG_TRANSPARENT_USE
      #define VOLFOG_TRANSPARENT_USE 0
    #endif

    // for bilateral filtering, always use mip 0
    #define DISTANT_FOG_MIP (VOLFOG_TRANSPARENT_USE ? 1 : 0)


    half4 apply_fog_layer(half4 background_fog, half4 foreground_fog)
    {
      foreground_fog.rgb += foreground_fog.a*background_fog.rgb;
      foreground_fog.a *= background_fog.a;
      return foreground_fog;
    }
    void apply_fog_layer(half3 background_fog_mul, half3 background_fog_add, inout half3 foreground_fog_mul, inout half3 foreground_fog_add)
    {
      foreground_fog_add += foreground_fog_mul*background_fog_add;
      foreground_fog_mul *= background_fog_mul;
    }

    // mostly to apply 4 component volfog to 2*3 component atmo scattering
    void apply_fog_layer_to_background(half4 foreground_fog, inout half3 background_fog_mul, inout half3 background_fog_add)
    {
      background_fog_mul *= foreground_fog.a;
      background_fog_add = mad(background_fog_add, foreground_fog.a, foreground_fog.rgb);
    }

##if volumetric_light_assumed_off == no
      #define distant_fog_lowres_depth_samplerstate view_result_inscatter_samplerstate
      #define distant_fog_result_inscatter_samplerstate view_result_inscatter_samplerstate
      #define distant_fog_reprojection_dist_samplerstate view_result_inscatter_samplerstate

      float depth_to_volume_pos(float v)
      {
        float volumePos = sqrt(v*volume_fog_use_params.z);
        // restricted to fog + scattering, to avoid interpolation with the last slice
        volumePos = min(volumePos * volume_fog_use_params_depth_transform.x, volume_fog_use_params_depth_transform.y);
        return volumePos;
      }

      half4 sample_froxel_fog_raw(float3 volume_pos)
      {
        half4 scatteringInvTransmittance = half4(tex3Dlod(view_result_inscatter, float4(volume_pos, 0.0f)));
        scatteringInvTransmittance *= half(volume_fog_use_params.x);
        scatteringInvTransmittance.a += half(volume_fog_use_params.y);
        return scatteringInvTransmittance;
      }

      // fog and scattering integrated
      half4 get_froxel_fog_screenpos(float2 jittered_tc, float w)
      {
        return sample_froxel_fog_raw(float3(jittered_tc, depth_to_volume_pos(w)));
      }

      half4 sample_distant_fog_raw(float2 texcoord)
      {
        return (half4)tex2Dlod(distant_fog_result_inscatter, float4(texcoord, 0, DISTANT_FOG_MIP));
      }
      half4 load_distant_fog_raw(int2 pos)
      {
##if DEBUG
        uint2 dim;
        distant_fog_result_inscatter.GetDimensions(dim.x, dim.y);
        ##assert(all(uint2(pos) < dim.xy), "Out of bounds: distant_fog_result_inscatter has size (%.f, %.f), but access to (%.f, %.f)",
                                      dim.x, dim.y, pos.x, pos.y);
##endif

        return (half4)distant_fog_result_inscatter[pos];
      }

      float load_distant_fog_lowres_depth(int2 pos)
      {
##if DEBUG
        uint2 dim;
        distant_fog_lowres_depth.GetDimensions(dim.x, dim.y);
        ##assert(all(uint2(pos) < dim.xy), "Out of bounds: distant_fog_lowres_depth has size (%.f, %.f), but access to (%.f, %.f)",
                                      dim.x, dim.y, pos.x, pos.y);
##endif

        return distant_fog_lowres_depth[pos].x;
      }

      float4 volfog_calc_z_weights(float4 depthDiff)
      {
        return rcp( 0.00001 + depthDiff);
      }
      float4 volfog_linearize_z4(float4 raw4, float2 decode_depth)
      {
        return rcp(decode_depth.xxxx + decode_depth.y * raw4);
      }

      half4 distant_fog_bilateral_get_2x2(float2 texcoord, float linearDepth)
      {
        half4 bilinearResult = sample_distant_fog_raw(texcoord);

        #if DISTANT_FOG_USE_BILINEAR_SAMPLING
          return bilinearResult;
        #endif

        BRANCH
        if (all(abs(bilinearResult - half4(0.h,0.h,0.h,1.h)) < 0.0001h))
          return half4(0.h,0.h,0.h,1.h);

        float2 lowResCoordsCorner = texcoord*lowres_tex_size.xy - 0.5;
        int4 lowResCoordsI;
        lowResCoordsI.xy = (int2)clamp(floor(lowResCoordsCorner), 0, lowres_tex_size.xy - 1);
        lowResCoordsI.zw = min(lowResCoordsI.xy + 1, lowres_tex_size.xy - 1);

        float4 lowResRawDepth;
        lowResRawDepth.x = load_distant_fog_lowres_depth(lowResCoordsI.xy);
        lowResRawDepth.y = load_distant_fog_lowres_depth(lowResCoordsI.zy);
        lowResRawDepth.z = load_distant_fog_lowres_depth(lowResCoordsI.xw);
        lowResRawDepth.w = load_distant_fog_lowres_depth(lowResCoordsI.zw);

        float4 linearLowResDepth = volfog_linearize_z4(lowResRawDepth, zn_zfar.zw);
        float4 depthDiff = abs(linearLowResDepth - linearDepth);
        float maxDiff = max4(depthDiff.x, depthDiff.y, depthDiff.z, depthDiff.w);
        float minDiff = min4(depthDiff.x, depthDiff.y, depthDiff.z, depthDiff.w);

        bool bTinyDiff = maxDiff < linearDepth*0.05;
        bool bHugeDiff = minDiff > linearDepth*0.5;
        BRANCH
        if (bTinyDiff || bHugeDiff)
          return bilinearResult;

        half4 linearLowResWeights = (half4)volfog_calc_z_weights(depthDiff);
        linearLowResWeights *= linearLowResWeights; // this sharpens the best candidate in case there is a big diff in depths
        linearLowResWeights /= dot(linearLowResWeights, 1.h);

        return load_distant_fog_raw(lowResCoordsI.xy)*linearLowResWeights.x+
              load_distant_fog_raw(lowResCoordsI.zy)*linearLowResWeights.y+
              load_distant_fog_raw(lowResCoordsI.xw)*linearLowResWeights.z+
              load_distant_fog_raw(lowResCoordsI.zw)*linearLowResWeights.w;
      }

      // TODO: optimize it, maybe a cross is enough (OR not, since 2x2 is much cheaper and probably good enough)
      half4 distant_fog_bilateral_get_3x3(float2 texcoord, float linearDepth)
      {
        int i;
        static const int reconstructionTexelOffsetsIntCnt = 9;
        static const int2 reconstructionTexelOffsetsInt[] =
        {
          int2( 0,  0),
          int2(-1,  0),
          int2( 0, -1),
          int2( 1,  0),
          int2( 0,  1),
          int2(-1, -1),
          int2( 1, -1),
          int2(-1,  1),
          int2( 1,  1)
        };

        int2 lowresCenter = int2(round(texcoord*lowres_tex_size.xy - 0.5));
        int2 maxLowresId = int2(lowres_tex_size.xy) - 1;

        int2 lowresCoords[reconstructionTexelOffsetsIntCnt];
        UNROLL for (i = 0; i < reconstructionTexelOffsetsIntCnt; ++i)
          lowresCoords[i] = clamp(lowresCenter + reconstructionTexelOffsetsInt[i], 0, maxLowresId);

        float minDiff = NaN;
        float maxDiff = NaN;
        float linearDiffs[reconstructionTexelOffsetsIntCnt];
        UNROLL for (i = 0; i < reconstructionTexelOffsetsIntCnt; ++i)
        {
          float lowresLinearDepth = linearize_z(distant_fog_lowres_depth[lowresCoords[i]].x, zn_zfar.zw);
          float diff = abs(lowresLinearDepth - linearDepth);
          linearDiffs[i] = diff;
          minDiff = min(minDiff, diff);
          maxDiff = max(maxDiff, diff);
        }

        bool bTinyDiff = maxDiff < linearDepth*0.05;
        bool bHugeDiff = minDiff > linearDepth*0.5;
        BRANCH
        if (bTinyDiff || bHugeDiff)
          return sample_distant_fog_raw(texcoord); // bilinear result

        float weights[reconstructionTexelOffsetsIntCnt];
        float wSum = 0;
        for (i = 0; i < reconstructionTexelOffsetsIntCnt; ++i)
        {
          float w = pow2(rcp(0.00001 + linearDiffs[i]));
          wSum += w;
          weights[i] = w;
        }
        float wSumInv = 1.0 / wSum;
        half4 result = 0.h;
        for (i = 0; i < reconstructionTexelOffsetsIntCnt; ++i)
          result += load_distant_fog_raw(lowresCoords[i]) * (half)(weights[i] * wSumInv);

        return result;
      }

      half4 get_distant_fog_raw(float2 screen_tc, float w)
      {
        // TODO: temporarily disabled due to a Vulkan compiler bug
        // BRANCH
        // if (distant_fog_use_params.x)
        //   return distant_fog_bilateral_get_3x3(screen_tc, w);
        // else
          return distant_fog_bilateral_get_2x2(screen_tc, w);
      }

      // fog only, no scattering integrated, only 1 slice, at blend start (NOT necessarily the last one)
      half4 get_froxel_fog_at_blend_start(float2 screen_tc)
      {
        return sample_froxel_fog_raw(float3(screen_tc, 1));
      }

      // fog only, no scattering integrated, but only works with w > blend_start
      half4 get_volfog_from_blend_start(float2 screen_tc, float2 jittered_screen_tc, float w)
      {
        half4 result = get_froxel_fog_at_blend_start(jittered_screen_tc);
        BRANCH
        if (volume_fog_use_params.w > 0) // use distant fog
        {
          half4 froxelFog = result;
    #if !VOLFOG_TRANSPARENT_USE
          half4 distantFog = get_distant_fog_raw(screen_tc, w);
          result = apply_fog_layer(distantFog, froxelFog);
    #else
          half froxelRange = volfog_froxel_range.x;
          half blendRange = froxelRange; // rather arbitrary, can be anything > 0, but froxel range seems fine
          half rangeLimit = volfog_blended_slice_start_depth; // must be < froxelRange, and consistent with get_volfog_from_blend_start caller (see get_fast_volfog_transition_weight)
          half reprojectionDist = (half)tex2Dlod(distant_fog_reprojection_dist, float4(screen_tc, 0, DISTANT_FOG_MIP)).x;
          reprojectionDist = max(reprojectionDist, froxelRange);
          half blendStartDist = max(reprojectionDist - blendRange, rangeLimit);
          half ds = w - blendStartDist;

          // fade out DF from reprojected distance by blendRange
          BRANCH
          if (ds > 0)
          {
            half4 distantFog = get_distant_fog_raw(screen_tc, w);
            half t = saturate(ds / (reprojectionDist - blendStartDist));
            half4 combinedFog = apply_fog_layer(distantFog, froxelFog);
            result = lerp(froxelFog, combinedFog, t);
          }
    #endif
        }
        return result;
      }
      half4 get_volumetric_light_sky(float2 screen_tc, float2 jittered_screen_tc)
      {
        float w = linearize_z(0, zn_zfar.zw); // TODO: put it in a shadervar
        return get_volfog_from_blend_start(screen_tc, jittered_screen_tc, w);
      }
##else
      #define get_volumetric_light_sky(a,b) float4(0,0,0,1)
##endif
  }
endmacro


// USE_VOLUMETRIC_LIGHT macros must be used _AFTER_ USE_BRUNETON_FOG macros, compile error otherwise
macro USE_VOLUMETRIC_LIGHT_WITH_SCATTERING(code)
  hlsl(code) {
#if APPLY_BRUNETON_FOG // these can be used ONLY if daskies fog is used too
##if volumetric_light_assumed_off == no

      half get_volfog_transition_weight(float volfog_dist)
      {
        return half(volume_fog_use_params_depth_transform.z + volume_fog_use_params_depth_transform.w*volfog_dist);
      }
      half get_fast_volfog_transition_weight(float w)
      {
        // blending of (froxel fog) with (froxel fog last slice + VS interpolated scattering + distant fog)
        // to be consistent with get_volfog_from_blend_start, it must have its end point where get_volfog_transition_weight begins, other than that it can be anything
        // adding +1 spares registers, and looks fine
        return get_volfog_transition_weight(w) + 1.h;
      }

      void get_volfog_with_scattering_impl(
        bool has_no_precalced_scattering, // should use it as compile time constant
        float2 tc, float2 jittered_tc, float3 view, float dist, float w,
        inout half3 fog_mul, inout half3 fog_add)
      {
        half scatterW = get_volfog_transition_weight(w);
        BRANCH
        if (scatterW > 0.h)
        {
          if (has_no_precalced_scattering)
            get_scattering_tc_fog(tc, view, dist, fog_mul, fog_add);
          apply_fog_layer_to_background(get_volfog_from_blend_start(tc, jittered_tc, w), fog_mul, fog_add);
        }
        else
        {
          fog_mul = 1.h, fog_add = 0.h;
        }
        BRANCH
        if (scatterW < 1.h)
        {
          scatterW = saturate(scatterW);
          half3 volfog_mul, volfog_add;
          get_scattering_scatter_colored_loss(get_froxel_fog_screenpos(jittered_tc, w), w, view, dist, volfog_mul, volfog_add);
          fog_mul = lerp(volfog_mul, fog_mul, scatterW);
          fog_add = lerp(volfog_add, fog_add, scatterW);
        }
      }

      void get_volfog_with_precalculated_scattering_fast(
          float2 tc, float2 jittered_tc, float3 view, float dist, float w,
          half3 scattering_ext_color, half4 scattering_base,
          out half3 fog_mul, out half3 fog_add)
      {
        fog_add = scattering_base.rgb;
        fog_mul = color_scatter_loss(scattering_base, scattering_ext_color);

        half scatterW = get_fast_volfog_transition_weight(w);

        BRANCH
        if (scatterW < 1.h)
        {
          scatterW = saturate(scatterW);

          half4 froxelFog = get_froxel_fog_screenpos(jittered_tc, w);
          half3 volfog_mul = color_scatter_loss(froxelFog, scattering_ext_color);
          half3 volfog_add = froxelFog.rgb;

          BRANCH
          if (scatterW > 0.h)
          {
            scatterW = saturate(scatterW);
            apply_fog_layer_to_background(get_froxel_fog_at_blend_start(tc), fog_mul, fog_add);
            fog_mul = lerp(volfog_mul, fog_mul, scatterW);
            fog_add = lerp(volfog_add, fog_add, scatterW);
          }
          else
          {
            fog_mul = volfog_mul;
            fog_add = volfog_add;
          }
        }
        else
        {
          apply_fog_layer_to_background(get_volfog_from_blend_start(tc, jittered_tc, w), fog_mul, fog_add);
        }
      }

      void get_volfog_with_scattering(float2 tc, float2 jittered_tc, float3 view, float dist, float w, out half3 fog_mul, out half3 fog_add)
      {
        get_volfog_with_scattering_impl(true, tc, jittered_tc, view, dist, w, fog_mul, fog_add);
      }

      void get_volfog_with_precalculated_scattering(float2 tc, float2 jittered_tc, float3 view, float dist, float w, inout half3 fog_mul, inout half3 fog_add)
      {
        get_volfog_with_scattering_impl(false, tc, jittered_tc, view, dist, w, fog_mul, fog_add);
      }

      // inside volfog frustum, it also contains scattering loss (since scattering is integrated into volfog), but it is not significant near the camera
      half3 get_volfog_with_scattering_loss(float2 tc, float2 jittered_tc, float3 view, float dist, float w)
      {
        half scatterW = get_volfog_transition_weight(w);
        half3 fogLoss = 1.h;
        BRANCH
        if (scatterW > 0.h)
          fogLoss = get_fog_loss(view, dist) * get_volfog_from_blend_start(tc, jittered_tc, w).w;
        BRANCH
        if (scatterW < 1.h)
          fogLoss = lerp(fogLoss, get_froxel_fog_screenpos(jittered_tc, w).w, saturate(scatterW));
        return fogLoss;
      }

      void get_volfog_with_scattering(float2 tc, float2 jittered_tc, float3 pointToEye, float w, out half3 fog_mul, out half3 fog_add)
      {
        float dist2 = dot(pointToEye, pointToEye);
        float rdist = rsqrt(dist2);
        get_volfog_with_scattering(tc, jittered_tc, pointToEye*rdist, dist2*rdist, w, fog_mul, fog_add);
      }

      void apply_volfog_with_scattering(inout half3 result, float2 tc, float2 jittered_tc, float3 view, float dist, float w)
      {
        half3 fog_mul, fog_add;
        get_volfog_with_scattering(tc, jittered_tc, view, dist, w, fog_mul, fog_add);
        result = mad(result, fog_mul, fog_add);
      }

      void apply_volfog_with_scattering(inout half3 result, float2 tc, float2 jittered_tc, float3 pointToEye, float w)
      {
        half3 fog_mul, fog_add;
        get_volfog_with_scattering(tc, jittered_tc, pointToEye, w, fog_mul, fog_add);
        result = mad(result, fog_mul, fog_add);
      }
##else // no volfog -> daskies fog only
      void get_volfog_with_scattering(float2 tc, float2 jittered_tc, float3 view, float dist, float w, out half3 fog_mul, out half3 fog_add)
      {
        get_scattering_tc_fog(tc, view, dist, fog_mul, fog_add);
      }
      void get_volfog_with_scattering(float2 tc, float2 jittered_tc, float3 pointToEye, float w, out half3 fog_mul, out half3 fog_add)
      {
        get_scattering_tc_fog(tc, pointToEye, fog_mul, fog_add);
      }
      void get_volfog_with_precalculated_scattering(float2 tc, float2 jittered_tc, float3 view, float dist, float w, inout half3 fog_mul, inout half3 fog_add)
      {
        // no volfog
      }
      void get_volfog_with_precalculated_scattering_fast(
          float2 tc, float2 jittered_tc, float3 view, float dist, float w,
          half3 scattering_ext_color, half4 scattering_base,
          out half3 fog_mul, out half3 fog_add)
      {
        fog_add = scattering_base.rgb;
        fog_mul = color_scatter_loss(scattering_base, scattering_ext_color);
      }
      half3 get_volfog_with_scattering_loss(float2 tc, float2 jittered_tc, float3 view, float dist, float w)
      {
        return (half3)get_fog_loss(view, dist);
      }
      void apply_volfog_with_scattering(inout half3 result, float2 tc, float2 jittered_tc, float3 view, float dist, float w)
      {
        apply_scattering_tc_fog(tc, view, dist, result);
      }
      void apply_volfog_with_scattering(inout half3 result, float2 tc, float2 jittered_tc, float3 pointToEye, float w)
      {
        apply_scattering_tc_fog(tc, pointToEye, result);
      }
##endif
#endif
  }
endmacro


macro USE_VOLUMETRIC_LIGHT_INC(code)
  USE_VOLUMETRIC_LIGHT_ONLY_INC(code)
  USE_VOLUMETRIC_LIGHT_WITH_SCATTERING(code)
endmacro


macro USE_VOLUMETRIC_LIGHT(code)
  USE_VOLUMETRIC_LIGHT_INC(code)
endmacro


macro USE_VOLUMETRIC_LIGHT_TRANSPARENT(code)
  hlsl(code) {
    #define VOLFOG_TRANSPARENT_USE 1
    #define DISTANT_FOG_USE_BILINEAR_SAMPLING 1

    #define volfog_froxel_range (volfog_froxel_range__volfog_blended_slice_start_depth.x)
    #define volfog_blended_slice_start_depth (volfog_froxel_range__volfog_blended_slice_start_depth.y)
  }
  USE_VOLUMETRIC_LIGHT_INC(code)
endmacro



macro USE_VOLFOG_POISSON_SAMPLES(code)
hlsl(code) {
#ifndef USE_VOLFOG_POISSON_SAMPLES_INCLUDE
#define USE_VOLFOG_POISSON_SAMPLES_INCLUDE 1

  #define VOLFOG_POISSON_SAMPLES_CNT 8
  static const float2 VOLFOG_POISSON_SAMPLES[VOLFOG_POISSON_SAMPLES_CNT] =
  {
    float2(0.228132254148f-0.5f, 0.67232428631f-0.5f),
    float2(0.848556554824f-0.5f, 0.135723477704f-0.5f),
    float2(0.74820789575f-0.5f, 0.63965073852f-0.5f),
    float2(0.203231652569f-0.5f, 0.12436704431f-0.5f),
    float2(0.472544801767f-0.5f, 0.351474129111f-0.5f),
    float2(0.962881642535f-0.5f, 0.387342871273f-0.5f),
    float2(0.0875977149838f-0.5f, 0.896250211998f-0.5f),
    float2(0.56452806916f-0.5f, 0.974024350484f-0.5f),
  };

#endif
}
endmacro

macro CUSTOM_FOG_INC(code)
  USE_VOLFOG_POISSON_SAMPLES(code)
  hlsl(code) {
    #include <interleavedGradientNoise.hlsl>

    #define APPLY_CUSTOM_FOG 1
    #if APPLY_CUSTOM_FOG
    void apply_sky_custom_fog(inout half3 val, float2 screen_tc, float2 jittered_screen_tc)
    {
      half4 fog = half4(get_volumetric_light_sky(screen_tc, jittered_screen_tc));
      val = val*fog.a + fog.rgb;
    }
    void apply_sky_custom_fog(inout half4 val, float2 screen_tc, float2 jittered_screen_tc)
    {
      half4 fog = half4(get_volumetric_light_sky(screen_tc, jittered_screen_tc));
      val = apply_fog_layer(val, fog);
    }
    #else
    #define apply_sky_custom_fog(a,b,c)
    #endif
  }
endmacro

macro CUSTOM_FOG(code)
  INIT_VOLUMETRIC_LIGHT(code)
  USE_VOLUMETRIC_LIGHT(code)
  CUSTOM_FOG_INC(code)
endmacro

macro USE_VOLFOG_DITHERING_NOISE(code)
  USE_VOLFOG_POISSON_SAMPLES(code)
  (code) {
    shadow_params@f4 = (shadow_frame, 0, inv_volfog_froxel_volume_res.x, inv_volfog_froxel_volume_res.y);
  }
  hlsl(code) {
    #include <interleavedGradientNoise.hlsl>

    float2 get_volfog_dithered_screen_tc(float2 screenpos, float2 tc)
    {
      float dither = interleavedGradientNoiseFramed( screenpos, shadow_params.x);
      return tc + VOLFOG_POISSON_SAMPLES[uint(dither*VOLFOG_POISSON_SAMPLES_CNT)]*shadow_params.zw;
    }
  }
endmacro
