/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float4 sharedNormalSpecHitT[BUFFER_Y][BUFFER_X];

// Helper functions

float isReprojectionTapValid(float3 currentWorldPos, float3 previousWorldPos, float3 currentNormal, float disocclusionThreshold)
{
    // Check if plane distance is acceptable
    float3 posDiff = currentWorldPos - previousWorldPos;
    float maxPlaneDistance = abs(dot(posDiff, currentNormal));

    return maxPlaneDistance > disocclusionThreshold ? 0.0 : 1.0;
}

// Returns reprojected data from previous frame calculated using filtering based on filters above.
// Returns reprojection search result based on surface motion:
// 2 - reprojection found, bicubic footprint was used
// 1 - reprojection found, bilinear footprint was used
// 0 - reprojection not found

float loadSurfaceMotionBasedPrevData(
    float3 prevWorldPos,
    float2 prevUVSMB,
    float currentLinearZ,
    float3 currentNormal,
#ifdef RELAX_SPECULAR
    float currentReflectionHitT,
#endif
    float NoV,
    float smbParallaxInPixelsMax,
    float currentMaterialID,
    float disocclusionThreshold,

    out float footprintQuality,
    out float historyLength
#ifdef RELAX_DIFFUSE
    , out float4 prevDiffuseIllumAnd2ndMoment
    , out float3 prevDiffuseResponsiveIllum
    #ifdef RELAX_SH
        , out float4 prevDiffuseSH
        , out float4 prevDiffuseResponsiveSH
    #endif
#endif
#ifdef RELAX_SPECULAR
    , out float4 prevSpecularIllumAnd2ndMoment
    , out float3 prevSpecularResponsiveIllum
    , out float  prevReflectionHitT
    #ifdef RELAX_SH
        , out float4 prevSpecularSH
        , out float4 prevSpecularResponsiveSH
    #endif
#endif
)
{
    // Calculating previous pixel position
    float2 prevPixelPosFloat = prevUVSMB * gRectSizePrev;

    // Calculating footprint origin and weights
    int2 bilinearOrigin = int2(floor(prevPixelPosFloat - 0.5));
    float2 bilinearWeights = frac(prevPixelPosFloat - 0.5);

    // Checking bicubic footprint (with cut corners)
    // remembering bilinear taps validity and worldspace position along the way,
    // for faster weighted bilinear and for calculating previous worldspace position
    // bc - bicubic tap,
    // bl - bicubic & bilinear tap
    //
    // -- bc bc --
    // bc bl bl bc
    // bc bl bl bc
    // -- bc bc --

    /// Fetching previous viewZs and materialIDs
    float2 gatherOrigin00 = (float2(bilinearOrigin) + float2(0.0, 0.0)) * gResourceSizeInvPrev;
    float2 gatherOrigin10 = (float2(bilinearOrigin) + float2(2.0, 0.0)) * gResourceSizeInvPrev;
    float2 gatherOrigin01 = (float2(bilinearOrigin) + float2(0.0, 2.0)) * gResourceSizeInvPrev;
    float2 gatherOrigin11 = (float2(bilinearOrigin) + float2(2.0, 2.0)) * gResourceSizeInvPrev;
    float4 prevViewZs00 = UnpackViewZ(gPrev_ViewZ.GatherRed(gNearestClamp, gatherOrigin00).wzxy);
    float4 prevViewZs10 = UnpackViewZ(gPrev_ViewZ.GatherRed(gNearestClamp, gatherOrigin10).wzxy);
    float4 prevViewZs01 = UnpackViewZ(gPrev_ViewZ.GatherRed(gNearestClamp, gatherOrigin01).wzxy);
    float4 prevViewZs11 = UnpackViewZ(gPrev_ViewZ.GatherRed(gNearestClamp, gatherOrigin11).wzxy);
    float4 prevMaterialIDs00 = gPrev_MateriallID.GatherRed(gNearestClamp, gatherOrigin00).wzxy * 255.0;
    float4 prevMaterialIDs10 = gPrev_MateriallID.GatherRed(gNearestClamp, gatherOrigin10).wzxy * 255.0;
    float4 prevMaterialIDs01 = gPrev_MateriallID.GatherRed(gNearestClamp, gatherOrigin01).wzxy * 255.0;
    float4 prevMaterialIDs11 = gPrev_MateriallID.GatherRed(gNearestClamp, gatherOrigin11).wzxy * 255.0;

    // Calculating disocclusion threshold
    float pixelSize = PixelRadiusToWorld(gUnproject, gOrthoMode, 1.0, currentLinearZ);
    float frustumSize = pixelSize * min(gRectSize.x, gRectSize.y);
    float disocclusionThresholdSlopeScale = 1.0 / lerp(lerp(0.05, 1.0, NoV), 1.0, saturate(smbParallaxInPixelsMax / 30.0));
    float4 smbDisocclusionThreshold = saturate(disocclusionThreshold * disocclusionThresholdSlopeScale) * frustumSize;
    smbDisocclusionThreshold *= IsInScreenBilinear(bilinearOrigin, gRectSizePrev);
    smbDisocclusionThreshold -= NRD_EPS;

    // Calculating validity of 12 bicubic taps, 4 of those are bilinear taps
    float3 prevViewPos = Geometry::AffineTransform(gWorldToViewPrev, prevWorldPos);
    float3 planeDist0 = abs(prevViewZs00.yzw - prevViewPos.zzz);
    float3 planeDist1 = abs(prevViewZs10.xzw - prevViewPos.zzz);
    float3 planeDist2 = abs(prevViewZs01.xyw - prevViewPos.zzz);
    float3 planeDist3 = abs(prevViewZs11.xyz - prevViewPos.zzz);
    float3 tapsValid0 = step(planeDist0, smbDisocclusionThreshold.x);
    float3 tapsValid1 = step(planeDist1, smbDisocclusionThreshold.y);
    float3 tapsValid2 = step(planeDist2, smbDisocclusionThreshold.z);
    float3 tapsValid3 = step(planeDist3, smbDisocclusionThreshold.w);

    float minMaterialID = min(gSpecMinMaterial, gDiffMinMaterial); // TODO: separation is expensive
    tapsValid0 *= CompareMaterials(currentMaterialID.xxx, prevMaterialIDs00.yzw, minMaterialID);
    tapsValid1 *= CompareMaterials(currentMaterialID.xxx, prevMaterialIDs10.xzw, minMaterialID);
    tapsValid2 *= CompareMaterials(currentMaterialID.xxx, prevMaterialIDs01.xyw, minMaterialID);
    tapsValid3 *= CompareMaterials(currentMaterialID.xxx, prevMaterialIDs11.xyz, minMaterialID);

    float bicubicFootprintValid = dot(tapsValid0 + tapsValid1 + tapsValid2 + tapsValid3, 1.0) > 11.5 ? 1.0 : 0.0;
    float4 bilinearTapsValid = float4(tapsValid0.z, tapsValid1.y, tapsValid2.y, tapsValid3.x);

    // Using bilinear to average 4 normal samples
    float2 uv = (float2(bilinearOrigin)+float2(1.0, 1.0)) * gResourceSizeInvPrev;
    float3 prevNormalFlat = UnpackPrevNormalRoughness(gPrev_Normal_Roughness.SampleLevel(gLinearClamp, uv, 0)).xyz;
    prevNormalFlat = Geometry::RotateVector(gWorldPrevToWorld, prevNormalFlat);

    // Reject backfacing history: if angle between current normal and previous normal is larger than 90 deg
    [flatten]
    if (dot(currentNormal, prevNormalFlat) < 0.0)
    {
        bilinearTapsValid = 0;
        bicubicFootprintValid = 0;
    }

    // Calculating bilinear weights in advance
    Filtering::Bilinear bilinear;
    bilinear.weights = bilinearWeights;
    float4 bilinearCustomWeights = Filtering::GetBilinearCustomWeights(bilinear, float4(bilinearTapsValid.x, bilinearTapsValid.y, bilinearTapsValid.z, bilinearTapsValid.w));

    bool useBicubic = (bicubicFootprintValid > 0);

    // Fetching normal history
    BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
        prevPixelPosFloat, gResourceSizeInvPrev,
        bilinearCustomWeights, useBicubic
    #ifdef RELAX_DIFFUSE
        , gHistory_Diff, prevDiffuseIllumAnd2ndMoment
    #endif
    #ifdef RELAX_SPECULAR
        , gHistory_Spec, prevSpecularIllumAnd2ndMoment
    #endif
    );

    // Fetching fast history
    float4 spec;
    float4 diff;
    BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
        prevPixelPosFloat, gResourceSizeInvPrev,
        bilinearCustomWeights, useBicubic
    #ifdef RELAX_DIFFUSE
        , gHistory_DiffFast, diff
    #endif
    #ifdef RELAX_SPECULAR
        , gHistory_SpecFast, spec
    #endif
    );

