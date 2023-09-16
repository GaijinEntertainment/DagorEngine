#include <pixelPacking/yCoCgSpace.hlsl>
#include <tex2d_bicubic.hlsl>
#include <fp16_aware_lerp.hlsl>
#define TAA_BETTER_MOTION_VECTOR 0//accurate motion vector. we don't need it, it seem
#ifndef TAA_BILINEAR
#define TAA_BILINEAR 1//if we fix discontinuities it should be 1. Otherwise it is also faster and blurrier (which is fine for clouds)
#endif

#if !TAA_BILINEAR && !SUPPORT_TEXTURE_GATHER
  #undef TAA_BILINEAR
  #define TAA_BILINEAR 1
#endif

#define TAA_LUMINANCE_MIN 0.05
// Disables clamping entirely
//#define TAA_DEBUG_DISABLE_CLAMPING 1
#define TAA_COLOR_CLIPPING_BASED_ON_ML 1

//ignore motion
//#define TAA_DEBUG_NO_MOTION_WEIGHT 1

//very simple taa, without clipping or motion weights
//#define TAA_DEBUG_SIMPLE_TAA 1

// Tonemap samples from HDR to perceptual space before integration, then inverse tonemap back to HDR space
#define TAA_COLOR_ALPHA 1

#define TAA_IN_HDR_SPACE 0

// Use optimized color neighborhood
//#define TAA_USE_OPTIMIZED_NEIGHBORHOOD

// Clamps luminance to a maximum value. This increases image stability for very bright pixels.
#define TAA_USE_LUMINANCE_CLAMPING 0

// Use AABB clipping instead of clamping
//#define TAA_USE_AABB_CLIPPING 1
#define TAA_VARIANCE_CLIP_WITH_MINMAX 1
#define TAA_VARIANCE_CLIPPING 1
#define TAA_CLAMPING_GAMMA 1.2

#define TAA_NEAREST_DEPTH_SEARCH_FILTER 1

//#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET 1//clip with offsetted min max

#ifndef TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN
#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN 1.125
#endif

//#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_ADDITION 1

#ifndef TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX
#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX 1.125
#endif

//#define TAA_ALWAYS_BLUR 0
//#define TAA_NEW_FRAME_WEIGHT_BLUR 0

#define color_type half4
#define color_attr rgba


#if CLOUDS_FULLRES
  #undef TAA_BILINEAR
  #define TAA_BILINEAR 0
  #undef TAA_USE_OPTIMIZED_NEIGHBORHOOD
  #define TAA_USE_OPTIMIZED_NEIGHBORHOOD 0
  #undef TAA_BETTER_MOTION_VECTOR
  #define TAA_BETTER_MOTION_VECTOR 1
#endif

color_type TAA_clip_history(color_type history, color_type current, color_type colorMin, color_type colorMax, float motionVectorPixelLength)
{
#if TAA_COLOR_CLIPPING_BASED_ON_ML
  float ml = saturate(0.5-motionVectorPixelLength);//If we move less than 0.5 pixel, we increase acceptance of history
  colorMin -= ml;
  colorMax += ml;
#endif

#if TAA_DEBUG_DISABLE_CLAMPING
  return history;
#elif TAA_USE_AABB_CLIPPING
  // Clip color difference against neighborhood min/max AABB
  float3 boxCenter = (colorMax.rgb + colorMin.rgb) * 0.5;
  float3 boxExtents = colorMax.rgb - boxCenter;

  float3 rayDir = current.rgb - history.rgb;
  float3 rayOrg = history.rgb - boxCenter;

  float clipLength = 1.0;

  if (length(rayDir) > 1e-6)
  {
    // Intersection using slabs
    float3 rcpDir = rcp(rayDir);
    float3 tNeg = ( boxExtents - rayOrg) * rcpDir;
    float3 tPos = (-boxExtents - rayOrg) * rcpDir;
    clipLength = saturate(max(max(min(tNeg.x, tPos.x), min(tNeg.y, tPos.y)), min(tNeg.z, tPos.z)));
  }

  return half4(lerp(history.rgb, current.rgb, clipLength), clamp(history.a, colorMin.a, colorMax.a));
#else
  return max(colorMin, min(colorMax, history));
#endif
}

