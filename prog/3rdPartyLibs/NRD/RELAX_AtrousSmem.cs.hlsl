/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NRD.hlsli"
#include "ml.hlsli"

#include "RELAX_Config.hlsli"
#include "RELAX_AtrousSmem.resources.hlsli"

#include "Common.hlsli"

#include "RELAX_Common.hlsli"

#if( NRD_DIFF )
    groupshared float4 s_Diff[BUFFER_Y][BUFFER_X];
    #if( NRD_MODE == SH )
        groupshared float4 s_DiffSH[BUFFER_Y][BUFFER_X];
    #endif
#endif

#if( NRD_SPEC )
    groupshared float4 s_Spec[BUFFER_Y][BUFFER_X];
    #if( NRD_MODE == SH )
        groupshared float4 s_SpecSH[BUFFER_Y][BUFFER_X];
    #endif
#endif

groupshared float4 s_Normal_Roughness[BUFFER_Y][BUFFER_X];
groupshared float4 s_WorldPos_MaterialID[BUFFER_Y][BUFFER_X];

// Helper functions

// computes a 3x3 gaussian blur of the variance, centered around
// the current pixel
void computeVariance(
    int2 threadPos
#if( NRD_SPEC )
    ,out float specularVariance
#endif
#if( NRD_DIFF )
    ,out float diffuseVariance
#endif
)
{
#if( NRD_SPEC )
    float4 specularSum = 0;
#endif
#if( NRD_DIFF )
    float4 diffuseSum = 0;
#endif

    static const float kernel[2][2] =
    {
        { 1.0 / 4.0, 1.0 / 8.0  },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };

    int2 sharedMemoryIndex = threadPos.xy + int2(NRD_BORDER, NRD_BORDER);
    [unroll]
    for (int dx = -1; dx <= 1; dx++)
    {
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        {
            int2 sharedMemoryIndexP = sharedMemoryIndex + int2(dx,dy);
            float k = kernel[abs(dx)][abs(dy)];
#if( NRD_SPEC )
            float4 specular = s_Spec[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
            specularSum += specular * k;
#endif
#if( NRD_DIFF )
            float4 diffuse = s_Diff[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
            diffuseSum += diffuse * k;
#endif
        }
    }
#if( NRD_SPEC )
    float specular1stMoment = Color::Luminance(specularSum.rgb);
    float specular2ndMoment = specularSum.a;
    specularVariance = max(0, specular2ndMoment - specular1stMoment * specular1stMoment);
#endif
#if( NRD_DIFF )
    float diffuse1stMoment = Color::Luminance(diffuseSum.rgb);
    float diffuse2ndMoment = diffuseSum.a;
    diffuseVariance = max(0, diffuse2ndMoment - diffuse1stMoment * diffuse1stMoment);
#endif
}

void Preload(uint2 sharedPos, int2 globalPos)
{
    globalPos = clamp(globalPos, 0, gRectSize - 1.0);

#if( NRD_SPEC )
    s_Spec[sharedPos.y][sharedPos.x] = gIn_Spec_Variance[globalPos];
    #if( NRD_MODE == SH )
        s_SpecSH[sharedPos.y][sharedPos.x] = gIn_SpecSh[globalPos];
    #endif
#endif

#if( NRD_DIFF )
    s_Diff[sharedPos.y][sharedPos.x] = gIn_Diff_Variance[globalPos];
    #if( NRD_MODE == SH )
        s_DiffSH[sharedPos.y][sharedPos.x] = gIn_DiffSh[globalPos];
    #endif
#endif
    float materialID;
    s_Normal_Roughness[sharedPos.y][sharedPos.x] = NRD_FrontEnd_UnpackNormalAndRoughness(gIn_Normal_Roughness[globalPos], materialID);

    float viewZ = UnpackViewZ(gIn_ViewZ[globalPos]);
    s_WorldPos_MaterialID[sharedPos.y][sharedPos.x] = float4(GetCurrentWorldPosFromPixelPos(globalPos, viewZ), materialID);

}

[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_REVERSED;

    // Preload
    float isSky = gIn_Tiles[pixelPos >> 4];
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Prev ViewZ
    float viewZpacked = gIn_ViewZ[pixelPos];
    gOut_ViewZ[pixelPos] = viewZpacked;

    // Prev normal and roughness
    int2 sharedMemoryIndex = threadPos.xy + int2(NRD_BORDER, NRD_BORDER);
    float4 normalRoughness = s_Normal_Roughness[sharedMemoryIndex.y][sharedMemoryIndex.x];
    float centerViewZ = UnpackViewZ(viewZpacked);
    if (centerViewZ > gDenoisingRange)
    {
        // Setting normal and roughness to close to zero for out of range pixels
        normalRoughness = 1.0 / 255.0;
    }
    gOut_NormalRoughness[pixelPos] = PackPrevNormalRoughness(normalRoughness);

    float4 centerWorldPosMaterialID = s_WorldPos_MaterialID[sharedMemoryIndex.y][sharedMemoryIndex.x];
    float3 centerWorldPos = centerWorldPosMaterialID.xyz;
    float centerMaterialID = centerWorldPosMaterialID.w;

#if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
    gOut_MaterialID[pixelPos] = centerMaterialID / 255.0;
#endif

    // Tile-based early out
    if (isSky != 0.0 || pixelPos.x >= gRectSize.x || pixelPos.y >= gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    if (centerViewZ > gDenoisingRange)
        return;

    float2 pixelUv = ( pixelPos + 0.5 ) * gRectSizeInv;

    float3 centerNormal = normalRoughness.rgb;
    float centerRoughness = normalRoughness.a;

    float historyLength = 255.0 * gIn_HistoryLength[pixelPos];

    [branch]
    if (historyLength >= gHistoryThreshold) // Running Atrous 3x3
    {
        // Calculating variance, filtered using 3x3 gaussin blur
#if( NRD_SPEC )
        float centerSpecularVar;
#endif
#if( NRD_DIFF )
        float centerDiffuseVar;
#endif
        computeVariance(
            threadPos.xy
#if( NRD_SPEC )
            , centerSpecularVar
#endif
#if( NRD_DIFF )
            , centerDiffuseVar
#endif
        );

        // Diffuse normal weight is used for diffuse and can be used for specular depending on settings.
        float diffuseLobeAngleFraction = gLobeAngleFraction;

#if( NRD_SPEC )
        float centerSpecularLuminance = Color::Luminance(s_Spec[sharedMemoryIndex.y][sharedMemoryIndex.x].rgb);
        float specularPhiLIlluminationInv = 1.0 / max(1.0e-4, gSpecPhiLuminance * sqrt(centerSpecularVar));

        float2 roughnessWeightParams = GetRoughnessWeightParams(centerRoughness, gRoughnessFraction);
        float diffuseLobeAngleFractionForSimplifiedSpecularNormalWeight = diffuseLobeAngleFraction;
        float specularLobeAngleFraction = gLobeAngleFraction;

        float specularReprojectionConfidence = gIn_SpecReprojectionConfidence[pixelPos];
        float specularLuminanceWeightRelaxation = lerp(1.0, specularReprojectionConfidence, gLuminanceEdgeStoppingRelaxation);
        if (gHasHistoryConfidence && NRD_SUPPORTS_HISTORY_CONFIDENCE)
        {
            // TODO: confidence is for previous frame, so "prev uv" should be used
            float specConfidenceDrivenRelaxation = saturate(gConfidenceDrivenRelaxationMultiplier * (1.0 - saturate(gIn_SpecConfidence.SampleLevel(gLinearClamp, pixelUv, 0))));

            // Relaxing normal weights for specular
            float r = saturate(specConfidenceDrivenRelaxation * gConfidenceDrivenNormalEdgeStoppingRelaxation);
            diffuseLobeAngleFractionForSimplifiedSpecularNormalWeight = lerp(diffuseLobeAngleFraction, 1.0, r);
            specularLobeAngleFraction = lerp(specularLobeAngleFraction, 1.0, r);

            // Relaxing luminance weight for specular
            r = saturate(specConfidenceDrivenRelaxation * gConfidenceDrivenLuminanceEdgeStoppingRelaxation);
            specularLuminanceWeightRelaxation *= 1.0 - r;
        }

        float specularNormalWeightParamSimplified = GetNormalWeightParam2(1.0, diffuseLobeAngleFractionForSimplifiedSpecularNormalWeight);
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
        #if( NRD_MODE == SH )
            float4 sumSpecularSH = 0;
            float roughnessModified = s_SpecSH[sharedMemoryIndex.y][sharedMemoryIndex.x].w;
        #endif
        float3 centerV = -normalize(centerWorldPos);
#endif

#if( NRD_DIFF )
        float centerDiffuseLuminance = Color::Luminance(s_Diff[sharedMemoryIndex.y][sharedMemoryIndex.x].rgb);
        float diffusePhiLIlluminationInv = 1.0 / max(1.0e-4, gDiffPhiLuminance * sqrt(centerDiffuseVar));

        float diffuseLuminanceWeightRelaxation = 1.0;
        if (gHasHistoryConfidence && NRD_SUPPORTS_HISTORY_CONFIDENCE)
        {
            // TODO: confidence is for previous frame, so "prev uv" should be used
            float diffConfidenceDrivenRelaxation = saturate(gConfidenceDrivenRelaxationMultiplier * (1.0 - saturate(gIn_DiffConfidence.SampleLevel(gLinearClamp, pixelUv, 0))));

            // Relaxing normal weights for diffuse
            float r = saturate(diffConfidenceDrivenRelaxation * gConfidenceDrivenNormalEdgeStoppingRelaxation);
            diffuseLobeAngleFraction = lerp(diffuseLobeAngleFraction, 1.0, r);

            // Relaxing luminance weight for diffuse
            r = saturate(diffConfidenceDrivenRelaxation * gConfidenceDrivenLuminanceEdgeStoppingRelaxation);
            diffuseLuminanceWeightRelaxation = 1.0 - r;
        }
        float diffuseNormalWeightParam = GetNormalWeightParam2(1.0, diffuseLobeAngleFraction);

        float sumWDiffuse = 0;
        float4 sumDiffuseIlluminationAnd2ndMoment = 0;
        #if( NRD_MODE == SH )
            float4 sumDiffuseSH = 0;
        #endif
#endif

        static const float kernelWeightGaussian3x3[2] = { 0.44198, 0.27901 };
        float depthThreshold = gDepthThreshold * (gOrthoMode == 0 ? centerViewZ : 1.0);

        [unroll]
        for (j = -1; j <= 1; j++)
        {
            [unroll]
            for (i = -1; i <= 1; i++)
            {
                const int2 p = pixelPos + int2(i, j);
                const bool isCenter = i == 0 && j == 0;
                const bool isInside = all(p >= 0) && all(p < gRectSize);
                const float kernel = isInside ? kernelWeightGaussian3x3[abs(i)] * kernelWeightGaussian3x3[abs(j)] : 0.0;

                int2 sharedMemoryIndexP = sharedMemoryIndex + int2(i, j);

                float4 sampleNormalRoughness = s_Normal_Roughness[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleNormal = sampleNormalRoughness.rgb;
                float sampleRoughness = sampleNormalRoughness.a;
                float4 sampleWorldPosMaterialID = s_WorldPos_MaterialID[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleWorldPos = sampleWorldPosMaterialID.rgb;
                float sampleMaterialID = sampleWorldPosMaterialID.a;

                // Calculating geometry weight for diffuse and specular
                float geometryW = GetPlaneDistanceWeight_Atrous(
                    centerWorldPos,
                    centerNormal,
                    sampleWorldPos,
                    depthThreshold);

                geometryW *= kernel;

#if( NRD_SPEC )
                // Calculating weights for specular

                // Getting sample view vector closer to center view vector
                // by adding gRoughnessEdgeStoppingRelaxation * centerWorldPos
                // relaxes view direction based rejection
                float angles = Math::AcosApprox(dot(centerNormal, sampleNormal));
                float3 sampleV = -normalize(sampleWorldPos + gRoughnessEdgeStoppingRelaxation * centerWorldPos);
                float normalWSpecularSimplified = ComputeWeight(angles, specularNormalWeightParamSimplified, 0.0);
                float normalWSpecular = GetSpecularNormalWeight_ATrous(specularNormalWeightParams, centerNormal, sampleNormal, centerV, sampleV);

                float roughnessWSpecular = ComputeWeight(sampleRoughness, roughnessWeightParams.x, roughnessWeightParams.y);

                // Summing up specular
                float4 sampleSpecularIlluminationAnd2ndMoment = s_Spec[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float sampleSpecularLuminance = Color::Luminance(sampleSpecularIlluminationAnd2ndMoment.rgb);

                float specularLuminanceW = abs(centerSpecularLuminance - sampleSpecularLuminance) * specularPhiLIlluminationInv;
                specularLuminanceW = min(gSpecMaxLuminanceRelativeDifference, specularLuminanceW);
                specularLuminanceW *= specularLuminanceWeightRelaxation;

                float wSpecular = geometryW * exp(-specularLuminanceW);
                wSpecular *= gRoughnessEdgeStoppingEnabled ? (normalWSpecular * roughnessWSpecular) : normalWSpecularSimplified;
                wSpecular *= CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMinMaterial);
                wSpecular = isCenter ? kernel : wSpecular;

                sumWSpecular += wSpecular;
                sumSpecularIlluminationAnd2ndMoment += wSpecular * sampleSpecularIlluminationAnd2ndMoment;
                #if( NRD_MODE == SH )
                    sumSpecularSH += wSpecular * s_SpecSH[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                #endif
#endif
#if( NRD_DIFF )
                // Calculating weights for diffuse
                float angled = Math::AcosApprox(dot(centerNormal, sampleNormal));
                float normalWDiffuse = ComputeWeight(angled, diffuseNormalWeightParam, 0.0);

                // Summing up diffuse
                float4 sampleDiffuseIlluminationAnd2ndMoment = s_Diff[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float sampleDiffuseLuminance = Color::Luminance(sampleDiffuseIlluminationAnd2ndMoment.rgb);

                float diffuseLuminanceW = abs(centerDiffuseLuminance - sampleDiffuseLuminance) * diffusePhiLIlluminationInv;
                diffuseLuminanceW = min(gDiffMaxLuminanceRelativeDifference, diffuseLuminanceW);
                diffuseLuminanceW *= diffuseLuminanceWeightRelaxation;

                float wDiffuse = geometryW * normalWDiffuse * exp(-diffuseLuminanceW);
                wDiffuse *= CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMinMaterial);
                wDiffuse = isCenter ? kernel : wDiffuse;

                sumWDiffuse += wDiffuse;
                sumDiffuseIlluminationAnd2ndMoment += wDiffuse * sampleDiffuseIlluminationAnd2ndMoment;
                #if( NRD_MODE == SH )
                    sumDiffuseSH += wDiffuse * s_DiffSH[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                #endif
#endif
            }
        }
#if( NRD_SPEC )
        sumWSpecular = max(sumWSpecular, 1e-6f);
        sumSpecularIlluminationAnd2ndMoment /= sumWSpecular;
        float specular1stMoment = Color::Luminance(sumSpecularIlluminationAnd2ndMoment.rgb);
        float specular2ndMoment = sumSpecularIlluminationAnd2ndMoment.a;
        float specularVariance = max(0, specular2ndMoment - specular1stMoment * specular1stMoment);
        float4 filteredSpecularIlluminationAndVariance = float4(sumSpecularIlluminationAnd2ndMoment.rgb, specularVariance);
        gOut_Spec_Variance[pixelPos] = filteredSpecularIlluminationAndVariance;
        #if( NRD_MODE == SH )
            gOut_SpecSh[pixelPos] = float4(sumSpecularSH.rgb / sumWSpecular, roughnessModified);
        #endif
#endif
#if( NRD_DIFF )
        sumWDiffuse = max(sumWDiffuse, 1e-6f);
        sumDiffuseIlluminationAnd2ndMoment /= sumWDiffuse;
        float diffuse1stMoment = Color::Luminance(sumDiffuseIlluminationAnd2ndMoment.rgb);
        float diffuse2ndMoment = sumDiffuseIlluminationAnd2ndMoment.a;
        float diffuseVariance = max(0, diffuse2ndMoment - diffuse1stMoment * diffuse1stMoment);
        float4 filteredDiffuseIlluminationAndVariance = float4(sumDiffuseIlluminationAnd2ndMoment.rgb, diffuseVariance);
        gOut_Diff_Variance[pixelPos] = filteredDiffuseIlluminationAndVariance;
        #if( NRD_MODE == SH )
            gOut_DiffSh[pixelPos] = sumDiffuseSH / sumWDiffuse;
        #endif
#endif
    }
    else
    // Running spatial variance estimation
    {
#if( NRD_SPEC )
        float sumWSpecularIllumination = 0;
        float3 sumSpecularIllumination = 0;
        float sumSpecular1stMoment = 0;
        float sumSpecular2ndMoment = 0;
        #if( NRD_MODE == SH )
            float4 sumSpecularSH = 0;
        #endif
#endif

#if( NRD_DIFF )
        float sumWDiffuseIllumination = 0;
        float3 sumDiffuseIllumination = 0;
        float sumDiffuse1stMoment = 0;
        float sumDiffuse2ndMoment = 0;
        #if( NRD_MODE == SH )
            float4 sumDiffuseSH = 0;
        #endif
#endif

        // Normal weight is same for diffuse and specular during spatial variance estimation
        float diffuseNormalWeightParam = GetNormalWeightParam2(1.0, gLobeAngleFraction);

        // Compute first and second moment spatially. This code also applies cross-bilateral
        // filtering on the input illumination.
        [unroll]
        for (int cx = -2; cx <= 2; cx++)
        {
            [unroll]
            for (int cy = -2; cy <= 2; cy++)
            {
                int2 sharedMemoryIndexP = sharedMemoryIndex + int2(cx, cy);

                float3 sampleNormal = s_Normal_Roughness[sharedMemoryIndexP.y][sharedMemoryIndexP.x].rgb;
                float sampleMaterialID = s_WorldPos_MaterialID[sharedMemoryIndexP.y][sharedMemoryIndexP.x].a;

                // Calculating weights
                float depthW = 1.0;// TODO: should we take in account depth here?
                float angle = Math::AcosApprox(dot(centerNormal, sampleNormal));
                float normalW = ComputeWeight(angle, diffuseNormalWeightParam, 0.0);

#if( NRD_SPEC )
                float4 sampleSpecular = s_Spec[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleSpecularIllumination = sampleSpecular.rgb;
                float sampleSpecular1stMoment = Color::Luminance(sampleSpecularIllumination);
                float sampleSpecular2ndMoment = sampleSpecular.a;
                float specularW = normalW * depthW;
                specularW *= CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMinMaterial);

                sumWSpecularIllumination += specularW;
                sumSpecularIllumination += sampleSpecularIllumination.rgb * specularW;
                sumSpecular1stMoment += sampleSpecular1stMoment * specularW;
                sumSpecular2ndMoment += sampleSpecular2ndMoment * specularW;
                #if( NRD_MODE == SH )
                    sumSpecularSH += s_SpecSH[sharedMemoryIndexP.y][sharedMemoryIndexP.x] * specularW;
                #endif
#endif

#if( NRD_DIFF )
                float4 sampleDiffuse = s_Diff[sharedMemoryIndexP.y][sharedMemoryIndexP.x];
                float3 sampleDiffuseIllumination = sampleDiffuse.rgb;
                float sampleDiffuse1stMoment = Color::Luminance(sampleDiffuseIllumination);
                float sampleDiffuse2ndMoment = sampleDiffuse.a;
                float diffuseW = normalW * depthW;
                diffuseW *= CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMinMaterial);

                sumWDiffuseIllumination += diffuseW;
                sumDiffuseIllumination += sampleDiffuseIllumination.rgb * diffuseW;
                sumDiffuse1stMoment += sampleDiffuse1stMoment * diffuseW;
                sumDiffuse2ndMoment += sampleDiffuse2ndMoment * diffuseW;
                #if( NRD_MODE == SH )
                    sumDiffuseSH += s_DiffSH[sharedMemoryIndexP.y][sharedMemoryIndexP.x] * diffuseW;
                #endif
#endif
            }
        }

        float boost = max(1.0, 4.0 / (historyLength + 1.0));

#if( NRD_SPEC )
        sumWSpecularIllumination = max(sumWSpecularIllumination, 1e-6f);
        sumSpecularIllumination /= sumWSpecularIllumination;
        sumSpecular1stMoment /= sumWSpecularIllumination;
        sumSpecular2ndMoment /= sumWSpecularIllumination;
        float specularVariance = max(0, sumSpecular2ndMoment - sumSpecular1stMoment * sumSpecular1stMoment);
        specularVariance *= boost;
        gOut_Spec_Variance[pixelPos] = float4(sumSpecularIllumination, specularVariance);
        #if( NRD_MODE == SH )
            float roughnessModified = s_SpecSH[sharedMemoryIndex.y][sharedMemoryIndex.x].w;
            gOut_SpecSh[pixelPos] = float4(sumSpecularSH.rgb / sumWSpecularIllumination, roughnessModified);
        #endif
#endif

#if( NRD_DIFF )
        sumWDiffuseIllumination = max(sumWDiffuseIllumination, 1e-6f);
        sumDiffuseIllumination /= sumWDiffuseIllumination;
        sumDiffuse1stMoment /= sumWDiffuseIllumination;
        sumDiffuse2ndMoment /= sumWDiffuseIllumination;
        float diffuseVariance = max(0, sumDiffuse2ndMoment - sumDiffuse1stMoment * sumDiffuse1stMoment);
        diffuseVariance *= boost;
        gOut_Diff_Variance[pixelPos] = float4(sumDiffuseIllumination, diffuseVariance);
        #if( NRD_MODE == SH )
            gOut_DiffSh[pixelPos] = sumDiffuseSH / sumWDiffuseIllumination;
        #endif
#endif
    }

}
