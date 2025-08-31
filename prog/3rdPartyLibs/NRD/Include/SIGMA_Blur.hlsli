/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float2 s_Penumbra_ViewZ[ BUFFER_Y ][ BUFFER_X ];
groupshared SIGMA_TYPE s_Shadow_Translucency[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    float2 data;
    data.x = gIn_Penumbra[ globalPos ];
    data.y = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( globalPos ) ] );

    s_Penumbra_ViewZ[ sharedPos.y ][ sharedPos.x ] = data;

    SIGMA_TYPE s;
    #if( !defined SIGMA_FIRST_PASS || defined SIGMA_TRANSLUCENT )
        s = gIn_Shadow_Translucency[ globalPos ];
    #else
        s = IsLit( data.x );
    #endif

    #ifndef SIGMA_FIRST_PASS
        s = SIGMA_BackEnd_UnpackShadow( s );
    #endif

    s_Shadow_Translucency[ sharedPos.y ][ sharedPos.x ] = s;
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
#ifdef SIGMA_FIRST_PASS
    NRD_CTA_ORDER_DEFAULT;
#else
    NRD_CTA_ORDER_REVERSED;
#endif

    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Center data
    int2 smemPos = threadPos + BORDER;
    float2 centerData = s_Penumbra_ViewZ[ smemPos.y ][ smemPos.x ];
    float centerPenumbra = centerData.x;
    float viewZ = centerData.y;

    // Early out
    if( viewZ > gDenoisingRange )
        return;

    // Tile-based early out ( potentially )
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float tileValue = TextureCubic( gIn_Tiles, pixelUv * gResolutionScale ).y;

    if( ( tileValue == 0.0 && NRD_USE_TILE_CHECK ) || centerPenumbra == 0.0 )
    {
        gOut_Penumbra[ pixelPos ] = centerPenumbra;
        gOut_Shadow_Translucency[ pixelPos ] = PackShadow( s_Shadow_Translucency[ smemPos.y ][ smemPos.x ] );

        return;
    }

    // Position
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );

    // Normal
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ] );
    float3 N = normalAndRoughness.xyz;
    float3 Nv = Geometry::RotateVector( gWorldToView, N );

    // Parameters
    float pixelSize = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ );
    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );
    float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, 0.0 );

    // Estimate penumbra size and filter shadow ( dense )
    float2 sum = 0;
    float penumbra = 0;
    SIGMA_TYPE result = 0;
    SIGMA_TYPE centerTap;

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            int2 pos = threadPos + int2( i, j );

            // Fetch data
            float2 data = s_Penumbra_ViewZ[ pos.y ][ pos.x ];
            float penum = data.x;
            float zs = data.y;

            SIGMA_TYPE s = s_Shadow_Translucency[ pos.y ][ pos.x ];

            // Sample weight
            float w = 1.0;
            if( i == BORDER && j == BORDER )
                centerTap = s;
            else
            {
                float2 uv = pixelUv + float2( i - BORDER, j - BORDER ) * gRectSizeInv;
                float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

                w *= ComputeWeight( dot( Nv, Xvs ), geometryWeightParams.x, geometryWeightParams.y );
                w *= AreBothLitOrUnlit( centerPenumbra, penum );
                w *= GetGaussianWeight( length( float2( i - BORDER, j - BORDER ) / BORDER ) );
            }

            // Accumulate
            result += w == 0.0 ? 0.0 : s * w;
            sum.x += w;

            w *= pixelSize / ( pixelSize + penum ); // prefer smaller penumbra, same as "w /= 1.0 + penumInPixels", where penumInPixels = penum / pixelSize
            w *= !IsLit( penum );

            penumbra += w == 0.0 ? 0.0 : penum * w;
            sum.y += w;
        }
    }

    result /= sum.x;
    sum.x = 1.0;

    penumbra /= max( sum.y, NRD_EPS ); // yes, without patching
    sum.y = float( sum.y != 0.0 );

    // Avoid blurry result if penumbra size < BORDER
    float penumbraInPixels = penumbra / pixelSize;
    float f = Math::SmoothStep( 0.0, BORDER, penumbraInPixels );
    result = lerp( centerTap, result, f ); // TODO: not the best solution

