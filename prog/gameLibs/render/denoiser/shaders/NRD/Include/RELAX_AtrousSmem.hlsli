/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifdef RELAX_DIFFUSE
    groupshared float4 sharedDiffuse[BUFFER_Y][BUFFER_X];
    #ifdef RELAX_SH
        groupshared float4 sharedDiffuseSH1[BUFFER_Y][BUFFER_X];
    #endif
#endif

#ifdef RELAX_SPECULAR
    groupshared float4 sharedSpecular[BUFFER_Y][BUFFER_X];
    #ifdef RELAX_SH
        groupshared float4 sharedSpecularSH1[BUFFER_Y][BUFFER_X];
    #endif
#endif

groupshared float4 sharedNormalRoughness[BUFFER_Y][BUFFER_X];
groupshared float4 sharedWorldPosMaterialID[BUFFER_Y][BUFFER_X];

// Helper functions

// computes a 3x3 gaussian blur of the variance, centered around
// the current pixel
void computeVariance(
    int2 threadPos
#ifdef RELAX_SPECULAR
    ,out float specularVariance
#endif
#ifdef RELAX_DIFFUSE
    ,out float diffuseVariance
#endif
)
{
#ifdef RELAX_SPECULAR
    float4 specularSum = 0;
#endif
#ifdef RELAX_DIFFUSE
    float4 diffuseSum = 0;
#endif

    static const float kernel[2][2] =
    {
        { 1.0 / 4.0, 1.0 / 8.0  },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };

    int2 sharedMemoryIndex = threadPos.xy + int2(BORDER, BORDER);
    [unroll]
    for (int dx = -1; dx <= 1; dx++)
    {
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        {
            int2 sharedMemoryIndexP = sharedMemoryIndex + int2(dx,dy);
            float k = kernel[abs(dx)][abs(dy)];
#ifdef RELAX_SPECULAR
            float4 specular = sharedSpecular[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
            specularSum += specular * k;
#endif
#ifdef RELAX_DIFFUSE
            float4 diffuse = sharedDiffuse[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
            diffuseSum += diffuse * k;
#endif
        }
    }
#ifdef RELAX_SPECULAR
    float specular1stMoment = STL::Color::Luminance(specularSum.rgb);
    float specular2ndMoment = specularSum.a;
    specularVariance = max(0, specular2ndMoment - specular1stMoment * specular1stMoment);
#endif
#ifdef RELAX_DIFFUSE
    float diffuse1stMoment = STL::Color::Luminance(diffuseSum.rgb);
    float diffuse2ndMoment = diffuseSum.a;
    diffuseVariance = max(0, diffuse2ndMoment - diffuse1stMoment * diffuse1stMoment);
#endif
}

void Preload(uint2 sharedPos, int2 globalPos)
{
    globalPos = clamp(globalPos, 0, gRectSize - 1.0);

#ifdef RELAX_SPECULAR
    sharedSpecular[sharedPos.y][sharedPos.x] = gSpecIlluminationAnd2ndMoment[globalPos];
    #ifdef RELAX_SH
        sharedSpecularSH1[sharedPos.y][sharedPos.x] = gSpecSH1[globalPos];
    #endif
#endif

#ifdef RELAX_DIFFUSE
    sharedDiffuse[sharedPos.y][sharedPos.x] = gDiffIlluminationAnd2ndMoment[globalPos];
    #ifdef RELAX_SH
        sharedDiffuseSH1[sharedPos.y][sharedPos.x] = gDiffSH1[globalPos];
    #endif
#endif
    float materialID;
    sharedNormalRoughness[sharedPos.y][sharedPos.x] = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[globalPos], materialID);

    float viewZ = abs(gViewZ[globalPos]);
    sharedWorldPosMaterialID[sharedPos.y][sharedPos.x] = float4(GetCurrentWorldPosFromPixelPos(globalPos, viewZ), materialID);

}

[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN(int2 pixelPos : SV_DispatchThreadId, uint2 threadPos : SV_GroupThreadId, uint threadIndex : SV_GroupIndex)
{
    // Preload
    float isSky = gTiles[pixelPos >> 4];
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    int2 sharedMemoryIndex = threadPos.xy + int2(BORDER, BORDER);

    float4 centerWorldPosMaterialID = sharedWorldPosMaterialID[sharedMemoryIndex.y][sharedMemoryIndex.x];
    float3 centerWorldPos = centerWorldPosMaterialID.xyz;
    float centerMaterialID = centerWorldPosMaterialID.w;
    float centerViewZ = abs(gViewZ[pixelPos]);
    gOutViewZ[pixelPos] = centerViewZ;

    // Repacking normal and roughness to prev normal roughness to be used in the next frame
    float4 normalRoughness = sharedNormalRoughness[sharedMemoryIndex.y][sharedMemoryIndex.x];
    if (centerViewZ > gDenoisingRange)
    {
        // Setting normal and roughness to close to zero for out of range pixels
        normalRoughness = 1.0 / 255.0;
    }
    gOutNormalRoughness[pixelPos] = PackPrevNormalRoughness(normalRoughness);

#if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
    gOutMaterialID[pixelPos] = centerMaterialID;
#endif

    // Tile-based early out
    if (isSky != 0.0 || pixelPos.x >= (int)gRectSize.x || pixelPos.y >= (int)gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    if (centerViewZ > gDenoisingRange)
        return;

    float3 centerNormal = normalRoughness.rgb;
    float centerRoughness = normalRoughness.a;

    float historyLength = 255.0 * gHistoryLength[pixelPos];

    [branch]
    if (historyLength >= gHistoryThreshold) // Running Atrous 3x3
    {
        // Calculating variance, filtered using 3x3 gaussin blur
#ifdef RELAX_SPECULAR
        float centerSpecularVar;
#endif
#ifdef RELAX_DIFFUSE
        float centerDiffuseVar;
#endif
        computeVariance(
            threadPos.xy
#ifdef RELAX_SPECULAR
            , centerSpecularVar
#endif
#ifdef RELAX_DIFFUSE
            , centerDiffuseVar
#endif
        );

        // Diffuse normal weight is used for diffuse and can be used for specular depending on settings.
        float diffuseLobeAngleFraction = gDiffLobeAngleFraction;

#ifdef RELAX_SPECULAR
        float specularReprojectionConfidence = gSpecReprojectionConfidence[pixelPos];
        float specularLuminanceWeightRelaxation = lerp(1.0, specularReprojectionConfidence, gLuminanceEdgeStoppingRelaxation);

        float centerSpecularLuminance = STL::Color::Luminance(sharedSpecular[sharedMemoryIndex.y][sharedMemoryIndex.x].rgb);
        float specularPhiLIlluminationInv = 1.0 / max(1.0e-4, gSpecPhiLuminance * sqrt(centerSpecularVar));
        float2 roughnessWeightParams = GetRoughnessWeightParams(centerRoughness, gRoughnessFraction);

        float diffuseLobeAngleFractionForSimplifiedSpecularNormalWeight = diffuseLobeAngleFraction;
        float specularLobeAngleFraction = gSpecLobeAngleFraction;

        if (gUseConfidenceInputs != 0)
        {
            float specConfidenceDrivenRelaxation =
                saturate(gConfidenceDrivenRelaxationMultiplier * (1.0 - gSpecConfidence[pixelPos]));

            // Relaxing normal weights for specular
            float r = saturate(specConfidenceDrivenRelaxation * gConfidenceDrivenNormalEdgeStoppingRelaxation);
            diffuseLobeAngleFractionForSimplifiedSpecularNormalWeight = lerp(diffuseLobeAngleFraction, 1.0, r);
            specularLobeAngleFraction = lerp(specularLobeAngleFraction, 1.0, r);

            // Relaxing luminance weight for specular
            r = saturate(specConfidenceDrivenRelaxation * gConfidenceDrivenLuminanceEdgeStoppingRelaxation);
            specularLuminanceWeightRelaxation *= 1.0 - r;
        }

        float specularNormalWeightParamsSimplified = GetNormalWeightParams(1.0, diffuseLobeAngleFractionForSimplifiedSpecularNormalWeight);
        float2 specularNormalWeightParams =
            GetNormalWeightParams_ATrous(
                centerRoughness,
                historyLength,
                specularReprojectionConfidence,
                gNormalEdgeStoppingRelaxation,
                specularLobeAngleFraction,
                gSpecLobeAngleSlack);

        float sumWSpecular = 0;
        float4 sumSpecularIlluminationAnd2ndMoment = 0;
        #ifdef RELAX_SH
            float4 sumSpecularSH1 = 0;
            float roughnessModified = sharedSpecularSH1[sharedMemoryIndex.y][sharedMemoryIndex.x].w;
        #endif
        float3 centerV = -normalize(centerWorldPos);
#endif

#ifdef RELAX_DIFFUSE
        float centerDiffuseLuminance = STL::Color::Luminance(sharedDiffuse[sharedMemoryIndex.y][sharedMemoryIndex.x].rgb);
        float diffusePhiLIlluminationInv = 1.0 / max(1.0e-4, gDiffPhiLuminance * sqrt(centerDiffuseVar));

        float diffuseLuminanceWeightRelaxation = 1.0;
        if (gUseConfidenceInputs != 0)
        {
            float diffConfidenceDrivenRelaxation =
                saturate(gConfidenceDrivenRelaxationMultiplier * (1.0 - gDiffConfidence[pixelPos]));

            // Relaxing normal weights for diffuse
            float r = saturate(diffConfidenceDrivenRelaxation * gConfidenceDrivenNormalEdgeStoppingRelaxation);
            diffuseLobeAngleFraction = lerp(diffuseLobeAngleFraction, 1.0, r);

            // Relaxing luminance weight for diffuse
            r = saturate(diffConfidenceDrivenRelaxation * gConfidenceDrivenLuminanceEdgeStoppingRelaxation);
            diffuseLuminanceWeightRelaxation = 1.0 - r;
        }
        float diffuseNormalWeightParams = GetNormalWeightParams(1.0, diffuseLobeAngleFraction);

        float sumWDiffuse = 0;
        float4 sumDiffuseIlluminationAnd2ndMoment = 0;
        #ifdef RELAX_SH
            float4 sumDiffuseSH1 = 0;
        #endif
#endif

        static const float kernelWeightGaussian3x3[2] = { 0.44198, 0.27901 };
        float depthThreshold = gDepthThreshold * (gOrthoMode == 0 ? centerViewZ : 1.0);

        [unroll]
        for (int cx = -1; cx <= 1; cx++)
        {
            [unroll]
            for (int cy = -1; cy <= 1; cy++)
            {
                const int2 p = pixelPos + int2(cx, cy);
                const bool isCenter = ((cx == 0) && (cy == 0));
                const bool isInside = all(p >= 0) && all(p < int2(gRectSize));
                const float kernel = isInside ? kernelWeightGaussian3x3[abs(cx)] * kernelWeightGaussian3x3[abs(cy)] : 0.0;

                int2 sharedMemoryIndexP = sharedMemoryIndex + int2(cx, cy);

                float4 sampleNormalRoughness = sharedNormalRoughness[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleNormal = sampleNormalRoughness.rgb;
                float sampleRoughness = sampleNormalRoughness.a;
                float4 sampleWorldPosMaterialID = sharedWorldPosMaterialID[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleWorldPos = sampleWorldPosMaterialID.rgb;
                float sampleMaterialID = sampleWorldPosMaterialID.a;

                // Calculating geometry weight for diffuse and specular
                float geometryW = GetPlaneDistanceWeight_Atrous(
                    centerWorldPos,
                    centerNormal,
                    sampleWorldPos,
                    depthThreshold);

                geometryW *= kernel;

#ifdef RELAX_SPECULAR
                // Calculating weights for specular

                // Getting sample view vector closer to center view vector
                // by adding gRoughnessEdgeStoppingRelaxation * centerWorldPos
                // relaxes view direction based rejection
                float angles = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
                float3 sampleV = -normalize(sampleWorldPos + gRoughnessEdgeStoppingRelaxation * centerWorldPos);
                float normalWSpecularSimplified = ComputeWeight(angles, specularNormalWeightParamsSimplified, 0.0);
                float normalWSpecular = GetSpecularNormalWeight_ATrous(specularNormalWeightParams, centerNormal, sampleNormal, centerV, sampleV);

                float roughnessWSpecular = ComputeWeight(sampleRoughness, roughnessWeightParams.x, roughnessWeightParams.y);

                // Summing up specular
                float4 sampleSpecularIlluminationAnd2ndMoment = sharedSpecular[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float sampleSpecularLuminance = STL::Color::Luminance(sampleSpecularIlluminationAnd2ndMoment.rgb);

                float specularLuminanceW = abs(centerSpecularLuminance - sampleSpecularLuminance) * specularPhiLIlluminationInv;
                specularLuminanceW = min(gSpecMaxLuminanceRelativeDifference, specularLuminanceW);
                specularLuminanceW *= specularLuminanceWeightRelaxation;
                float wSpecular = geometryW * exp(-specularLuminanceW);

                wSpecular *= gRoughnessEdgeStoppingEnabled ? (normalWSpecular * roughnessWSpecular) : normalWSpecularSimplified;
                wSpecular = isCenter ? kernel : wSpecular;
                wSpecular *= CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMaterialMask);

                sumWSpecular += wSpecular;
                sumSpecularIlluminationAnd2ndMoment += wSpecular * sampleSpecularIlluminationAnd2ndMoment;
                #ifdef RELAX_SH
                    sumSpecularSH1 += wSpecular * sharedSpecularSH1[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                #endif
#endif
#ifdef RELAX_DIFFUSE
                // Calculating weights for diffuse
                float angled = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
                float normalWDiffuse = ComputeWeight(angled, diffuseNormalWeightParams, 0.0);

                // Summing up diffuse
                float4 sampleDiffuseIlluminationAnd2ndMoment = sharedDiffuse[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float sampleDiffuseLuminance = STL::Color::Luminance(sampleDiffuseIlluminationAnd2ndMoment.rgb);

                float diffuseLuminanceW = abs(centerDiffuseLuminance - sampleDiffuseLuminance) * diffusePhiLIlluminationInv;
                diffuseLuminanceW = min(gDiffMaxLuminanceRelativeDifference, diffuseLuminanceW);
                if (gUseConfidenceInputs != 0)
                    diffuseLuminanceW *= diffuseLuminanceWeightRelaxation;

                float wDiffuse = geometryW * normalWDiffuse * exp(-diffuseLuminanceW);
                wDiffuse = isCenter ? kernel : wDiffuse;
                wDiffuse *= CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMaterialMask);

                sumWDiffuse += wDiffuse;
                sumDiffuseIlluminationAnd2ndMoment += wDiffuse * sampleDiffuseIlluminationAnd2ndMoment;
                #ifdef RELAX_SH
                    sumDiffuseSH1 += wDiffuse * sharedDiffuseSH1[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                #endif
#endif
            }
        }
#ifdef RELAX_SPECULAR
        sumWSpecular = max(sumWSpecular, 1e-6f);
        sumSpecularIlluminationAnd2ndMoment /= sumWSpecular;
        float specular1stMoment = STL::Color::Luminance(sumSpecularIlluminationAnd2ndMoment.rgb);
        float specular2ndMoment = sumSpecularIlluminationAnd2ndMoment.a;
        float specularVariance = max(0, specular2ndMoment - specular1stMoment * specular1stMoment);
        float4 filteredSpecularIlluminationAndVariance = float4(sumSpecularIlluminationAnd2ndMoment.rgb, specularVariance);
        gOutSpecularIlluminationAndVariance[pixelPos] = filteredSpecularIlluminationAndVariance;
        #ifdef RELAX_SH
            gOutSpecularSH1[pixelPos] = float4(sumSpecularSH1.rgb / sumWSpecular, roughnessModified);
        #endif
#endif
#ifdef RELAX_DIFFUSE
        sumWDiffuse = max(sumWDiffuse, 1e-6f);
        sumDiffuseIlluminationAnd2ndMoment /= sumWDiffuse;
        float diffuse1stMoment = STL::Color::Luminance(sumDiffuseIlluminationAnd2ndMoment.rgb);
        float diffuse2ndMoment = sumDiffuseIlluminationAnd2ndMoment.a;
        float diffuseVariance = max(0, diffuse2ndMoment - diffuse1stMoment * diffuse1stMoment);
        float4 filteredDiffuseIlluminationAndVariance = float4(sumDiffuseIlluminationAnd2ndMoment.rgb, diffuseVariance);
        gOutDiffuseIlluminationAndVariance[pixelPos] = filteredDiffuseIlluminationAndVariance;
        #ifdef RELAX_SH
            gOutDiffuseSH1[pixelPos] = sumDiffuseSH1 / sumWDiffuse;
        #endif
#endif
    }
    else
    // Running spatial variance estimation
    {
#ifdef RELAX_SPECULAR
        float sumWSpecularIllumination = 0;
        float3 sumSpecularIllumination = 0;
        float sumSpecular1stMoment = 0;
        float sumSpecular2ndMoment = 0;
        #ifdef RELAX_SH
            float4 sumSpecularSH1 = 0;
        #endif
#endif

#ifdef RELAX_DIFFUSE
        float sumWDiffuseIllumination = 0;
        float3 sumDiffuseIllumination = 0;
        float sumDiffuse1stMoment = 0;
        float sumDiffuse2ndMoment = 0;
        #ifdef RELAX_SH
            float4 sumDiffuseSH1 = 0;
        #endif
#endif

        // Normal weight is same for diffuse and specular during spatial variance estimation
        float diffuseNormalWeightParams = GetNormalWeightParams(1.0, gDiffLobeAngleFraction);

        // Compute first and second moment spatially. This code also applies cross-bilateral
        // filtering on the input illumination.
        [unroll]
        for (int cx = -2; cx <= 2; cx++)
        {
            [unroll]
            for (int cy = -2; cy <= 2; cy++)
            {
                int2 sharedMemoryIndexP = sharedMemoryIndex + int2(cx, cy);

                float3 sampleNormal = sharedNormalRoughness[sharedMemoryIndexP.y][sharedMemoryIndexP.x].rgb;
                float sampleMaterialID = sharedWorldPosMaterialID[sharedMemoryIndexP.y][sharedMemoryIndexP.x].a;

                // Calculating weights
                float depthW = 1.0;// TODO: should we take in account depth here?
                float angle = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
                float normalW = ComputeWeight(angle, diffuseNormalWeightParams, 0.0);

#ifdef RELAX_SPECULAR
                float4 sampleSpecular = sharedSpecular[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleSpecularIllumination = sampleSpecular.rgb;
                float sampleSpecular1stMoment = STL::Color::Luminance(sampleSpecularIllumination);
                float sampleSpecular2ndMoment = sampleSpecular.a;
                float specularW = normalW * depthW;
                specularW *= CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMaterialMask);

                sumWSpecularIllumination += specularW;
                sumSpecularIllumination += sampleSpecularIllumination.rgb * specularW;
                sumSpecular1stMoment += sampleSpecular1stMoment * specularW;
                sumSpecular2ndMoment += sampleSpecular2ndMoment * specularW;
                #ifdef RELAX_SH
                    sumSpecularSH1 += sharedSpecularSH1[sharedMemoryIndexP.y][sharedMemoryIndexP.x] * specularW;
                #endif
#endif

#ifdef RELAX_DIFFUSE
                float4 sampleDiffuse = sharedDiffuse[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleDiffuseIllumination = sampleDiffuse.rgb;
                float sampleDiffuse1stMoment = STL::Color::Luminance(sampleDiffuseIllumination);
                float sampleDiffuse2ndMoment = sampleDiffuse.a;
                float diffuseW = normalW * depthW;
                diffuseW *= CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMaterialMask);

                sumWDiffuseIllumination += diffuseW;
                sumDiffuseIllumination += sampleDiffuseIllumination.rgb * diffuseW;
                sumDiffuse1stMoment += sampleDiffuse1stMoment * diffuseW;
                sumDiffuse2ndMoment += sampleDiffuse2ndMoment * diffuseW;
                #ifdef RELAX_SH
                    sumDiffuseSH1 += sharedDiffuseSH1[sharedMemoryIndexP.y][sharedMemoryIndexP.x] * diffuseW;
                #endif
#endif
            }
        }

        float boost = max(1.0, 4.0 / (historyLength + 1.0));

#ifdef RELAX_SPECULAR
        sumWSpecularIllumination = max(sumWSpecularIllumination, 1e-6f);
        sumSpecularIllumination /= sumWSpecularIllumination;
        sumSpecular1stMoment /= sumWSpecularIllumination;
        sumSpecular2ndMoment /= sumWSpecularIllumination;
        float specularVariance = max(0, sumSpecular2ndMoment - sumSpecular1stMoment * sumSpecular1stMoment);
        specularVariance *= boost;
        gOutSpecularIlluminationAndVariance[pixelPos] = float4(sumSpecularIllumination, specularVariance);
        #ifdef RELAX_SH
            float roughnessModified = sharedSpecularSH1[sharedMemoryIndex.y][sharedMemoryIndex.x].w;
            gOutSpecularSH1[pixelPos] = float4(sumSpecularSH1.rgb / sumWSpecularIllumination, roughnessModified);
        #endif
#endif

#ifdef RELAX_DIFFUSE
        sumWDiffuseIllumination = max(sumWDiffuseIllumination, 1e-6f);
        sumDiffuseIllumination /= sumWDiffuseIllumination;
        sumDiffuse1stMoment /= sumWDiffuseIllumination;
        sumDiffuse2ndMoment /= sumWDiffuseIllumination;
        float diffuseVariance = max(0, sumDiffuse2ndMoment - sumDiffuse1stMoment * sumDiffuse1stMoment);
        diffuseVariance *= boost;
        gOutDiffuseIlluminationAndVariance[pixelPos] = float4(sumDiffuseIllumination, diffuseVariance);
        #ifdef RELAX_SH
            gOutDiffuseSH1[pixelPos] = sumDiffuseSH1 / sumWDiffuseIllumination;
        #endif
#endif
    }

}
