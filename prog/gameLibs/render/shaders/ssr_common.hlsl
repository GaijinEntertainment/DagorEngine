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

#ifndef PREV_FRAME_UNPACK
#define PREV_FRAME_UNPACK(x) (x)
#endif

#include "scrambleTea.hlsl"
#include "interleavedGradientNoise.hlsl"
#include "noise/Value3D.hlsl"

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
float ssr_linearize_z(float rawDepth)
{
  return linearize_z(rawDepth, zn_zfar.zw);
}
// hierarchical raymarch function
// TODO - reduce first step length to find close reflections
float4 hierarchRayMarch(float3 rayStart_uv_z, float3 R, float linear_roughness, float linearDepth, float3 cameraToPoint,
                        float stepOfs, float4x4 viewProjTmNoOfs, float waterLevel)
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
  if (rayStepClip.w<0)//disallalow move beyond znear
    borderScale = min(borderScale, (0-rayStartClip.w)/rayStepClip.w);
  rayStepScreen *= borderScale;
  float2 toleranceUpMAD = -float2(abs(rayStepScreen.z*0.25), abs(rayStepScreen.z*STEP));
  float3 rayStartUVz = float3( rayStartScreen.xy * float2( 0.5, -0.5 ) + float2(0.5,0.5), rayStartScreen.z );
  float3 rayStepUVz  = float3( rayStepScreen.xy * float2( 0.5, -0.5 ), rayStepScreen.z );

  //float toleranceScale = rayStepScreen.z*STEP;

  int numSteps = NUM_STEPS;

  float4 result = float4(1000, 1000, 0, 0);
  float levelToSample = SSR_QUALITY > 2 ? 0 : 1;

  #ifdef SSR_FFX
  {
    float3 origin = rayStartUVz;
    float3 direction = rayStepUVz;
    float2 screen_size = lowres_rt_params.xy;
    int most_detailed_mip = 0;
    uint max_traversal_intersections = 128;
    bool valid_hit = false;

    result.xyz = FFX_SSSR_HierarchicalRaymarch(origin, direction, screen_size, most_detailed_mip,
                                               max_traversal_intersections, valid_hit);
    result.w = 1;
  }
  #else
  {
    float traceStep = STEP;
    float4 sampleNormDistT = (stepOfs + float4(1, 2, 3, 4)) * traceStep;

    float stepFactor = saturate(3*(length(rayStepUVz.xy) - 0.02*NUM_STEPS));

    LOOP
    for (int i = 0; i < numSteps; i += 4)
    {
      float4 sampleNormDist = lerp(sampleNormDistT, ssr_smootherstep_vec4(sampleNormDistT), stepFactor);
      float4 tc0 = rayStartUVz.xyxy + rayStepUVz.xyxy * sampleNormDist.xxyy;
      float4 tc1 = rayStartUVz.xyxy + rayStepUVz.xyxy * sampleNormDist.zzww;
      float4 tcZ = rayStartUVz.zzzz + rayStepUVz.zzzz * sampleNormDist;//we allow to get farther than zfar

      // Use lower res for farther samples
      float4 rawDepth = sample_4depths(result.w > 0 ? 0 : levelToSample, tc0, tc1);

      float4 depthDiff = tcZ - rawDepth;
      //float compareToleranceUp = abs(rayStepUVz.z) * STEP;
      float4 compareToleranceUp = toleranceUpMAD.x * sampleNormDist + toleranceUpMAD.y;

      bool4 hasHit = bool4(depthDiff < 0) && bool4(depthDiff > compareToleranceUp);

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
      }
      else
      {
        sampleNormDistT += 4.0 * traceStep;
      }
      levelToSample = min(levelToSample + 1, 2);
    }
  }
  #endif

  // water surface
  #if SSR_TRACEWATER == 1
    float worldPosY = cameraToPoint.y+world_view_pos.y;
    float3 wRayToWater = float3(cameraToPoint - R * (worldPosY - waterLevel) / (underwater_params.w > 0 ? min(R.y, -0.0001f) : max(R.y, 0.0001f)));
    float4 rayToWater = mul(float4(wRayToWater.xyz, 1), viewProjTmNoOfs);
    float3 rayEndWater = rayToWater.xyz / rayToWater.w;
    rayEndWater = float3(0.5+rayEndWater.xy * float2(0.5, -0.5), rayEndWater.z);
    float lenToWater = ((rayEndWater.z>0) && (R.y*underwater_params.w<0) && (rayEndWater.x>0) && (rayEndWater.y>0) && (rayEndWater.x<1) && (rayEndWater.y<1)) ? length(rayEndWater.xy - rayStart_uv_z.xy) : 1000000;
    float lenToTrace = length(result.xy - rayStart_uv_z.xy);
    result = lenToWater<lenToTrace ? float4(rayEndWater.xy, rayToWater.z, 1) : result;
  #endif

  return result;
}

