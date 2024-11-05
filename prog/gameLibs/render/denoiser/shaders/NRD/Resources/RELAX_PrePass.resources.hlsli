/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_PrePassConstants )
    RELAX_SHARED_CONSTANTS
    NRD_CONSTANT( float4, gRotator )
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
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 4 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 5 )
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 6 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIllumination, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIllumination, u, 1 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 2 )
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 3 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gDiffIllumination, t, 1 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 2 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 3 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gDiffSH1, t, 4 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseIllumination, u, 0 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutDiffuseSH1, u, 1 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gTiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gSpecIllumination, t, 1 )
        NRD_INPUT( Texture2D<float4>, gNormalRoughness, t, 2 )
        NRD_INPUT( Texture2D<float>, gViewZ, t, 3 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gSpecSH1, t, 4 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularIllumination, u, 0 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOutSpecularSH1, u, 1 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_PrePassGroupX 16
#define RELAX_PrePassGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_PrePassGroupX
#define GROUP_Y RELAX_PrePassGroupY