float2 TAAGetReprojectedMotionVector(float depth, float2 uv, float2 screenSize, out float motionVectorPixelLengthTolerance)
{
  //todo:reflection matrix is oblique, and can't be obtained with viewVec
  //use invglobtm instead
  float3 viewVect = getViewVecOptimized(uv);
  float3 view = normalize(viewVect);
  float3 cameraPos = view*depth;
  //---
  {
    float4 curClip = mul(float4(cameraPos + move_world_view_pos, 0), globtm_no_ofs_psf);
    float2 movePixelScreen = curClip.xy*rcp(curClip.w);
    motionVectorPixelLengthTolerance = curClip.w <= 0 ? 1000 : length((movePixelScreen*float2(0.5, -0.5) + float2(0.5, 0.5) - uv)*screenSize);
  }
  float4 prevClip = mul(float4(cameraPos, 0), prev_globtm_no_ofs_psf) + reprojected_world_view_pos;
  float2 prevScreen = prevClip.xy*rcp(prevClip.w);
  float2 prevUV = prevScreen*float2(0.5, -0.5) + float2(0.5, 0.5);//todo: this can be removed/(injected to prev_globtm_no_ofs_psf)
  float2 ret = prevClip.w > 0 ? prevUV.xy : float2(2,2);
  return ret;

  //acually it is way faster. we'd better use that code! (as well as for ssr/ssao)
  //todo:
  //float4 prevUV = mul(float4(uv.x, uv.y, rawDepth, 1.0), reprojectionMatrix);
  //prevUV.xy /= prevUV.w;
  //float2 ret = prevUV.xy - uv;
  //return ret;
}

struct TAACurrent
{
  color_type color;
  color_type colorBlurred, colorVeryBlurred;
  color_type colorMin;
  color_type colorMax;
  half  lumaContrast;
  float nearestDepth, farDepth;//cloudDepth
  float2 motionVectorUV;
};

