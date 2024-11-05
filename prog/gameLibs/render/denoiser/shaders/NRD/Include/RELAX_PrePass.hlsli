/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define POISSON_SAMPLE_NUM      8
#define POISSON_SAMPLES         g_Poisson8

[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN(int2 pixelPos : SV_DispatchThreadId, uint2 threadPos : SV_GroupThreadId, uint threadIndex : SV_GroupIndex)
{
    // Tile-based early out
    float isSky = gTiles[pixelPos >> 4];
    if (isSky != 0.0 || pixelPos.x >= (int)gRectSize.x || pixelPos.y >= (int)gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    float centerViewZ = abs(gViewZ[WithRectOrigin(pixelPos)]);
    if (centerViewZ > gDenoisingRange)
        return;

    // Checkerboard resolve weights
    uint checkerboard = STL::Sequence::CheckerBoard(pixelPos, gFrameIndex);

    int3 checkerboardPos = pixelPos.xxy + int3( -1, 1, 0 );
    checkerboardPos.x = max( checkerboardPos.x, 0 );
    checkerboardPos.y = min( checkerboardPos.y, gRectSize.x - 1 );

    float materialID0 = 0;
    float materialID1 = 0;
    float2 checkerboardResolveWeights = 1.0;
#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )
    if ((gSpecCheckerboard != 2) || (gDiffCheckerboard != 2))
#elif( defined RELAX_DIFFUSE )
    if (gDiffCheckerboard != 2)
#elif( defined RELAX_SPECULAR )
    if (gSpecCheckerboard != 2)
#endif
    {
        float viewZ0 = abs(gViewZ[WithRectOrigin(checkerboardPos.xz)]);
        float viewZ1 = abs(gViewZ[WithRectOrigin(checkerboardPos.yz)]);

    #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[WithRectOrigin(checkerboardPos.xz)], materialID0);
        NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[WithRectOrigin(checkerboardPos.yz)], materialID1);
    #endif

        checkerboardResolveWeights = GetBilateralWeight(float2(viewZ0, viewZ1), centerViewZ);
        checkerboardResolveWeights.x = (viewZ0 > gDenoisingRange || pixelPos.x < 1) ? 0.0 : checkerboardResolveWeights.x;
        checkerboardResolveWeights.y = (viewZ1 > gDenoisingRange || pixelPos.x > (int)gRectSize.x - 2) ? 0.0 : checkerboardResolveWeights.y;
    }

    checkerboardPos.xy >>= 1;

    float centerMaterialID;
    float4 centerNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[WithRectOrigin(pixelPos)], centerMaterialID);
    float3 centerNormal = centerNormalRoughness.xyz;
    float centerRoughness = centerNormalRoughness.w;

    float3 centerWorldPos = GetCurrentWorldPosFromPixelPos(pixelPos, centerViewZ);
    float4 rotator = GetBlurKernelRotation(NRD_FRAME, pixelPos, gRotator, gFrameIndex);

    float2 pixelUv = float2(pixelPos + 0.5) * gRectSizeInv;

#ifdef RELAX_DIFFUSE
    bool diffHasData = true;
    int2 diffPos = pixelPos;
    if (gDiffCheckerboard != 2)
    {
        diffHasData = (checkerboard == gDiffCheckerboard);
        diffPos.x >>= 1;
    }

    // Reading diffuse & resolving diffuse checkerboard
    float4 diffuseIllumination = gDiffIllumination[diffPos];
    #ifdef RELAX_SH
        float4 diffuseSH1 = gDiffSH1[diffPos];
    #endif

    if (!diffHasData)
    {
        float2 wc = checkerboardResolveWeights;
        #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
            wc.x *= CompareMaterials(centerMaterialID, materialID0, gDiffMaterialMask);
            wc.y *= CompareMaterials(centerMaterialID, materialID1, gDiffMaterialMask);
        #endif
        wc *= STL::Math::PositiveRcp( wc.x + wc.y );

        float4 d0 = gDiffIllumination[checkerboardPos.xz];
        float4 d1 = gDiffIllumination[checkerboardPos.yz];
        d0 = Denanify( wc.x, d0 );
        d1 = Denanify( wc.y, d1 );
        diffuseIllumination = d0 * wc.x + d1 * wc.y;

        #ifdef RELAX_SH
            float4 d0SH1 = gDiffSH1[checkerboardPos.xz];
            float4 d1SH1 = gDiffSH1[checkerboardPos.yz];
            d0SH1 = Denanify( wc.x, d0SH1 );
            d1SH1 = Denanify( wc.y, d1SH1 );
            diffuseSH1 = d0SH1 * wc.x + d1SH1 * wc.y;
        #endif
    }

    // Pre-blur for diffuse
    if (gDiffBlurRadius > 0)
    {
        // Diffuse blur radius
        float frustumSize = PixelRadiusToWorld(gUnproject, gOrthoMode, min(gRectSize.x, gRectSize.y), centerViewZ);
        float hitDist = (diffuseIllumination.w == 0.0 ? 1.0 : diffuseIllumination.w);
        float hitDistFactor = GetHitDistFactor(hitDist, frustumSize); // NoD = 1
        float blurRadius = gDiffBlurRadius * hitDistFactor;

        if (diffuseIllumination.w == 0.0)
            blurRadius = max(blurRadius, 1.0);

        float worldBlurRadius = PixelRadiusToWorld(gUnproject, gOrthoMode, blurRadius, centerViewZ) * min(gResolutionScale.x, gResolutionScale.y);
        float normalWeightParams = GetNormalWeightParams(1.0, 0.25 * gDiffLobeAngleFraction);
        float2 hitDistanceWeightParams = GetHitDistanceWeightParams(diffuseIllumination.w, 1.0 / 9.0);

        float weightSum = 1.0;

        float diffMinHitDistanceWeight = RELAX_HIT_DIST_MIN_WEIGHT;

        // Spatial blur
        [unroll]
        for (uint i = 0; i < POISSON_SAMPLE_NUM; i++)
        {
            float3 offset = POISSON_SAMPLES[i];

            // Sample coordinates
            float2 uv = pixelUv * gRectSize + STL::Geometry::RotateVector(rotator, offset.xy) * blurRadius;
            uv = floor(uv) + 0.5;
            uv = ApplyCheckerboardShift(uv, gDiffCheckerboard, i, gFrameIndex) * gRectSizeInv;

            float2 uvScaled = ClampUvToViewport( uv );
            float2 checkerboardUvScaled = float2( uvScaled.x * ( gDiffCheckerboard != 2 ? 0.5 : 1.0 ), uvScaled.y );

            // Fetch data
            float sampleMaterialID;
            float3 sampleNormal = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0), sampleMaterialID).rgb;
            float sampleViewZ = abs(gViewZ.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0));
            float3 sampleWorldPos = GetCurrentWorldPosFromClipSpaceXY(uv * 2.0 - 1.0, sampleViewZ);

            // Sample weight
            float sampleWeight = IsInScreenNearest(uv);
            sampleWeight *= float(sampleViewZ < gDenoisingRange);
            sampleWeight *= CompareMaterials(centerMaterialID, sampleMaterialID, gDiffMaterialMask);

            sampleWeight *= GetPlaneDistanceWeight(
                centerWorldPos,
                centerNormal,
                gOrthoMode == 0 ? centerViewZ : 1.0,
                sampleWorldPos,
                gDepthThreshold);

            float angle = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
            sampleWeight *= ComputeWeight(angle, normalWeightParams, 0.0);

            float4 sampleDiffuseIllumination = gDiffIllumination.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
            sampleDiffuseIllumination = Denanify( sampleWeight, sampleDiffuseIllumination );

            sampleWeight *= lerp(diffMinHitDistanceWeight, 1.0, ComputeExponentialWeight(sampleDiffuseIllumination.a, hitDistanceWeightParams.x, hitDistanceWeightParams.y));
            sampleWeight *= GetGaussianWeight(offset.z);

            // Accumulate
            weightSum += sampleWeight;

            diffuseIllumination += sampleDiffuseIllumination * sampleWeight;
            #ifdef RELAX_SH
                float4 sampleDiffuseSH1 = gDiffSH1.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
                sampleDiffuseSH1 = Denanify( sampleWeight, sampleDiffuseSH1 );
                diffuseSH1 += sampleDiffuseSH1 * sampleWeight;
            #endif
        }

        diffuseIllumination /= weightSum;
        #ifdef RELAX_SH
            diffuseSH1 /= weightSum;
        #endif
    }

    gOutDiffuseIllumination[pixelPos] = clamp(diffuseIllumination, 0, NRD_FP16_MAX);
    #ifdef RELAX_SH
        gOutDiffuseSH1[pixelPos] = clamp(diffuseSH1, -NRD_FP16_MAX, NRD_FP16_MAX);
    #endif