#ifdef RELAX_DIFFUSE
    prevDiffuseIllumAnd2ndMoment = max(prevDiffuseIllumAnd2ndMoment, 0);
    prevDiffuseResponsiveIllum = max(diff.rgb, 0);
#endif
#ifdef RELAX_SPECULAR
    prevSpecularIllumAnd2ndMoment = max(prevSpecularIllumAnd2ndMoment, 0.0);
    prevSpecularResponsiveIllum = max(spec.rgb, 0);
#endif

    // Fitering previous SH data
#ifdef RELAX_SH
    #ifdef RELAX_DIFFUSE
        prevDiffuseSH = BilinearWithCustomWeightsFloat4(gHistory_DiffSh, bilinearOrigin, bilinearCustomWeights);
        prevDiffuseResponsiveSH = BilinearWithCustomWeightsFloat4(gHistory_DiffShFast, bilinearOrigin, bilinearCustomWeights);
    #endif
    #ifdef RELAX_SPECULAR
        prevSpecularSH = BilinearWithCustomWeightsFloat4(gHistory_SpecSh, bilinearOrigin, bilinearCustomWeights);
        prevSpecularResponsiveSH = BilinearWithCustomWeightsFloat4(gHistory_SpecShFast, bilinearOrigin, bilinearCustomWeights);
    #endif
#endif

    // Fitering more previous data that does not need bicubic
    float2 gatherOrigin = (float2(bilinearOrigin) + 1.0) * gResourceSizeInvPrev;
    float4 prevHistoryLengths = gPrev_HistoryLength.GatherRed(gNearestClamp, gatherOrigin).wzxy;
    historyLength = 255.0 * BilinearWithCustomWeightsImmediateFloat(
        prevHistoryLengths.x,
        prevHistoryLengths.y,
        prevHistoryLengths.z,
        prevHistoryLengths.w,
        bilinearCustomWeights);

#ifdef RELAX_SPECULAR
    float4 prevReflectionHitTs = gPrev_SpecHitDist.GatherRed(gNearestClamp, gatherOrigin).wzxy;
    prevReflectionHitT = BilinearWithCustomWeightsImmediateFloat(prevReflectionHitTs.x, prevReflectionHitTs.y, prevReflectionHitTs.z, prevReflectionHitTs.w, bilinearCustomWeights);
    prevReflectionHitT = max(0.001, prevReflectionHitT);
#endif

    float reprojectionFound = (bicubicFootprintValid > 0) ? 2.0 : 1.0;
    footprintQuality = (bicubicFootprintValid > 0) ? 1.0 : dot(bilinearCustomWeights, 1.0);

    [flatten]
    if (!any(bilinearTapsValid))
    {
        reprojectionFound = 0;
        footprintQuality = 0;
    }

    return reprojectionFound;
}