float get_vignette_value(float2 screenPos)
{
  float2 vignette = saturate(abs(screenPos) * 10 - 9);
  return saturate(1.0 - dot(vignette, vignette));
}

bool RaySphereIntersection(float3 rayPos, float3 rayDir, float3 spherePos, float sphereRadius, out float dist1, out float dist2)
{
  float3 o_minus_c = rayPos - spherePos;

  float p = dot(rayDir, o_minus_c);
  float q = dot(o_minus_c, o_minus_c) - (sphereRadius * sphereRadius);
  float discriminant = (p * p) - q;
  float dRoot = sqrt(max(0, discriminant));
  dist1 = -p - dRoot;
  dist2 = -p + dRoot;

  return discriminant > 0;
}
half4 sample_analytic_sphere(float hitDist, float3 cameraToPoint, float3 R, float3 worldPos)
{
  half4 result = half4(0, 0, 0, 0);
  float d1 = hitDist, d2 = hitDist;
  BRANCH
  if (RaySphereIntersection(worldPos, R, analytic_light_sphere_pos_r.xyz, analytic_light_sphere_pos_r.w, d1, d2))
  {
    float NOISE_SCALE = 150; //noise params are found by experiments
    float noise = clamp(noise_Value3D(NOISE_SCALE*worldPos + float3(0.2, 0.05, 0.01)*SSRParams.zzz), 0.33, 1.0);
    float distance_fade = saturate(d1 / analytic_light_sphere_pos_r.w);
    float size_fade = (0.5 * abs(d2 - d1) / analytic_light_sphere_pos_r.w);
    float a = noise * analytic_light_sphere_color.w * distance_fade * pow2(size_fade) * saturate(1000 * (hitDist - d1));
    result = float4(analytic_light_sphere_color.xyz, a);
  }
  return result;
}

