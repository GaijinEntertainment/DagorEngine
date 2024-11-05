/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN(uint2 pixelPos : SV_DispatchThreadId)
{
    // Tile-based early out
    float isSky = gTiles[pixelPos >> 4];
    if (isSky != 0.0 || pixelPos.x >= gRectSize.x || pixelPos.y >= gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    float centerViewZ = abs(gViewZ[pixelPos]);
    if (centerViewZ > gDenoisingRange)
        return;

    float centerMaterialID;
    float4 centerNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[pixelPos], centerMaterialID);
    float3 centerNormal = centerNormalRoughness.rgb;
    float centerRoughness = centerNormalRoughness.a;
    float historyLength = 255.0 * gHistoryLength[pixelPos];

    // Diffuse normal weight is used for diffuse and can be used for specular depending on settings.
    // Weight strictness is higher as the Atrous step size increases.
    float diffuseLobeAngleFraction = gDiffLobeAngleFraction / sqrt(gStepSize);
    #ifdef RELAX_SH
        diffuseLobeAngleFraction = 1.0 / sqrt(gStepSize);
    #endif
    diffuseLobeAngleFraction = lerp(0.99, diffuseLobeAngleFraction, saturate(historyLength / 5.0));

#ifdef RELAX_SPECULAR
    float4 centerSpecularIlluminationAndVariance = gSpecIlluminationAndVariance[pixelPos];
    float centerSpecularLuminance = STL::Color::Luminance(centerSpecularIlluminationAndVariance.rgb);
    float centerSpecularVar = centerSpecularIlluminationAndVariance.a;

    float specularReprojectionConfidence = gSpecReprojectionConfidence[pixelPos];
    float specularLuminanceWeightRelaxation = 1.0;
    if (gStepSize <= 4)
        specularLuminanceWeightRelaxation = lerp(1.0, specularReprojectionConfidence, gLuminanceEdgeStoppingRelaxation);

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

    float sumWSpecular = 0.44198 * 0.44198;
    float4 sumSpecularIlluminationAndVariance = centerSpecularIlluminationAndVariance * float4(sumWSpecular.xxx, sumWSpecular * sumWSpecular);
    #ifdef RELAX_SH
        float4 centerSpecularSH1 = gSpecSH1[pixelPos];
        float4 sumSpecularSH1 = centerSpecularSH1 * sumWSpecular;
        float roughnessModified = centerSpecularSH1.w;
    #endif
#endif

#ifdef RELAX_DIFFUSE
    float4 centerDiffuseIlluminationAndVariance = gDiffIlluminationAndVariance[pixelPos];
    float centerDiffuseLuminance = STL::Color::Luminance(centerDiffuseIlluminationAndVariance.rgb);
    float centerDiffuseVar = centerDiffuseIlluminationAndVariance.a;
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

    float sumWDiffuse = 0.44198 * 0.44198;
    float4 sumDiffuseIlluminationAndVariance = centerDiffuseIlluminationAndVariance * float4(sumWDiffuse.xxx, sumWDiffuse * sumWDiffuse);
    #ifdef RELAX_SH
        float4 centerDiffuseSH1 = gDiffSH1[pixelPos];
        float4 sumDiffuseSH1 = centerDiffuseSH1 * sumWDiffuse;
    #endif
#endif

    float3 centerWorldPos = GetCurrentWorldPosFromPixelPos(pixelPos, centerViewZ);
    float3 centerV = -normalize(centerWorldPos);
    static const float kernelWeightGaussian3x3[2] = { 0.44198, 0.27901 };
    float depthThreshold = gDepthThreshold * (gOrthoMode == 0 ? centerViewZ : 1.0);

    // Adding random offsets to minimize "ringing" at large A-Trous steps
    int2 offset = 0;
    if (gStepSize > 4)
    {
        STL::Rng::Hash::Initialize(pixelPos, gFrameIndex);
        offset = int2(gStepSize.xx * 0.5 * (STL::Rng::Hash::GetFloat2() - 0.5));
    }

    [unroll]
    for (int yy = -1; yy <= 1; yy++)
    {
        [unroll]
        for (int xx = -1; xx <= 1; xx++)
        {
            int2 p = pixelPos + offset + int2(xx, yy) * gStepSize;
            bool isCenter = ((xx == 0) && (yy == 0));
            if (isCenter)
                continue;

            bool isInside = all(p >= int2(0, 0)) && all(p < int2(gRectSize));
            float kernel = kernelWeightGaussian3x3[abs(xx)] * kernelWeightGaussian3x3[abs(yy)];

            // Fetching normal, roughness, linear Z
            float sampleMaterialID;
            float4 sampleNormalRoughnes = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[p], sampleMaterialID);
            float3 sampleNormal = sampleNormalRoughnes.rgb;
            float sampleRoughness = sampleNormalRoughnes.a;
            float sampleViewZ = abs(gViewZ[p]);

            // Calculating sample world position
            float3 sampleWorldPos = GetCurrentWorldPosFromPixelPos(p, sampleViewZ);

            // Calculating geometry weight for diffuse and specular
            float geometryW = GetPlaneDistanceWeight_Atrous(centerWorldPos, centerNormal, sampleWorldPos, depthThreshold);
            geometryW *= kernel;
            geometryW *= float(isInside && sampleViewZ < gDenoisingRange);

#ifdef RELAX_SPECULAR
            // Getting sample view vector closer to center view vector
            // by adding gRoughnessEdgeStoppingRelaxation * centerWorldPos
            // relaxes view direction based rejection
            float3 sampleV = -normalize(sampleWorldPos + gRoughnessEdgeStoppingRelaxation * centerWorldPos);

            // Calculating weights for specular
            float angles = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
            float normalWSpecularSimplified = ComputeWeight(angles, specularNormalWeightParamsSimplified, 0.0);
            float normalWSpecular = GetSpecularNormalWeight_ATrous(specularNormalWeightParams, centerNormal, sampleNormal, centerV, sampleV);
            float roughnessWSpecular = ComputeWeight(sampleRoughness, roughnessWeightParams.x, roughnessWeightParams.y);

            // Summing up specular
            float wSpecular = geometryW * (gRoughnessEdgeStoppingEnabled ? (normalWSpecular * roughnessWSpecular) : normalWSpecularSimplified);
            wSpecular *= CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMaterialMask);
            if (wSpecular > 1e-4)
            {
                float4 sampleSpecularIlluminationAndVariance = gSpecIlluminationAndVariance[p];
                float sampleSpecularLuminance = STL::Color::Luminance(sampleSpecularIlluminationAndVariance.rgb);

                float specularLuminanceW = abs(centerSpecularLuminance - sampleSpecularLuminance) * specularPhiLIlluminationInv;
                specularLuminanceW = min(gSpecMaxLuminanceRelativeDifference, specularLuminanceW);
                specularLuminanceW *= specularLuminanceWeightRelaxation;
                wSpecular *= exp(-specularLuminanceW);

                sumWSpecular += wSpecular;
                sumSpecularIlluminationAndVariance += float4(wSpecular.xxx, wSpecular * wSpecular) * sampleSpecularIlluminationAndVariance;
                #ifdef RELAX_SH
                    sumSpecularSH1 += gSpecSH1[p] * wSpecular;
                #endif
            }
#endif

#ifdef RELAX_DIFFUSE
            // Calculating weights for diffuse
            float angled = STL::Math::AcosApprox(dot(centerNormal, sampleNormal));
            float normalWDiffuse = ComputeWeight(angled, diffuseNormalWeightParams, 0.0);

            // Summing up diffuse
            float wDiffuse = geometryW * normalWDiffuse;
            wDiffuse *= CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMaterialMask);
            if (wDiffuse > 1e-4)
            {
                float4 sampleDiffuseIlluminationAndVariance = gDiffIlluminationAndVariance[p];
                float sampleDiffuseLuminance = STL::Color::Luminance(sampleDiffuseIlluminationAndVariance.rgb);

                float diffuseLuminanceW = abs(centerDiffuseLuminance - sampleDiffuseLuminance) * diffusePhiLIlluminationInv;
                diffuseLuminanceW = min(gDiffMaxLuminanceRelativeDifference, diffuseLuminanceW);
                if (gUseConfidenceInputs != 0)
                    diffuseLuminanceW *= diffuseLuminanceWeightRelaxation;
                wDiffuse *= exp(-diffuseLuminanceW);

                sumWDiffuse += wDiffuse;
                sumDiffuseIlluminationAndVariance +=  float4(wDiffuse.xxx, wDiffuse * wDiffuse) * sampleDiffuseIlluminationAndVariance;
                #ifdef RELAX_SH
                    sumDiffuseSH1 += gDiffSH1[p] * wDiffuse;
                #endif
            }
