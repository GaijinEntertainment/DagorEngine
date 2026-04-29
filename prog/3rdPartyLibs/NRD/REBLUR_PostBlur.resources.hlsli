/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( REBLUR_PostBlurConstants )
    REBLUR_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D, REBLUR_TILE_TYPE, gIn_Tiles, t, 0 )
    NRD_INPUT( Texture2D, float4, gIn_Normal_Roughness, t, 1 )
    NRD_INPUT( Texture2D, REBLUR_DATA1_TYPE, gIn_Data1, t, 2 )
    NRD_INPUT( Texture2D, float, gIn_ViewZ, t, 3 )
    #if( NRD_DIFF && NRD_SPEC )
        NRD_INPUT( Texture2D, REBLUR_TYPE, gIn_Diff, t, 4 )
        NRD_INPUT( Texture2D, REBLUR_TYPE, gIn_Spec, t, 5 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, REBLUR_SH_TYPE, gIn_DiffSh, t, 6 )
            NRD_INPUT( Texture2D, REBLUR_SH_TYPE, gIn_SpecSh, t, 7 )
        #endif
    #elif( NRD_DIFF )
        NRD_INPUT( Texture2D, REBLUR_TYPE, gIn_Diff, t, 4 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, REBLUR_SH_TYPE, gIn_DiffSh, t, 5 )
        #endif
    #else
        NRD_INPUT( Texture2D, REBLUR_TYPE, gIn_Spec, t, 4 )
        #if( NRD_MODE == SH )
            NRD_INPUT( Texture2D, REBLUR_SH_TYPE, gIn_SpecSh, t, 5 )
        #endif
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D, float4, gOut_Normal_Roughness, u, 0 )
    #if( NRD_DIFF && NRD_SPEC )
        NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_Diff, u, 1 )
        NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_Spec, u, 2 )
        #if( TEMPORAL_STABILIZATION == 0 )
            NRD_OUTPUT( RWTexture2D, uint, gOut_InternalData, u, 3 )
            NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_DiffCopy, u, 4 )
            NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_SpecCopy, u, 5 )
            #if( NRD_MODE == SH )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_DiffShCopy, u, 6 )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_SpecShCopy, u, 7 )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_DiffSh, u, 8 )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_SpecSh, u, 9 )
            #endif
        #else
            #if( NRD_MODE == SH )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_DiffSh, u, 3 )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_SpecSh, u, 4 )
            #endif
        #endif
    #elif( NRD_DIFF )
        NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_Diff, u, 1 )
        #if( TEMPORAL_STABILIZATION == 0 )
            NRD_OUTPUT( RWTexture2D, uint, gOut_InternalData, u, 2 )
            NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_DiffCopy, u, 3 )
            #if( NRD_MODE == SH )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_DiffShCopy, u, 4 )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_DiffSh, u, 5 )
            #endif
        #else
            #if( NRD_MODE == SH )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_DiffSh, u, 2 )
            #endif
        #endif
    #else
        NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_Spec, u, 1 )
        #if( TEMPORAL_STABILIZATION == 0 )
            NRD_OUTPUT( RWTexture2D, uint, gOut_InternalData, u, 2 )
            NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_SpecCopy, u, 3 )
            #if( NRD_MODE == SH )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_SpecShCopy, u, 4 )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_SpecSh, u, 5 )
            #endif
        #else
            #if( NRD_MODE == SH )
                NRD_OUTPUT( RWTexture2D, REBLUR_SH_TYPE, gOut_SpecSh, u, 2 )
            #endif
        #endif
    #endif
NRD_OUTPUTS_END

// Macro magic
#define REBLUR_PostBlurGroupX 8
#define REBLUR_PostBlurGroupY 16

// Shader only
#ifndef __cplusplus

#define GROUP_X REBLUR_PostBlurGroupX
#define GROUP_Y REBLUR_PostBlurGroupY

#endif
