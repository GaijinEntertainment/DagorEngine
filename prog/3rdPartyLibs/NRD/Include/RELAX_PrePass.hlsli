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
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_REVERSED;

    // Tile-based early out
    float isSky = gIn_Tiles[pixelPos >> 4];
    if (isSky != 0.0 || pixelPos.x >= gRectSize.x || pixelPos.y >= gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    float centerViewZ = UnpackViewZ(gIn_ViewZ[WithRectOrigin(pixelPos)]);
    if (centerViewZ > gDenoisingRange)
        return;

    // Checkerboard resolve weights
    uint checkerboard = Sequence::CheckerBoard(pixelPos, gFrameIndex);

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
        float viewZ0 = UnpackViewZ(gIn_ViewZ[WithRectOrigin(checkerboardPos.xz)]);
        float viewZ1 = UnpackViewZ(gIn_ViewZ[WithRectOrigin(checkerboardPos.yz)]);

    #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness[WithRectOrigin(checkerboardPos.xz)], materialID0);
        NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness[WithRectOrigin(checkerboardPos.yz)], materialID1);
    #endif

        checkerboardResolveWeights = GetBilateralWeight(float2(viewZ0, viewZ1), centerViewZ);
        checkerboardResolveWeights.x = (viewZ0 > gDenoisingRange || pixelPos.x < 1) ? 0.0 : checkerboardResolveWeights.x;
        checkerboardResolveWeights.y = (viewZ1 > gDenoisingRange || pixelPos.x > gRectSize.x - 2) ? 0.0 : checkerboardResolveWeights.y;
    }

    checkerboardPos.xy >>= 1;

    float centerMaterialID;
    float4 centerNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness[WithRectOrigin(pixelPos)], centerMaterialID);
    float3 centerNormal = centerNormalRoughness.xyz;
    float centerRoughness = centerNormalRoughness.w;

    float3 centerWorldPos = GetCurrentWorldPosFromPixelPos(pixelPos, centerViewZ);
    float4 rotator = GetBlurKernelRotation(NRD_FRAME, pixelPos, gRotatorPre, gFrameIndex);

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
    float4 diffuseIllumination = gIn_Diff[diffPos];
    #ifdef RELAX_SH
        float4 diffuseSH = gIn_DiffSh[diffPos];
    #endif

    if (!diffHasData)
    {
        float2 wc = checkerboardResolveWeights;
        #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
            wc.x *= CompareMaterials(centerMaterialID, materialID0, gDiffMinMaterial);
            wc.y *= CompareMaterials(centerMaterialID, materialID1, gDiffMinMaterial);
        #endif
        wc *= Math::PositiveRcp( wc.x + wc.y );

        float4 d0 = gIn_Diff[checkerboardPos.xz];
        float4 d1 = gIn_Diff[checkerboardPos.yz];
        d0 = Denanify( wc.x, d0 );
        d1 = Denanify( wc.y, d1 );
        diffuseIllumination = d0 * wc.x + d1 * wc.y;

        #ifdef RELAX_SH
            float4 d0SH = gIn_DiffSh[checkerboardPos.xz];
            float4 d1SH = gIn_DiffSh[checkerboardPos.yz];
            d0SH = Denanify( wc.x, d0SH );
            d1SH = Denanify( wc.y, d1SH );
            diffuseSH = d0SH * wc.x + d1SH * wc.y;
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
        float normalWeightParam = GetNormalWeightParam2(1.0, 0.25 * gLobeAngleFraction);
        float2 hitDistanceWeightParams = GetHitDistanceWeightParams(diffuseIllumination.w, 1.0 / 9.0);

        float weightSum = 1.0;

        float diffMinHitDistanceWeight = gMinHitDistanceWeight;

        // Spatial blur
        [unroll]
        for (uint i = 0; i < POISSON_SAMPLE_NUM; i++)
        {
            float3 offset = POISSON_SAMPLES[i];

            // Sample coordinates
            float2 uv = pixelUv * gRectSize + Geometry::RotateVector(rotator, offset.xy) * blurRadius;
            uv = floor(uv) + 0.5;
            uv = ApplyCheckerboardShift(uv, gDiffCheckerboard, i, gFrameIndex) * gRectSizeInv;

            float2 uvScaled = ClampUvToViewport( uv );
            float2 checkerboardUvScaled = float2( uvScaled.x * ( gDiffCheckerboard != 2 ? 0.5 : 1.0 ), uvScaled.y );

            // Fetch data
            float sampleMaterialID;
            float3 sampleNormal = NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0), sampleMaterialID).rgb;
            float sampleViewZ = UnpackViewZ(gIn_ViewZ.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0));
            float3 sampleWorldPos = GetCurrentWorldPosFromClipSpaceXY(uv * 2.0 - 1.0, sampleViewZ);

            // Sample weight
            float sampleWeight = IsInScreenNearest(uv);
            sampleWeight *= float(sampleViewZ < gDenoisingRange);
            sampleWeight *= CompareMaterials(centerMaterialID, sampleMaterialID, gDiffMinMaterial);

            sampleWeight *= GetPlaneDistanceWeight(
                centerWorldPos,
                centerNormal,
                gOrthoMode == 0 ? centerViewZ : 1.0,
                sampleWorldPos,
                gDepthThreshold);

            float angle = Math::AcosApprox(dot(centerNormal, sampleNormal));
            sampleWeight *= ComputeWeight(angle, normalWeightParam, 0.0);

            float4 sampleDiffuseIllumination = gIn_Diff.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
            sampleDiffuseIllumination = Denanify( sampleWeight, sampleDiffuseIllumination );

            sampleWeight *= lerp(diffMinHitDistanceWeight, 1.0, ComputeExponentialWeight(sampleDiffuseIllumination.a, hitDistanceWeightParams.x, hitDistanceWeightParams.y));
            sampleWeight *= GetGaussianWeight(offset.z);

            // Accumulate
            weightSum += sampleWeight;

            diffuseIllumination += sampleDiffuseIllumination * sampleWeight;
            #ifdef RELAX_SH
                float4 sampleDiffuseSH = gIn_DiffSh.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
                sampleDiffuseSH = Denanify( sampleWeight, sampleDiffuseSH );
                diffuseSH += sampleDiffuseSH * sampleWeight;
            #endif
        }

        diffuseIllumination /= weightSum;
        #ifdef RELAX_SH
            diffuseSH /= weightSum;
        #endif
    }

    gOut_Diff[pixelPos] = clamp(diffuseIllumination, 0, NRD_FP16_MAX);
    #ifdef RELAX_SH
        gOut_DiffSh[pixelPos] = clamp(diffuseSH, -NRD_FP16_MAX, NRD_FP16_MAX);
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
    float4 specularIllumination = gIn_Spec[specPos];
    #ifdef RELAX_SH
        float4 specularSH = gIn_SpecSh[specPos];
    #endif

    if (!specHasData)
    {
        float2 wc = checkerboardResolveWeights;
#if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        wc.x *= CompareMaterials(centerMaterialID, materialID0, gSpecMinMaterial);
        wc.y *= CompareMaterials(centerMaterialID, materialID1, gSpecMinMaterial);
#endif
        wc *= Math::PositiveRcp( wc.x + wc.y );

        float4 s0 = gIn_Spec[checkerboardPos.xz];
        float4 s1 = gIn_Spec[checkerboardPos.yz];
        s0 = Denanify( wc.x, s0 );
        s1 = Denanify( wc.y, s1 );
        specularIllumination = s0 * wc.x + s1 * wc.y;

        #ifdef RELAX_SH
            float4 s0SH = gIn_SpecSh[checkerboardPos.xz];
            float4 s1SH = gIn_SpecSh[checkerboardPos.yz];
            s0SH = Denanify( wc.x, s0SH );
            s1SH = Denanify( wc.y, s1SH );
            specularSH = s0SH * wc.x + s1SH * wc.y;
        #endif
    }

    specularIllumination.a = max(0, min(gDenoisingRange, specularIllumination.a));

    // Pre-blur for specular
    if (gSpecBlurRadius > 0)
    {
        // Specular blur radius
        float3 viewVector = (gOrthoMode == 0) ? normalize(-centerWorldPos) : gFrustumForward.xyz;
        float4 D = ImportanceSampling::GetSpecularDominantDirection(centerNormal, viewVector, centerRoughness, ML_SPECULAR_DOMINANT_DIRECTION_G2);
        float NoD = abs(dot(centerNormal, D.xyz));

        float frustumSize = PixelRadiusToWorld(gUnproject, gOrthoMode, min(gRectSize.x, gRectSize.y), centerViewZ);
        float hitDist = (specularIllumination.w == 0.0 ? 1.0 : specularIllumination.w);

        float hitDistFactor = GetHitDistFactor(hitDist * NoD, frustumSize);

        float smc = GetSpecMagicCurve(centerRoughness);
        float blurRadius = gSpecBlurRadius * hitDistFactor * smc;
        float lobeTanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle(centerRoughness);
        float lobeRadius = hitDist * NoD * lobeTanHalfAngle;
        float minBlurRadius = lobeRadius / PixelRadiusToWorld(gUnproject, gOrthoMode, 1.0, centerViewZ + hitDist * D.w);

        blurRadius = min(blurRadius, minBlurRadius);

        if (specularIllumination.w == 0.0)
            blurRadius = max(blurRadius, 1.0);

        float normalWeightParam = GetNormalWeightParam2(centerRoughness, 0.5 * gLobeAngleFraction);
        float2 hitDistanceWeightParams = GetHitDistanceWeightParams(specularIllumination.w, 1.0 / 9.0, centerRoughness);
        float2 roughnessWeightParams = GetRoughnessWeightParams(centerRoughness, gRoughnessFraction);

        float specMinHitDistanceWeight = (specularIllumination.a == 0) ? 1.0 : gMinHitDistanceWeight * smc;
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
            float2 uv = pixelUv * gRectSize + Geometry::RotateVector(rotator, offset.xy) * blurRadius;
            uv = floor(uv) + 0.5;
            uv = ApplyCheckerboardShift(uv, gSpecCheckerboard, i, gFrameIndex) * gRectSizeInv;

            float2 uvScaled = ClampUvToViewport( uv );
            float2 checkerboardUvScaled = float2( uvScaled.x * ( gSpecCheckerboard != 2 ? 0.5 : 1.0 ), uvScaled.y );


            // Fetch data
            float sampleMaterialID;
            float4 sampleNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0), sampleMaterialID);
            float3 sampleNormal = sampleNormalRoughness.rgb;
            float sampleRoughness = sampleNormalRoughness.a;
            float sampleViewZ = UnpackViewZ(gIn_ViewZ.SampleLevel(gNearestClamp, WithRectOffset(uvScaled), 0));

            // Sample weight
            float sampleWeight = IsInScreenNearest(uv);
            sampleWeight *= float(sampleViewZ < gDenoisingRange);
            sampleWeight *= CompareMaterials(centerMaterialID, sampleMaterialID, gSpecMinMaterial);
            sampleWeight *= ComputeWeight(sampleRoughness, roughnessWeightParams.x, roughnessWeightParams.y);

            float angle = Math::AcosApprox(dot(centerNormal, sampleNormal));
            sampleWeight *= ComputeWeight(angle, normalWeightParam, 0.0);

            float3 sampleWorldPos = GetCurrentWorldPosFromClipSpaceXY(uv * 2.0 - 1.0, sampleViewZ);
            sampleWeight *= GetPlaneDistanceWeight(
                centerWorldPos,
                centerNormal,
                gOrthoMode == 0 ? centerViewZ : 1.0,
                sampleWorldPos,
                gDepthThreshold);

            float4 sampleSpecularIllumination = gIn_Spec.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
            sampleSpecularIllumination = Denanify( sampleWeight, sampleSpecularIllumination );

            sampleWeight *= lerp(specMinHitDistanceWeight, 1.0, ComputeExponentialWeight(sampleSpecularIllumination.a, hitDistanceWeightParams.x, hitDistanceWeightParams.y));
            sampleWeight *= GetGaussianWeight(offset.z);

            // Decreasing weight for samples that most likely are very close to reflection contact which should not be pre-blurred
            float d = length(sampleWorldPos - centerWorldPos);
            float h = sampleSpecularIllumination.a;
            float t = h / (specularIllumination.a + d);
            sampleWeight *= lerp(saturate(t), 1.0, Math::LinearStep(0.5, 1.0, centerRoughness));

            // Accumulate
            weightSum += sampleWeight;

            specularIllumination.rgb += sampleSpecularIllumination.rgb * sampleWeight;
            #ifdef RELAX_SH
                float4 sampleSpecularSH = gIn_SpecSh.SampleLevel(gNearestClamp, checkerboardUvScaled, 0);
                sampleSpecularSH = Denanify( sampleWeight, sampleSpecularSH );
                specularSH += sampleSpecularSH * sampleWeight;
            #endif

            if (sampleWeight != 0.0)
                minHitT = min(minHitT, sampleSpecularIllumination.a == 0.0 ? NRD_INF : sampleSpecularIllumination.a);
        }
        specularIllumination.rgb /= weightSum;
        specularIllumination.a = minHitT == NRD_INF ? 0.0 : minHitT;
        #ifdef RELAX_SH
            specularSH /= weightSum;
        #endif
    }

    gOut_Spec[pixelPos] = clamp(specularIllumination, 0, NRD_FP16_MAX);
    #ifdef RELAX_SH
        gOut_SpecSh[pixelPos] = clamp(specularSH, -NRD_FP16_MAX, NRD_FP16_MAX);
    #endif
#endif
}