#endif

#ifdef RELAX_SPECULAR
    bool specHasData = true;
    int2 specPos = pixelPos;
    if (gSpecCheckerboard != 2)
    {
        specHasData = (checkerboard == gSpecCheckerboard);
        specPos.x >>= 1;
    }

    // Reading specular & resolving specular checkerboard
    float4 specularIllumination = gSpecIllumination[specPos];
    #ifdef RELAX_SH
        float4 specularSH1 = gSpecSH1[specPos];
    #endif

    if (!specHasData)
    {
        float2 wc = checkerboardResolveWeights;
#if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        wc.x *= CompareMaterials(centerMaterialID, materialID0, gSpecMaterialMask);
        wc.y *= CompareMaterials(centerMaterialID, materialID1, gSpecMaterialMask);
#endif
        wc *= STL::Math::PositiveRcp( wc.x + wc.y );

        float4 s0 = gSpecIllumination[checkerboardPos.xz];
        float4 s1 = gSpecIllumination[checkerboardPos.yz];
        s0 = Denanify( wc.x, s0 );
        s1 = Denanify( wc.y, s1 );
        specularIllumination = s0 * wc.x + s1 * wc.y;

        #ifdef RELAX_SH
            float4 s0SH1 = gSpecSH1[checkerboardPos.xz];
            float4 s1SH1 = gSpecSH1[checkerboardPos.yz];
            s0SH1 = Denanify( wc.x, s0SH1 );
            s1SH1 = Denanify( wc.y, s1SH1 );
            specularSH1 = s0SH1 * wc.x + s1SH1 * wc.y;
        #endif
    }

    specularIllumination.a = max(0, min(gDenoisingRange, specularIllumination.a));

    // Pre-blur for specular
    if (gSpecBlurRadius > 0)
    {
        // Specular blur radius
        float3 viewVector = (gOrthoMode == 0) ? normalize(-centerWorldPos) : gFrustumForward.xyz;
        float4 D = STL::ImportanceSampling::GetSpecularDominantDirection(centerNormal, viewVector, centerRoughness, STL_SPECULAR_DOMINANT_DIRECTION_G2);
        float NoD = abs(dot(centerNormal, D.xyz));

        float frustumSize = PixelRadiusToWorld(gUnproject, gOrthoMode, min(gRectSize.x, gRectSize.y), centerViewZ);
        float hitDist = (specularIllumination.w == 0.0 ? 1.0 : specularIllumination.w);

        float hitDistFactor = GetHitDistFactor(hitDist * NoD, frustumSize);

        float blurRadius = gSpecBlurRadius * hitDistFactor * GetSpecMagicCurve(centerRoughness);
        float lobeTanHalfAngle = STL::ImportanceSampling::GetSpecularLobeTanHalfAngle(centerRoughness);
        float lobeRadius = hitDist * NoD * lobeTanHalfAngle;
        float minBlurRadius = lobeRadius / PixelRadiusToWorld(gUnproject, gOrthoMode, 1.0, centerViewZ + hitDist * D.w);

        blurRadius = min(blurRadius, minBlurRadius);

        if (specularIllumination.w == 0.0)
            blurRadius = max(blurRadius, 1.0);

        float normalWeightParams = GetNormalWeightParams(centerRoughness, 0.5 * gSpecLobeAngleFraction);
        float2 hitDistanceWeightParams = GetHitDistanceWeightParams(specularIllumination.w, 1.0 / 9.0, centerRoughness);
        float2 roughnessWeightParams = GetRoughnessWeightParams(centerRoughness, gRoughnessFraction);

        float specMinHitDistanceWeight = (specularIllumination.a == 0) ? 1.0 : RELAX_HIT_DIST_MIN_WEIGHT;
        float specularHitT = (specularIllumination.a == 0) ? gDenoisingRange : specularIllumination.a;

        float NoV = abs(dot(centerNormal, viewVector));

        float minHitT = specularHitT == 0.0 ? NRD_INF : specularHitT;
        float weightSum = 1.0;

        // Spatial blur
        [unroll]
        for (uint i = 0; i < POISSON_SAMPLE_NUM; i++)
        {
            float3 offset = POISSON_SAMPLES[i];

            // Sample coordinates
            float2 uv = pixelUv * gRectSize + STL::Geometry::RotateVector(rotator, offset.xy) * blurRadius;
            uv = floor(uv) + 0.5;
            uv = ApplyCheckerboardShift(uv, gSpecCheckerboard, i, gFrameIndex) * gRectSizeInv;

            float2 uvScaled = ClampUvToViewport( uv );
            float2 checkerboardUvScaled = float2( uvScaled.x * ( gSpecCheckerboard != 2 ? 0.5 : 1.0 ), uvScaled.y );


            // Fetch data
            float sampleMaterialID;
            float4 sampleNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0), sampleMaterialID);
            float3 sampleNormal = sampleNormalRoughness.rgb;
            float sampleRoughness = sampleNormalRoughness.a;
            float sampleViewZ = abs(gViewZ.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0));

            // Sample weight
            float sampleWeight = IsInScreenNearest(uv);
            sampleWeight *= float(sampleViewZ < gDenoisingRange);
            sampleWeight *= CompareMaterials(centerMaterialID, sampleMaterialID, gSpecMaterialMask);
            sampleWeight *= ComputeWeight(sampleRoughness, roughnessWeightParams.x, roughnessWeightParams.y);

            float angle = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
            sampleWeight *= ComputeWeight(angle, normalWeightParams, 0.0);

            float3 sampleWorldPos = GetCurrentWorldPosFromClipSpaceXY(uv * 2.0 - 1.0, sampleViewZ);
            sampleWeight *= GetPlaneDistanceWeight(
                centerWorldPos,
                centerNormal,
                gOrthoMode == 0 ? centerViewZ : 1.0,
                sampleWorldPos,
                gDepthThreshold);

            float4 sampleSpecularIllumination = gSpecIllumination.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
            sampleSpecularIllumination = Denanify( sampleWeight, sampleSpecularIllumination );

            sampleWeight *= lerp(specMinHitDistanceWeight, 1.0, ComputeExponentialWeight(sampleSpecularIllumination.a, hitDistanceWeightParams.x, hitDistanceWeightParams.y));
            sampleWeight *= GetGaussianWeight(offset.z);

            // Decreasing weight for samples that most likely are very close to reflection contact which should not be pre-blurred
            float d = length(sampleWorldPos - centerWorldPos);
            float h = sampleSpecularIllumination.a;
            float t = h / (specularIllumination.a + d);
            sampleWeight *= lerp(saturate(t), 1.0, STL::Math::LinearStep(0.5, 1.0, centerRoughness));

            // Accumulate
            weightSum += sampleWeight;

            specularIllumination.rgb += sampleSpecularIllumination.rgb * sampleWeight;
            #ifdef RELAX_SH
                float4 sampleSpecularSH1 = gSpecSH1.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
                sampleSpecularSH1 = Denanify( sampleWeight, sampleSpecularSH1 );
                specularSH1 += sampleSpecularSH1 * sampleWeight;
            #endif

            if (sampleWeight != 0.0)
                minHitT = min(minHitT, sampleSpecularIllumination.a == 0.0 ? NRD_INF : sampleSpecularIllumination.a);
        }
        specularIllumination.rgb /= weightSum;
        specularIllumination.a = minHitT == NRD_INF ? 0.0 : minHitT;
        #ifdef RELAX_SH
            specularSH1 /= weightSum;
        #endif
    }

    gOutSpecularIllumination[pixelPos] = clamp(specularIllumination, 0, NRD_FP16_MAX);
    #ifdef RELAX_SH
        gOutSpecularSH1[pixelPos] = clamp(specularSH1, -NRD_FP16_MAX, NRD_FP16_MAX);
    #endif
#endif
}