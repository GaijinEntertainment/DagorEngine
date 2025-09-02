/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float s_Penumbra[ BUFFER_Y ][ BUFFER_X ];
groupshared SIGMA_TYPE s_Shadow_Translucency[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    SIGMA_TYPE s = gIn_Shadow_Translucency[ globalPos ];
    s = SIGMA_BackEnd_UnpackShadow( s );

    s_Shadow_Translucency[ sharedPos.y ][ sharedPos.x ] = s;
    s_Penumbra[ sharedPos.y ][ sharedPos.x ] = gIn_Penumbra[ globalPos ];
}

uint PackViewZAndHistoryLength( float viewZ, float historyLength )
{
    uint p = asuint( viewZ ) & ~7;
    p |= min( uint( historyLength + 0.5 ), 7 );

    return p;
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<SIGMA_TYPE> tex0, out SIGMA_TYPE c0 )
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Center data
    int2 smemPos = threadPos + BORDER;
    float centerPenumbra = s_Penumbra[ smemPos.y ][ smemPos.x ];
    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );


    // Gaijin change
    // For tiles which are sky, write one, as we mark pixels under water as "sky" as well.
    // Here we are making sure that the underwater part has no leaks from previous frames
    // as marking the underwater parts as "sky" is not entirely perfect.
    // Early out #1
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) || viewZ > gDenoisingRange )
    {
      gOut_Shadow_Translucency[ pixelPos ] = 1;
      return;
    }

    // Early out #2
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float tileValue = TextureCubic( gIn_Tiles, pixelUv * gResolutionScale ).y;
    bool isHardShadow = ( ( tileValue == 0.0 && NRD_USE_TILE_CHECK ) || centerPenumbra == 0.0 ) && SIGMA_USE_EARLY_OUT_IN_TS;

    if( isHardShadow && SIGMA_SHOW == 0 )
    {
        gOut_Shadow_Translucency[ pixelPos ] = PackShadow( s_Shadow_Translucency[ smemPos.y ][ smemPos.x ] );
        gOut_HistoryLength[ pixelPos ] = PackViewZAndHistoryLength( viewZ, SIGMA_MAX_ACCUM_FRAME_NUM ); // TODO: yes, SIGMA_MAX_ACCUM_FRAME_NUM to allow accumulation in neighbors

        return;
    }

    // Local variance
    float sum = 0.0;
    SIGMA_TYPE m1 = 0;
    SIGMA_TYPE m2 = 0;
    SIGMA_TYPE input = 0;

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            int2 pos = threadPos + int2( i, j );
            SIGMA_TYPE s = s_Shadow_Translucency[ pos.y ][ pos.x ];

            float w = 1.0;
            if( i == BORDER && j == BORDER )
                input = s;
            else
            {
                float penum = s_Penumbra[ pos.y ][ pos.x ];

                w = AreBothLitOrUnlit( centerPenumbra, penum );
                w *= GetGaussianWeight( length( float2( i - BORDER, j - BORDER ) / BORDER ) );
            }

            m1 += s * w;
            m2 += s * s * w;
            sum += w;
        }
    }

    m1 /= sum; // sum can't be 0
    m2 /= sum;

    SIGMA_TYPE sigma = GetStdDev( m1, m2 );

    // Current and previous positions
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 X = Geometry::RotateVectorInverse( gWorldToView, Xv );

    float3 mv = gIn_Mv[ WithRectOrigin( pixelPos ) ] * gMvScale.xyz;
    float3 Xprev = X;
    float2 smbPixelUv = pixelUv + mv.xy;

    if( gMvScale.w == 0.0 )
    {
        if( gMvScale.z == 0.0 )
            mv.z = Geometry::AffineTransform( gWorldToViewPrev, X ).z - viewZ;

        float viewZprev = viewZ + mv.z;
        float3 Xvprevlocal = Geometry::ReconstructViewPosition( smbPixelUv, gFrustumPrev, viewZprev, gOrthoMode ); // TODO: use gOrthoModePrev

        Xprev = Geometry::RotateVectorInverse( gWorldToViewPrev, Xvprevlocal ) + gCameraDelta.xyz;
    }
    else
    {
        Xprev += mv;
        smbPixelUv = Geometry::GetScreenUv( gWorldToClipPrev, Xprev );
    }

    // History length
    Filtering::Bilinear smbBilinearFilter = Filtering::GetBilinearFilter( smbPixelUv, gRectSizePrev );
    float2 smbBilinearGatherUv = ( smbBilinearFilter.origin + 1.0 ) * gResourceSizeInvPrev;
    uint4 prevData = gIn_HistoryLength.GatherRed( gNearestClamp, smbBilinearGatherUv ).wzxy;
    float4 prevViewZ = asfloat( prevData & ~7 );
    float4 prevHistoryLength = float4( prevData & 7 );

    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    float disocclusionThreshold = GetDisocclusionThreshold( NRD_DISOCCLUSION_THRESHOLD, frustumSize, 1.0 ); // TODO: slope scale?
    disocclusionThreshold *= IsInScreenNearest( smbPixelUv );
    disocclusionThreshold -= NRD_EPS;

    float3 Xvprev = Geometry::AffineTransform( gWorldToViewPrev, Xprev );
    float4 smbPlaneDist = abs( prevViewZ - Xvprev.z );
    float4 smbOcclusion = step( smbPlaneDist, disocclusionThreshold );

    float4 smbOcclusionWeights = Filtering::GetBilinearCustomWeights( smbBilinearFilter, smbOcclusion );
    float historyLength = Filtering::ApplyBilinearCustomWeights( prevHistoryLength.x, prevHistoryLength.y, prevHistoryLength.z, prevHistoryLength.w, smbOcclusionWeights );

    // Sample history
    bool isCatRomAllowed = dot( smbOcclusionWeights, 1.0 ) > 3.5;

    SIGMA_TYPE history;
    BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
        saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
        smbOcclusionWeights, isCatRomAllowed,
        gIn_History, history );

    history = saturate( history );
    history = SIGMA_BackEnd_UnpackShadow( history );

    // Clamp history
    sigma *= lerp( SIGMA_TS_SIGMA_SCALE, 1.0, 1.0 / ( 1.0 + historyLength ) ); // TODO: lerp( SIGMA_TS_SIGMA_SCALE, 1.0, 0.125 ) != SIGMA_TS_SIGMA_SCALE

    SIGMA_TYPE inputMin = m1 - sigma;
    SIGMA_TYPE inputMax = m1 + sigma;
    SIGMA_TYPE historyClamped = clamp( history, inputMin, inputMax );

    // Antilag
    float antilag = abs( historyClamped.x - history.x );
    #if( SIGMA_ADJUST_HISTORY_LENGTH_BY_ANTILAG == 1 )
        antilag = Math::Sqrt01( antilag );
    #endif
    antilag = saturate( 1.0 - antilag );

    #if( SIGMA_ADJUST_HISTORY_LENGTH_BY_ANTILAG == 1 )
        historyLength *= antilag; // TODO: reduce influence if history is short?
    #endif

    // History weight
    float historyWeight = historyLength / ( 1.0 + historyLength );
    #if( SIGMA_ADJUST_HISTORY_LENGTH_BY_ANTILAG == 0 )
        historyWeight *= antilag;
    #endif

    // Street magic ( helps to smooth out "penumbra to 1" regions )
    float streetMagic = 0.6 * historyWeight * antilag; // TODO: * historyClamped.x? previously was without "* antilag"
    historyClamped = lerp( historyClamped, history, streetMagic );

    // Combine with the current frame
    SIGMA_TYPE result = lerp( input, historyClamped, min( gStabilizationStrength, historyWeight ) );

    // Debug ( don't forget that ".x" is used in antilag computations! )
    #if( SIGMA_SHOW == SIGMA_SHOW_TILES )
        tileValue = gIn_Tiles[ pixelPos >> 4 ].y;
        tileValue = float( tileValue != 0.0 ); // optional, just to show fully discarded tiles

        #ifdef SIGMA_TRANSLUCENT
            result = lerp( float4( 0, 0, 1, 0 ), result, tileValue );
        #else
            result = tileValue;
        #endif

        result *= all( ( pixelPos & 15 ) != 0 );
    #elif( SIGMA_SHOW == SIGMA_SHOW_HISTORY_WEIGHT )
        #ifdef SIGMA_TRANSLUCENT
            result.yzw = historyWeight * float( !isHardShadow );
        #endif
    #elif( SIGMA_SHOW == SIGMA_SHOW_HISTORY_LENGTH )
        #ifdef SIGMA_TRANSLUCENT
            result.yzw = historyLength / SIGMA_MAX_ACCUM_FRAME_NUM;
        #endif
    #elif( SIGMA_SHOW == SIGMA_SHOW_PENUMBRA_SIZE )
        result = centerPenumbra; // like in SplitScreen
    #endif

    // Update history length for the next frame
    historyLength = min( historyLength + 1.0, SIGMA_MAX_ACCUM_FRAME_NUM );

    // Output
    gOut_Shadow_Translucency[ pixelPos ] = PackShadow( result );
    gOut_HistoryLength[ pixelPos ] = PackViewZAndHistoryLength( viewZ, historyLength );
}
