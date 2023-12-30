#if SSR_QUALITY == 1
  #define NUM_STEPS 8
  #define NUM_RAYS 1
#elif SSR_QUALITY == 2
  #define NUM_STEPS 16
  #define NUM_RAYS 1
#elif SSR_QUALITY == 3
  #define NUM_STEPS 8
  #define NUM_RAYS 4
#else // SSR_QUALITY == 4
  #define NUM_STEPS 12
  #define NUM_RAYS 12
#endif

#define STEP (1.0 / NUM_STEPS)
#define TEA_ITERATIONS 2

#include "scrambleTea.hlsl"
#include "interleavedGradientNoise.hlsl"

#ifndef SSR_USE_SEPARATE_DEPTH_MIPS
float4 sample_4depths(float Level, float4 tc0, float4 tc1)
{
  float4 rawDepth;
  rawDepth.x = tex2Dlod(ssr_depth, float4(tc0.xy, 0, Level)).r;
  rawDepth.y = tex2Dlod(ssr_depth, float4(tc0.zw, 0, Level)).r;
  rawDepth.z = tex2Dlod(ssr_depth, float4(tc1.xy, 0, Level)).r;
  rawDepth.w = tex2Dlod(ssr_depth, float4(tc1.zw, 0, Level)).r;
  return rawDepth;
}
#else
float4 sample_4depths(float Level, float4 tc0, float4 tc1)
{
  return sample_4depths_separate(Level, tc0, tc1);
}
#endif
float4 ssr_smootherstep_vec4(float4 x)
{
  return saturate(x * x * x * (x * (x * 6 - 15) + 10));
}
// hierarchical raymarch function
// TODO - reduce first step length to find close reflections
float4 hierarchRayMarch(float3 rayStart_uv_z, float3 R, float linear_roughness, float linearDepth, float3 cameraToPoint,
                        float stepOfs, float4x4 viewProjTmNoOfs, out float hitRawDepth)
{
  float dist = linearDepth*0.97;
  float4 rayStartClip = mul(float4(cameraToPoint, 1), viewProjTmNoOfs);
  float4 rayStepClip = mul(float4(R*dist, 0), viewProjTmNoOfs);
  float4 rayEndClip = rayStartClip + rayStepClip;

  float3 rayStartScreen = rayStartClip.xyz / rayStartClip.w;
  float3 rayEndScreen = rayEndClip.xyz / rayEndClip.w;
  float3 rayStepScreen = rayEndScreen - rayStartScreen;

  // calculate border of screen
  float2 screenBorder = (rayStepScreen.xy >= 0) ? float2(1,1) : -float2(1,1);
  float2 bScale = (abs(rayStepScreen.xy) < 1e-5) ? 1 : (screenBorder - rayStartScreen.xy) / rayStepScreen.xy;
  bScale = (screenBorder - rayStartScreen.xy) / rayStepScreen.xy;
  float bScaleW = max(bScale.x, bScale.y);
  float borderScale = bScaleW > 0 ? min(bScale.x, bScale.y) : 1;
  FLATTEN
  if (rayStepScreen.z > 0)
    borderScale = min(borderScale, (1-rayStartScreen.z)/rayStepScreen.z);
  rayStepScreen *= borderScale;
  float2 toleranceUpMAD = -float2(abs(rayStepScreen.z*0.25), abs(rayStepScreen.z*STEP));
  float3 rayStartUVz = float3( rayStartScreen.xy * float2( 0.5, -0.5 ) + float2(0.5,0.5), rayStartScreen.z );
  float3 rayStepUVz  = float3( rayStepScreen.xy * float2( 0.5, -0.5 ), rayStepScreen.z );

  //float toleranceScale = rayStepScreen.z*STEP;

  int numSteps = NUM_STEPS;

  float4 result = float4(1000, 1000, 0, -1);
  float levelToSample = 0;

  float traceStep = STEP;
  float4 sampleNormDistT = (stepOfs + float4(1, 2, 3, 4)) * traceStep;

  bool useHitHack = false;

  LOOP
  for (int i = 0; i < numSteps; i += 4)
  {
    float4 sampleNormDist = ssr_smootherstep_vec4(sampleNormDistT);//saturate(sampleNormDistT * sampleNormDistT);
    float4 tc0 = rayStartUVz.xyxy + rayStepUVz.xyxy * sampleNormDist.xxyy;
    float4 tc1 = rayStartUVz.xyxy + rayStepUVz.xyxy * sampleNormDist.zzww;
    float4 tcZ = rayStartUVz.zzzz + rayStepUVz.zzzz * sampleNormDist;

    // Use lower res for farther samples
    float4 rawDepth = sample_4depths(levelToSample, tc0, tc1);
    //float4 rawDepth = sample_4depths(0, tc0, tc1);

    float4 depthDiff = tcZ - rawDepth;
    float4 compareToleranceUp = toleranceUpMAD.x * sampleNormDist + toleranceUpMAD.y;

    bool4 toleranceUpResult = bool4(depthDiff > compareToleranceUp);

    bool4 hasHit = bool4(depthDiff < 0) && toleranceUpResult;

    useHitHack = useHitHack || any(!toleranceUpResult);

    BRANCH if (any(hasHit))
    {
      float hitDpth = hasHit.x ? rawDepth.x : (hasHit.y ? rawDepth.y : (hasHit.z ? rawDepth.z : rawDepth.w));
      float2 hit_scr = hasHit.x ? tc0.xy : (hasHit.y ? tc0.zw : (hasHit.z ? tc1.xy : tc1.zw));
      result = float4(hit_scr.xy, hitDpth, 1);
      // Use rest of tracings to find more accurate intersection
      float minHitNormDist = hasHit.x ? sampleNormDistT.x : (hasHit.y ? sampleNormDistT.y : (hasHit.z ? sampleNormDistT.z : sampleNormDistT.w));
      sampleNormDistT.xyzw = max(0.04, minHitNormDist - traceStep); // point before closest hit
      traceStep *= 0.25;
      sampleNormDistT.xyzw += float4(1.0, 2.0, 3.0, 4.0)* traceStep;
      levelToSample = max(levelToSample - 1, 0);
    }
    else
    {
      sampleNormDistT += 4.0 * traceStep;
      levelToSample = min(levelToSample + 1, 2);
    }
  }

  hitRawDepth = result.z;

  // hack: offset texture 1 pixel to raymarch direction for prevention of "leaking" of close objects to reflections of far objects
  float2 hackResult = normalize(rayStepUVz.xy) * ssr_inv_target_size.xy;

  if (useHitHack)
    result.xy += 2*hackResult;

  result.z = linearize_z(result.z, zn_zfar.zw);

  return result;
}

