/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_AtrousConstants )
    RELAX_SHARED_CONSTANTS
    NRD_CONSTANT( uint, gStepSize )
    NRD_CONSTANT( uint, gIsLastPass )
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined RELAX_DIFFUSE && defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Spec_Variance, t, 1 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff_Variance, t, 2 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 3 )
        NRD_INPUT( Texture2D<float>, gIn_SpecReprojectionConfidence, t, 4 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 5 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 6 )
        NRD_INPUT( Texture2D<float>, gIn_SpecConfidence, t, 7 )
        NRD_INPUT( Texture2D<float>, gIn_DiffConfidence, t, 8 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 9 )
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 10 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec_Variance, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff_Variance, u, 1 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 2 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 3 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff_Variance, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 4 )
        NRD_INPUT( Texture2D<float>, gIn_DiffConfidence, t, 5 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 6 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff_Variance, u, 0 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 1 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Spec_Variance, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_HistoryLength, t, 2 )
        NRD_INPUT( Texture2D<float>, gIn_SpecReprojectionConfidence, t, 3 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 4 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 5 )
        NRD_INPUT( Texture2D<float>, gIn_SpecConfidence, t, 6 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 7 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec_Variance, u, 0 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 1 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_AtrousGroupX 16
#define RELAX_AtrousGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_AtrousGroupX
#define GROUP_Y RELAX_AtrousGroupY
