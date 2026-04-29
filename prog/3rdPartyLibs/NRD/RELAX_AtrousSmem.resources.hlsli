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

NRD_INPUTS_START
    NRD_INPUT( Texture2D, float, gIn_Tiles, t, 0 )
    NRD_INPUT( Texture2D, float, gIn_HistoryLength, t, 1 )
    NRD_INPUT( Texture2D, float4, gIn_Normal_Roughness, t, 2 )
    NRD_INPUT( Texture2D, float, gIn_ViewZ, t, 3 )
    #if( NRD_DIFF && NRD_SPEC )
        NRD_INPUT( Texture2D, float4, gIn_Spec_Variance, t, 4 )
        NRD_INPUT( Texture2D, float4, gIn_Diff_Variance, t, 5 )
        NRD_INPUT( Texture2D, float, gIn_SpecReprojectionConfidence, t, 6 )
        NRD_INPUT( Texture2D, float, gIn_SpecConfidence, t, 7 )
        NRD_INPUT( Texture2D, float, gIn_DiffConfidence, t, 8 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_SpecSh, t, 9 )
            NRD_INPUT( Texture2D, float4, gIn_DiffSh, t, 10 )
        #endif
    #elif( NRD_DIFF )
        NRD_INPUT( Texture2D, float4, gIn_Diff_Variance, t, 4 )
        NRD_INPUT( Texture2D, float, gIn_DiffConfidence, t, 5 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_DiffSh, t, 6 )
        #endif
    #else
        NRD_INPUT( Texture2D, float4, gIn_Spec_Variance, t, 4 )
        NRD_INPUT( Texture2D, float, gIn_SpecReprojectionConfidence, t, 5 )
        NRD_INPUT( Texture2D, float, gIn_SpecConfidence, t, 6 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_SpecSh, t, 7 )
        #endif
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    #if( NRD_DIFF && NRD_SPEC )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec_Variance, u, 0 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff_Variance, u, 1 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_NormalRoughness, u, 2 )
        NRD_OUTPUT( RWTexture2D, float, gOut_MaterialID, u, 3 )
        NRD_OUTPUT( RWTexture2D, float, gOut_ViewZ, u, 4 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecSh, u, 5 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffSh, u, 6 )
        #endif
    #elif( NRD_DIFF )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff_Variance, u, 0 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_NormalRoughness, u, 1 )
        NRD_OUTPUT( RWTexture2D, float, gOut_MaterialID, u, 2 )
        NRD_OUTPUT( RWTexture2D, float, gOut_ViewZ, u, 3 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffSh, u, 4 )
        #endif
    #else
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec_Variance, u, 0 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_NormalRoughness, u, 1 )
        NRD_OUTPUT( RWTexture2D, float, gOut_MaterialID, u, 2 )
        NRD_OUTPUT( RWTexture2D, float, gOut_ViewZ, u, 3 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecSh, u, 4 )
        #endif
    #endif
NRD_OUTPUTS_END

// Macro magic
#define RELAX_AtrousSmemGroupX 8
#define RELAX_AtrousSmemGroupY 8

// Shader only
#ifndef __cplusplus

#define NRD_BORDER 2

#define GROUP_X RELAX_AtrousSmemGroupX
#define GROUP_Y RELAX_AtrousSmemGroupY

#endif
