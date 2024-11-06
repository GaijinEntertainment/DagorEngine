#ifdef SSR_FFX
  #if SSR_QUALITY == 1
    #define NUM_STEPS 32
  #elif SSR_QUALITY == 2
    #define NUM_STEPS 64
  #elif SSR_QUALITY == 3
    #define NUM_STEPS 128
  #else
    #error "UNDEFINED"
  #endif
#else
  #define NUM_STEPS 8
#endif

#define STEP (1.0 / NUM_STEPS)

#ifndef PREV_FRAME_UNPACK
#define PREV_FRAME_UNPACK(x) (x)
#endif

#include "interleavedGradientNoise.hlsl"
#include "noise/Value3D.hlsl"

float4 sample_4depths(float Level, float4 tc0, float4 tc1)
{
  float4 rawDepth;
  rawDepth.x = tex2Dlod(ssr_depth, float4(tc0.xy, 0, Level)).r;
  rawDepth.y = tex2Dlod(ssr_depth, float4(tc0.zw, 0, Level)).r;
  rawDepth.z = tex2Dlod(ssr_depth, float4(tc1.xy, 0, Level)).r;
  rawDepth.w = tex2Dlod(ssr_depth, float4(tc1.zw, 0, Level)).r;
  return rawDepth;
}

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
float4 hierarchRayMarch(float2 rayStart_uv, float3 R, float linear_roughness, float linearDepth, float3 cameraToPoint,
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
    uint max_traversal_intersections = NUM_STEPS;
    bool valid_hit = false;

    result.xyz = FFX_SSSR_HierarchicalRaymarch(origin, direction, screen_size, most_detailed_mip,
                                               max_traversal_intersections, valid_hit);

    bool hitSky = result.z == 0;
    if (!valid_hit && !hitSky)
      return float4(0,0,0,0);
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
    float lenToWater = ((rayEndWater.z>0) && (R.y*underwater_params.w<0) && (rayEndWater.x>0) && (rayEndWater.y>0) && (rayEndWater.x<1) && (rayEndWater.y<1)) ? length(rayEndWater.xy - rayStart_uv) : 1000000;
    float lenToTrace = length(result.xy - rayStart_uv);
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

  float p = dot(normalize(rayDir), o_minus_c);
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

bool get_prev_frame_disocclusion_weight_sample(float2 historyUV, float prev_linear_depth, float3 cameraToPoint, float lod, inout half3 historySample)
{
  bool weight = all(abs(historyUV*2-1) < 1);
  #if USE_PREV_DOWNSAMPLED_CLOSE_DEPTH
    float4 depths = prev_downsampled_close_depth_tex.GatherRed(prev_downsampled_close_depth_tex_samplerstate, historyUV).wzxy;
    float4 linearDepths = linearize_z4(depths, prev_zn_zfar.zw);
    float4 depthDiff = abs(linearDepths - prev_linear_depth);
    float threshold = 0.05*prev_linear_depth;
    float4 weights = depthDiff < threshold;
    if (all(weights)) // bilinear filtering is valid
    {
      historySample = tex2Dlod(prev_frame_tex, float4(historyUV, 0, lod)).rgb;
    } else
    {
      float2 historyCrdf = historyUV*prev_downsampled_close_depth_tex_target_size.xy - 0.5;
      float4 bil = float4(frac(historyCrdf), 1-frac(historyCrdf));
      weights *= float4(bil.zx*bil.w, bil.zx*bil.y);
      float sumW = dot(weights, 1);
      weight = weight && sumW >= 0.0001;
      weights *= rcp(max(1e-6, sumW));
      float2 uv = (floor(historyCrdf) + 0.5)*prev_downsampled_close_depth_tex_target_size.zw;//todo: one madd
      // we rely on prev_downsampled_close_depth_tex_samplerstate being point sample
      //float2 uv = historyUV;
      half3 lt = prev_frame_tex.SampleLevel(prev_downsampled_close_depth_tex_samplerstate, uv, lod).rgb;
      half3 rt = prev_frame_tex.SampleLevel(prev_downsampled_close_depth_tex_samplerstate, uv, lod, int2(1,0)).rgb;
      half3 lb = prev_frame_tex.SampleLevel(prev_downsampled_close_depth_tex_samplerstate, uv, lod, int2(0,1)).rgb;
      half3 rb = prev_frame_tex.SampleLevel(prev_downsampled_close_depth_tex_samplerstate, uv, lod, int2(1,1)).rgb;
      historySample = lt*weights.x + rt*weights.y + lb*weights.z + rb*weights.w;
      //weight = 0;
    }
  #else
    historySample = tex2Dlod(prev_frame_tex, float4(historyUV, 0, lod)).rgb;
  #endif
  historySample = PREV_FRAME_UNPACK(historySample);
  return weight;
}

half4 sample_vignetted_color(float3 hit_uv_z, float linear_roughness, float hitDist,
                             float3 cameraToPoint, float3 R, float3 worldPos)
{
  half4 result;
  float3 cameraToHitPoint = getViewVecOptimized(hit_uv_z.xy)* hit_uv_z.z;
  #ifdef REPROJECT_TO_PREV_SCREEN

    float4 prevClipHitPos = mul(float4(cameraToHitPoint, 1), prev_globtm_no_ofs_psf);
    float3 oldExactUVZ = prevClipHitPos.w > 1e-6 ? prevClipHitPos.xyz/prevClipHitPos.w : float3(2,2,0);
    oldExactUVZ.z = max(prev_zn_zfar.x, linearize_z(oldExactUVZ.z, prev_zn_zfar.zw));

    #if SSR_MOTIONREPROJ == 1
      motion_type surface_motion = tex2Dlod(MOTION_VECTORS_TEXTURE, float4(hit_uv_z.xy,0,0)).motion_attr;
      #if MOTION_VECTORS_3D
        float3 surface_3d_motion = surface_motion;
      #else
        float3 surface_3d_motion = float3(surface_motion, oldExactUVZ.z - hit_uv_z.z);
      #endif
      if (!CHECK_VALID_MOTION_VECTOR(surface_motion))
        surface_3d_motion = oldExactUVZ - hit_uv_z;
    #else
      #if PREV_HERO_SPHERE
        bool isHero = apply_hero_matrix(hit_uv_z.xy, cameraToHitPoint);
      #endif
      float2 screenPos1;
      float2 oldUv = get_reprojected_history_uvz1(cameraToHitPoint, prev_globtm_no_ofs_psf, screenPos1).xy;

      float3 surface_3d_motion = float3(oldUv, isHero ? oldExactUVZ.z : hit_uv_z.z) - hit_uv_z;
    #endif

    #define SSR_MIPS 1
    float prevLinearDepth = hit_uv_z.z + surface_3d_motion.z;
    float2 sampleUV = hit_uv_z.xy + surface_3d_motion.xy;
  #else
    float prevLinearDepth = hit_uv_z.z;
    float2 sampleUV = hit_uv_z.xy;
  #endif
  float2 screenPos = sampleUV.xy * 2 - 1;

  #if SSR_MIPS
    // mip selection
    float mipBias = 0.15;
    float TXlod = mipBias * linear_roughness * 26.0;
  #else
    float TXlod = 0;
  #endif
  half3 prevScreenColor = 0;
  bool hit = get_prev_frame_disocclusion_weight_sample(sampleUV.xy, prevLinearDepth, cameraToHitPoint, TXlod, prevScreenColor);
  result.rgb = prevScreenColor;
  //hit = true;
  //result.rgb = PREV_FRAME_UNPACK(tex2Dlod(prev_frame_tex, float4(sampleUV.xy, 0, TXlod)).rgb);

  BRANCH if (analytic_light_sphere_pos_r.w > 0)
  {
    float4 add_ref = sample_analytic_sphere(hitDist, cameraToPoint, R, worldPos);
    result.rgb = lerp(result.rgb, add_ref.rgb, add_ref.a);
  }

  result.a = hit ? get_vignette_value(screenPos) : 0;
  return result;
}

float brdf_G_GGX(float3 n, float3 v, float linear_roughness)
{
  float alpha2 = pow4(linear_roughness);
  float sup = pow2(dot(n, v));
  return 2 * sup / (1e-6 + sup + sqrt(sup*(alpha2 + (1 - alpha2)*sup)));//avoid nan 0/0
}

half4 performSSR(uint2 pixelPos, float2 UV, float linear_roughness, float3 N,
                 float linearDepth, float3 cameraToPoint, float4x4 viewProjNoOfsTm, float waterLevel,
                 float3 worldPos, out float reflectionDistance)
{
  half4 result = 0;

  float3 originToPoint = cameraToPoint;
  originToPoint = normalize(originToPoint);
  float stepOfs = interleavedGradientNoiseFramed(pixelPos.xy, uint(SSRParams.z)&7) - 0.25;

  // Sample set dithered over 4x4 pixels
  float3 R = reflect(originToPoint, N);

  float4 hit_uv_z_fade = hierarchRayMarch(UV, R, linear_roughness, linearDepth, cameraToPoint, stepOfs, viewProjNoOfsTm, waterLevel);
  bool wasHitNotZfar = hit_uv_z_fade.z != 0;// hit_uv_z_fade.w was a hit, hit_uv_z_fade.z > 0 - actual hit with something, not with zfar

  bool hitSky = hit_uv_z_fade.z == 0 && hit_uv_z_fade.w != 0;
  // if there was a hit
  BRANCH if (wasHitNotZfar)
  {
    const float linearZHit = ssr_linearize_z(hit_uv_z_fade.z);
    reflectionDistance = length(cameraToPoint - getViewVecOptimized(hit_uv_z_fade.xy) * linearZHit);
    result = sample_vignetted_color(float3(hit_uv_z_fade.xy, linearZHit), linear_roughness, linearZHit, cameraToPoint, R, worldPos);
  } else
    reflectionDistance = 1e6;

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

  return float4(result.rgb*brdf_G_GGX(N, originToPoint, linear_roughness), result.a);
}