/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float s_Tile[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gTilesSizeMinusOne );

    s_Tile[ sharedPos.y ][ sharedPos.x ] = gIn_Tiles[ globalPos ].x;
}

[numthreads( GROUP_X, GROUP_X, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;
    PRELOAD_INTO_SMEM;

    float3 center = gIn_Tiles[ pixelPos ];
    float blurry = 0.0;
    float sum = 0.0;
    float k = 1.01 / ( center.y + 0.01 );

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            float d = length( float2( i, j ) - BORDER );
            float w = exp2( -k * d * d );

            blurry += s_Tile[ threadPos.y + j ][ threadPos.x + i ] * w;
            sum += w;
        }
    }

    blurry /= sum;

    gOut_Tiles[ pixelPos ] = float2( center.z, blurry );
}