//todo: use linear_roughness
half4 sample_vignetted_color(float3 hit_uv_z, float fade, float linear_roughness)
{
  half4 result;
  half farFade=1;

  float viewDepth = 0;
  viewDepth = hit_uv_z.z;

  float2 screenPos = hit_uv_z.xy * 2 - 1;
  float2 sampleUV = hit_uv_z.xy;

  farFade = saturate(20 * (1 - 1.1*viewDepth / zn_zfar.y));

  #if SSR_MIPS
    // mip selection
    float mipBias = 0.15+(1-fade)+(1-farFade);
    float TXlod = mipBias * linear_roughness * 26.0;
  #else
    float TXlod = 0;
  #endif

  fade *= farFade;
  result = 0;

  BRANCH
  if (fade>0)
  {
    result.rgb = tex2Dlod(prev_frame_tex, float4(sampleUV.xy, 0, 0*TXlod)).rgb;
    result.a = 1;
    float2 vignette = saturate(abs(screenPos) * 10 - 9);

    result *= saturate(1.0 - dot(vignette, vignette));
    result.rgb = max(result.rgb, 0.0);
    result *= fade;
  }

  return result;
}

float brdf_G_GGX(float3 n, float3 v, float linear_roughness)
{
  float alpha2 = pow4(linear_roughness);
  float sup = pow2(dot(n, v));
  return 2 * sup / (1e-6 + sup + sqrt(sup*(alpha2 + (1 - alpha2)*sup)));//avoid nan 0/0
}


#define JITTER_SIZE 4
#define JITTER_MASK (JITTER_SIZE-1)
#define JITTER_COUNT (JITTER_SIZE*JITTER_SIZE)
#define JITTER_COUNT_MASK (JITTER_COUNT-1)

half4 performSSR(uint2 pixelPos, float2 UV, float linear_roughness, float3 N, float rawDepth,
                 float linearDepth, float3 cameraToPoint, float4x4 viewProjNoOfsTm,
                 inout float depth, out float ssrRawDepth)
{
  half4 result = 0;

  float3 originToPoint = cameraToPoint;
  originToPoint = normalize(originToPoint);
  uint frameRandom = uint(SSRParams.w);
  float stepOfs = interleavedGradientNoiseFramed(pixelPos.xy, uint(SSRParams.z)) - 0.25;

  // Sample set dithered over 4x4 pixels
  float3 R = reflect(originToPoint, N);

  float4 hit_uv_z_fade = hierarchRayMarch(float3(UV, rawDepth), R, linear_roughness, linearDepth, cameraToPoint, stepOfs, viewProjNoOfsTm, ssrRawDepth);

  // if there was a hit
  BRANCH if (hit_uv_z_fade.w > 0)
  {
    result = sample_vignetted_color(hit_uv_z_fade.xyz, hit_uv_z_fade.w, linear_roughness);
  }
  depth = hit_uv_z_fade.z;

  return float4(result.rgb*brdf_G_GGX(N, originToPoint, linear_roughness), result.a);
}
