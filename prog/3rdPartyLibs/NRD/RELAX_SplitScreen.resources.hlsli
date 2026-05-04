/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_SplitScreenConstants )
    RELAX_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D, float, gIn_ViewZ, t, 0 )
    #if( NRD_DIFF && NRD_SPEC )
        NRD_INPUT( Texture2D, float4, gIn_Diff, t, 1 )
        NRD_INPUT( Texture2D, float4, gIn_Spec, t, 2 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_DiffSh, t, 3 )
            NRD_INPUT( Texture2D, float4, gIn_SpecSh, t, 4 )
        #endif
    #elif( NRD_DIFF )
        NRD_INPUT( Texture2D, float4, gIn_Diff, t, 1 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_DiffSh, t, 2 )
        #endif
    #else
        NRD_INPUT( Texture2D, float4, gIn_Spec, t, 1 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, float4, gIn_SpecSh, t, 2 )
        #endif
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    #if( NRD_DIFF && NRD_SPEC )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff, u, 0 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec, u, 1 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffSh, u, 2 )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecSh, u, 3 )
        #endif
    #elif( NRD_DIFF )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff, u, 0 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_DiffSh, u, 1 )
        #endif
    #else
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec, u, 0 )
        #if( NRD_MODE == SH )
            NRD_OUTPUT( RWTexture2D, float4, gOut_SpecSh, u, 1 )
        #endif
    #endif
NRD_OUTPUTS_END

// Macro magic
#define RELAX_SplitScreenGroupX 8
#define RELAX_SplitScreenGroupY 16

// Shader only
#ifndef __cplusplus

#define GROUP_X RELAX_SplitScreenGroupX
#define GROUP_Y RELAX_SplitScreenGroupY

#endif
