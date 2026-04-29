/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_CopyConstants )
    RELAX_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    #if( NRD_DIFF && NRD_SPEC )
        NRD_INPUT( Texture2D, float4, gIn_Spec, t, 0 )
        NRD_INPUT( Texture2D, float4, gIn_Diff, t, 1 )
    #elif( NRD_DIFF )
        NRD_INPUT( Texture2D, float4, gIn_Diff, t, 0 )
    #else
        NRD_INPUT( Texture2D, float4, gIn_Spec, t, 0 )
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    #if( NRD_DIFF && NRD_SPEC )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff, u, 1 )
    #elif( NRD_DIFF )
        NRD_OUTPUT( RWTexture2D, float4, gOut_Diff, u, 0 )
    #else
        NRD_OUTPUT( RWTexture2D, float4, gOut_Spec, u, 0 )
    #endif
NRD_OUTPUTS_END

// Macro magic
#define RELAX_CopyGroupX 8
#define RELAX_CopyGroupY 8

// Shader only
#ifndef __cplusplus

#define GROUP_X RELAX_CopyGroupX
#define GROUP_Y RELAX_CopyGroupY

#endif
