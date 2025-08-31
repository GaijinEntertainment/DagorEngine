/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( SIGMA_SplitScreenConstants )
    SIGMA_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 0 )
    NRD_INPUT( Texture2D<float>, gIn_Penumbra, t, 1 )
    #ifdef SIGMA_TRANSLUCENT
        NRD_INPUT( Texture2D<float4>, gIn_Shadow_Translucency, t, 2 )
    #endif
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<SIGMA_TYPE>, gOut_Shadow_Translucency, u, 0 )
NRD_OUTPUTS_END

// Macro magic
#define SIGMA_SplitScreenGroupX 8
#define SIGMA_SplitScreenGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X SIGMA_SplitScreenGroupX
#define GROUP_Y SIGMA_SplitScreenGroupY
