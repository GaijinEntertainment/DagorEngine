#include <pixelPacking/yCoCgSpace.hlsl>
#include <tex2d_bicubic.hlsl>
#include <dof/circleOfConfusion.hlsl>

#define TAA_LUMINANCE_MIN 0.05
#define __XBOX_WAVESIM_ITERATION 1
#define __XBOX_IMPROVE_MAD 1

// Disables clamping entirely
//#define TAA_DEBUG_DISABLE_CLAMPING 1

//ignore motion
//#define TAA_DEBUG_NO_MOTION_WEIGHT 1

//very simple taa, without clipping or motion weights
//#define TAA_DEBUG_SIMPLE_TAA 1

// Tonemap samples from HDR to perceptual space before integration, then inverse tonemap back to HDR space
#ifndef TAA_MOTION_DIFFERENCE
#define TAA_MOTION_DIFFERENCE 1
#endif

#ifndef TAA_COLOR_ALPHA
#define TAA_COLOR_ALPHA 0
#endif

#ifndef TAA_IN_HDR_SPACE
#define TAA_IN_HDR_SPACE 1
#endif

// Filter pixels with a blackman-harris filter
#ifndef TAA_USE_FILTERING
#define TAA_USE_FILTERING 1
#endif

// Use optimized color neighborhood
//#define TAA_USE_OPTIMIZED_NEIGHBORHOOD

// Clamps luminance to a maximum value. This increases image stability for very bright pixels. 
#define TAA_USE_LUMINANCE_CLAMPING 1

// Use AABB clipping instead of clamping
#ifndef TAA_USE_AABB_CLIPPING
  #define TAA_USE_AABB_CLIPPING 1
#endif
#define TAA_VARIANCE_CLIP_WITH_MINMAX 1

//#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET 1//clip with offsetted min max

#ifndef TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN
#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN 1.25
#endif

#ifndef TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX
#define TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX 1.25
#endif

#ifndef TAA_NEW_FRAME_WEIGHT_BLUR
  #define TAA_NEW_FRAME_WEIGHT_BLUR 1
#endif
#ifndef TAA_FRAME_WEIGHT_BLUR_F0
  #define TAA_FRAME_WEIGHT_BLUR_F0 (1.0/(0.4 - TAA_NEW_FRAME_WEIGHT))
#endif
#ifndef TAA_FRAME_WEIGHT_BLUR_F1
  #define TAA_FRAME_WEIGHT_BLUR_F1 (TAA_NEW_FRAME_WEIGHT / (0.4 - TAA_NEW_FRAME_WEIGHT))
#endif

#ifndef TAA_ADAPTIVE_FILTER
#define TAA_ADAPTIVE_FILTER 0
#endif

#ifndef TAA_SCALE_AABB_WITH_MOTION
#define TAA_SCALE_AABB_WITH_MOTION 0
#endif

#ifndef TAA_GET_DYNAMIC_FUNC
#define TAA_GET_DYNAMIC_FUNC isGbufferDynamic
#endif

#if TAA_COLOR_ALPHA
  #define color_type half4
  #define color_attr rgba
#else
  #define color_type half3
  #define color_attr rgb
#endif

static const int BIT_DEPTH_MUL = 65535;
static const int BIT_DEPTH_MASK = 0xFFFE;

float simple_luma_tonemap(float luma, float exposure) { return rcp(luma * exposure + 1.0); }

float simple_luma_tonemap_inv(float luma, float exposure) { return rcp(max(1.0 - luma * exposure, 0.001)); }

color_type TAA_clip_history(color_type history, color_type current, color_type colorMin, color_type colorMax, half scale
#ifdef TAA_FILTER_DOF
  ,float current_dof, inout float history_dof
#endif
)
{
#if TAA_DEBUG_DISABLE_CLAMPING
  return history;
#elif TAA_USE_AABB_CLIPPING
  // Clip color difference against neighborhood min/max AABB
  float3 boxCenter = (colorMax.rgb + colorMin.rgb) * 0.5;
  float3 boxExtents = (colorMax.rgb - boxCenter) * scale;

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
#ifdef TAA_FILTER_DOF
  history_dof = lerp(history_dof, current_dof, clipLength);
#endif
  #if TAA_COLOR_ALPHA
    return half4(lerp(history, current, clipLength), clamp(history.a, colorMin.a, colorMax.a));
  #else
    return lerp(history, current, clipLength);
  #endif
#else
  return max(colorMin, min(colorMax, history));
#endif
}

