/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared uint s_Mask;
groupshared uint s_Radius;

[numthreads( 8, 4, 1 )]
NRD_EXPORT void NRD_CS_MAIN( uint2 threadPos : SV_GroupThreadId, uint2 tilePos : SV_GroupId, uint threadIndex : SV_GroupIndex )
{
    if( threadIndex == 0 )
    {
        s_Mask = 0;
        s_Radius = 0;
    }

    GroupMemoryBarrier();

    uint2 pixelPos = tilePos * 16 + threadPos * uint2( 2, 4 );

    uint mask = 0;
    float maxRadius = 0.0;

    [unroll]
    for( uint i = 0; i < 2; i++ )
    {
        [unroll]
        for( uint j = 0; j < 4; j++ )
        {
            uint2 pos = pixelPos + uint2( i, j );
            float2 data = gIn_Hit_ViewZ[ pos ];

            float viewZ = abs( data.y ) / NRD_FP16_VIEWZ_SCALE;

            bool isInf = viewZ > gDenoisingRange;
            bool isShadow = data.x == 0;
            bool isLit = data.x == NRD_FP16_MAX;

            bool isOpaque = true;
            #ifdef SIGMA_TRANSLUCENT
                float3 translucency = gIn_Shadow_Translucency[ pos ].yzw;
                isOpaque = STL::Color::Luminance( translucency ) < 0.003; // TODO: replace with a uniformity test?
            #endif

            mask += ( ( isLit || isInf || isShadow ) ? 1 : 0 ) << 0;
            mask += ( ( ( !isLit && isOpaque ) || isInf || isShadow ) ? 1 : 0 ) << 9;
            mask += ( isInf ? 1 : 0 ) << 18;

            float worldRadius = ( isLit || isInf ) ? 0 : ( data.x * gBlurRadiusScale );
            float unprojectZ = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ );
            float pixelRadius = worldRadius * STL::Math::PositiveRcp( unprojectZ );
            pixelRadius = min( pixelRadius, SIGMA_MAX_PIXEL_RADIUS );

            maxRadius = max( pixelRadius, maxRadius );
        }
    }

    InterlockedAdd( s_Mask, mask );
    InterlockedMax( s_Radius, asuint( maxRadius ) );

    GroupMemoryBarrier();

    if( threadIndex == 0 )
    {
        bool isLit = ( ( s_Mask >> 0 ) & 511 ) == 256;
        bool isUmbra = ( ( s_Mask >> 9 ) & 511 ) == 256;
        bool isInf = ( ( s_Mask >> 18 ) & 511 ) == 256;

        float4 result;
        result.x = ( isLit || isUmbra ) ? 0.0 : 1.0;
        result.y = saturate( asfloat( s_Radius ) / 16.0 );
        result.z = isInf ? 1.0 : 0.0;
        result.w = 0.0;

        gOut_Tiles[ tilePos ] = result;
    }
}
