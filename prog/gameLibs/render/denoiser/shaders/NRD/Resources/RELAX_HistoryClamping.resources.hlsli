/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_HistoryClampingConstants )
    RELAX_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gNoisySpecularIllumination, t, 1 )
        NRD_INPUT( Texture2D<float4>, gNoisyDiffuseIllumination, t, 2 )
        NRD_INPUT( Texture2D<float4>, gSpecIllumination, t, 3 )
        NRD_INPUT( Texture2D<float4>, gDiffIllumination, t, 4 )
        NRD_INPUT( Texture2D<float4>, gSpecIlluminationResponsive, t, 5 )
        NRD_INPUT( Texture2D<float4>, gDiffIlluminationResponsive, t, 6 )
        NRD_INPUT( Texture2D<float>, gHistoryLength, t, 7 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 8 )
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 9 )
            NRD_INPUT( Texture2D<float4>, gSpecResponsiveSH1, t, 10 )
            NRD_INPUT( Texture2D<float4>, gDiffResponsiveSH1, t, 11 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIllumination, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIllumination, u, 1 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIlluminationResponsive, u, 2 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIlluminationResponsive, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOutHistoryLength, u, 4 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 5 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 6 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularResponsiveSH1, u, 7 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseResponsiveSH1, u, 8 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gNoisyDiffuseIllumination, t, 1 )
        NRD_INPUT( Texture2D<float4>, gDiffIllumination, t, 2 )
        NRD_INPUT( Texture2D<float4>, gDiffIlluminationResponsive, t, 3 )
        NRD_INPUT( Texture2D<float>, gHistoryLength, t, 4 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 5 )
            NRD_INPUT( Texture2D<float4>, gDiffResponsiveSH1, t, 6 )
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
        NRD_INPUT( Texture2D<float4>, gNoisySpecularIllumination, t, 1 )
        NRD_INPUT( Texture2D<float4>, gSpecIllumination, t, 2 )
        NRD_INPUT( Texture2D<float4>, gSpecIlluminationResponsive, t, 3 )
        NRD_INPUT( Texture2D<float>, gHistoryLength, t, 4 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 5 )
            NRD_INPUT( Texture2D<float4>, gSpecResponsiveSH1, t, 6 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIllumination, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIlluminationResponsive, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOutHistoryLength, u, 2 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 3 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularResponsiveSH1, u, 4 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_HistoryClampingGroupX 8
#define RELAX_HistoryClampingGroupY 8

#define NRD_USE_BORDER_2

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_HistoryClampingGroupX
#define GROUP_Y RELAX_HistoryClampingGroupY