float2 TAAGetReprojectedMotionVector(float rawDepth, float2 uv, float4x4 reprojectionMatrix)
{
  float4 prevUV = mul(float4(uv.x, uv.y, rawDepth, 1.0), reprojectionMatrix);
  prevUV.xy /= prevUV.w;
  #if defined(TAA_HERO_MATRIX_REPROJECTION)
    prevUV.xy = TAA_HERO_MATRIX_REPROJECTION(prevUV.xy, uv, rawDepth);
  #endif
  float2 ret = prevUV.xy - uv;
#if defined(TAA_SAMPLER_OBJECT_MOTION)
  float2 objectMotionVector = tex2D(TAA_SAMPLER_OBJECT_MOTION, uv).rg;
#if defined(TAA_IS_DYNAMIC_CHECK)
  FLATTEN
  if (TAA_IS_DYNAMIC_CHECK(objectMotionVector))
#else
  FLATTEN
  if (objectMotionVector.x!=0)
#endif
  {
    ret = TAA_SAMPLER_OBJECT_MOTION_FUNC( objectMotionVector );
  }
#endif
  return ret;
}

float gaussianKernel(const float2 delta)
{
  static const float sigma = 0.47;
  static const float exponentInputFactor = (-0.5 / (sigma * sigma));

  return exp(exponentInputFactor * dot(delta,delta));
}

#if TAA_USE_OPTIMIZED_NEIGHBORHOOD
  #define TAP_COUNT 5
#else
  #define TAP_COUNT 9
#endif

struct ReconstructionTaps
{
  float2 uv[TAP_COUNT];
  float weight[TAP_COUNT];
  float quality;
};

static const float2 reconstructionTexelOffsets[] =
{
  float2( 0.0,  0.0),
  float2(-1.0,  0.0),
  float2( 0.0, -1.0),
  float2( 1.0,  0.0),
  float2( 0.0,  1.0),
  float2(-1.0, -1.0),
  float2( 1.0, -1.0),
  float2(-1.0,  1.0),
  float2( 1.0,  1.0)
};

ReconstructionTaps computeReconstructionTaps(float2 uv, float2 res, float2 resInv, float2 upsamplingRatio)
{
  float2 texelCoord = uv*res;
  float2 texelCenter = floor(texelCoord) + 0.5;

  ReconstructionTaps taps;
  taps.quality = 0;

  UNROLL
  for(uint i = 0; i < TAP_COUNT; i++)
  {
    const float2 currentTexelCenter = texelCenter + reconstructionTexelOffsets[i];
    taps.uv[i] = currentTexelCenter * resInv;

    const float2 deltaOnDisplay = (texelCoord - currentTexelCenter)*upsamplingRatio;
    taps.weight[i] = gaussianKernel(deltaOnDisplay);

    taps.quality = max(taps.quality, taps.weight[i]);
  }

  return taps;
}

ReconstructionTaps fetchReconstructionTaps(float2 uv, float2 res, float2 resInv)
{
  ReconstructionTaps taps = {
    { // uvs
      uv + reconstructionTexelOffsets[0]*resInv,
      uv + reconstructionTexelOffsets[1]*resInv,
      uv + reconstructionTexelOffsets[2]*resInv,
      uv + reconstructionTexelOffsets[3]*resInv,
      uv + reconstructionTexelOffsets[4]*resInv,
    #if !TAA_USE_OPTIMIZED_NEIGHBORHOOD
      uv + reconstructionTexelOffsets[5]*resInv,
      uv + reconstructionTexelOffsets[6]*resInv,
      uv + reconstructionTexelOffsets[7]*resInv,
      uv + reconstructionTexelOffsets[8]*resInv
    #endif
    },
    { // weights
      taa_filter0.x,
      taa_filter0.y,
      taa_filter0.z,
      taa_filter0.w,
      taa_filter1.x,
    #if !TAA_USE_OPTIMIZED_NEIGHBORHOOD
      taa_filter1.y,
      taa_filter1.z,
      taa_filter1.w,
      taa_filter2.x
    #endif
    },
    1 // constant perfect quality for compatibility
  };

  return taps;
}