void TAA_gather_current(Texture2D<float4> sceneTex, SamplerState sceneTex_samplerstate,
                        Texture2D<float4> cloudsDepthTex, SamplerState cloudsDepthTex_samplerstate,
                        float2 screenUV, float2 screenSize, float2 screenSizeInverse, float exposure,
                        out TAACurrent current)
{
  const float INFINITY = 65535.0;

  current = (TAACurrent)0;

  float3 colorMoment1 = 0.0;
  float3 colorMoment2 = 0.0;
  color_type colorMin =  INFINITY;
  color_type colorMax = -INFINITY;
  color_type colorFiltered = 0.0;
  half  colorWeightTotal = 0.0;

  const uint TAP_COUNT_NEIGHBORHOOD_LIMITED = 5;
  const uint TAP_COUNT_NEIGHBORHOOD_FULL = 9;

  #if TAA_USE_OPTIMIZED_NEIGHBORHOOD
    const float blurWeights[TAP_COUNT_NEIGHBORHOOD_LIMITED] = {
        4./8.,
        1./8.,
        1./8.,
        1./8.,
        1./8.
      };
  #else
    const float blurWeights[TAP_COUNT_NEIGHBORHOOD_FULL] = {
      4./10.,
      1./10.,
      1./10.,
      1./10.,
      1./10.,
      0.5/10.,
      0.5/10.,
      0.5/10.,
      0.5/10.
    };
  #endif


  const float2 uvOffsets[TAP_COUNT_NEIGHBORHOOD_FULL] =
  {
    screenUV,
    screenUV + float2(-screenSizeInverse.x,  0.0),
    screenUV + float2( 0.0,       -screenSizeInverse.y),
    screenUV + float2( screenSizeInverse.x,  0.0),
    screenUV + float2( 0.0,        screenSizeInverse.y),
    screenUV + float2(-screenSizeInverse.x, -screenSizeInverse.y),//todo: remove this mul, use indexed CB
    screenUV + float2( screenSizeInverse.x, -screenSizeInverse.y),
    screenUV + float2(-screenSizeInverse.x,  screenSizeInverse.y),
    screenUV + float2( screenSizeInverse.x,  screenSizeInverse.y)
  };

  //
  // Color neighborhood calculation
  //

  #if TAA_USE_OPTIMIZED_NEIGHBORHOOD
    const uint neighborhoodSize = TAP_COUNT_NEIGHBORHOOD_LIMITED;
  #else
    const uint neighborhoodSize = TAP_COUNT_NEIGHBORHOOD_FULL;
  #endif

  current.colorBlurred = 0;
  UNROLL
  for (uint i = 0; i < neighborhoodSize; i++)
  {
    color_type color = tex2Dlod(sceneTex, float4(uvOffsets[i],0,0));
    #if !ALREADY_TONEMAPPED_SCENE
      color = PackToYCoCgAlpha(color) . color_attr;
      #if TAA_IN_HDR_SPACE
        color.rgb *= simple_luma_tonemap(color.x, exposure);
      #endif
    #elif ALREADY_TONEMAPPED_SCENE == CLOUDS_TONEMAPPED_TO_SRGB
      color.x *= 2.0;//todo: should be exposure value!
      color.gb = color.gb*2-1;
    #endif

    if (i == 0)
      current.color = color;

    #if TAA_USE_FILTERING
      colorFiltered += color * filterWeights[i];
      colorWeightTotal += filterWeights[i];
    #endif

    #if TAA_NEW_FRAME_WEIGHT_BLUR || TAA_ALWAYS_BLUR || ((TAA_NEW_FRAME_WEIGHT_BLUR_WHERE_ALLOWED || TAA_ALWAYS_BLUR_WHERE_ALLOWED) && SUPPORT_TEXTURE_GATHER && !DONT_CHECK_DEPTH_DISCONTINUITY)
      current.colorBlurred += color * (blurWeights[i]);
      current.colorVeryBlurred += color;
    #endif

    colorMoment1.rgb += color.rgb;
    colorMoment2.rgb += color.rgb * color.rgb;
    colorMin = min(colorMin, color);
    colorMax = max(colorMax, color);
  }
  #if TAA_NEW_FRAME_WEIGHT_BLUR || TAA_ALWAYS_BLUR || ((TAA_NEW_FRAME_WEIGHT_BLUR_WHERE_ALLOWED || TAA_ALWAYS_BLUR_WHERE_ALLOWED) && SUPPORT_TEXTURE_GATHER && !DONT_CHECK_DEPTH_DISCONTINUITY)
    current.colorVeryBlurred *= 1./neighborhoodSize;
  #endif


  //
  // Motion Vector UV calculation by dilating velocity using nearest depth
  //
  {
    #if SUPPORT_TEXTURE_GATHER && !TAA_BETTER_MOTION_VECTOR
    {
      //this cost around 10% of all clouds TAA, but reduces ghosting, as we cover whole 3x3 area instead of just X
      float4 cloudsDepth4 = cloudsDepthTex.GatherRed(cloudsDepthTex_samplerstate, screenUV-0.5*screenSizeInverse);
      float4 cloudsDepth4Max = cloudsDepth4, cloudsDepth4Min = cloudsDepth4;
      cloudsDepth4 = cloudsDepthTex.GatherRed(cloudsDepthTex_samplerstate, screenUV+float2(+0.5*screenSizeInverse.x, -0.5*screenSizeInverse.y));
      cloudsDepth4Min = min(cloudsDepth4, cloudsDepth4Min); cloudsDepth4Max = max(cloudsDepth4, cloudsDepth4Max);
      cloudsDepth4 = cloudsDepthTex.GatherRed(cloudsDepthTex_samplerstate, screenUV+float2(-0.5*screenSizeInverse.x, +0.5*screenSizeInverse.y));
      cloudsDepth4Min = min(cloudsDepth4, cloudsDepth4Min); cloudsDepth4Max = max(cloudsDepth4, cloudsDepth4Max);
      cloudsDepth4 = cloudsDepthTex.GatherRed(cloudsDepthTex_samplerstate, screenUV+float2(+0.5*screenSizeInverse.x, +0.5*screenSizeInverse.y));
      cloudsDepth4Min = min(cloudsDepth4, cloudsDepth4Min); cloudsDepth4Max = max(cloudsDepth4, cloudsDepth4Max);
      current.nearestDepth = min(min(cloudsDepth4Min.x, cloudsDepth4Min.y), min(cloudsDepth4Min.z, cloudsDepth4Min.w));
      current.farDepth = max(max(cloudsDepth4Max.x, cloudsDepth4Max.y), max(cloudsDepth4Max.z, cloudsDepth4Max.w));
    }
    #else
      const int TAP_COUNT_CROSS = 5;
      float depthCross[TAP_COUNT_CROSS] =
      {
        tex2Dlod(cloudsDepthTex, float4(screenUV,0,0)).x,
        tex2Dlod(cloudsDepthTex, float4(uvOffsets[5],0,0)).x,
        tex2Dlod(cloudsDepthTex, float4(uvOffsets[6],0,0)).x,
        tex2Dlod(cloudsDepthTex, float4(uvOffsets[7],0,0)).x,
        tex2Dlod(cloudsDepthTex, float4(uvOffsets[8],0,0)).x,
      };

      //current.cloudDepth =
      current.nearestDepth = current.farDepth = depthCross[0];
      current.motionVectorUV = screenUV;
      #if TAA_NEAREST_DEPTH_SEARCH_FILTER
      UNROLL
      for (int i = 1; i < TAP_COUNT_CROSS; i++)
      {
        current.nearestDepth = min(depthCross[i], current.nearestDepth);
        current.farDepth = max(depthCross[i], current.farDepth);
        #if TAA_BETTER_MOTION_VECTOR
          if (depthCross[i] < current.nearestDepth)
              current.motionVectorUV = uvOffsets[i];
        #endif
      }
      #endif
    #endif
    //current.cloudDepth *= TRACE_DIST;
    current.nearestDepth *= TRACE_DIST;
    current.farDepth *= TRACE_DIST;
  }
  //
  // Compute color bounds using the standard deviation of the neighborhood.
  //
  colorMoment1 /= (float)neighborhoodSize;
  colorMoment2 /= (float)neighborhoodSize;

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
        float3 colorBoxCenter = 0.5*(colorMin+colorMax);
        float3 colorBoxExt = colorMax - colorBoxCenter;
        float3 colorMinInset = clamp(colorBoxCenter - TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN*colorBoxExt, YCOCG_MIN, YCOCG_MAX);
        float3 colorMaxInset = clamp(colorBoxCenter + TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX*colorBoxExt, YCOCG_MIN, YCOCG_MAX);
        colorMin2 = clamp(colorMin2, colorMinInset, colorMaxInset);
        colorMax2 = clamp(colorMax2, colorMinInset, colorMaxInset);
      #else
        colorMin2 = clamp(colorMin2, colorMin.rgb, colorMax.rgb);
        colorMax2 = clamp(colorMax2, colorMin.rgb, colorMax.rgb);
      #endif
    #elif TAA_IN_HDR_SPACE
      colorMin2 = clamp(colorMin2, YCOCG_MIN, YCOCG_MAX);
      colorMax2 = clamp(colorMax2, YCOCG_MIN, YCOCG_MAX);
    #endif

    #if TAA_VARIANCE_CLIPPING
      colorMin.rgb = colorMin2.rgb;
      colorMax.rgb = colorMax2.rgb;
    #endif
  #endif

  half lumaContrast = colorMax.x - colorMin.x;

  //
  // Compute color filtering
  //
  #if TAA_USE_FILTERING
  {
    colorFiltered /= colorWeightTotal;
    current.color = lerp(current.color, colorFiltered, saturate(lumaContrast));//todo: check if pixel is totally new, than we can always use filtered
  }
  #endif

  current.colorMin = colorMin;
  current.colorMax = colorMax;
  current.lumaContrast = lumaContrast;
}