// Returns specular reprojection search result based on virtual motion
#ifdef RELAX_SPECULAR
float loadVirtualMotionBasedPrevData(
    float3 currentWorldPos,
    float3 currentNormal,
    float currentLinearZ,
    float hitDistFocused,
    float hitDistOriginal,
    float3 currentViewVector,
    float3 prevWorldPos,
    bool surfaceBicubicValid,
    float currentMaterialID,
    float2 prevUVSMB,
    float smbParallaxInPixelsMax,
    float NoV,
    float disocclusionThreshold,
    out float4 prevSpecularIllumAnd2ndMoment,
    out float4 prevSpecularResponsiveIllum,
    out float3 prevNormal,
    out float prevRoughness,
    out float prevReflectionHitT,
    out float2 prevUVVMB
    #ifdef RELAX_SH
        , out float4 prevSpecularSH
        , out float4 prevSpecularResponsiveSH
    #endif
    )
{
    // Calculating previous worldspace virtual position based on reflection hitT
    float3 virtualViewVector = normalize(currentViewVector) * hitDistFocused;
    float3 prevVirtualWorldPos = prevWorldPos + virtualViewVector;

    float currentViewVectorLength = length(currentViewVector);
    float accumulatedSpecularVMBZ = currentViewVectorLength + hitDistFocused;

    float4 prevVirtualClipPos = mul(gWorldToClipPrev, float4(prevVirtualWorldPos, 1.0));
    prevVirtualClipPos.xy /= prevVirtualClipPos.w;
    prevUVVMB = prevVirtualClipPos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
    prevUVVMB = currentMaterialID == gCameraAttachedReflectionMaterialID ? prevUVSMB : prevUVVMB;

    float2 prevVirtualPixelPosFloat = prevUVVMB * gRectSizePrev;

    // Calculating footprint origin and weights
    int2 bilinearOrigin = int2(floor(prevVirtualPixelPosFloat - 0.5));
    float2 bilinearWeights = frac(prevVirtualPixelPosFloat - 0.5);
    float2 gatherOrigin = (bilinearOrigin + 1.0) * gResourceSizeInvPrev;

    // Taking care of camera motion, because world-space is always centered at camera position in NRD
    currentWorldPos -= gCameraDelta.xyz;

    // Calculating disocclusion threshold
    float4 vmbDisocclusionThreshold = disocclusionThreshold * (gOrthoMode == 0 ? currentLinearZ : 1.0);
    vmbDisocclusionThreshold *= IsInScreenBilinear(bilinearOrigin, gRectSizePrev);
    vmbDisocclusionThreshold -= NRD_EPS;

    // Checking bilinear footprint only for virtual motion based specular reprojection
    float4 prevViewZs = UnpackViewZ(gPrev_ViewZ.GatherRed(gNearestClamp, gatherOrigin).wzxy);
    float4 prevMaterialIDs = gPrev_MateriallID.GatherRed(gNearestClamp, gatherOrigin).wzxy * 255.0;
    float3 prevWorldPosInTap;
    float4 bilinearTapsValid;

    prevWorldPosInTap = GetPreviousWorldPosFromPixelPos(bilinearOrigin + int2(0, 0), prevViewZs.x);
    bilinearTapsValid.x = isReprojectionTapValid(currentWorldPos, prevWorldPosInTap, currentNormal, vmbDisocclusionThreshold.x);
    prevWorldPosInTap = GetPreviousWorldPosFromPixelPos(bilinearOrigin + int2(1, 0), prevViewZs.y);
    bilinearTapsValid.y = isReprojectionTapValid(currentWorldPos, prevWorldPosInTap, currentNormal, vmbDisocclusionThreshold.y);
    prevWorldPosInTap = GetPreviousWorldPosFromPixelPos(bilinearOrigin + int2(0, 1), prevViewZs.z);
    bilinearTapsValid.z = isReprojectionTapValid(currentWorldPos, prevWorldPosInTap, currentNormal, vmbDisocclusionThreshold.z);
    prevWorldPosInTap = GetPreviousWorldPosFromPixelPos(bilinearOrigin + int2(1, 1), prevViewZs.w);
    bilinearTapsValid.w = isReprojectionTapValid(currentWorldPos, prevWorldPosInTap, currentNormal, vmbDisocclusionThreshold.w);

    bilinearTapsValid *= CompareMaterials(currentMaterialID.xxxx, prevMaterialIDs.xyzw, gSpecMinMaterial);

    // Applying reprojection
    prevSpecularIllumAnd2ndMoment = 0;
    prevSpecularResponsiveIllum = 0;
    prevNormal = currentNormal;
    prevRoughness = 0;
    prevReflectionHitT = gDenoisingRange;
    #ifdef RELAX_SH
        prevSpecularSH = 0;
        prevSpecularResponsiveSH = 0;
    #endif

    // Weighted bilinear (or bicubic optionally) for prev specular data based on virtual motion.
    if (any(bilinearTapsValid))
    {
        // Calculating bilinear weights in advance
        Filtering::Bilinear bilinear;
        bilinear.weights = bilinearWeights;
        float4 bilinearCustomWeights = Filtering::GetBilinearCustomWeights(bilinear, float4(bilinearTapsValid.x, bilinearTapsValid.y, bilinearTapsValid.z, bilinearTapsValid.w));

        bool useBicubic = (surfaceBicubicValid > 0) & all(bilinearTapsValid);

        // Fetching normal virtual motion based specular history
        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            prevVirtualPixelPosFloat, gResourceSizeInvPrev,
            bilinearCustomWeights, useBicubic,
            gHistory_Spec, prevSpecularIllumAnd2ndMoment);

        prevSpecularIllumAnd2ndMoment = max(prevSpecularIllumAnd2ndMoment, 0.0);

        // Fetching fast virtual motion based specular history
        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            prevVirtualPixelPosFloat, gResourceSizeInvPrev,
            bilinearCustomWeights, useBicubic,
            gHistory_SpecFast, prevSpecularResponsiveIllum);

        prevSpecularResponsiveIllum = max(prevSpecularResponsiveIllum, 0.0);

        // Fitering previous SH data
        #ifdef RELAX_SH
            prevSpecularSH = BilinearWithCustomWeightsFloat4(gHistory_SpecSh, bilinearOrigin, bilinearCustomWeights);
            prevSpecularResponsiveSH = BilinearWithCustomWeightsFloat4(gHistory_SpecShFast, bilinearOrigin, bilinearCustomWeights);
        #endif

        // Fitering previous data that does not need bicubic
        prevReflectionHitT = gPrev_SpecHitDist.SampleLevel(gLinearClamp, prevUVVMB * gResolutionScalePrev, 0).x;
        prevReflectionHitT = max(0.001, prevReflectionHitT);

        float4 prevNormalRoughness = UnpackPrevNormalRoughness(gPrev_Normal_Roughness.SampleLevel(gLinearClamp, prevUVVMB * gResolutionScalePrev, 0));
        prevNormal = prevNormalRoughness.xyz;
        prevNormal = Geometry::RotateVector(gWorldPrevToWorld, prevNormal);
        prevRoughness = prevNormalRoughness.w;
    }
    // Using all() marks entire virtual motion based specular history footprint
    // invalid for specular reprojection logic down the shader code even if at least one bilinear tap is invalid.
    // This helps rejecting potentially incorrect data.
    return all(bilinearTapsValid) ? 1.0 : 0.0;
}
#endif

void Preload(uint2 sharedPos, int2 globalPos)
{
    globalPos = clamp(globalPos, 0, gRectSize - 1.0);

    float4 normalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness[WithRectOrigin(globalPos)]);
    float4 normalSpecHitT = normalRoughness;

#ifdef RELAX_SPECULAR
    float4 inSpecularIllumination = gIn_Spec[globalPos];
    normalSpecHitT.a = inSpecularIllumination.a;
#endif

    sharedNormalSpecHitT[sharedPos.y][sharedPos.x] = normalSpecHitT;
}