struct TAACurrent
{
  color_type color;
  color_type colorBlurred;
  color_type colorMean;
  color_type colorMin;
  color_type colorMax;
  half  lumaContrast;
  float rawDepth;
  float2 motionVectorUV;
  float linearNearestDepth;
  uint isDynamic, nextDynamic;
  float quality;
};


struct TAAParams
{
  float4x4 reprojectionMatrix;
  float4 depthUVTransform;
  float2 screenUV;
  float2 displayResolution;
  float2 displayResolutionInverse;
  float2 renderResolution;
  float2 renderResolutionInverse;
  float2 upsamplingRatio;
  float2 jitterOffset;
  float exposure;
};

TAACurrent TAA_gather_current(Texture2D<float4> sceneTex, SamplerState sceneTex_samplerstate,
                              Texture2D<float4> nativeDepthTex, SamplerState nativeDepthTex_samplerstate,
                              const TAAParams params)
{
  const float INFINITY = 65535.0;

  TAACurrent current = (TAACurrent)0;

  color_type colorMoment1 = 0.0;
  color_type colorMoment2 = 0.0;
  color_type colorMin =  INFINITY;
  color_type colorMax = -INFINITY;
  color_type colorFiltered = 0.0;
  half  colorWeightTotal = 0.0;

  // shift UV to jittered screenspace for subsequent operations
  float2 jitteredUV = params.screenUV + params.jitterOffset * params.renderResolutionInverse;

  #if TAA_ADAPTIVE_FILTER
    ReconstructionTaps taps = computeReconstructionTaps(jitteredUV, params.renderResolution,
                                                        params.renderResolutionInverse, params.upsamplingRatio);
  #else
    ReconstructionTaps taps = fetchReconstructionTaps(jitteredUV, params.renderResolution, params.renderResolutionInverse);
  #endif

  current.quality = taps.quality;

  #if TAA_USE_OPTIMIZED_NEIGHBORHOOD
    const float blurWeights[] = {
        6./10.,
        1./10.,
        1./10.,
        1./10.,
        1./10.
      };
  #else
    const float blurWeights[] = {
      8./14.,
      1./14.,
      1./14.,
      1./14.,
      1./14.,
      0.5/14.,
      0.5/14.,
      0.5/14.,
      0.5/14.
    };
  #endif

  //
  // Color neighborhood calculation
  //

  current.colorBlurred = 0;
  UNROLL
  for (uint i = 0; i < TAP_COUNT; i++)
  {
    color_type color = PackToYCoCgAlpha(tex2Dlod(sceneTex, float4(taps.uv[i] * params.depthUVTransform.xy + params.depthUVTransform.zw, 0, 0))).color_attr;

    #if TAA_IN_HDR_SPACE
      color.rgb *= simple_luma_tonemap(color.x, params.exposure);
    #endif

    if (i == 0)
      current.color = color;

    #if TAA_USE_FILTERING
      colorFiltered += color * taps.weight[i];
      colorWeightTotal += taps.weight[i];
    #endif

    #if TAA_NEW_FRAME_WEIGHT_BLUR
      current.colorBlurred += color * (blurWeights[i]);
    #endif

    colorMoment1 += color;
    colorMoment2 += color * color;
    colorMin = min(colorMin, color);
    colorMax = max(colorMax, color);
  }


  current.isDynamic = 0;
  #if HAS_DYNAMIC
  current.nextDynamic = TAA_GET_DYNAMIC_FUNC(jitteredUV);
  current.isDynamic = current.nextDynamic;
  UNROLL
  for (uint j = 5; j < 9; j++)
    current.isDynamic |= TAA_GET_DYNAMIC_FUNC(jitteredUV + params.renderResolutionInverse * reconstructionTexelOffsets[j]);
  #endif


  //
  // Motion Vector UV calculation by dilating velocity using nearest depth
  //
  float depthNearest = 0;
  {
    const int TAP_COUNT_CROSS = 5;
    const float2 uvOffsetsCross[TAP_COUNT_CROSS] =
    {
      jitteredUV,
      jitteredUV + params.displayResolutionInverse * float2(-2.0, -2.0),
      jitteredUV + params.displayResolutionInverse * float2( 2.0, -2.0),
      jitteredUV + params.displayResolutionInverse * float2(-2.0,  2.0),
      jitteredUV + params.displayResolutionInverse * float2( 2.0,  2.0)
    };

    float depthCross[TAP_COUNT_CROSS] =
    {
      tex2Dlod(nativeDepthTex, float4(uvOffsetsCross[0] * params.depthUVTransform.xy + params.depthUVTransform.zw,0,0)).x,
      tex2Dlod(nativeDepthTex, float4(uvOffsetsCross[1] * params.depthUVTransform.xy + params.depthUVTransform.zw,0,0)).x,
      tex2Dlod(nativeDepthTex, float4(uvOffsetsCross[2] * params.depthUVTransform.xy + params.depthUVTransform.zw,0,0)).x,
      tex2Dlod(nativeDepthTex, float4(uvOffsetsCross[3] * params.depthUVTransform.xy + params.depthUVTransform.zw,0,0)).x,
      tex2Dlod(nativeDepthTex, float4(uvOffsetsCross[4] * params.depthUVTransform.xy + params.depthUVTransform.zw,0,0)).x,
    };

    depthNearest = current.rawDepth = depthCross[0];
    current.motionVectorUV = jitteredUV;
    UNROLL
    for (int i = 1; i < TAP_COUNT_CROSS; i++)
    {
      if (depthCross[i] > depthNearest)
      {
        depthNearest = depthCross[i];
        current.motionVectorUV = uvOffsetsCross[i];
      }
    }

    // shift back to unjittered screen space before returning
    current.motionVectorUV -= params.jitterOffset * params.renderResolutionInverse;

  }

  //
  // Compute color bounds using the standard deviation of the neighborhood.
  //
  colorMoment1 *= (1.f/TAP_COUNT);
  colorMoment2 *= (1.f/TAP_COUNT);

  current.linearNearestDepth = linearize_z(depthNearest, zn_zfar.zw);
  #if TAA_VARIANCE_CLIPPING || defined(TAA_DISTANCE_FOR_VARIANCE_CLIPPING)
    #if TAA_COLOR_ALPHA
      const float4 YCOCG_MIN = float4(0.0, -0.5, -0.5, 0);
      const float4 YCOCG_MAX = float4(1.0,  0.5,  0.5, 1);
    #else
      const float3 YCOCG_MIN = float3(0.0, -0.5, -0.5);
      const float3 YCOCG_MAX = float3(1.0,  0.5,  0.5);
    #endif

    color_type colorVariance = colorMoment2 - colorMoment1 * colorMoment1;
    color_type colorSigma = sqrt(max(0, colorVariance)) * TAA_CLAMPING_GAMMA;
    color_type colorMin2 = colorMoment1 - colorSigma;
    color_type colorMax2 = colorMoment1 + colorSigma;

      // When we tonemap, it's important to keep the min / max bounds within limits, we keep it within previous aabb (instead of some predefined)
    #if TAA_VARIANCE_CLIP_WITH_MINMAX || defined(TAA_VARIANCE_CLIP_WITH_MINMAX_INSET)
      #if defined(TAA_VARIANCE_CLIP_WITH_MINMAX_INSET)
        color_type colorBoxCenter = 0.5*(colorMin+colorMax);
        color_type colorBoxExt = colorMax - colorBoxCenter;
        color_type colorMinInset = clamp(colorBoxCenter - TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MIN*colorBoxExt, YCOCG_MIN, YCOCG_MAX);
        color_type colorMaxInset = clamp(colorBoxCenter + TAA_VARIANCE_CLIP_WITH_MINMAX_INSET_MAX*colorBoxExt, YCOCG_MIN, YCOCG_MAX);
        colorMin2 = clamp(colorMin2, colorMinInset, colorMaxInset);
        colorMax2 = clamp(colorMax2, colorMinInset, colorMaxInset);
      #else
        colorMin2 = clamp(colorMin2, colorMin, colorMax);
        colorMax2 = clamp(colorMax2, colorMin, colorMax);
      #endif
    #elif TAA_IN_HDR_SPACE
      //colorMax2 = clamp(colorMax2, YCOCG_MIN, YCOCG_MAX);
      colorMin2 = clamp(colorMin2, YCOCG_MIN, YCOCG_MAX);
      colorMax2 = clamp(colorMax2, YCOCG_MIN, YCOCG_MAX);
    #endif

    #if TAA_VARIANCE_CLIPPING
      colorMin = colorMin2;
      colorMax = colorMax2;
    #else
      half distScale = saturate(current.linearNearestDepth*(1./(TAA_DISTANCE_FOR_VARIANCE_CLIPPING*2))-0.5);
      colorMin = lerp(colorMin, colorMin2, distScale);
      colorMax = lerp(colorMax, colorMax2, distScale);
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

  current.colorMean = colorMoment1;
  current.colorMin = colorMin;
  current.colorMax = colorMax;
  current.lumaContrast = lumaContrast;

  return current;
}
struct TAAHistory
{
  half4 color;
  half3 colorMin;
  half3 colorMax;
  half3 colorMean;
  half  lumaContrast;
  uint isDynamic;
  #ifdef TAA_FILTER_DOF
    float dof_blend;
  #endif
};

