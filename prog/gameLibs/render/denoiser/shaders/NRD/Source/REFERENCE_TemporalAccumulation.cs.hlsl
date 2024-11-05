/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NRD.hlsli"
#include "STL.hlsli"

#include "REFERENCE_TemporalAccumulation.resources.hlsli"

#include "Common.hlsli"

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( uint2 pixelPos : SV_DispatchThreadId )
{
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;

    float4 input = gIn_Input[ pixelPos ];
    float4 history = gInOut_History[ pixelPos ];
    float4 result = lerp( history, input, gAccumSpeed );

    gInOut_History[ pixelPos ] = pixelUv.x > gSplitScreen ? result : input;
}

