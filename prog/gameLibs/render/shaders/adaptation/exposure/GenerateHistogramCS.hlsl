//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//
// The group size is 16x16, but one group iterates over an entire 16-wide column of pixels (384 pixels tall)
// Assuming the total workspace is 640x384, there will be 40 thread groups computing the histogram in parallel.
// The histogram measures logarithmic luminance ranging from 2^-12 up to 2^4.  This should provide a nice window
// where the exposure would range from 2^-4 up to 2^4.

//SLOWER than naive
#include "PostEffectsRS.hlsli"

Texture2D<float> LumaBuf : register( t0 );
RWStructuredBuffer<uint> Histogram : register(u0);//RWByteAddressBuffer

groupshared uint g_TileHistogram[256];

//[RootSignature(PostEffects_RootSig)]
[numthreads( 16, 16, 1 )]
void main( uint2 gId : SV_GroupID, uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID )
{
    g_TileHistogram[GI] = 0;

    GroupMemoryBarrierWithGroupSync();

    // Loop 24 times until the entire column has been processed
    for (uint TopY = 0, i = 0; TopY < 384; TopY += 16, i++)
    {
        //g_TileHistogram[i] = LumaBuf[DTid.xy + uint2(0, TopY)]*255.f+0.5f;
        uint QuantizedLogLuma = LumaBuf[DTid.xy + uint2(0, TopY)]*255.f+0.5f;
        InterlockedAdd( g_TileHistogram[QuantizedLogLuma], 1 );
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(Histogram[GI],g_TileHistogram[GI]);
}
