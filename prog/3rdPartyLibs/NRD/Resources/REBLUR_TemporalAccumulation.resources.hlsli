/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( REBLUR_TemporalAccumulationConstants )
    REBLUR_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

#if( defined REBLUR_DIFFUSE && defined REBLUR_SPECULAR )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<REBLUR_TILE_TYPE>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 2 )
        NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 3 )
        NRD_INPUT( Texture2D<float>, gPrev_ViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gPrev_Normal_Roughness, t, 5 )
        NRD_INPUT( Texture2D<uint>, gPrev_InternalData, t, 6 )
        NRD_INPUT( Texture2D<float>, gIn_DisocclusionThresholdMix, t, 7 )
        NRD_INPUT( Texture2D<float>, gIn_DiffConfidence, t, 8 )
        NRD_INPUT( Texture2D<float>, gIn_SpecConfidence, t, 9 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Diff, t, 10 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Spec, t, 11 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gHistory_Diff, t, 12 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gHistory_Spec, t, 13 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gHistory_DiffFast, t, 14 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gHistory_SpecFast, t, 15 )
        NRD_INPUT( Texture2D<float>, gPrev_SpecHitDistForTracking, t, 16 )
        #ifndef REBLUR_OCCLUSION
            NRD_INPUT( Texture2D<float>, gIn_SpecHitDistForTracking, t, 17 )
        #endif
        #ifdef REBLUR_SH
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_DiffSh, t, 18 )
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_SpecSh, t, 19 )
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gHistory_DiffSh, t, 20 )
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gHistory_SpecSh, t, 21 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Diff, u, 0 )
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Spec, u, 1 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_DiffFast, u, 2 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_SpecFast, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_SpecHitDistForTracking, u, 4 )
        NRD_OUTPUT( RWTexture2D<REBLUR_DATA1_TYPE>, gOut_Data1, u, 5 )
        #ifndef REBLUR_OCCLUSION
            NRD_OUTPUT( RWTexture2D<uint>, gOut_Data2, u, 6 )
        #endif
        #ifdef REBLUR_SH
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_DiffSh, u, 7 )
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_SpecSh, u, 8 )
        #endif
    NRD_OUTPUTS_END

#elif( defined REBLUR_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<REBLUR_TILE_TYPE>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 2 )
        NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 3 )
        NRD_INPUT( Texture2D<float>, gPrev_ViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gPrev_Normal_Roughness, t, 5 )
        NRD_INPUT( Texture2D<uint>, gPrev_InternalData, t, 6 )
        NRD_INPUT( Texture2D<float>, gIn_DisocclusionThresholdMix, t, 7 )
        NRD_INPUT( Texture2D<float>, gIn_DiffConfidence, t, 8 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Diff, t, 9 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gHistory_Diff, t, 10 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gHistory_DiffFast, t, 11 )
        #ifdef REBLUR_SH
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_DiffSh, t, 12 )
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gHistory_DiffSh, t, 13 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Diff, u, 0 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_DiffFast, u, 1 )
        NRD_OUTPUT( RWTexture2D<REBLUR_DATA1_TYPE>, gOut_Data1, u, 2 )
        #ifndef REBLUR_OCCLUSION
            NRD_OUTPUT( RWTexture2D<uint>, gOut_Data2, u, 3 )
        #endif
        #ifdef REBLUR_SH
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_DiffSh, u, 4 )
        #endif
    NRD_OUTPUTS_END

#else

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<REBLUR_TILE_TYPE>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 2 )
        NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 3 )
        NRD_INPUT( Texture2D<float>, gPrev_ViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gPrev_Normal_Roughness, t, 5 )
        NRD_INPUT( Texture2D<uint>, gPrev_InternalData, t, 6 )
        NRD_INPUT( Texture2D<float>, gIn_DisocclusionThresholdMix, t, 7 )
        NRD_INPUT( Texture2D<float>, gIn_SpecConfidence, t, 8 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gIn_Spec, t, 9 )
        NRD_INPUT( Texture2D<REBLUR_TYPE>, gHistory_Spec, t, 10 )
        NRD_INPUT( Texture2D<REBLUR_FAST_TYPE>, gHistory_SpecFast, t, 11 )
        NRD_INPUT( Texture2D<float>, gPrev_SpecHitDistForTracking, t, 12 )
        #ifndef REBLUR_OCCLUSION
            NRD_INPUT( Texture2D<float>, gIn_SpecHitDistForTracking, t, 13 )
        #endif
        #ifdef REBLUR_SH
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gIn_SpecSh, t, 14 )
            NRD_INPUT( Texture2D<REBLUR_SH_TYPE>, gHistory_SpecSh, t, 15 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<REBLUR_TYPE>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<REBLUR_FAST_TYPE>, gOut_SpecFast, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_SpecHitDistForTracking, u, 2 )
        NRD_OUTPUT( RWTexture2D<REBLUR_DATA1_TYPE>, gOut_Data1, u, 3 )
        #ifndef REBLUR_OCCLUSION
            NRD_OUTPUT( RWTexture2D<uint>, gOut_Data2, u, 4 )
        #endif
        #ifdef REBLUR_SH
            NRD_OUTPUT( RWTexture2D<REBLUR_SH_TYPE>, gOut_SpecSh, u, 5 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define REBLUR_TemporalAccumulationGroupX 8
#define REBLUR_TemporalAccumulationGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X REBLUR_TemporalAccumulationGroupX
#define GROUP_Y REBLUR_TemporalAccumulationGroupY
