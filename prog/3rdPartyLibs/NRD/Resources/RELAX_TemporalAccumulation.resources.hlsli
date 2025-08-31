/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( RELAX_TemporalAccumulationConstants )
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
        NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 3 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 4 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 5 )
        NRD_INPUT( Texture2D<float4>, gHistory_SpecFast, t, 6 )
        NRD_INPUT( Texture2D<float4>, gHistory_DiffFast, t, 7 )
        NRD_INPUT( Texture2D<float4>, gHistory_Spec, t, 8 )
        NRD_INPUT( Texture2D<float4>, gHistory_Diff, t, 9 )
        NRD_INPUT( Texture2D<float4>, gPrev_Normal_Roughness, t, 10 )
        NRD_INPUT( Texture2D<float>, gPrev_ViewZ, t, 11 )
        NRD_INPUT( Texture2D<float>, gPrev_SpecHitDist, t, 12 )
        NRD_INPUT( Texture2D<float>, gPrev_HistoryLength, t, 13 )
        NRD_INPUT( Texture2D<float>, gPrev_MateriallID, t, 14 )
        NRD_INPUT( Texture2D<float>, gIn_SpecConfidence, t, 15 )
        NRD_INPUT( Texture2D<float>, gIn_DiffConfidence, t, 16 )
        NRD_INPUT( Texture2D<float>, gIn_DisocclusionThresholdMix, t, 17 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 18 )
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 19 )
            NRD_INPUT( Texture2D<float4>, gHistory_SpecShFast, t, 20 )
            NRD_INPUT( Texture2D<float4>, gHistory_DiffShFast, t, 21 )
            NRD_INPUT( Texture2D<float4>, gHistory_SpecSh, t, 22 )
            NRD_INPUT( Texture2D<float4>, gHistory_DiffSh, t, 23 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Diff, u, 1 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecFast, u, 2 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffFast, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_SpecHitDist, u, 4 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_HistoryLength, u, 5 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_SpecReprojectionConfidence, u, 6 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 7 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffSh, u, 8 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecShFast, u, 9 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_DiffShFast, u, 10 )
        #endif
    NRD_OUTPUTS_END

#elif( defined RELAX_DIFFUSE )

    NRD_INPUTS_START
        NRD_INPUT( Texture2D<float>, gIn_Tiles, t, 0 )
        NRD_INPUT( Texture2D<float4>, gIn_Diff, t, 1 )
        NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gHistory_DiffFast, t, 5 )
        NRD_INPUT( Texture2D<float4>, gHistory_Diff, t, 6 )
        NRD_INPUT( Texture2D<float4>, gPrev_Normal_Roughness, t, 7 )
        NRD_INPUT( Texture2D<float>, gPrev_ViewZ, t, 8 )
        NRD_INPUT( Texture2D<float>, gPrev_HistoryLength, t, 9 )
        NRD_INPUT( Texture2D<float>, gPrev_MateriallID, t, 10 )
        NRD_INPUT( Texture2D<float>, gIn_DiffConfidence, t, 11 )
        NRD_INPUT( Texture2D<float>, gIn_DisocclusionThresholdMix, t, 12)
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_DiffSh, t, 13 )
            NRD_INPUT( Texture2D<float4>, gHistory_DiffShFast, t, 14 )
            NRD_INPUT( Texture2D<float4>, gHistory_DiffSh, t, 15 )
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
        NRD_INPUT( Texture2D<float4>, gIn_Spec, t, 1 )
        NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 2 )
        NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 3 )
        NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 4 )
        NRD_INPUT( Texture2D<float4>, gHistory_SpecFast, t, 5 )
        NRD_INPUT( Texture2D<float4>, gHistory_Spec, t, 6 )
        NRD_INPUT( Texture2D<float4>, gPrev_Normal_Roughness, t, 7 )
        NRD_INPUT( Texture2D<float>, gPrev_ViewZ, t, 8 )
        NRD_INPUT( Texture2D<float>, gPrev_SpecHitDist, t, 9 )
        NRD_INPUT( Texture2D<float>, gPrev_HistoryLength, t, 10 )
        NRD_INPUT( Texture2D<float>, gPrev_MateriallID, t, 11 )
        NRD_INPUT( Texture2D<float>, gIn_SpecConfidence, t, 12 )
        NRD_INPUT( Texture2D<float>, gIn_DisocclusionThresholdMix, t, 13 )
        #ifdef RELAX_SH
            NRD_INPUT( Texture2D<float4>, gIn_SpecSh, t, 14 )
            NRD_INPUT( Texture2D<float4>, gHistory_SpecShFast, t, 15 )
            NRD_INPUT( Texture2D<float4>, gHistory_SpecSh, t, 16 )
        #endif
    NRD_INPUTS_END

    NRD_OUTPUTS_START
        NRD_OUTPUT( RWTexture2D<float4>, gOut_Spec, u, 0 )
        NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecFast, u, 1 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_SpecHitDist, u, 2 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_HistoryLength, u, 3 )
        NRD_OUTPUT( RWTexture2D<float>, gOut_SpecReprojectionConfidence, u, 4 )
        #ifdef RELAX_SH
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecSh, u, 5 )
            NRD_OUTPUT( RWTexture2D<float4>, gOut_SpecShFast, u, 6 )
        #endif
    NRD_OUTPUTS_END

#endif

// Macro magic
#define RELAX_TemporalAccumulationGroupX 8
#define RELAX_TemporalAccumulationGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X RELAX_TemporalAccumulationGroupX
#define GROUP_Y RELAX_TemporalAccumulationGroupY