struct TAAHistory
{
  half4 color;
};

//historyTex_samplerstate HAS to be linear
void TAA_gather_history(Texture2D<float4> historyTex, SamplerState historyTex_samplerstate,
                      float2 screenUV, float2 screenSize, float2 screenSizeInverse, half exposure, out TAAHistory history,
                      bool use_bilinear)
{
  if (use_bilinear)
  {
    float4 c = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(screenUV,0,0)));
    #if TAA_IN_HDR_SPACE
    c.rgb *= simple_luma_tonemap(c10.x, exposure);
    #endif
    history.color = c;
    return;
  }
  BicubicSharpenWeights weights;
  compute_bicubic_sharpen_weights(screenUV, screenSize, screenSizeInverse, TAA_SHARPENING_FACTOR, weights);

  float4 c10 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv0,0,0)));
  float4 c01 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv1,0,0)));
  float4 c11 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv2,0,0)));
  float4 c21 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv3,0,0)));
  float4 c12 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv4,0,0)));

  #if TAA_IN_HDR_SPACE
    c10.rgb *= simple_luma_tonemap(c10.x, exposure);
    c01.rgb *= simple_luma_tonemap(c01.x, exposure);
    c11.rgb *= simple_luma_tonemap(c11.x, exposure);
    c21.rgb *= simple_luma_tonemap(c21.x, exposure);
    c12.rgb *= simple_luma_tonemap(c12.x, exposure);
  #endif

  history.color = (c10 * weights.w0 + c01 * weights.w1 + c11 * weights.w2 + c21 * weights.w3 + c12 * weights.w4) / weights.weightsSum;
  history.color.ra = max(history.color.ra, 0);
}

//historyTex_samplerstate HAS to be linear
//sceneTex_samplerstate HAS to be point filtered

