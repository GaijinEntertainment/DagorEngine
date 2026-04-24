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

NRD_INPUTS_START
    NRD_INPUT( Texture2D, float, gIn_Tiles, t, 0 )
    NRD_INPUT( Texture2D, float, gIn_ViewZ, t, 1 )
    NRD_INPUT( Texture2D, float, gIn_HistoryLength, t, 2 )
    #if( NRD_DIFF && NRD_SPEC )
        NRD_INPUT( Texture2D, float4, gIn_SpecNoisy, t, 3 )
        NRD_INPUT( Texture2D, float4, gIn_DiffNoisy, t, 4 )
        NRD_INPUT( Texture2D, float4, gIn_Spec, t, 5 )
        NRD_INPUT( Texture2D, float4, gIn_Diff, t, 6 )
        NRD_INPUT( Texture2D, float4, gIn_SpecFast, t, 7 )
        NRD_INPUT( Texture2D, float4, gIn_DiffFast, t, 8 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_SpecSh, t, 9 )
            NRD_INPUT( Texture2D, float4, gIn_DiffSh, t, 10 )
            NRD_INPUT( Texture2D, float4, gIn_SpecShFast, t, 11 )
            NRD_INPUT( Texture2D, float4, gIn_DiffShFast, t, 12 )
        #endif
    #elif( NRD_DIFF )
        NRD_INPUT( Texture2D, float4, gIn_DiffNoisy, t, 3 )
        NRD_INPUT( Texture2D, float4, gIn_Diff, t, 4 )
        NRD_INPUT( Texture2D, float4, gIn_DiffFast, t, 5 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_DiffSh, t, 6 )
            NRD_INPUT( Texture2D, float4, gIn_DiffShFast, t, 7 )
        #endif
    #else
        NRD_INPUT( Texture2D, float4, gIn_SpecNoisy, t, 3 )
        NRD_INPUT( Texture2D, float4, gIn_Spec, t, 4 )
        NRD_INPUT( Texture2D, float4, gIn_SpecFast, t, 5 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_SpecSh, t, 6 )
            NRD_INPUT( Texture2D, float4, gIn_SpecShFast, t, 7 )
        #endif
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D, float, gOut_HistoryLength, u, 0 )
    #if( NRD_DIFF && NRD_SPEC )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec, u, 1 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff, u, 2 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_SpecFast, u, 3 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_DiffFast, u, 4 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecSh, u, 5 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffSh, u, 6 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecShFast, u, 7 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffShFast, u, 8 )
        #endif
    #elif( NRD_DIFF )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff, u, 1 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_DiffFast, u, 2 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffSh, u, 3 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffShFast, u, 4 )
        #endif
    #else
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec, u, 1 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_SpecFast, u, 2 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecSh, u, 3 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecShFast, u, 4 )
        #endif
    #endif
NRD_OUTPUTS_END

// Macro magic
#define RELAX_HistoryClampingGroupX 8
#define RELAX_HistoryClampingGroupY 8

// Shader only
#ifndef __cplusplus

#define NRD_BORDER 2

#define GROUP_X RELAX_HistoryClampingGroupX
#define GROUP_Y RELAX_HistoryClampingGroupY

#endif
