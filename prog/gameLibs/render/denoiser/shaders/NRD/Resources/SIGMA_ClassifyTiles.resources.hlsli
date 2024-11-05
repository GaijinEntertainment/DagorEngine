/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( SIGMA_ClassifyTilesConstants )
    SIGMA_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float2>, gIn_Hit_ViewZ, t, 0 )
    #ifdef SIGMA_TRANSLUCENT
        NRD_INPUT( Texture2D<SIGMA_TYPE>, gIn_Shadow_Translucency, t, 1 )
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<float4>, gOut_Tiles, u, 0 )
NRD_OUTPUTS_END

// Macro magic
#define SIGMA_ClassifyTilesGroupX 16
#define SIGMA_ClassifyTilesGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X SIGMA_ClassifyTilesGroupX
#define GROUP_Y SIGMA_ClassifyTilesGroupY
