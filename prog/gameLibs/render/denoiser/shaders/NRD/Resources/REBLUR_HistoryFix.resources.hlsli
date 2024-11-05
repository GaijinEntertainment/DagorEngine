/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( REBLUR_HistoryFixConstants )
    REBLUR_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined REBLUR_DIFFUSE && defined REBLUR_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_Data1, t, 2 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 3 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Diff, t, 4 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Spec, t, 5 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gIn_DiffFast, t, 6 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gIn_SpecFast, t, 7 )
        #ifdef REBLUR_SH
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_DiffSh, t, 8 )
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_SpecSh, t, 9 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Diff, u, 0 )
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Spec, u, 1 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_DiffFast, u, 2 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_SpecFast, u, 3 )
        #ifdef REBLUR_SH
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_DiffSh, u, 4 )
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_SpecSh, u, 5 )
        #endif
    NRD_OUTPUTS_END

#elif( defined REBLUR_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_Data1, t, 2 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 3 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Diff, t, 4 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gIn_DiffFast, t, 5 )
        #ifdef REBLUR_SH
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_DiffSh, t, 6 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Diff, u, 0 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_DiffFast, u, 1 )
        #ifdef REBLUR_SH
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_DiffSh, u, 2 )
        #endif
    NRD_OUTPUTS_END

#else

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_Data1, t, 2 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 3 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Spec, t, 4 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gIn_SpecFast, t, 5 )
        #ifdef REBLUR_SH
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_SpecSh, t, 6 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_SpecFast, u, 1 )
        #ifdef REBLUR_SH
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_SpecSh, u, 2 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define REBLUR_HistoryFixGroupX 8
#define REBLUR_HistoryFixGroupY 16

#define NRD_USE_BORDER_2

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X REBLUR_HistoryFixGroupX
#define GROUP_Y REBLUR_HistoryFixGroupY
