/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( SpecularDeltaMv_ComputeConstants )
    NRD_CONSTANT( float4x4, gWorldToClipPrev )
    NRD_CONSTANT( float4, gMvScale )
    NRD_CONSTANT( uint2, gRectOrigin )
    NRD_CONSTANT( uint2, gRectSize )
    NRD_CONSTANT( float2, gRectSizeInv )
    NRD_CONSTANT( float, gDebug )
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 0 )
    NRD_INPUT( Texture2D<float3>, gIn_DeltaPrimaryPos, t, 1 )
    NRD_INPUT( Texture2D<float3>, gIn_DeltaSecondaryPos, t, 2 )
    NRD_INPUT( Texture2D<float3>, gIn_PrevDeltaSecondaryPos, t, 3 )
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<float2>, gOut_DeltaMv, u, 0 )
    NRD_OUTPUT( RWTexture2D<float3>, gOut_DeltaSecondaryPos, u, 1 )
NRD_OUTPUTS_END

// Macro magic
#define SpecularDeltaMv_ComputeGroupX 16
#define SpecularDeltaMv_ComputeGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X SpecularDeltaMv_ComputeGroupX
#define GROUP_Y SpecularDeltaMv_ComputeGroupY
