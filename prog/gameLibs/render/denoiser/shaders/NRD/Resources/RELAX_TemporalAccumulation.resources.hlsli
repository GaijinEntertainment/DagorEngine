/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_TemporalAccumulationConstants )
    RELAX_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gSpecIllumination, t, 1 )
        NRD_INPUT( Texture2D<float4>, gDiffIllumination, t, 2 )
        NRD_INPUT( Texture2D<float3>, gMv, t, 3 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 4 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 5 )
        NRD_INPUT( Texture2D<float4>, gPrevSpecularIlluminationResponsive, t, 6 )
        NRD_INPUT( Texture2D<float4>, gPrevDiffuseIlluminationResponsive, t, 7 )
        NRD_INPUT( Texture2D<float4>, gPrevSpecularIllumination, t, 8 )
        NRD_INPUT( Texture2D<float4>, gPrevDiffuseIllumination, t, 9 )
        NRD_INPUT( Texture2D<float4>, gPrevNormalRoughness, t, 10 )
        NRD_INPUT( Texture2D<float>, gPrevViewZ, t, 11 )
        NRD_INPUT( Texture2D<float>, gPrevReflectionHitT, t, 12 )
        NRD_INPUT( Texture2D<float>, gPrevHistoryLength, t, 13 )
        NRD_INPUT( Texture2D<float>, gPrevMaterialID, t, 14 )
        NRD_INPUT( Texture2D<float>, gSpecConfidence, t, 15 )
        NRD_INPUT( Texture2D<float>, gDiffConfidence, t, 16 )
        NRD_INPUT( Texture2D<float>, gDisocclusionThresholdMix, t, 17 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 18 )
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 19 )
            NRD_INPUT( Texture2D<float4>, gPrevSpecularResponsiveSH1, t, 20 )
            NRD_INPUT( Texture2D<float4>, gPrevDiffuseResponsiveSH1, t, 21 )
            NRD_INPUT( Texture2D<float4>, gPrevSpecularSH1, t, 22 )
            NRD_INPUT( Texture2D<float4>, gPrevDiffuseSH1, t, 23 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIllumination, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIllumination, u, 1 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIlluminationResponsive, u, 2 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIlluminationResponsive, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOutReflectionHitT, u, 4 )
        NRD_OUTPUT( RWTexture2D<float>, gOutHistoryLength, u, 5 )
        NRD_OUTPUT( RWTexture2D<float>, gOutSpecularReprojectionConfidence, u, 6 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 7 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 8 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularResponsiveSH1, u, 9 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseResponsiveSH1, u, 10 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gDiffIllumination, t, 1 )
        NRD_INPUT( Texture2D<float3>, gMv, t, 2 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gPrevDiffuseIlluminationResponsive, t, 5 )
        NRD_INPUT( Texture2D<float4>, gPrevDiffuseIllumination, t, 6 )
        NRD_INPUT( Texture2D<float4>, gPrevNormalRoughness, t, 7 )
        NRD_INPUT( Texture2D<float>, gPrevViewZ, t, 8 )
        NRD_INPUT( Texture2D<float>, gPrevHistoryLength, t, 9 )
        NRD_INPUT( Texture2D<float>, gPrevMaterialID, t, 10 )
        NRD_INPUT( Texture2D<float>, gDiffConfidence, t, 11 )
        NRD_INPUT( Texture2D<float>, gDisocclusionThresholdMix, t, 12)
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 13 )
            NRD_INPUT( Texture2D<float4>, gPrevDiffuseResponsiveSH1, t, 14 )
            NRD_INPUT( Texture2D<float4>, gPrevDiffuseSH1, t, 15 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIllumination, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIlluminationResponsive, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOutHistoryLength, u, 2 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 3 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseResponsiveSH1, u, 4 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gSpecIllumination, t, 1 )
        NRD_INPUT( Texture2D<float3>, gMv, t, 2 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gPrevSpecularIlluminationResponsive, t, 5 )
        NRD_INPUT( Texture2D<float4>, gPrevSpecularIllumination, t, 6 )
        NRD_INPUT( Texture2D<float4>, gPrevNormalRoughness, t, 7 )
        NRD_INPUT( Texture2D<float>, gPrevViewZ, t, 8 )
        NRD_INPUT( Texture2D<float>, gPrevReflectionHitT, t, 9 )
        NRD_INPUT( Texture2D<float>, gPrevHistoryLength, t, 10 )
        NRD_INPUT( Texture2D<float>, gPrevMaterialID, t, 11 )
        NRD_INPUT( Texture2D<float>, gSpecConfidence, t, 12 )
        NRD_INPUT( Texture2D<float>, gDisocclusionThresholdMix, t, 13 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 14 )
            NRD_INPUT( Texture2D<float4>, gPrevSpecularResponsiveSH1, t, 15 )
            NRD_INPUT( Texture2D<float4>, gPrevSpecularSH1, t, 16 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIllumination, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIlluminationResponsive, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOutReflectionHitT, u, 2 )
        NRD_OUTPUT( RWTexture2D<float>, gOutHistoryLength, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOutSpecularReprojectionConfidence, u, 4 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 5 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularResponsiveSH1, u, 6 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_TemporalAccumulationGroupX 8
#define RELAX_TemporalAccumulationGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_TemporalAccumulationGroupX
#define GROUP_Y RELAX_TemporalAccumulationGroupY
