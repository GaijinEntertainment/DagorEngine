/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( SpecularReflectionMv_ComputeConstants )
    NRD_CONSTANT( float4x4, gViewToWorld )
    NRD_CONSTANT( float4x4, gWorldToClip )
    NRD_CONSTANT( float4x4, gWorldToClipPrev )
    NRD_CONSTANT( float4x4, gWorldToViewPrev )
    NRD_CONSTANT( float4, gFrustumPrev )
    NRD_CONSTANT( float4, gFrustum )
    NRD_CONSTANT( float4, gViewVectorWorld ) // .w - unused
    NRD_CONSTANT( float4, gCameraDelta ) // .w - unused
    NRD_CONSTANT( float4, gMvScale )
    NRD_CONSTANT( float2, gRectSize )
    NRD_CONSTANT( float2, gRectSizeInv )
    NRD_CONSTANT( float2, gRectOffset )
    NRD_CONSTANT( float2, gResolutionScale )
    NRD_CONSTANT( uint2, gRectOrigin )
    NRD_CONSTANT( float, gDenoisingRange )
    NRD_CONSTANT( float, gOrthoMode )
    NRD_CONSTANT( float, gUnproject )
    NRD_CONSTANT( float, gDebug )
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float3>, gIn_Mv, t, 0 )
    NRD_INPUT( Texture2D<float4>, gIn_Normal_Roughness, t, 1 )
    NRD_INPUT( Texture2D<float>, gIn_ViewZ, t, 2 )
    NRD_INPUT( Texture2D<float>, gIn_HitDist, t, 3 )
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<float2>, gOut_SpecularReflectionMv, u, 0 )
NRD_OUTPUTS_END

// Macro magic
#define SpecularReflectionMv_ComputeGroupX 16
#define SpecularReflectionMv_ComputeGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X SpecularReflectionMv_ComputeGroupX
#define GROUP_Y SpecularReflectionMv_ComputeGroupY
