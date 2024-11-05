/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( SIGMA_TemporalStabilizationConstants )
    SIGMA_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 0 )
    NRD_INPUT( Texture2D<float2>, gIn_Hit_ViewZ, t, 1 )
    NRD_INPUT( Texture2D<SIGMA_TYPE>, gIn_Shadow_Translucency, t, 2 )
    NRD_INPUT( Texture2D<SIGMA_TYPE>, gIn_History, t, 3 )
    NRD_INPUT( Texture2D<float2>, gIn_Tiles, t, 4 )
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<SIGMA_TYPE>, gOut_Shadow_Translucency, u, 0 )
NRD_OUTPUTS_END

// Macro magic
#define SIGMA_TemporalStabilizationGroupX 8
#define SIGMA_TemporalStabilizationGroupY 16

#if( SIGMA_5X5_TEMPORAL_KERNEL == 1 )
    #define NRD_USE_BORDER_2
#endif

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X SIGMA_TemporalStabilizationGroupX
#define GROUP_Y SIGMA_TemporalStabilizationGroupY
