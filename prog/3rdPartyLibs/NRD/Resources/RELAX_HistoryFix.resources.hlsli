/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_HistoryFixConstants )
    RELAX_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Spec, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff, t, 2 )
        NRD_INPUT( Texture2D<float>,  gIn_HistoryLength, t, 3 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 4 )
        NRD_INPUT( Texture2D<float>,  gIn_ViewZ, t, 5 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 6 )
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 7 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff, u, 1 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 2 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 3 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 4 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 5 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff, u, 0 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 1 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Spec, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 4 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 5 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec, u, 0 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 1 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_HistoryFixGroupX 8
#define RELAX_HistoryFixGroupY 8

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_HistoryFixGroupX
#define GROUP_Y RELAX_HistoryFixGroupY