#endif
        }
    }

#ifdef RELAX_SPECULAR
    float4 filteredSpecularIlluminationAndVariance = float4(sumSpecularIlluminationAndVariance / float4(sumWSpecular.xxx, sumWSpecular * sumWSpecular));
    #ifdef RELAX_SH
        // Luminance output is expected in YCoCg color space in SH mode, converting to YCoCg in last A-Trous pass
        if (gIsLastPass == 1)
            filteredSpecularIlluminationAndVariance.rgb = _NRD_LinearToYCoCg(filteredSpecularIlluminationAndVariance.rgb);
        gOutSpecularSH1[pixelPos] = float4(sumSpecularSH1.rgb / sumWSpecular, roughnessModified);
    #endif
    gOutSpecularIlluminationAndVariance[pixelPos] = filteredSpecularIlluminationAndVariance;
#endif

#ifdef RELAX_DIFFUSE
    float4 filteredDiffuseIlluminationAndVariance = float4(sumDiffuseIlluminationAndVariance / float4(sumWDiffuse.xxx, sumWDiffuse * sumWDiffuse));
    #ifdef RELAX_SH
        // Luminance output is expected in YCoCg color space in SH mode, converting to YCoCg in last A-Trous pass
        if (gIsLastPass == 1)
            filteredDiffuseIlluminationAndVariance.rgb = _NRD_LinearToYCoCg(filteredDiffuseIlluminationAndVariance.rgb);
        gOutDiffuseSH1[pixelPos] = sumDiffuseSH1 / sumWDiffuse;
    #endif
    gOutDiffuseIlluminationAndVariance[pixelPos] = filteredDiffuseIlluminationAndVariance;
#endif
}
