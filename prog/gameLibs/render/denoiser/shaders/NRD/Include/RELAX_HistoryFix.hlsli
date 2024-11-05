/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// Helper functions
float getDiffuseNormalWeight(float3 centerNormal, float3 pointNormal)
{
    return pow(max(0.01, dot(centerNormal, pointNormal)), max(gHistoryFixEdgeStoppingNormalPower, 0.01));
}

float getRadius(float historyLength)
{
    // IMPORTANT: progression is "{8, 4, 2, 1} + 1". "+1" is important to better break blobs
    return exp2(gHistoryFixFrameNum - historyLength) + 1.0;
}

// Main
[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN(uint2 pixelPos : SV_DispatchThreadId)
{
    // Tile-based early out
    float isSky = gTiles[pixelPos >> 4];
    if (isSky != 0.0 || pixelPos.x >= gRectSize.x || pixelPos.y >= gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    // Early out if no disocclusion detected
    float centerViewZ = abs(gViewZ[pixelPos]);
    float historyLength = 255.0 * gHistoryLength[pixelPos];
    if ((centerViewZ > gDenoisingRange) || (historyLength > gHistoryFixFrameNum || gHistoryFixFrameNum == 1.0))
        return;

    // Loading center data
    float centerMaterialID;
    float4 centerNormalRoughness = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[pixelPos], centerMaterialID);
    float3 centerNormal = centerNormalRoughness.rgb;
    float centerRoughness = centerNormalRoughness.a;
    float3 centerWorldPos = GetCurrentWorldPosFromPixelPos(pixelPos, centerViewZ);
    float3 centerV = -normalize(centerWorldPos);
    float depthThreshold = gDepthThreshold * (gOrthoMode == 0 ? centerViewZ : 1.0);

#ifdef RELAX_DIFFUSE
    float4 diffuseIlluminationAnd2ndMomentSum = gDiffIllumination[pixelPos];
    #ifdef RELAX_SH
        float4 diffuseSumSH1 = gDiffSH1[pixelPos];
    #endif
    float diffuseWSum = 1;
#endif
#ifdef RELAX_SPECULAR
    float4 specularIlluminationAnd2ndMomentSum = gSpecIllumination[pixelPos];
    #ifdef RELAX_SH
        float4 specularSumSH1 = gSpecSH1[pixelPos];
        float roughnessModified = specularSumSH1.w;
    #endif
    float specularWSum = 1;
    float2 specularNormalWeightParams =
        GetNormalWeightParams_ATrous(
            centerRoughness,
            5.0, // History length,
            1.0, // Specular reprojection confidence
            0.0, // Normal edge stopping relaxation
            gSpecLobeAngleFraction,
            gSpecLobeAngleSlack);
#endif

    // Running sparse cross-bilateral filter
    float r = getRadius(historyLength);
    [unroll]
    for (int j = -2; j <= 2; j++)
    {
        [unroll]
        for (int i = -2; i <= 2; i++)
        {
            int dx = (int)(i * r);
            int dy = (int)(j * r);

            int2 samplePosInt = (int2)pixelPos + int2(dx, dy);

            bool isInside = all(samplePosInt >= int2(0, 0)) && all(samplePosInt < (int2)gRectSize);
            if ((i == 0) && (j == 0))
                continue;

            float sampleMaterialID;
            float3 sampleNormal = NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[samplePosInt], sampleMaterialID).rgb;

            float sampleViewZ = abs(gViewZ[samplePosInt]);
            float3 sampleWorldPos = GetCurrentWorldPosFromPixelPos(samplePosInt, sampleViewZ);
            float geometryWeight = GetPlaneDistanceWeight_Atrous(
                centerWorldPos,
                centerNormal,
                sampleWorldPos,
                depthThreshold);

#ifdef RELAX_DIFFUSE
            // Summing up diffuse result
            float diffuseW = geometryWeight;
            diffuseW *= getDiffuseNormalWeight(centerNormal, sampleNormal);
            diffuseW = isInside ? diffuseW : 0;
            diffuseW *= CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMaterialMask);

            if (diffuseW > 1e-4)
            {
                float4 sampleDiffuseIlluminationAnd2ndMoment = gDiffIllumination[samplePosInt];
                diffuseIlluminationAnd2ndMomentSum += sampleDiffuseIlluminationAnd2ndMoment * diffuseW;
                #ifdef RELAX_SH
                    float4 sampleDiffuseSH1 = gDiffSH1[samplePosInt];
                    diffuseSumSH1 += sampleDiffuseSH1 * diffuseW;
                #endif
                diffuseWSum += diffuseW;
            }
#endif
#ifdef RELAX_SPECULAR
            // Getting sample view vector closer to center view vector
            // by adding gRoughnessEdgeStoppingRelaxation * centerWorldPos
            // relaxes view direction based rejection
            float3 sampleV = -normalize(sampleWorldPos + gRoughnessEdgeStoppingRelaxation * centerWorldPos);

            // Summing up specular result
            float specularW = geometryWeight;
            specularW *=
                GetSpecularNormalWeight_ATrous(specularNormalWeightParams, centerNormal, sampleNormal, centerV, sampleV);
            specularW = isInside ? specularW : 0;
            specularW *= CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMaterialMask);

            if (specularW > 1e-4)
            {
                float4 sampleSpecularIlluminationAnd2ndMoment = gSpecIllumination[samplePosInt];
                specularIlluminationAnd2ndMomentSum += sampleSpecularIlluminationAnd2ndMoment * specularW;
                #ifdef RELAX_SH
                    float4 sampleSpecularSH1 = gSpecSH1[samplePosInt];
                    specularSumSH1 += sampleSpecularSH1 * specularW;
                #endif
                specularWSum += specularW;
            }
#endif
        }
    }

    // Output buffers will hold the pixels with disocclusion processed by history fix.
    // The next shader will have to copy these areas to normal and responsive history buffers.
#ifdef RELAX_DIFFUSE
    float4 outDiffuseIlluminationAnd2ndMoment = diffuseIlluminationAnd2ndMomentSum / diffuseWSum;
    gOutDiffuseIllumination[pixelPos] = outDiffuseIlluminationAnd2ndMoment;
    #ifdef RELAX_SH
        gOutDiffuseSH1[pixelPos] = diffuseSumSH1 / diffuseWSum;
    #endif
#endif

#ifdef RELAX_SPECULAR
    float4 outSpecularIlluminationAnd2ndMoment = specularIlluminationAnd2ndMomentSum / specularWSum;
    gOutSpecularIllumination[pixelPos] = outSpecularIlluminationAnd2ndMoment;
    #ifdef RELAX_SH
        gOutSpecularSH1[pixelPos] = float4(specularSumSH1.rgb / specularWSum, roughnessModified);
    #endif
#endif
}