//historyTex_samplerstate HAS to be linear
void TAA_gather_history(Texture2D<float4> historyTex, SamplerState historyTex_samplerstate,
                      float2 screenUV, float2 screenSize, float2 screenSizeInverse, half exposure, out TAAHistory history
                      #ifdef TAA_FILTER_DOF
                        ,Texture2D<float4> dofTex, SamplerState dofTex_samplerstate
                      #endif
                      )
{
  BicubicSharpenWeights weights;
  compute_bicubic_sharpen_weights(screenUV, screenSize, screenSizeInverse, TAA_SHARPENING_FACTOR, weights);

  float4 c10 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv0,0,0)));
  float4 c01 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv1,0,0)));
  float4 c11 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv2,0,0)));
  float4 c21 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv3,0,0)));
  float4 c12 = PackToYCoCgAlpha(tex2Dlod(historyTex, float4(weights.uv4,0,0)));

  #if defined(SEPARATE_ALPHA_TEX)
  c10.a = tex2Dlod(SEPARATE_ALPHA_TEX, float4(weights.uv0,0,0)).r;
  c01.a = tex2Dlod(SEPARATE_ALPHA_TEX, float4(weights.uv1,0,0)).r;
  c11.a = tex2Dlod(SEPARATE_ALPHA_TEX, float4(weights.uv2,0,0)).r;
  c21.a = tex2Dlod(SEPARATE_ALPHA_TEX, float4(weights.uv3,0,0)).r;
  c12.a = tex2Dlod(SEPARATE_ALPHA_TEX, float4(weights.uv4,0,0)).r;
  #endif

  #if TAA_IN_HDR_SPACE
    c10.rgb *= simple_luma_tonemap(c10.x, exposure);
    c01.rgb *= simple_luma_tonemap(c01.x, exposure);
    c11.rgb *= simple_luma_tonemap(c11.x, exposure);
    c21.rgb *= simple_luma_tonemap(c21.x, exposure);
    c12.rgb *= simple_luma_tonemap(c12.x, exposure);
  #endif

  history.isDynamic = 0;
  #if HAS_DYNAMIC
    float2 exactUV = (floor(screenSize*screenUV) + 0.5)*screenSizeInverse;

    #if defined(TAA_SAMPLER_WAS_DYNAMIC)
    history.isDynamic = tex2D(TAA_SAMPLER_WAS_DYNAMIC, float2(screenUV.xy)).r > 0;
    #else
      #if defined(SEPARATE_ALPHA_TEX)
      history.isDynamic =
        (uint(tex2Dlod(SEPARATE_ALPHA_TEX, float4(exactUV,0,0)).r*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(SEPARATE_ALPHA_TEX, float4(exactUV.xy-screenSizeInverse.xy,0,0)).r*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(SEPARATE_ALPHA_TEX, float4(exactUV.xy+screenSizeInverse.xy,0,0)).r*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(SEPARATE_ALPHA_TEX, float4(exactUV.x-screenSizeInverse.x,exactUV.y+screenSizeInverse.y,0,0)).r*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(SEPARATE_ALPHA_TEX, float4(exactUV.x+screenSizeInverse.x,exactUV.y-screenSizeInverse.y,0,0)).r*BIT_DEPTH_MUL+0.1)
        )&1;
      #else
      history.isDynamic =
        (uint(tex2Dlod(historyTex, float4(exactUV,0,0)).w*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(historyTex, float4(exactUV.xy-screenSizeInverse.xy,0,0)).w*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(historyTex, float4(exactUV.xy+screenSizeInverse.xy,0,0)).w*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(historyTex, float4(exactUV.x-screenSizeInverse.x,exactUV.y+screenSizeInverse.y,0,0)).w*BIT_DEPTH_MUL+0.1)
        |uint(tex2Dlod(historyTex, float4(exactUV.x+screenSizeInverse.x,exactUV.y-screenSizeInverse.y,0,0)).w*BIT_DEPTH_MUL+0.1)
        )&1;
      #endif
    #endif
  #endif

  history.color = (c10 * weights.w0 + c01 * weights.w1 + c11 * weights.w2 + c21 * weights.w3 + c12 * weights.w4) / weights.weightsSum;
