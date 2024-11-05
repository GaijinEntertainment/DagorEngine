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

#include "RELAX_Config.hlsli"
#include "RELAX_ClassifyTiles.resources.hlsli"

groupshared uint s_isSky;

[numthreads( 8, 4, 1 )]
NRD_EXPORT void NRD_CS_MAIN( uint2 threadPos : SV_GroupThreadId, uint2 tilePos : SV_GroupId, uint threadIndex : SV_GroupIndex )
{
    if( threadIndex == 0 )
        s_isSky = 0;

    GroupMemoryBarrier();

    uint2 pixelPos = tilePos * 16 + threadPos * uint2( 2, 4 );
    uint isSky = 0;

    [unroll]
    for( uint i = 0; i < 2; i++ )
    {
        [unroll]
        for( uint j = 0; j < 4; j++ )
        {
            uint2 pos = pixelPos + uint2( i, j );
            float viewZ = abs( gIn_ViewZ[ pos ] );

            isSky += viewZ > gDenoisingRange ? 1 : 0;
        }
    }

    InterlockedAdd( s_isSky, isSky );

    GroupMemoryBarrier();

    if( threadIndex == 0 )
        gOut_Tiles[ tilePos ] = s_isSky == 256 ? 1.0 : 0.0;
}