float4 linearize_z4(float4 raw4, float2 decode_depth)
{
  return rcp(decode_depth.xxxx + decode_depth.y * raw4);
}

void TAA(out float4 result_color, out float taaWeight,
         Texture2D<float4> sceneTex, SamplerState sceneTex_samplerstate,
         Texture2D<float4> cloudsDepthTex, SamplerState cloudsDepthTex_samplerstate,
         Texture2D<float4> historyTex, SamplerState historyTex_samplerstate,
         Texture2D<float4> nativeDepthTex, SamplerState nativeDepthTex_samplerstate,
         Texture2D<float4> nativePrevDepthTex, SamplerState nativePrevDepthTex_samplerstate,
         Texture2D<float4> taaPrevWeight, SamplerState taaPrevWeight_samplerstate,
         //float4x4 reprojectionMatrix,
         const float2 screenUV, const float2 screenSize, const float2 screenSizeInverse, const float exposure
         #if TAA_OUTPUT_FRAME_WEIGHT
           ,out float frameWeightOut
         #endif
         )
{
  //
  // Gather the current frame neighborhood information.
  //
  TAACurrent current;
  TAA_gather_current(sceneTex, sceneTex_samplerstate, cloudsDepthTex, cloudsDepthTex_samplerstate, screenUV, screenSize, screenSizeInverse, exposure, current);
  float reprojectionDepth = current.nearestDepth;

  #if !DONT_CHECK_DEPTH_DISCONTINUITY
    float viewVecLen = length(getViewVecOptimized(screenUV));
    float rawDepth = tex2Dlod(nativeDepthTex, float4(screenUV,0,0)).x;
    float linearDepthW = linearize_z(rawDepth, zn_zfar.zw);
    float linearDepth = linearDepthW*viewVecLen;
    reprojectionDepth = min(reprojectionDepth, linearDepth);
    //since we no longer embed native depth into clouds depth, it can happen that nearest clouds pixel is farther than native depth
    //but it is obvious that we can't reproject real cloud pixel with farther depth that was used for raymarching
    //the reason why we NOT embed native depth into clouds depth, is that otherwise it can find that closest native depth on NEAR pixels, using depth search (3x3)
    //which is obviously also incorrect
  #endif

  // Reproject uv value to motion from previous frame.
  half motionVectorPixelLengthTolerance;
  #if TAA_BETTER_MOTION_VECTOR
    float2 motionVector = TAAGetReprojectedMotionVector(reprojectionDepth, current.motionVectorUV, screenSize, motionVectorPixelLengthTolerance);//, reprojectionMatrix
    float2 historyUV = (screenUV - current.motionVectorUV + motionVector);
  #else
    float2 historyUV = TAAGetReprojectedMotionVector(reprojectionDepth, screenUV, screenSize, motionVectorPixelLengthTolerance);//, reprojectionMatrix
    float2 motionVector = historyUV-screenUV;
  #endif
  bool bOffscreen = historyUV.x >= 1.0 || historyUV.x <= TAA_RESTART_TEMPORAL_X || historyUV.y >= 1.0 || historyUV.y <= 0.0;

  bool bilinear = TAA_BILINEAR;

  #if !DONT_CHECK_DEPTH_DISCONTINUITY
  if (!bOffscreen)
  {
    float acceptanceThreshold = move_world_view_pos_tolerance + 0.02*linearDepth;
    float historyViewVecLenScale = length(getPrevViewVecOptimized(historyUV));

    float2 histCrd = historyUV*screenSize - 0.5;//should be 0.5, but for some reason it is not
    float2 histCrdFloored = floor(histCrd);
    float2 histTCBilinear = histCrdFloored*screenSizeInverse + screenSizeInverse;
    #if !TAA_BILINEAR && !CLOUDS_FULLRES
    {
      BicubicSharpenWeights weights;
      compute_bicubic_sharpen_weights(historyUV, screenSize, screenSizeInverse, TAA_SHARPENING_FACTOR, weights);
      float4 rawHistoryDepth4_1 = nativePrevDepthTex.GatherRed(nativePrevDepthTex_samplerstate, floor(weights.uv0*screenSize - 0.5)*screenSizeInverse + screenSizeInverse);
      float4 rawHistoryDepth4_2 = nativePrevDepthTex.GatherRed(nativePrevDepthTex_samplerstate, floor(weights.uv1*screenSize - 0.5)*screenSizeInverse + screenSizeInverse);
      float4 rawHistoryDepth4_3 = nativePrevDepthTex.GatherRed(nativePrevDepthTex_samplerstate, floor(weights.uv2*screenSize - 0.5)*screenSizeInverse + screenSizeInverse);
      float4 rawHistoryDepth4_4 = nativePrevDepthTex.GatherRed(nativePrevDepthTex_samplerstate, floor(weights.uv3*screenSize - 0.5)*screenSizeInverse + screenSizeInverse);
      float4 rawHistoryDepth4_5 = nativePrevDepthTex.GatherRed(nativePrevDepthTex_samplerstate, floor(weights.uv4*screenSize - 0.5)*screenSizeInverse + screenSizeInverse);
      float4 rawHistoryDepthMin4 = min(min(min(rawHistoryDepth4_1, rawHistoryDepth4_2), min(rawHistoryDepth4_3, rawHistoryDepth4_4)), rawHistoryDepth4_5);
      float4 rawHistoryDepthMax4 = max(max(max(rawHistoryDepth4_1, rawHistoryDepth4_2), max(rawHistoryDepth4_3, rawHistoryDepth4_4)), rawHistoryDepth4_5);
      float  rawHistoryDepthMin = min(min(rawHistoryDepthMin4.x, rawHistoryDepthMin4.y), min(rawHistoryDepthMin4.z, rawHistoryDepthMin4.w));
      float  rawHistoryDepthMax = max(max(rawHistoryDepthMax4.x, rawHistoryDepthMax4.y), max(rawHistoryDepthMax4.z, rawHistoryDepthMax4.w));

      float closestZbufferDepth = linearize_z(rawHistoryDepthMax, zn_zfar.zw)*historyViewVecLenScale;
      float farthestZbufferDepth = linearize_z(rawHistoryDepthMin, zn_zfar.zw)*historyViewVecLenScale;
      float maxCubeDifference = max(abs(closestZbufferDepth - linearDepth), abs(farthestZbufferDepth - linearDepth));

      if ((rawDepth!=0 || rawHistoryDepthMax!=0)//only if history or current depth is not infinite. otherwise it is both just clouds (infinity)
        && min(closestZbufferDepth, linearDepth) < current.farDepth*1.05//if closest depth buffer depth is closer than farthest clouds depth. otherwise clouds are in front of scene
        && maxCubeDifference > acceptanceThreshold//if there is discontinuity between history and current depth
        )//already offscreen, history ignored
        bilinear = true;
    }
    #endif
    if (bilinear)
    {
      //fixme: should be exactly size of nativePrevDepthTex, not screenSize!
      float2 wght = histCrd - histCrdFloored;
      float4 rawHistoryDepth4;
      #if SUPPORT_TEXTURE_GATHER
        rawHistoryDepth4 = nativePrevDepthTex.GatherRed(nativePrevDepthTex_samplerstate, histTCBilinear).wzxy;
      #else
        int2 histCrdI = histCrd;
        rawHistoryDepth4.x = nativePrevDepthTex[histCrdI].x;
        rawHistoryDepth4.y = nativePrevDepthTex[histCrdI+int2(1,0)].x;
        rawHistoryDepth4.z = nativePrevDepthTex[histCrdI+int2(0,1)].x;
        rawHistoryDepth4.w = nativePrevDepthTex[histCrdI+int2(1,1)].x;
      #endif
      float4 linearHistoryDepth4 = linearize_z4(rawHistoryDepth4,zn_zfar.zw);
      linearHistoryDepth4 *= historyViewVecLenScale;
      float closestZbufferDepth = min3(min(linearHistoryDepth4.x, linearHistoryDepth4.y), min(linearHistoryDepth4.z, linearHistoryDepth4.w), linearDepth);
      float4 difference = abs(linearHistoryDepth4 - linearDepth.xxxx);
      bool4 cornerWeights = bool4(wght<0.999, wght>0.001);
      float4 maxDifference = difference * float4(cornerWeights.x&&cornerWeights.y, cornerWeights.z&&cornerWeights.y,
                                                 cornerWeights.x&&cornerWeights.w, cornerWeights.z&&cornerWeights.w);


      if ((rawDepth!=0 || any(rawHistoryDepth4!=0))//only if history or current depth is not infinite. otherwise it is both just clouds (infinity)
        && closestZbufferDepth < current.farDepth*1.05//if closest depth buffer depth is closer than farthest clouds depth. otherwise clouds are in front of scene
        && max(max(maxDifference.x, maxDifference.y), max(maxDifference.z, maxDifference.w)) > acceptanceThreshold//if there is discontinuity between history and current depth
        )//already offscreen, history ignored
        //&& closestZbufferDepth < current.nearestDepth*TRACE_DIST)//also works, but a bit ghosting
      {
        //difference += 0.02*linearDepth*float4(wght.x + wght.y, (1-wght.x + wght.y), wght.x + 1-wght.y, 2-wght.x-wght.y);//add some bilinear weights
        float minDiff = min(min(difference.x, difference.y), min(difference.z, difference.w));
        #if !TAA_DONT_ALLOW_LINES_HISTORY_LERP
        if (max(difference.x, difference.y) < acceptanceThreshold)//horizonatal linear interpolations
          wght.y = 0;
        else if (max(difference.z, difference.w) < acceptanceThreshold)//horizonatal linear interpolations
          wght.y = 1;
        else if (max(difference.x, difference.z) < acceptanceThreshold)//vertical linear interpolations
          wght.x = 0;
        else if (max(difference.y, difference.w) < acceptanceThreshold)//vertical linear interpolations
          wght.x = 1;
        else
        #endif
        {
          //select one best corner
          wght = minDiff == difference.x ? float2(0,0) : (minDiff == difference.y ? float2(1,0) : (minDiff == difference.z ? float2(0,1) : float2(1,1)));
        }
        historyUV = (histCrdFloored + wght + 0.5)*screenSizeInverse;
        motionVector = historyUV-screenUV;
        FLATTEN
        if (minDiff > acceptanceThreshold)
          bOffscreen = true;
      }
    }
  }
  #endif
  FLATTEN

  //motionVectorPixelLengthTolerance = length(motionVector*screenSize);
  if (bOffscreen)
    current.colorBlurred = current.colorVeryBlurred;
  //return;

  #if TAA_DEBUG_SIMPLE_TAA
  {
    float3 hist = PackToYCoCg(tex2Dlod(historyTex, float4(historyUV,0,0)));
    hist.rgb *= simple_luma_tonemap(hist.x, exposure);
    result_color = lerp(current.color, hist.rgb, bOffscreen ? 1 : TAA_NEW_FRAME_WEIGHT);
    result_color *= simple_luma_tonemap_inv(result_color.x, exposure);
    result_color = UnpackFromYCoCg(result_color);
    history_buffer = 0;
    return;
  }
  #endif
  //
  // Gather the previous frame neighborhood information.
  //
  TAAHistory history;
  TAA_gather_history(historyTex, historyTex_samplerstate, historyUV, screenSize, screenSizeInverse, exposure, history, bilinear);

  //
  // Compute temporal blend weight.
  //

  // Motion vector changes.
  float newTaaEventFrame = 0;
  const float blurredFrameNo = all(abs(screenUV*2-1) < 2*screenSizeInverse) ? 3./255 : 0./255;//blurred color isn't as bad, as completely new one. but blurring outside isngt
  #if TAA_ALWAYS_BLUR
    current.color = current.colorBlurred;
    newTaaEventFrame = blurredFrameNo;
  #elif TAA_NEW_FRAME_WEIGHT_BLUR
    current.color = bOffscreen ? current.colorBlurred : current.color;
    newTaaEventFrame = blurredFrameNo;
  #elif (TAA_NEW_FRAME_WEIGHT_BLUR_WHERE_ALLOWED || TAA_ALWAYS_BLUR_WHERE_ALLOWED) && SUPPORT_TEXTURE_GATHER && !DONT_CHECK_DEPTH_DISCONTINUITY
    #if !TAA_ALWAYS_BLUR_WHERE_ALLOWED
    BRANCH
    if (bOffscreen)
    #endif
    {
      float4 rawCurrentDepth4Min, rawCurrentDepth4Max, rawCurrentDepth4;
      rawCurrentDepth4 = nativeDepthTex.GatherRed(nativeDepthTex_samplerstate, screenUV-0.5*screenSizeInverse);
      rawCurrentDepth4Min = rawCurrentDepth4Max = rawCurrentDepth4;
      rawCurrentDepth4 = nativeDepthTex.GatherRed(nativeDepthTex_samplerstate, screenUV+float2(0.5*screenSizeInverse.x, -0.5*screenSizeInverse.y));
      rawCurrentDepth4Min = min(rawCurrentDepth4, rawCurrentDepth4Min); rawCurrentDepth4Max = max(rawCurrentDepth4, rawCurrentDepth4Max);
      rawCurrentDepth4 = nativeDepthTex.GatherRed(nativeDepthTex_samplerstate, screenUV+float2(-0.5*screenSizeInverse.x, 0.5*screenSizeInverse.y));
      rawCurrentDepth4Min = min(rawCurrentDepth4, rawCurrentDepth4Min); rawCurrentDepth4Max = max(rawCurrentDepth4, rawCurrentDepth4Max);
      rawCurrentDepth4 = nativeDepthTex.GatherRed(nativeDepthTex_samplerstate, screenUV+0.5*screenSizeInverse);
      rawCurrentDepth4Min = min(rawCurrentDepth4, rawCurrentDepth4Min); rawCurrentDepth4Max = max(rawCurrentDepth4, rawCurrentDepth4Max);

      float closestZbufferDepthRaw = max(max(rawCurrentDepth4Max.x, rawCurrentDepth4Max.y), max(rawCurrentDepth4Max.z, rawCurrentDepth4Max.w));
      float farthestZbufferDepthRaw = min(min(rawCurrentDepth4Min.x, rawCurrentDepth4Min.y), min(rawCurrentDepth4Min.z, rawCurrentDepth4Min.w));
      float linearCloseZbufferDepth = linearize_z(closestZbufferDepthRaw, zn_zfar.zw);
      float linearFarZbufferDepth = linearize_z(farthestZbufferDepthRaw, zn_zfar.zw);
      //if (closestZbufferDepthRaw*zn_zfar.w/(farthestZbufferDepthRaw*zn_zfar.w + zn_zfar.z) < 0.05)//this is actually same as (linearFar - linearClosest) < 0.05*linearClosest
      if (linearFarZbufferDepth-linearCloseZbufferDepth < 0.05*linearCloseZbufferDepth ||
          linearCloseZbufferDepth*viewVecLen > current.farDepth*1.01)
      {
        newTaaEventFrame = blurredFrameNo;
        current.color = current.colorBlurred;
        //result_color = float4(1,0,0,1);
        //return;
      }
    }
  #endif

  // Temporal restart path
  // Perform clamp and integrate.
  half4 historyClamped = TAA_clip_history(history.color.color_attr, current.color, current.colorMin, current.colorMax, motionVectorPixelLengthTolerance);
  float clampEventFrame = tex2Dlod(taaPrevWeight, float4(historyUV, 0,0)).x;//1./255
  clampEventFrame = clampEventFrame > TAA_CLOUDS_FRAMES/255. ? 255.0 : ceil(255*clampEventFrame);//actually, as soon as clampEventFrame*255 >= frameOver, we should assume it is 1. as all frames has been taken into account
  half newFrameWeight = bOffscreen ? 1.0 : max(TAA_NEW_FRAME_WEIGHT, rcp(2+clampEventFrame));//since the formula is 1/(1+frames_since_event), and we read frames_since_event-1 from texture (we write 0 when event happen, not 0)
  newFrameWeight = max(clouds_taa_min_new_frame_weight, newFrameWeight);
  #if TAA_ALWAYS_BLUR || (TAA_ALWAYS_BLUR_WHERE_ALLOWED && SUPPORT_TEXTURE_GATHER && !DONT_CHECK_DEPTH_DISCONTINUITY)
    const float taaEventFrameStep = 1.75/255;//blurred frame is better than just frame, it has more information. Step 1 frame faster
  #else
    const float taaEventFrameStep = 1.5/255;
  #endif
  taaWeight = bOffscreen ? newTaaEventFrame : saturate(clampEventFrame*(1./255)+taaEventFrameStep);
  taaWeight = max(clouds_taa_min_new_frame_weight, taaWeight);
  float4 result = current.color;
  if (all(isfinite(historyClamped)))
    result = fp16_aware_lerp4(historyClamped, result, newFrameWeight);
  else
    taaWeight = 1;
  //half4 result = lerp(historyClamped, current.color, frameWeight);
  //result = fp16_aware_lerp4(historyClamped, current.color, bOffscreen ? 1.0 : TAA_NEW_FRAME_WEIGHT);

  // Convert back to HDR RGB space and write result.
  #if TAA_IN_HDR_SPACE
    result.rgb *= simple_luma_tonemap_inv(result.x, exposure);
  #endif

  result.rgb = UnpackFromYCoCg(result.rgb);

  #if TAA_USE_LUMINANCE_CLAMPING
    //result.rgb = h3nanofilter(result.rgb);
    //result.rgb = min(result.rgb, TAA_LUMINANCE_MAX);
  #endif

  result_color = result;
}