#ifdef TAA_FILTER_DOF
  float dof0 = tex2Dlod(dofTex, float4(weights.uv0,0,0)).x;
  float dof1 = tex2Dlod(dofTex, float4(weights.uv1,0,0)).x;
  float dof2 = tex2Dlod(dofTex, float4(weights.uv2,0,0)).x;
  float dof3 = tex2Dlod(dofTex, float4(weights.uv3,0,0)).x;
  float dof4 = tex2Dlod(dofTex, float4(weights.uv4,0,0)).x;
  history.dof_blend = (dof0 * weights.w0 + dof1 * weights.w1 + dof2 * weights.w2 + dof3 * weights.w3 + dof4 * weights.w4) / weights.weightsSum;
#endif
  history.colorMin = min(min(min(c10.rgb, c01.rgb), min(c11.rgb, c21.rgb)), c12.rgb);
  history.colorMax = max(max(max(c10.rgb, c01.rgb), max(c11.rgb, c21.rgb)), c12.rgb);
  history.colorMean = (c10.rgb + c01.rgb + c11.rgb + c21.rgb + c12.rgb) / 5.0;
  history.lumaContrast = history.colorMax.x - history.colorMin.x;
}

//historyTex_samplerstate HAS to be linear
//sceneTex_samplerstate HAS to be point filtered

