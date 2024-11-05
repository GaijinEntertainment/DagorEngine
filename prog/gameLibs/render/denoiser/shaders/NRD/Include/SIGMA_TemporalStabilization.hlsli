/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float2 s_Data[ BUFFER_Y ][ BUFFER_X ];
groupshared SIGMA_TYPE s_Shadow_Translucency[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    float2 data = gIn_Hit_ViewZ[ globalPos ];
    data.y = abs( data.y ) / NRD_FP16_VIEWZ_SCALE;

    s_Data[ sharedPos.y ][ sharedPos.x ] = data;

    SIGMA_TYPE s = gIn_Shadow_Translucency[ globalPos ];
    s = UnpackShadow( s );

    s_Shadow_Translucency[ sharedPos.y ][ sharedPos.x ] = s;
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( int2 threadPos : SV_GroupThreadId, int2 pixelPos : SV_DispatchThreadId, uint threadIndex : SV_GroupIndex )
{
    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ].y;
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Gaijin change
    // For tiles which are sky, write one, as we mark pixels under water as "sky" as well.
    // Here we are making sure that the underwater part has no leaks from previous frames
    // as marking the underwater parts as "sky" is not entirely perfect.
    if ( any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Tile-based early out
    if( isSky != 0.0 )
    {
        gOut_Shadow_Translucency[ pixelPos ] = 1;
        return;
    }

    // Center data
    int2 smemPos = threadPos + BORDER;
    float2 centerData = s_Data[ smemPos.y ][ smemPos.x ];
    float centerHitDist = centerData.x;
    float centerSignNoL = float( centerData.x != 0.0 );
    float viewZ = centerData.y;

    // Early out
    if( viewZ > gDenoisingRange )
    {
        // Gaijin change
        // Just as with the tiles, "sky" pixels are set to no shadow.
        gOut_Shadow_Translucency[ pixelPos ] = 1;
        return;
    }

    // Early out
    if( centerHitDist == 0.0 && SIGMA_SHOW_TILES == 0 )
    {
        gOut_Shadow_Translucency[ pixelPos ] = PackShadow( s_Shadow_Translucency[ smemPos.y ][ smemPos.x ] );

        return;
    }

    // Local variance
    float sum = 0.0;
    SIGMA_TYPE m1 = 0;
    SIGMA_TYPE m2 = 0;
    SIGMA_TYPE input = 0;

    float viewZnearest = viewZ;
    int2 offseti = int2( BORDER, BORDER );

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            int2 pos = threadPos + int2( i, j );
            float2 data = s_Data[ pos.y ][ pos.x ];

            SIGMA_TYPE s = s_Shadow_Translucency[ pos.y ][ pos.x ];
            float signNoL = float( data.x != 0.0 );
            float z = data.y;

            float w = 1.0;
            if( i == BORDER && j == BORDER )
                input = s;
            else
            {
                w = GetBilateralWeight( z, viewZ );
                w *= saturate( 1.0 - abs( centerSignNoL - signNoL ) );

                if( z < viewZnearest )
                {
                    viewZnearest = z;
                    offseti = int2( i, j );
                }
            }

            m1 += s * w;
            m2 += s * s * w;
            sum += w;
        }
    }

    float invSum = STL::Math::PositiveRcp( sum );
    m1 *= invSum;
    m2 *= invSum;

    SIGMA_TYPE sigma = GetStdDev( m1, m2 );

    // Compute previous pixel position
    float2 pixelUv = ( float2( pixelPos ) + 0.5 ) * gRectSizeInv;
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZnearest, gOrthoMode );
    float3 X = STL::Geometry::RotateVectorInverse( gWorldToView, Xv );
    float3 mv = gIn_Mv[ WithRectOrigin( pixelPos ) + offseti - BORDER ] * gMvScale.xyz;
    float2 pixelUvPrev = pixelUv + mv.xy;

    if( gMvScale.w != 0.0 )
        pixelUvPrev = STL::Geometry::GetScreenUv( gWorldToClipPrev, X + mv );

    // Sample history
    SIGMA_TYPE history;
    BicubicFilterNoCorners( saturate( pixelUvPrev ) * gRectSizePrev, gResourceSizeInvPrev, SIGMA_USE_CATROM, gIn_History, history );

    history = max( history, 0.0 );
    history = UnpackShadow( history );

    // Clamp history
    float2 a = m1.xx;
    float2 b = history.xx;

    #ifdef SIGMA_TRANSLUCENT
        a.y = STL::Color::Luminance( m1.yzw );
        b.y = STL::Color::Luminance( history.yzw );
    #endif

    float2 ratio = abs( a - b ) / ( min( a, b ) + 0.05 );
    float2 ratioNorm = ratio / ( 1.0 + ratio );
    float2 scale = lerp( SIGMA_MAX_SIGMA_SCALE, 1.0, STL::Math::Sqrt01( ratioNorm ) );

    #ifdef SIGMA_TRANSLUCENT
        sigma *= scale.xyyy;
    #else
        sigma *= scale.x;
    #endif

    SIGMA_TYPE inputMin = m1 - sigma;
    SIGMA_TYPE inputMax = m1 + sigma;
    SIGMA_TYPE historyClamped = clamp( history, inputMin, inputMax );

    // History weight
    float isInScreen = IsInScreenNearest( pixelUvPrev );
    float motionLength = length( pixelUvPrev - pixelUv );
    float2 historyWeight = 0.93 * lerp( 1.0, 0.7, ratioNorm ); // use FPS-dependent value, like 0.25 * FPS
    historyWeight = lerp( historyWeight, 0.1, saturate( motionLength / SIGMA_TS_MOTION_MAX_REUSE ) );
    historyWeight *= isInScreen;
    historyWeight *= gContinueAccumulation;

    // Reduce history in regions with hard shadows
    float worldRadius = centerHitDist * gBlurRadiusScale;
    float unprojectZ = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ );
    float pixelRadius = worldRadius * STL::Math::PositiveRcp( unprojectZ );
    historyWeight *= STL::Math::LinearStep( 0.0, 3.0, pixelRadius );

    // Combine with current frame
    SIGMA_TYPE result;
    result.x = lerp( input.x, historyClamped.x, historyWeight.x );

    #ifdef SIGMA_TRANSLUCENT
        result.yzw = lerp( input.yzw, historyClamped.yzw, historyWeight.y );
    #endif

    // Reference
    #if( SIGMA_REFERENCE == 1 )
        result = lerp( input, history, 0.95 * isInScreen );
    #endif

    // Debug
    #if( SIGMA_SHOW_TILES == 1 )
        float tileValue = gIn_Tiles[ pixelPos >> 4 ].x;
        tileValue = float( tileValue != 0.0 ); // optional, just to show fully discarded tiles

        #ifdef SIGMA_TRANSLUCENT
            result = lerp( float4( 0, 0, 1, 0 ), result, tileValue );
        #else
            result = tileValue;
        #endif

        // Show grid (works badly with TAA)
        result *= all( ( pixelPos & 15 ) != 0 );
    #endif

    // Output
    gOut_Shadow_Translucency[ pixelPos ] = PackShadow( result );
}
