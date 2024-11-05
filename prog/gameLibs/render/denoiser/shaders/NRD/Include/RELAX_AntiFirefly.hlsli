/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifdef RELAX_SPECULAR
    groupshared float4 sharedSpecular[BUFFER_Y][BUFFER_X];
#endif

#ifdef RELAX_DIFFUSE
    groupshared float4 sharedDiffuse[BUFFER_Y][BUFFER_X];
#endif

    groupshared float sharedMaterialID[BUFFER_Y][BUFFER_X];

// Helper functions
void Preload(uint2 sharedPos, int2 globalPos)
{
    globalPos = clamp(globalPos, 0, gRectSize - 1.0);

    float materialID;
    NRD_FrontEnd_UnpackNormalAndRoughness(gNormalRoughness[globalPos], materialID);
    sharedMaterialID[sharedPos.y][sharedPos.x] = materialID;

#ifdef RELAX_SPECULAR
    sharedSpecular[sharedPos.y][sharedPos.x] = gSpecIllumination[globalPos];
#endif

#ifdef RELAX_DIFFUSE
    sharedDiffuse[sharedPos.y][sharedPos.x] = gDiffIllumination[globalPos];
#endif
}

// Cross bilateral Rank-Conditioned Rank-Selection (RCRS) filter
void runRCRS(
    int2 pixelPos,
    int2 threadPos
#ifdef RELAX_SPECULAR
    ,out float4 outSpecular
#endif
#ifdef RELAX_DIFFUSE
    ,out float4 outDiffuse
#endif
    )
{
    // Fetching center data
    uint2 sharedMemoryIndex = threadPos + int2(BORDER, BORDER);
    float centerMaterialID = sharedMaterialID[sharedMemoryIndex.y][sharedMemoryIndex.x];

#ifdef RELAX_SPECULAR
    float4 s = sharedSpecular[sharedMemoryIndex.y][sharedMemoryIndex.x];
    float3 specularIlluminationCenter = s.rgb;
    float specular2ndMomentCenter = s.a;
    float specularLuminanceCenter = STL::Color::Luminance(specularIlluminationCenter);

    float maxSpecularLuminance = -1.0;
    float minSpecularLuminance = 1.0e6;
    int2 maxSpecularLuminanceCoords = sharedMemoryIndex;
    int2 minSpecularLuminanceCoords = sharedMemoryIndex;
#endif

#ifdef RELAX_DIFFUSE
    float4 d = sharedDiffuse[sharedMemoryIndex.y][sharedMemoryIndex.x];
    float3 diffuseIlluminationCenter = d.rgb;
    float diffuse2ndMomentCenter = d.a;
    float diffuseLuminanceCenter = STL::Color::Luminance(diffuseIlluminationCenter);

    float maxDiffuseLuminance = -1.0;
    float minDiffuseLuminance = 1.0e6;
    int2 maxDiffuseLuminanceCoords = sharedMemoryIndex;
    int2 minDiffuseLuminanceCoords = sharedMemoryIndex;
#endif

    [unroll]
    for (int yy = -1; yy <= 1; yy++)
    {
        [unroll]
        for (int xx = -1; xx <= 1; xx++)
        {
            int2 p = pixelPos + int2(xx, yy);
            int2 sharedMemoryIndexSample = threadPos + int2(BORDER, BORDER) + int2(xx,yy);

            if ((xx == 0) && (yy == 0))
                continue;

            if (any(p < int2(0, 0)) || any(p >= (int2)gRectSize))
                continue;

            // Fetching sample data
#ifdef RELAX_SPECULAR
            float3 specularIlluminationSample = sharedSpecular[sharedMemoryIndexSample.y][sharedMemoryIndexSample.x].rgb;
            float specularLuminanceSample = STL::Color::Luminance(specularIlluminationSample);
#endif

#ifdef RELAX_DIFFUSE
            float3 diffuseIlluminationSample = sharedDiffuse[sharedMemoryIndexSample.y][sharedMemoryIndexSample.x].rgb;
            float diffuseLuminanceSample = STL::Color::Luminance(diffuseIlluminationSample);
#endif

            float sampleMaterialID = sharedMaterialID[sharedMemoryIndexSample.y][sharedMemoryIndexSample.x];

#ifdef RELAX_SPECULAR
            if (CompareMaterials(sampleMaterialID, centerMaterialID, gSpecMaterialMask) > 0)
            {
                if (specularLuminanceSample > maxSpecularLuminance)
                {
                    maxSpecularLuminance = specularLuminanceSample;
                    maxSpecularLuminanceCoords = sharedMemoryIndexSample;
                }
                if (specularLuminanceSample < minSpecularLuminance)
                {
                    minSpecularLuminance = specularLuminanceSample;
                    minSpecularLuminanceCoords = sharedMemoryIndexSample;
                }
            }
#endif

#ifdef RELAX_DIFFUSE
            if (CompareMaterials(sampleMaterialID, centerMaterialID, gDiffMaterialMask) > 0)
            {
                if (diffuseLuminanceSample > maxDiffuseLuminance)
                {
                    maxDiffuseLuminance = diffuseLuminanceSample;
                    maxDiffuseLuminanceCoords = sharedMemoryIndexSample;
                }
                if (diffuseLuminanceSample < minDiffuseLuminance)
                {
                    minDiffuseLuminance = diffuseLuminanceSample;
                    minDiffuseLuminanceCoords = sharedMemoryIndexSample;
                }
            }
#endif
        }
    }

    // Replacing current value with min or max in the neighborhood if outside min..max range,
    // or leaving sample as it is if it's within the range
#ifdef RELAX_SPECULAR
    int2 specularCoords = sharedMemoryIndex;
    if(specularLuminanceCenter > maxSpecularLuminance)
        specularCoords = maxSpecularLuminanceCoords;
    if(specularLuminanceCenter < minSpecularLuminance)
        specularCoords = minSpecularLuminanceCoords;
    outSpecular = float4(sharedSpecular[specularCoords.y][specularCoords.x].rgb, specular2ndMomentCenter);
#endif

#ifdef RELAX_DIFFUSE
    int2 diffuseCoords = sharedMemoryIndex;
    if(diffuseLuminanceCenter > maxDiffuseLuminance)
        diffuseCoords = maxDiffuseLuminanceCoords;
    if(diffuseLuminanceCenter < minDiffuseLuminance)
        diffuseCoords = minDiffuseLuminanceCoords;
    outDiffuse = float4(sharedDiffuse[diffuseCoords.y][diffuseCoords.x].rgb, diffuse2ndMomentCenter);
#endif
}