void TAA(out float motion_data, out float3 result_color,
         Texture2D<float4> sceneTex, SamplerState sceneTex_samplerstate,
         Texture2D<float4> nativeDepthTex, SamplerState nativeDepthTex_samplerstate,
         Texture2D<float4> historyTex, SamplerState historyTex_samplerstate,
         const TAAParams params
         #if TAA_OUTPUT_FRAME_WEIGHT
           ,out float frameWeightOut
         #endif
         #ifdef TAA_FILTER_DOF
           ,Texture2D<float4> dofTex, SamplerState dofTex_samplerstate, out float result_dof
         #endif
         )
{
  //
  // Gather the current frame neighborhood information.
  //
  TAACurrent current = TAA_gather_current(sceneTex, sceneTex_samplerstate, nativeDepthTex, nativeDepthTex_samplerstate, params);

  // Reproject uv value to motion from previous frame.
  #if defined(TAA_MOTION_VECTOR_PROVIDER)
    float2 motionVector = TAA_MOTION_VECTOR_PROVIDER(current.rawDepth, current.motionVectorUV);
  #else
    float2 motionVector = TAAGetReprojectedMotionVector(current.rawDepth, current.motionVectorUV, params.reprojectionMatrix);
  #endif
  float2 historyUV = (params.screenUV + motionVector);
  bool bOffscreen = historyUV.x >= 1.0 || historyUV.x <= TAA_RESTART_TEMPORAL_X || historyUV.y >= 1.0 || historyUV.y <= 0.0;
  #if TAA_DEBUG_SIMPLE_TAA
  {
    float3 hist = PackToYCoCg(tex2Dlod(historyTex, float4(historyUV,0,0)));
    hist.rgb *= simple_luma_tonemap(hist.x, params.exposure);
    result_color = lerp(hist.rgb, current.color, bOffscreen ? 1 : TAA_NEW_FRAME_WEIGHT);
    result_color *= simple_luma_tonemap_inv(result_color.x, params.exposure);
    result_color = UnpackFromYCoCg(result_color);
    motion_data = 0;
    return;
  }
  #endif
  //
  // Gather the previous frame neighborhood information.
  //
  TAAHistory history;
  TAA_gather_history(historyTex, historyTex_samplerstate, historyUV, params.displayResolution, params.displayResolutionInverse, params.exposure, history
  #ifdef TAA_FILTER_DOF
    ,dofTex, dofTex_samplerstate
  #endif
  );

  half motionVectorLength = length(motionVector);

  //
  // Compute temporal blend weight.
  //
  #if HAS_DYNAMIC && defined(TAA_NEW_FRAME_DYNAMIC_WEIGHT)
    #if TAA_USE_WEIGHT_MOTION_LERP

      half frameWeightFrom = current.nextDynamic ? TAA_NEW_FRAME_DYNAMIC_WEIGHT.x : TAA_NEW_FRAME_WEIGHT.x;
      half frameWeightTo = current.nextDynamic ? TAA_NEW_FRAME_DYNAMIC_WEIGHT.y : TAA_NEW_FRAME_WEIGHT.y;

      half frameWeightLerp = saturate(motionVectorLength * TAA_NEW_FRAME_MOTION_WEIGHT_LERP_1 + TAA_NEW_FRAME_MOTION_WEIGHT_LERP_0);
      half frameWeight = lerp( frameWeightFrom, frameWeightTo, frameWeightLerp);
    #else
      half frameWeight = current.nextDynamic ? TAA_NEW_FRAME_DYNAMIC_WEIGHT.x : TAA_NEW_FRAME_WEIGHT.x;
    #endif
  #else
    half frameWeight = TAA_NEW_FRAME_WEIGHT.x;
  #endif

  frameWeight *= current.quality;

  #if defined(TAA_USE_ANTI_FLICKER_FILTER_DISTANCE_START)
  {
    // Values are in YCoCg space (luma in x coordinate)
    half contrast_ratio = current.lumaContrast / max(TAA_LUMINANCE_MIN, history.lumaContrast);
    half luminance_ratio = current.colorMean.x / max(TAA_LUMINANCE_MIN, history.colorMean.x);
    half a = smoothstep(TAA_CLAMPING_FACTOR, TAA_CLAMPING_FACTOR + 1.0, saturate(contrast_ratio / luminance_ratio));
    #if TAA_USE_ANTI_FLICKER_FILTER_DISTANCE_START>0
      half distScale = saturate(current.linearNearestDepth/TAA_USE_ANTI_FLICKER_FILTER_DISTANCE_START-1);
    #else
      half distScale = 1;
    #endif
    frameWeight = frameWeight * (1 - a*distScale);
  }
  #endif

  // Motion vector changes.
  #if TAA_MOTION_DIFFERENCE
  half motionDifference = saturate(abs(motionVectorLength - history.color.a) * TAA_MOTION_DIFFERENCE_MAX_INVERSE);
  #else
  half motionDifference = 0;
  #endif
  #if !TAA_DEBUG_NO_MOTION_WEIGHT
    #if defined(TAA_MOTION_LENGTH_WEIGHT)
      motionDifference = max(motionDifference, saturate(motionVectorLength*params.displayResolution.x/TAA_MOTION_LENGTH_WEIGHT));
    #endif
    frameWeight = lerp(frameWeight, TAA_MOTION_DIFFERENCE_MAX_WEIGHT, motionDifference);
  #endif

  #if defined(TAA_LOW_QUALITY_MOTION_MAX)
    float2 motionVectorPixels = motionVector*params.displayResolution;
    FLATTEN
    if (current.nextDynamic && (max(abs(motionVectorPixels.x),abs(motionVectorPixels.y)) >= TAA_LOW_QUALITY_MOTION_MAX))
      frameWeight = TAA_MOTION_DIFFERENCE_MAX_WEIGHT;//we have written clamped motion vectors. we'd better converge fast!
  #endif

  #if HAS_DYNAMIC
    if (!current.isDynamic & history.isDynamic)
      frameWeight = saturate(frameWeight+0.4);
  #endif

  #if TAA_NEW_FRAME_WEIGHT_BLUR

    FLATTEN
    if (!current.nextDynamic)//never blur even new dynamic pixels!
      current.color = lerp(current.color, current.colorBlurred, 
        saturate(frameWeight * TAA_FRAME_WEIGHT_BLUR_F0 - TAA_FRAME_WEIGHT_BLUR_F1));
  #endif

  // Temporal restart path
  FLATTEN
  if (bOffscreen)
    frameWeight = 1.0;

  // Perform clamp and integrate.
  #if defined(TAA_SCALE_AABB_WITH_MOTION_STEEPNESS) && defined(TAA_SCALE_AABB_WITH_MOTION_MAX)
  half aabbScalingFactor = 1 + rcp(motionVectorLength * TAA_SCALE_AABB_WITH_MOTION_STEEPNESS + TAA_SCALE_AABB_WITH_MOTION_MAX);
  #else
  half aabbScalingFactor = 1;
  #endif

  #if TAA_DISABLE_AABB_SCALING_FOR_DYNAMIC && HAS_DYNAMIC
  aabbScalingFactor = ( current.isDynamic || history.isDynamic ) ? 1 : aabbScalingFactor;
  #endif

  #ifdef TAA_FILTER_DOF
    float2 jitteredUV = params.screenUV + params.jitterOffset * params.renderResolutionInverse;
    float depth = tex2Dlod(dof_blend_depth_tex, float4(jitteredUV, 0, 0)).x;
    float dof_blend_current = ComputeFarCircleOfConfusion(depth, dof_focus_params);
  #endif

  half3 historyClamped = TAA_clip_history(history.color.color_attr, current.color, current.colorMin, current.colorMax, aabbScalingFactor
  #ifdef TAA_FILTER_DOF
    ,dof_blend_current, history.dof_blend
  #endif
  ).rgb;
  #if TAA_DISABLE_CLAMPING_FOR_STATIC && HAS_DYNAMIC
  historyClamped = ( current.isDynamic || history.isDynamic ) ? historyClamped : history.color.rgb;
  #endif

  half3 result = lerp(historyClamped.rgb, current.color.rgb, max(frameWeight, 0.005));
  #if TAA_OUTPUT_FRAME_WEIGHT
    frameWeightOut = frameWeight;
  #endif

  // Convert back to HDR RGB space and write result.
  #if TAA_IN_HDR_SPACE
    result *= simple_luma_tonemap_inv(result.x, params.exposure);
  #endif

  result = UnpackFromYCoCg(result);

  #if TAA_USE_LUMINANCE_CLAMPING
    result = h3nanofilter(result);
    result = min(result, TAA_LUMINANCE_MAX);
  #endif

  motion_data = motionVectorLength;
  #if HAS_DYNAMIC
    #if defined(TAA_SAMPLER_WAS_DYNAMIC)
      motion_data = current.nextDynamic ? 1 : 0;
    #else
      motion_data = ((uint(saturate(motion_data)*BIT_DEPTH_MUL)&BIT_DEPTH_MASK)|current.nextDynamic)/float(BIT_DEPTH_MUL);
    #endif
  #endif
  result_color = result;

  #ifdef TAA_FILTER_DOF
    result_dof = lerp(history.dof_blend, dof_blend_current, max(frameWeight, 0.005));
  #endif
}