#if( SIGMA_USE_SPARSE_BLUR == 1 )
    // Avoid unnecessary weight increase for the unfiltered center sample if the blur radius is small
    f = lerp( 4.0, 1.0, f ); // TODO: adds blurriness

    result *= f;
    penumbra *= f;
    sum *= f;

    // Blur radius
    float blurRadius = GetKernelRadiusInPixels( penumbra, pixelSize, tileValue );

    // Tangent basis with anisotropy
    #ifdef SIGMA_FIRST_PASS
        float4 rotator = GetBlurKernelRotation( SIGMA_ROTATOR_MODE, pixelPos, gRotator, gFrameIndex );
    #else
        float4 rotator = GetBlurKernelRotation( SIGMA_ROTATOR_MODE, pixelPos, gRotatorPost, gFrameIndex );
    #endif

    #if( SIGMA_USE_SCREEN_SPACE_SAMPLING == 1 )
        float2 skew = lerp( 1.0 - abs( Nv.xy ), 1.0, NoV );
        skew /= max( skew.x, skew.y );

        skew *= gRectSizeInv * blurRadius;

        float4 scaledRotator = Geometry::ScaleRotator( rotator, skew );
    #else
        float3x3 mWorldToLocal = Geometry::GetBasis( Nv );
        float3 Tv = mWorldToLocal[ 0 ];
        float3 Bv = mWorldToLocal[ 1 ];

        float3 t = cross( gLightDirectionView.xyz, Nv ); // TODO: add support for other light types to bring proper anisotropic filtering
        if( length( t ) > 0.001 )
        {
            Tv = normalize( t );
            Bv = cross( Tv, Nv );

            float cosa = abs( dot( Nv, gLightDirectionView.xyz ) );
            float skewFactor = lerp( 0.25, 1.0, cosa );

            //Tv *= skewFactor; // TODO: let's not srink filtering in the other direction
            Bv /= skewFactor; // TODO: good for test 197, but adds bad correlations in test 212
        }

        float worldRadius = blurRadius * pixelSize;

        Tv *= worldRadius;
        Bv *= worldRadius;
    #endif

    // Estimate penumbra size and filter shadow ( sparse )
    float invEstimatedPenumbra = 1.0 / max( penumbra, NRD_EPS );

    [unroll]
    for( uint n = 0; n < SIGMA_POISSON_SAMPLE_NUM; n++ )
    {
        // Sample coordinates
        float3 offset = SIGMA_POISSON_SAMPLES[ n ];

        #if( SIGMA_USE_SCREEN_SPACE_SAMPLING == 1 )
            float2 uv = pixelUv + Geometry::RotateVector( scaledRotator, offset.xy );
        #else
            float2 uv = GetKernelSampleCoordinates( gViewToClip, offset, Xv, Tv, Bv, rotator );
        #endif

        // Snap to the pixel center!
        uv = ( floor( uv * gRectSize ) + 0.5 ) * gRectSizeInv;

        // Texture coordinates
        float2 uvScaled = ClampUvToViewport( uv );

        // Fetch data
        float penum = gIn_Penumbra.SampleLevel( gNearestClamp, uvScaled, 0 );
        float zs = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, WithRectOffset( uvScaled ), 0 ) );

        SIGMA_TYPE s;
        #if( !defined SIGMA_FIRST_PASS || defined SIGMA_TRANSLUCENT )
            s = gIn_Shadow_Translucency.SampleLevel( gNearestClamp, uvScaled, 0 );
        #else
            s = IsLit( penum );
        #endif

        #ifndef SIGMA_FIRST_PASS
            s = SIGMA_BackEnd_UnpackShadow( s );
        #endif

        // Sample weight
        float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

        float w = IsInScreenNearest( uv );
        w *= ComputeWeight( dot( Nv, Xvs ), geometryWeightParams.x, geometryWeightParams.y );
        w *= AreBothLitOrUnlit( centerPenumbra, penum );
        w *= GetGaussianWeight( offset.z );

        // Avoid umbra leaking inside wide penumbra
        w *= saturate( penum * invEstimatedPenumbra ); // TODO: it works surprisingly well, keep an eye on it!

        // Accumulate
        result += w == 0.0 ? 0.0 : s * w;
        sum.x += w;

        w *= pixelSize / ( pixelSize + penum ); // prefer smaller penumbra, same as "w /= 1.0 + penumInPixels", where penumInPixels = penum / pixelSize
        w *= !IsLit( penum );

        penumbra += w == 0.0 ? 0.0 : penum * w;
        sum.y += w;
    }
#endif

    result /= sum.x;
    penumbra = sum.y == 0.0 ? centerPenumbra : penumbra / sum.y;

    // Output
    #ifndef SIGMA_FIRST_PASS
        if( gStabilizationStrength != 0 )
    #endif
            gOut_Penumbra[ pixelPos ] = penumbra;

    gOut_Shadow_Translucency[ pixelPos ] = PackShadow( result );
}
