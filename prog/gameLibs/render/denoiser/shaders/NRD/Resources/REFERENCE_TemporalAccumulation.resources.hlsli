/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( REFERENCE_TemporalAccumulationConstants )
    NRD_CONSTANT( uint2, gRectOrigin )
    NRD_CONSTANT( float2, gRectSizeInv )
    NRD_CONSTANT( float, gSplitScreen )
    NRD_CONSTANT( float, gAccumSpeed )
    NRD_CONSTANT( float, gDebug )
NRD_CONSTANTS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float4>, gIn_Input, t, 0 )
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<float4>, gInOut_History, u, 0 )
NRD_OUTPUTS_END

// Macro magic
#define REFERENCE_TemporalAccumulationGroupX 16
#define REFERENCE_TemporalAccumulationGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X REFERENCE_TemporalAccumulationGroupX
#define GROUP_Y REFERENCE_TemporalAccumulationGroupY