[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN(uint2 pixelPos : SV_DispatchThreadId, uint2 threadPos : SV_GroupThreadId, uint threadIndex : SV_GroupIndex)
{
    float isSky = gTiles[pixelPos >> 4];
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out
    if (isSky != 0.0 || pixelPos.x >= gRectSize.x || pixelPos.y >= gRectSize.y)
        return;

    // Early out if linearZ is beyond denoising range
    float centerViewZ = abs(gViewZ[pixelPos]);
    if (centerViewZ > gDenoisingRange)
        return;

    // Running firefly filter
#ifdef RELAX_SPECULAR
    float4 outSpecularIlluminationAnd2ndMoment;
#endif

#ifdef RELAX_DIFFUSE
    float4 outDiffuseIlluminationAnd2ndMoment;
#endif

    runRCRS(
        pixelPos.xy,
        threadPos.xy
#ifdef RELAX_SPECULAR
        ,outSpecularIlluminationAnd2ndMoment
#endif
#ifdef RELAX_DIFFUSE
        ,outDiffuseIlluminationAnd2ndMoment
#endif
    );

#ifdef RELAX_SPECULAR
    gOutSpecularIllumination[pixelPos.xy] = outSpecularIlluminationAnd2ndMoment;
#endif

#ifdef RELAX_DIFFUSE
    gOutDiffuseIllumination[pixelPos.xy] = outDiffuseIlluminationAnd2ndMoment;
#endif
}