// Main
[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    float isSky = gIn_Tiles[pixelPos >> 4];
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out
    if (isSky != 0.0 || pixelPos.x >= gRectSize.x || pixelPos.y >= gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    float currentLinearZ = UnpackViewZ(gIn_ViewZ[WithRectOrigin(pixelPos)]);
    if (currentLinearZ > gDenoisingRange)
        return;

    int2 sharedMemoryIndex = threadPos.xy + int2(BORDER, BORDER);

    // Reading current GBuffer data
    float currentMaterialID;
    float4 currentNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness[WithRectOrigin(pixelPos)], currentMaterialID);
    float3 currentNormal = currentNormalRoughness.xyz;
    float currentRoughness = currentNormalRoughness.w;

    // Getting current position and view vector for current pixel
    float3 currentWorldPos = GetCurrentWorldPosFromPixelPos(pixelPos, currentLinearZ);
    float3 currentViewVector = (gOrthoMode == 0) ? currentWorldPos : currentLinearZ * normalize(gFrustumForward.xyz);
    float3 V = -normalize(currentViewVector);
    float NoV = abs(dot(currentNormal, V));

    // Getting previous position
    float2 pixelUv = float2(pixelPos + 0.5) * gRectSizeInv;
    float3 mv = gIn_Mv[WithRectOrigin(pixelPos)] * gMvScale.xyz;
    float3 prevWorldPos = currentWorldPos;
    float2 prevUVSMB = pixelUv + mv.xy;

    if (gMvScale.w == 0.0)
    {
        if (gMvScale.z == 0.0)
            mv.z = Geometry::AffineTransform(gWorldToViewPrev, currentWorldPos).z - currentLinearZ;

        prevWorldPos = GetPreviousWorldPosFromClipSpaceXY(prevUVSMB * 2.0 - 1.0, currentLinearZ + mv.z) + gCameraDelta.xyz;
    }
    else
    {
        prevWorldPos += mv;
        prevUVSMB = Geometry::GetScreenUv(gWorldToClipPrev, prevWorldPos);
    }

    // Input noisy data
#ifdef RELAX_DIFFUSE
    float3 diffuseIllumination = gIn_Diff[pixelPos].rgb;
    #ifdef RELAX_SH
        float4 diffuseSH = gIn_DiffSh[pixelPos];
    #endif
#endif

#ifdef RELAX_SPECULAR
    float4 specularIllumination = gIn_Spec[pixelPos];
    #ifdef RELAX_SH
        float4 specularSH = gIn_SpecSh[pixelPos];
    #endif
#endif

    // Calculating average normal, minHitDist and specular sigma
    float hitTM1 = sharedNormalSpecHitT[sharedMemoryIndex.y][sharedMemoryIndex.x].a;
    float minHitDist3x3 = hitTM1 == 0.0 ? NRD_INF : hitTM1;
    float3 currentNormalAveraged = currentNormal;

    [unroll]
    for (i = -1; i <= 1; i++)
    {
        [unroll]
        for (j = -1; j <= 1; j++)
        {
            // Skipping center pixel
            if ((i == 0) && (j == 0))
                continue;

            float4 normalSpecHitT = sharedNormalSpecHitT[sharedMemoryIndex.y + j][sharedMemoryIndex.x + i];

            minHitDist3x3 = min(minHitDist3x3, normalSpecHitT.a == 0.0 ? NRD_INF : normalSpecHitT.a);
            currentNormalAveraged += normalSpecHitT.rgb;
        }
    }
    currentNormalAveraged /= 9.0;

#ifdef RELAX_SPECULAR
    float currentRoughnessModified = Filtering::GetModifiedRoughnessFromNormalVariance(currentRoughness, currentNormalAveraged);
#endif

    // Computing 2nd moments of input noisy luminance
#ifdef RELAX_SPECULAR
    float specular1stMoment = Color::Luminance(specularIllumination.rgb);
    float specular2ndMoment = specular1stMoment * specular1stMoment;
#endif

#ifdef RELAX_DIFFUSE
    float diffuse1stMoment = Color::Luminance(diffuseIllumination.rgb);
    float diffuse2ndMoment = diffuse1stMoment * diffuse1stMoment;
#endif

    // Calculating surface parallax
    float smbParallaxInPixels1 = ComputeParallaxInPixels( prevWorldPos + gCameraDelta.xyz, gOrthoMode == 0.0 ? prevUVSMB : pixelUv, gWorldToClipPrev, gRectSize );
    float smbParallaxInPixels2 = ComputeParallaxInPixels( prevWorldPos - gCameraDelta.xyz, gOrthoMode == 0.0 ? pixelUv : prevUVSMB, gWorldToClip, gRectSize );

    float smbParallaxInPixelsMax = max( smbParallaxInPixels1, smbParallaxInPixels2 );
    float smbParallaxInPixelsMin = min( smbParallaxInPixels1, smbParallaxInPixels2 );

    float pixelSize = PixelRadiusToWorld(gUnproject, gOrthoMode, 1.0, currentLinearZ);

    // Calculating disocclusion threshold
    float disocclusionThresholdMix = 0;
    if(currentMaterialID == gStrandMaterialID)
        disocclusionThresholdMix = NRD_GetNormalizedStrandThickness(gStrandThickness, pixelSize);
    if(gHasDisocclusionThresholdMix && NRD_USE_DISOCCLUSION_THRESHOLD_MIX)
        disocclusionThresholdMix = gIn_DisocclusionThresholdMix[WithRectOrigin(pixelPos)];

    float disocclusionThreshold = lerp(gDisocclusionThreshold, gDisocclusionThresholdAlternate, disocclusionThresholdMix);

    // Loading previous data based on surface motion vectors
    float footprintQuality;
#ifdef RELAX_DIFFUSE
    float4 prevDiffuseIlluminationAnd2ndMomentSMB;
    float3 prevDiffuseIlluminationAnd2ndMomentSMBResponsive;
    #ifdef RELAX_SH
        float4 prevDiffuseSH;
        float4 prevDiffuseResponsiveSH;
    #endif
#endif

#ifdef RELAX_SPECULAR
    float4 prevSpecularIlluminationAnd2ndMomentSMB;
    float3 prevSpecularIlluminationAnd2ndMomentSMBResponsive;
    float  prevReflectionHitTSMB;
    #ifdef RELAX_SH
        float4 prevSpecularSMBSH;
        float4 prevSpecularSMBResponsiveSH;
    #endif
#endif

    float historyLength;
    float SMBReprojectionFound = loadSurfaceMotionBasedPrevData(
        prevWorldPos,
        prevUVSMB,
        currentLinearZ,
        normalize(currentNormalAveraged),
    #ifdef RELAX_SPECULAR
        specularIllumination.a,
    #endif
        NoV,
        smbParallaxInPixelsMax,
        currentMaterialID,
        disocclusionThreshold,
        footprintQuality,
        historyLength
    #ifdef RELAX_DIFFUSE
        , prevDiffuseIlluminationAnd2ndMomentSMB
        , prevDiffuseIlluminationAnd2ndMomentSMBResponsive
        #ifdef RELAX_SH
            , prevDiffuseSH
            , prevDiffuseResponsiveSH
        #endif
    #endif
    #ifdef RELAX_SPECULAR
        , prevSpecularIlluminationAnd2ndMomentSMB
        , prevSpecularIlluminationAnd2ndMomentSMBResponsive
        , prevReflectionHitTSMB
        #ifdef RELAX_SH
            , prevSpecularSMBSH
            , prevSpecularSMBResponsiveSH
        #endif
    #endif
    );

    // History length is based on surface motion based disocclusion
    historyLength = historyLength + 1.0;
    historyLength = min(RELAX_MAX_ACCUM_FRAME_NUM, historyLength);

    // Avoid footprint momentary stretching due to changed viewing angle
    float3 Vprev = (gOrthoMode == 0) ? -normalize(prevWorldPos - gCameraDelta.xyz) : -normalize(gPrevFrustumForward.xyz);
    float NoVprev = abs(dot(currentNormal, Vprev));
    float sizeQuality = (NoVprev + 1e-3) / (NoV + 1e-3); // this order because we need to fix stretching only, shrinking is OK
    sizeQuality *= sizeQuality;
    sizeQuality *= sizeQuality;
    footprintQuality *= lerp(0.1, 1.0, saturate(sizeQuality + abs(gOrthoMode)));

    // Minimize "getting stuck in history" effect when only fraction of bilinear footprint is valid
    // by shortening the history length
    [flatten]
    if (footprintQuality < 1.0)
    {
        historyLength *= sqrt(footprintQuality);
        historyLength = max(historyLength, 1.0);
    }

    // Handling history reset if needed
    historyLength = (gResetHistory != 0) ? 1.0 : historyLength;

    // Limiting history length: HistoryFix must be invoked if history length <= gHistoryFixFrameNum
#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )
    float maxAccumulatedFrameNum = 1.0 + max(gDiffMaxAccumulatedFrameNum, gSpecMaxAccumulatedFrameNum);
#elif( defined RELAX_DIFFUSE )
    float maxAccumulatedFrameNum = 1.0 + gDiffMaxAccumulatedFrameNum;
#else
    float maxAccumulatedFrameNum = 1.0 + gSpecMaxAccumulatedFrameNum;
#endif
    historyLength = min(historyLength, maxAccumulatedFrameNum);

    // Calculating checkerboard fields
    uint checkerboard = Sequence::CheckerBoard(pixelPos, gFrameIndex);

#ifdef RELAX_DIFFUSE
    // Temporal accumulation of diffuse illumination
    float diffMaxAccumulatedFrameNum = gDiffMaxAccumulatedFrameNum;
    float diffMaxFastAccumulatedFrameNum = gDiffMaxFastAccumulatedFrameNum;

    if (gHasHistoryConfidence && NRD_USE_HISTORY_CONFIDENCE)
    {
        float inDiffConfidence = gIn_DiffConfidence[WithRectOrigin(pixelPos)];
        diffMaxAccumulatedFrameNum *= inDiffConfidence;
        diffMaxFastAccumulatedFrameNum *= inDiffConfidence;
    }

    float diffHistoryLength = historyLength;

    float diffuseAlpha = (SMBReprojectionFound > 0) ? max(1.0 / (diffMaxAccumulatedFrameNum + 1.0), 1.0 / diffHistoryLength) : 1.0;
    float diffuseAlphaResponsive = (SMBReprojectionFound > 0) ? max(1.0 / (diffMaxFastAccumulatedFrameNum + 1.0), 1.0 / diffHistoryLength) : 1.0;

    bool diffHasData = true;
    if (gDiffCheckerboard != 2)
        diffHasData = checkerboard == gDiffCheckerboard;

    if ((!diffHasData) && (diffHistoryLength > 1.0))
    {
        // Adjusting diffuse accumulation weights for checkerboard
        diffuseAlpha *= 1.0 - gCheckerboardResolveAccumSpeed;
        diffuseAlphaResponsive *= 1.0 - gCheckerboardResolveAccumSpeed;
    }

    float4 accumulatedDiffuseIlluminationAnd2ndMoment = lerp(prevDiffuseIlluminationAnd2ndMomentSMB, float4(diffuseIllumination.rgb, diffuse2ndMoment), diffuseAlpha);
    float3 accumulatedDiffuseIlluminationResponsive = lerp(prevDiffuseIlluminationAnd2ndMomentSMBResponsive.rgb, diffuseIllumination.rgb, diffuseAlphaResponsive);

    // Write out the diffuse results
    gOut_Diff[pixelPos] = accumulatedDiffuseIlluminationAnd2ndMoment;
    gOut_DiffFast[pixelPos] = float4(accumulatedDiffuseIlluminationResponsive, 0);

    #ifdef RELAX_SH
        float4 accumulatedDiffuseSH = lerp(prevDiffuseSH, diffuseSH, diffuseAlpha);
        float4 accumulatedDiffuseResponsiveSH = lerp(prevDiffuseResponsiveSH, diffuseSH, diffuseAlphaResponsive);
        gOut_DiffSh[pixelPos] = accumulatedDiffuseSH;
        gOut_DiffShFast[pixelPos] = float4(accumulatedDiffuseResponsiveSH);
    #endif
#endif

    gOut_HistoryLength[pixelPos] = historyLength / 255.0;

#ifdef RELAX_SPECULAR
    float specMaxAccumulatedFrameNum = gSpecMaxAccumulatedFrameNum;
    float specMaxFastAccumulatedFrameNum = gSpecMaxFastAccumulatedFrameNum;
    if (gHasHistoryConfidence && NRD_USE_HISTORY_CONFIDENCE)
    {
        float inSpecConfidence = gIn_SpecConfidence[WithRectOrigin(pixelPos)];
        specMaxAccumulatedFrameNum *= inSpecConfidence;
        specMaxFastAccumulatedFrameNum *= inSpecConfidence;
    }

    float specHistoryLength = historyLength;
    float specHistoryFrames = min(specMaxAccumulatedFrameNum, specHistoryLength);
    float specHistoryResponsiveFrames = min(specMaxFastAccumulatedFrameNum, specHistoryLength);

    // Picking hitDist as minimal value in 3x3 area
    float hitDist = minHitDist3x3 == NRD_INF ? 0.0 : minHitDist3x3;

    // Calculating curvature along the direction of motion
    float curvature = 0.0;
    {
        // IMPORTANT: non-zero parallax on objects attached to the camera is needed
        // IMPORTANT: the direction of "deltaUv" is important ( test 1 )
        float2 uvForZeroParallax = gOrthoMode == 0.0 ? prevUVSMB : pixelUv;
        float2 deltaUv = uvForZeroParallax - Geometry::GetScreenUv( gWorldToClipPrev, prevWorldPos + gCameraDelta.xyz ); // TODO: repeats code for "smbParallaxInPixels1" with "-" sign
        deltaUv *= gRectSize;
        deltaUv /= max( smbParallaxInPixels1, 1.0 / 256.0 );

        // 10 edge
        float3 n10, x10;
        {
            float3 x = GetCurrentWorldPosFromClipSpaceXY( ( pixelUv + float2( 1, 0 ) * gRectSizeInv ) * 2.0 - 1.0, 1.0 );
            float3 v = gOrthoMode == 0.0 ? normalize( -x ) : gFrustumForward.xyz;
            float3 o = gOrthoMode == 0.0 ? 0 : x;

            x10 = o + v * dot( currentWorldPos - o, currentNormal ) / dot( currentNormal, v ); // line-plane intersection
            n10 = sharedNormalSpecHitT[ threadPos.y + BORDER ][ threadPos.x + BORDER + 1 ].xyz;
        }

        // 01 edge
        float3 n01, x01;
        {
            float3 x = GetCurrentWorldPosFromClipSpaceXY( ( pixelUv + float2( 0, 1 ) * gRectSizeInv ) * 2.0 - 1.0, 1.0 );
            float3 v = gOrthoMode == 0.0 ? normalize( -x ) : gFrustumForward.xyz;
            float3 o = gOrthoMode == 0.0 ? 0 : x;

            x01 = o + v * dot( currentWorldPos - o, currentNormal ) / dot( currentNormal, v ); // line-plane intersection
            n01 = sharedNormalSpecHitT[ threadPos.y + BORDER + 1 ][ threadPos.x + BORDER ].xyz;
        }

        // Mix
        float2 w = abs( deltaUv ) + 1.0 / 256.0;
        w /= w.x + w.y;

        float3 x = x10 * w.x + x01 * w.y;
        float3 n = normalize( n10 * w.x + n01 * w.y );

        // High parallax - flattens surface on high motion ( test 132, 172, 173, 174, 190, 201, 202, 203, e9 )
        // IMPORTANT: a must for 8-bit and 10-bit normals ( tests b7, b10, b33, 202 )
        float deltaUvLenFixed = smbParallaxInPixelsMin; // "min" because not needed for objects attached to the camera!
        deltaUvLenFixed *= NRD_USE_HIGH_PARALLAX_CURVATURE_SILHOUETTE_FIX ? NoV : 1.0; // it fixes silhouettes, but leads to less flattening
        deltaUvLenFixed *= 1.0 + gFramerateScale * Sequence::Bayer4x4( pixelPos, gFrameIndex ); // improves behavior if FPS is high, dithering is needed to avoid artefacts in test 1

        float2 motionUvHigh = pixelUv + deltaUvLenFixed * deltaUv * gRectSizeInv;
        motionUvHigh = ( floor( motionUvHigh * gRectSize ) + 0.5 ) * gRectSizeInv; // Snap to the pixel center!

        if( NRD_USE_HIGH_PARALLAX_CURVATURE && deltaUvLenFixed > 1.0 && IsInScreenNearest( motionUvHigh ) )
        {
            float2 uvScaled = WithRectOffset( ClampUvToViewport( motionUvHigh ) );

            float zHigh = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, uvScaled, 0 ) );
            float3 xHigh = GetCurrentWorldPosFromClipSpaceXY( motionUvHigh * 2.0 - 1.0, zHigh );

            float3 nHigh = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness.SampleLevel( gNearestClamp, uvScaled, 0 ) ).xyz;

            // Replace if same surface
            float zError = abs( zHigh - currentLinearZ ) * rcp( max( zHigh, currentLinearZ ) );
            bool cmp = zError < NRD_CURVATURE_Z_THRESHOLD; // TODO: use common disocclusion logic?

            n = cmp ? nHigh : n;
            x = cmp ? xHigh : x;
        }

        // Estimate curvature for the edge { x; currentWorldPos }
        float3 edge = x - currentWorldPos;
        float edgeLenSq = Math::LengthSquared( edge );
        curvature = dot( n - currentNormal, edge ) * Math::PositiveRcp( edgeLenSq );

    #if( NRD_USE_SPECULAR_MOTION_V2 == 0 ) // needed only for the old version
        // Correction #1 - this is needed if camera is "inside" a concave mirror ( tests 133, 164, 171 - 176 )
        if( length( currentWorldPos ) < -1.0 / curvature ) // TODO: test 78
            curvature *= NoV;

        // Correction #2 - very negative inconsistent with previous frame curvature blows up reprojection ( tests 164, 171 - 176 )
        float2 uv1 = Geometry::GetScreenUv( gWorldToClipPrev, currentWorldPos - V * ApplyThinLensEquation( hitDist, curvature ) );
        float2 uv2 = Geometry::GetScreenUv( gWorldToClipPrev, currentWorldPos );
        float a = length( ( uv1 - uv2 ) * gRectSize );
        curvature *= float( a < NRD_MAX_ALLOWED_VIRTUAL_MOTION_ACCELERATION * smbParallaxInPixelsMax + gRectSizeInv.x );
    #endif
    }

    // Thin lens equation for adjusting reflection HitT
    float hitDistFocused = ApplyThinLensEquation(hitDist, curvature);

    // Loading specular data based on virtual motion
    float4 prevSpecularIlluminationAnd2ndMomentVMB;
    float4 prevSpecularIlluminationAnd2ndMomentVMBResponsive;
    float3 prevNormalVMB;
    float2 prevUVVMB;
    float prevRoughnessVMB;
    float prevReflectionHitTVMB;
    #ifdef RELAX_SH
        float4 prevSpecularVMBSH;
        float4 prevSpecularVMBResponsiveSH;
    #endif

    float VMBReprojectionFound = loadVirtualMotionBasedPrevData(
        currentWorldPos,
        currentNormal,
        currentLinearZ,
        hitDistFocused,
        hitDist,
        currentViewVector,
        prevWorldPos,
        SMBReprojectionFound == 2.0 ? true : false,
        currentMaterialID,
        prevUVSMB,
        smbParallaxInPixelsMax,
        NoV,
        disocclusionThreshold,
        prevSpecularIlluminationAnd2ndMomentVMB,
        prevSpecularIlluminationAnd2ndMomentVMBResponsive,
        prevNormalVMB,
        prevRoughnessVMB,
        prevReflectionHitTVMB,
        prevUVVMB
    #ifdef RELAX_SH
        , prevSpecularVMBSH
        , prevSpecularVMBResponsiveSH
    #endif
    );

    // Amount of virtual motion - dominant factor
    float4 D = ImportanceSampling::GetSpecularDominantDirection(currentNormal, V, currentRoughnessModified, ML_SPECULAR_DOMINANT_DIRECTION_G2);
    float virtualHistoryAmount = VMBReprojectionFound * D.w;

    // Decreasing virtual history amount for ortho case
    virtualHistoryAmount *= (gOrthoMode == 0) ? 1.0 : 0.75;

    // Virtual motion amount - back-facing
    virtualHistoryAmount *= float(dot(prevNormalVMB, currentNormalAveraged) > 0.0);

    // Curvature angle for virtual motion based reprojection
    float2 uvDiff = prevUVVMB - prevUVSMB;
    float uvDiffLengthInPixels = length(uvDiff * gRectSize);

    float tanCurvature = abs(curvature * pixelSize);
    tanCurvature *= max(uvDiffLengthInPixels / max(NoV, 0.01), 1.0); // path length
    float curvatureAngle = atan(tanCurvature);

    // Normal weight for virtual motion based reprojection
    float lobeHalfAngle = max(atan(GetSpecLobeTanHalfAngle(currentRoughnessModified)), RELAX_NORMAL_ULP);
    float normalWeight = GetEncodingAwareNormalWeight(currentNormal, prevNormalVMB, lobeHalfAngle, curvatureAngle, RELAX_NORMAL_ULP, true);
    virtualHistoryAmount *= lerp(1.0 - saturate(uvDiffLengthInPixels), 1.0, normalWeight); // jitter friendly

    // Roughness weight for virtual motion based reprojection
    float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams(currentRoughness * currentRoughness, gRoughnessFraction);
    float virtualRoughnessWeight = ComputeWeight(prevRoughnessVMB * prevRoughnessVMB, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y);
    virtualRoughnessWeight = lerp(1.0 - saturate(uvDiffLengthInPixels), 1.0, virtualRoughnessWeight); // jitter friendly
    virtualHistoryAmount *= (gOrthoMode == 0) ? virtualRoughnessWeight : 1.0;
    float specVMBConfidence = virtualRoughnessWeight * 0.9 + 0.1;

    // "Looking back" 1 and 2 frames and applying normal weight to decrease lags
    uvDiff *= Math::Rsqrt(Math::LengthSquared(uvDiff));
    uvDiff /= gRectSizePrev;
    uvDiff *= saturate(uvDiffLengthInPixels / 0.1) + uvDiffLengthInPixels / 2.0;
    float2 backUV1 = prevUVVMB + 1.0 * uvDiff;
    float2 backUV2 = prevUVVMB + 2.0 * uvDiff;
    float4 backNormalRoughness1 = UnpackPrevNormalRoughness(gPrev_Normal_Roughness.SampleLevel(gLinearClamp, backUV1 * gResolutionScalePrev, 0));
    float4 backNormalRoughness2 = UnpackPrevNormalRoughness(gPrev_Normal_Roughness.SampleLevel(gLinearClamp, backUV2 * gResolutionScalePrev, 0));
    backNormalRoughness1.rgb = Geometry::RotateVector(gWorldPrevToWorld, backNormalRoughness1.rgb);
    backNormalRoughness2.rgb = Geometry::RotateVector(gWorldPrevToWorld, backNormalRoughness2.rgb);
    float prevPrevNormalWeight = IsInScreenNearest(backUV1) ? GetEncodingAwareNormalWeight(prevNormalVMB, backNormalRoughness1.rgb, lobeHalfAngle, curvatureAngle * 2.0, RELAX_NORMAL_ULP, true) : 1.0;
    prevPrevNormalWeight *= IsInScreenNearest(backUV2) ? GetEncodingAwareNormalWeight(prevNormalVMB, backNormalRoughness2.rgb, lobeHalfAngle, curvatureAngle * 3.0, RELAX_NORMAL_ULP, true) : 1.0;
    virtualHistoryAmount *= 0.33 + 0.67 * prevPrevNormalWeight;
    specVMBConfidence *= 0.33 + 0.67 * prevPrevNormalWeight;
    // Taking in account roughness 1 and 2 frames back helps cleaning up surfaces wigh varying roughness
    float rw = ComputeWeight(backNormalRoughness1.w * backNormalRoughness1.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y);
    rw *= ComputeWeight(backNormalRoughness2.w * backNormalRoughness2.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y);
    virtualHistoryAmount *= (gOrthoMode == 0) ? rw * 0.9 + 0.1 : 1.0;

    // Virtual history confidence - hit distance
    float SMC = GetSpecMagicCurve(currentRoughnessModified);
    float hitDistC = lerp(specularIllumination.a, prevReflectionHitTSMB, SMC);
    float hitDist1 = ApplyThinLensEquation(hitDistC, curvature);
    float hitDist2 = ApplyThinLensEquation(prevReflectionHitTVMB, curvature);
    float maxDist = max(hitDist1, hitDist2);
    float dHitT = abs(hitDist1 - hitDist2);
    float dHitTMultiplier = lerp(20.0, 0.0, SMC);
    float virtualHistoryHitDistConfidence = 1.0 - saturate(dHitTMultiplier * dHitT / (currentLinearZ + maxDist));
    virtualHistoryHitDistConfidence = lerp(virtualHistoryHitDistConfidence, 1.0, SMC);

    // Virtual history confidence - virtual UV discrepancy
    float3 virtualWorldPos = GetXvirtual(hitDist, curvature, currentWorldPos, prevWorldPos, currentNormal, V, currentRoughness);
    float virtualWorldPosLength = length(virtualWorldPos);
    float hitDistForTrackingPrev = prevSpecularIlluminationAnd2ndMomentVMBResponsive.a;
    float3 prevVirtualWorldPos = GetXvirtual(hitDistForTrackingPrev, curvature, currentWorldPos, prevWorldPos, currentNormal, V, currentRoughness);
    float virtualWorldPosLengthPrev = length(prevVirtualWorldPos);
    float2 prevUVVMBTest = Geometry::GetScreenUv(gWorldToClipPrev, prevVirtualWorldPos, false);
    prevUVVMBTest = currentMaterialID == gCameraAttachedReflectionMaterialID ? prevUVSMB : prevUVVMBTest;

    float percentOfVolume = 0.6;
    float lobeTanHalfAngle = GetSpecLobeTanHalfAngle(currentRoughness, percentOfVolume);
    lobeTanHalfAngle = max(lobeTanHalfAngle, 0.5 * gRectSizeInv.x);

    float unproj1 = min(hitDist, hitDistForTrackingPrev) / PixelRadiusToWorld(gUnproject, gOrthoMode, 1.0, max(virtualWorldPosLength, virtualWorldPosLengthPrev));
    float lobeRadiusInPixels = lobeTanHalfAngle * unproj1;

    float deltaParallaxInPixels = length((prevUVVMBTest - prevUVVMB) * gRectSize);
    virtualHistoryHitDistConfidence *= Math::SmoothStep(lobeRadiusInPixels + 0.25, 0.0, deltaParallaxInPixels);

    // Current specular signal ( surface motion )
    float specSMBConfidence = (SMBReprojectionFound > 0 ? 1.0 : 0.0) *
        GetEncodingAwareNormalWeight(V, Vprev, lobeHalfAngle * NoV / gFramerateScale, 0.0, 0.0, false);

    float specSMBAlpha = 1.0 - specSMBConfidence;
    float specSMBResponsiveAlpha = 1.0 - specSMBConfidence;
    specSMBAlpha = max(specSMBAlpha, 1.0 / (1.0 + specHistoryFrames));
    specSMBResponsiveAlpha = max(specSMBAlpha, 1.0 / (1.0 + specHistoryResponsiveFrames));

    bool specHasData = true;
    if (gSpecCheckerboard != 2)
        specHasData = checkerboard == gSpecCheckerboard;

    if (!specHasData && (smbParallaxInPixelsMax < 0.5))
    {
        // Adjusting surface motion based specular accumulation weights for checkerboard
        specSMBAlpha *= 1.0 - gCheckerboardResolveAccumSpeed * (SMBReprojectionFound > 0 ? 1.0 : 0.0);
        specSMBResponsiveAlpha *= 1.0 - gCheckerboardResolveAccumSpeed * (SMBReprojectionFound > 0 ? 1.0 : 0.0);
    }

    float4 accumulatedSpecularSMB;
    accumulatedSpecularSMB.rgb = lerp(prevSpecularIlluminationAnd2ndMomentSMB.rgb, specularIllumination.rgb, specSMBAlpha);
    accumulatedSpecularSMB.w = lerp(prevReflectionHitTSMB, specularIllumination.w, max(specSMBAlpha, 0.1));
    float accumulatedSpecularM2SMB = lerp(prevSpecularIlluminationAnd2ndMomentSMB.a, specular2ndMoment, specSMBAlpha);
    float3 accumulatedSpecularSMBResponsive = lerp(prevSpecularIlluminationAnd2ndMomentSMBResponsive, specularIllumination.xyz, specSMBResponsiveAlpha);

    // Current specular signal ( virtual motion )
    float specVMBAlpha = 1.0 - specVMBConfidence;
    float specVMBResponsiveAlpha = 1.0 - specVMBConfidence * virtualHistoryHitDistConfidence;
    float specVMBHitTAlpha = specVMBResponsiveAlpha;

    specVMBAlpha = max(specVMBAlpha, 1.0 / (1.0 + specHistoryFrames));
    specVMBResponsiveAlpha = max(specVMBResponsiveAlpha, 1.0 / (1.0 + specHistoryResponsiveFrames));
    specVMBHitTAlpha = max(specVMBHitTAlpha, 1.0 / (1.0 + specHistoryFrames));

    [flatten]
    if (!specHasData && (smbParallaxInPixelsMax < 0.5))
    {
        // Adjusting virtual motion based specular accumulation weights for checkerboard
        specVMBAlpha *= 1.0 - gCheckerboardResolveAccumSpeed * (VMBReprojectionFound > 0 ? 1.0 : 0.0);
        specVMBResponsiveAlpha *= 1.0 - gCheckerboardResolveAccumSpeed * (VMBReprojectionFound > 0 ? 1.0 : 0.0);
        specVMBHitTAlpha *= 1.0 - gCheckerboardResolveAccumSpeed * (VMBReprojectionFound > 0 ? 1.0 : 0.0);
    }

    float4 accumulatedSpecularVMB;
    accumulatedSpecularVMB.rgb = lerp(prevSpecularIlluminationAnd2ndMomentVMB.rgb, specularIllumination.rgb, specVMBAlpha);
    accumulatedSpecularVMB.a = lerp(prevReflectionHitTVMB, specularIllumination.a, max(specVMBHitTAlpha, 0.1));
    float accumulatedSpecularM2VMB = lerp(prevSpecularIlluminationAnd2ndMomentVMB.a, specular2ndMoment, specVMBAlpha);
    float3 accumulatedSpecularVMBResponsive = lerp(prevSpecularIlluminationAnd2ndMomentVMBResponsive.rgb, specularIllumination.xyz, specVMBResponsiveAlpha);

    // Fallback to surface motion if virtual motion doesn't go well
    virtualHistoryAmount *= saturate(specVMBConfidence / (specSMBConfidence + NRD_EPS));

    // Temporal accumulation of reflection HitT
    float accumulatedReflectionHitT = lerp(accumulatedSpecularSMB.a, accumulatedSpecularVMB.a, virtualHistoryAmount);

    // Temporal accumulation of specular illumination
    float3 accumulatedSpecularIllumination = lerp(accumulatedSpecularSMB.xyz, accumulatedSpecularVMB.xyz, virtualHistoryAmount);
    float3 accumulatedSpecularIlluminationResponsive = lerp(accumulatedSpecularSMBResponsive.xyz, accumulatedSpecularVMBResponsive.xyz, virtualHistoryAmount);
    float accumulatedSpecular2ndMoment = lerp(accumulatedSpecularM2SMB, accumulatedSpecularM2VMB, virtualHistoryAmount);

    #ifdef RELAX_SH
        float4 accumulatedSpecularSMBSH = lerp(prevSpecularSMBSH, specularSH, specSMBAlpha);
        float4 accumulatedSpecularSMBResponsiveSH = lerp(prevSpecularSMBResponsiveSH, specularSH, specSMBResponsiveAlpha);

        float4 accumulatedSpecularVMBSH = lerp(prevSpecularVMBSH, specularSH, specVMBAlpha);
        float4 accumulatedSpecularVMBResponsiveSH = lerp(prevSpecularVMBResponsiveSH, specularSH, specVMBResponsiveAlpha);

        float4 accumulatedSpecularSH = lerp(accumulatedSpecularSMBSH, accumulatedSpecularVMBSH, virtualHistoryAmount);
        float4 accumulatedSpecularResponsiveSH = lerp(accumulatedSpecularSMBResponsiveSH, accumulatedSpecularVMBResponsiveSH, virtualHistoryAmount);
        gOut_SpecSh[pixelPos] = float4(accumulatedSpecularSH.rgb, currentRoughnessModified);
        gOut_SpecShFast[pixelPos] = accumulatedSpecularResponsiveSH;
    #endif

    // If zero specular sample (color = 0), artificially adding variance for pixels with low reprojection confidence
    float specularHistoryConfidence = lerp(specSMBConfidence, specVMBConfidence, virtualHistoryAmount);
    if (accumulatedSpecular2ndMoment == 0) accumulatedSpecular2ndMoment = gSpecVarianceBoost * (1.0 - specularHistoryConfidence);

    // Write out the results
    gOut_Spec[pixelPos] = float4(accumulatedSpecularIllumination, accumulatedSpecular2ndMoment);
    gOut_SpecFast[pixelPos] = float4(accumulatedSpecularIlluminationResponsive, hitDist);

    gOut_SpecHitDist[pixelPos] = accumulatedReflectionHitT;
    gOut_SpecReprojectionConfidence[pixelPos] = specularHistoryConfidence;
#endif
}
