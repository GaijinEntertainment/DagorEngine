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
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_SpecNoisy, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_DiffNoisy, t, 3 )
        NRD_INPUT( Texture2D<float4>, gIn_Spec, t, 4 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff, t, 5 )
        NRD_INPUT( Texture2D<float4>, gIn_SpecFast, t, 6 )
        NRD_INPUT( Texture2D<float4>, gIn_DiffFast, t, 7 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 8 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 9 )
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 10 )
            NRD_INPUT( Texture2D<float4>, gIn_SpecShFast, t, 11 )
            NRD_INPUT( Texture2D<float4>, gIn_DiffShFast, t, 12 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff, u, 1 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecFast, u, 2 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffFast, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_HistoryLength, u, 4 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 5 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 6 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecShFast, u, 7 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffShFast, u, 8 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_DiffNoisy, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff, t, 3 )
        NRD_INPUT( Texture2D<float4>, gIn_DiffFast, t, 4 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 5 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 6 )
            NRD_INPUT( Texture2D<float4>, gIn_DiffShFast, t, 7 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffFast, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_HistoryLength, u, 2 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 3 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffShFast, u, 4 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_SpecNoisy, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Spec, t, 3 )
        NRD_INPUT( Texture2D<float4>, gIn_SpecFast, t, 4 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 5 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 6 )
            NRD_INPUT( Texture2D<float4>, gIn_SpecShFast, t, 7 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecFast, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_HistoryLength, u, 2 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 3 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecShFast, u, 4 )
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

