/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_AtrousSmemConstants )
    RELAX_SHARED_CONSTANTS
    NRD_CONSTANT( uint, gStepSize )
    NRD_CONSTANT( uint, gIsLastPass )
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gSpecIlluminationAnd2ndMoment, t, 1 )
        NRD_INPUT( Texture2D<float4>, gDiffIlluminationAnd2ndMoment, t, 2 )
        NRD_INPUT( Texture2D<float>, gHistoryLength, t, 3 )
        NRD_INPUT( Texture2D<float>, gSpecReprojectionConfidence, t, 4 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 5 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 6 )
        NRD_INPUT( Texture2D<float>, gSpecConfidence, t, 7 )
        NRD_INPUT( Texture2D<float>, gDiffConfidence, t, 8 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 9 )
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 10 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIlluminationAndVariance, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIlluminationAndVariance, u, 1 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutNormalRoughness, u, 2 )
        NRD_OUTPUT( RWTexture2D<float>, gOutMaterialID, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOutViewZ, u, 4 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 5 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 6 )
        #endif

    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gDiffIlluminationAnd2ndMoment, t, 1 )
        NRD_INPUT( Texture2D<float>, gHistoryLength, t, 2 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 4 )
        NRD_INPUT( Texture2D<float>, gDiffConfidence, t, 5 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 6 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIlluminationAndVariance, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutNormalRoughness, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOutMaterialID, u, 2 )
        NRD_OUTPUT( RWTexture2D<float>, gOutViewZ, u, 3 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 4 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gSpecIlluminationAnd2ndMoment, t, 1 )
        NRD_INPUT( Texture2D<float>, gHistoryLength, t, 2 )
        NRD_INPUT( Texture2D<float>, gSpecReprojectionConfidence, t, 3 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 4 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 5 )
        NRD_INPUT( Texture2D<float>, gSpecConfidence, t, 6 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 7 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIlluminationAndVariance, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutNormalRoughness, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOutMaterialID, u, 2 )
        NRD_OUTPUT( RWTexture2D<float>, gOutViewZ, u, 3 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 4 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_AtrousSmemGroupX 8
#define RELAX_AtrousSmemGroupY 8

#define NRD_USE_BORDER_2

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_AtrousSmemGroupX
#define GROUP_Y RELAX_AtrousSmemGroupY