half4 sample_vignetted_color(float3 hit_uv_z, float linear_roughness, float hitDist,
                             float3 cameraToPoint, float3 R, float3 worldPos)
{
  half4 result;
  #ifdef REPROJECT_TO_PREV_SCREEN
    float2 oldUv = 0;
    float2 screenPos = 0;

    #if SSR_MOTIONREPROJ == 1
      float2 motion = tex2Dlod(MOTION_VECTORS_TEXTURE, float4(hit_uv_z.xy, 0, 0)).rg;
      if (CHECK_VALID_MOTION_VECTOR(motion)) // if we have  motion vectors
      {
        oldUv = hit_uv_z.xy + decode_motion_vector(motion);
        screenPos = oldUv * 2 - 1;
      }
      else
    #endif
      {
        float3 viewVect = getViewVecOptimized(hit_uv_z.xy);
        float viewDepth = hit_uv_z.z;
        float3 cameraToPoint = viewVect * viewDepth;
        #if SSR_MOTIONREPROJ != 1 && PREV_HERO_SPHERE
          apply_hero_matrix(cameraToPoint);
        #endif
        oldUv = get_reprojected_history_uv(cameraToPoint, prev_globtm_no_ofs_psf, screenPos);
      }
    #define SSR_MIPS 1

    float2 sampleUV = oldUv.xy;
  #else
    float2 screenPos = hit_uv_z.xy * 2 - 1;
    float2 sampleUV = hit_uv_z.xy;
  #endif

  #if SSR_MIPS
    // mip selection
    float mipBias = 0.15;
    float TXlod = mipBias * linear_roughness * 26.0;
  #else
    float TXlod = 0;
  #endif

  result.rgb = PREV_FRAME_UNPACK(tex2Dlod(prev_frame_tex, float4(sampleUV.xy, 0, TXlod)).rgb);

  BRANCH if (analytic_light_sphere_pos_r.w > 0)
  {
    float4 add_ref = sample_analytic_sphere(hitDist, cameraToPoint, R, worldPos);
    result.rgb = lerp(result.rgb, add_ref.rgb, add_ref.a);
  }

  result.a = get_vignette_value(screenPos);
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
                 float linearDepth, float3 cameraToPoint, float4x4 viewProjNoOfsTm, float waterLevel,
                 float3 worldPos)
{
  half4 result = 0;

  float3 originToPoint = cameraToPoint;
  originToPoint = normalize(originToPoint);
  uint frameRandom = uint(SSRParams.w);
  float stepOfs = interleavedGradientNoiseFramed(pixelPos.xy, uint(SSRParams.z)) - 0.25;

  // Sample set dithered over 4x4 pixels
#if NUM_RAYS > 1

  uint2 random = scramble_TEA(uint2(pixelPos.x ^ frameRandom, pixelPos.y ^ frameRandom));

  float3 R = reflect(originToPoint, N);
  // Shoot multiple rays
  LOOP for (uint i = 0; i < NUM_RAYS; i++)
  {

    float2 E = hammersley(i, NUM_RAYS, random);
    float3x3 tbnTransform = create_tbn_matrix(N);
    float3 viewDirTC = mul(-originToPoint, tbnTransform);

    float3 sampledNormalTC = importance_sample_GGX_VNDF(E, viewDirTC, linear_roughness);

    float3 reflectedDirTC = reflect(-viewDirTC, sampledNormalTC);
    float3 R = mul(reflectedDirTC, transpose(tbnTransform));

    float4 hit_uv_z_fade = hierarchRayMarch(float3(UV, rawDepth), R, linear_roughness, linearDepth, cameraToPoint, stepOfs, viewProjNoOfsTm, waterLevel);

    // if there was a hit
    BRANCH if (hit_uv_z_fade.z) // hit_uv_z_fade.w was a hit, hit_uv_z_fade.z > 0 - actual hit with something, not with zfar
    {
      hit_uv_z_fade.z = ssr_linearize_z(hit_uv_z_fade.z);
      half4 sampleColor = sample_vignetted_color(hit_uv_z_fade.xyz, linear_roughness, hit_uv_z_fade.z, cameraToPoint, R, worldPos);
      sampleColor.rgb*=sampleColor.a;
      sampleColor.rgb /= 1 + luminance(sampleColor.rgb);
      result += sampleColor;
    }
  }

  result *= rcp(NUM_RAYS);
  result.rgb /= 1 - luminance(result.rgb);
#else
  float3 R = reflect(originToPoint, N);

  float4 hit_uv_z_fade = hierarchRayMarch(float3(UV, rawDepth), R, linear_roughness, linearDepth, cameraToPoint, stepOfs, viewProjNoOfsTm, waterLevel);
  bool wasHitNotZfar = hit_uv_z_fade.z != 0;// hit_uv_z_fade.w was a hit, hit_uv_z_fade.z > 0 - actual hit with something, not with zfar

  bool hitSky = hit_uv_z_fade.z == 0 && hit_uv_z_fade.w != 0;
  // if there was a hit
  BRANCH if (wasHitNotZfar)
  {
    hit_uv_z_fade.z = ssr_linearize_z(hit_uv_z_fade.z);
    result = sample_vignetted_color(hit_uv_z_fade.xyz, linear_roughness, hit_uv_z_fade.z, cameraToPoint, R, worldPos);
  }
  result.rgb *= result.a;
#ifdef ENCODE_HIT_SKY
  // if SSR hits sky, result.a is 0 and result.b is used as an "inverted" vignette value
  // that can be used to fade out VTR where it meets SSR
  result.a = 1.0 / 255.0 + result.a * 254.0 / 255.0;
  BRANCH if (hitSky)
  {
    result.a = 0;
    float2 screenPos = hit_uv_z_fade.xy * 2 - 1;
    result.b = 1.0 - get_vignette_value(screenPos);
  }
#endif
#endif

  return float4(result.rgb*brdf_G_GGX(N, originToPoint, linear_roughness), result.a);
}