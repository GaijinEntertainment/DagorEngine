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

    SIGMA_TYPE s;
    #if( !defined SIGMA_FIRST_PASS || defined SIGMA_TRANSLUCENT )
        s = gIn_Shadow_Translucency[ globalPos ];
    #else
        s = float( data.x == NRD_FP16_MAX );
    #endif

    #ifndef SIGMA_FIRST_PASS
        s = UnpackShadowSpecial( s );
    #endif

    s_Shadow_Translucency[ sharedPos.y ][ sharedPos.x ] = s;
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( int2 threadPos : SV_GroupThreadId, int2 pixelPos : SV_DispatchThreadId, uint threadIndex : SV_GroupIndex )
{
    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ].y;
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Center data
    int2 smemPos = threadPos + BORDER;
    float2 centerData = s_Data[ smemPos.y ][ smemPos.x ];
    float centerHitDist = centerData.x;
    float centerSignNoL = float( centerData.x != 0.0 );
    float viewZ = centerData.y;

    // Early out
    if( viewZ > gDenoisingRange )
        return;

    // Copy history
    #ifdef SIGMA_FIRST_PASS
        gOut_History[ pixelPos ] = gIn_History[ pixelPos ];
    #endif

    // Tile-based early out ( potentially )
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float tileValue = TextureCubic( gIn_Tiles, pixelUv * gResolutionScale );
    #ifdef SIGMA_FIRST_PASS
        tileValue *= all( pixelPos < gRectSize ); // due to USE_MAX_DIMS
    #endif

    if( ( tileValue == 0.0 && NRD_USE_TILE_CHECK ) || centerHitDist == 0.0 )
    {
        gOut_Shadow_Translucency[ pixelPos ] = PackShadow( s_Shadow_Translucency[ smemPos.y ][ smemPos.x ] );
        gOut_Hit_ViewZ[ pixelPos ] = float2( 0.0, viewZ * NRD_FP16_VIEWZ_SCALE );

        return;
    }

    // Reference
    #if( SIGMA_REFERENCE == 1 )
        gOut_Shadow_Translucency[ pixelPos ] = PackShadow( s_Shadow_Translucency[ smemPos.y ][ smemPos.x ] );
        gOut_Hit_ViewZ[ pixelPos ] = float2( centerHitDist * centerSignNoL, viewZ * NRD_FP16_VIEWZ_SCALE );

        return;
    #endif

    // Position
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );

    // Normal
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ] );
    float3 N = normalAndRoughness.xyz;
    float3 Nv = STL::Geometry::RotateVector( gWorldToView, N );

    // Estimate average distance to occluder
    float sum = 0;
    float hitDist = 0;
    SIGMA_TYPE result = 0;

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            int2 pos = threadPos + int2( i, j );
            float2 data = s_Data[ pos.y ][ pos.x ];

            SIGMA_TYPE s = s_Shadow_Translucency[ pos.y ][ pos.x ];
            float h = data.x;
            float signNoL = float( data.x != 0.0 );
            float z = data.y;

            float w = 1.0;
            if( !( i == BORDER && j == BORDER ) )
            {
                w = GetBilateralWeight( z, viewZ );
                w *= saturate( 1.0 - abs( centerSignNoL - signNoL ) );
            }

            result += s * w;
            hitDist += h * float( s.x != 1.0 ) * w;
            sum += w;
        }
    }

    float invSum = 1.0 / sum;
    result *= invSum;
    hitDist *= invSum;

    // Blur radius
    float unprojectZ = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ );

    float innerShadowRadiusScale = lerp( 0.5, 1.0, result.x );
    float outerShadowRadiusScale = 1.0; // TODO: find a way to improve penumbra
    float pixelRadius = innerShadowRadiusScale * outerShadowRadiusScale;
    pixelRadius *= tileValue;
    pixelRadius *= hitDist / unprojectZ;
    pixelRadius *= gBlurRadiusScale;

    float centerWeight = STL::Math::LinearStep( 0.9, 1.0, result.x );
    float penumbraFixWeight = lerp( saturate( pixelRadius / 1.5 ), 1.0, centerWeight ) * result.x;
    pixelRadius += SIGMA_PENUMBRA_FIX_BLUR_RADIUS_ADDON * penumbraFixWeight; // TODO: improve

    pixelRadius = min( pixelRadius, SIGMA_MAX_PIXEL_RADIUS );

    // Tangent basis
    float worldRadius = pixelRadius * unprojectZ;
    float3x3 mWorldToLocal = STL::Geometry::GetBasis( Nv );
    float3 Tv = mWorldToLocal[ 0 ] * worldRadius;
    float3 Bv = mWorldToLocal[ 1 ] * worldRadius;

    // Random rotation
    float4 rotator = GetBlurKernelRotation( SIGMA_ROTATOR_MODE, pixelPos, gRotator, gFrameIndex );

    // Denoising
    sum = 1.0;

    float frustumSize = PixelRadiusToWorld( gUnproject, gOrthoMode, min( gRectSize.x, gRectSize.y ), viewZ );
    float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, 1.0 );

    [unroll]
    for( uint n = 0; n < SIGMA_POISSON_SAMPLE_NUM; n++ )
    {
        // Sample coordinates
        float3 offset = SIGMA_POISSON_SAMPLES[ n ];
        float2 uv = GetKernelSampleCoordinates( gViewToClip, offset, Xv, Tv, Bv, rotator );

        // Snap to the pixel center!
        uv = ( floor( uv * gRectSize ) + 0.5 ) * gRectSizeInv;

        // Texture coordinates
        float2 uvScaled = ClampUvToViewport( uv );

        // Fetch data
        float2 data = gIn_Hit_ViewZ.SampleLevel( gNearestClamp, uvScaled, 0 );
        float h = data.x;
        float signNoL = float( data.x != 0.0 );
        float z = abs( data.y ) / NRD_FP16_VIEWZ_SCALE;

        // Sample weight
        float3 Xvs = STL::Geometry::ReconstructViewPosition( uv, gFrustum, z, gOrthoMode );
        float NoX = dot( Nv, Xvs );

        float w = IsInScreenNearest( uv );
        w *= GetGaussianWeight( offset.z );
        w *= float( z < gDenoisingRange );
        w *= ComputeWeight( NoX, geometryWeightParams.x, geometryWeightParams.y );
        w *= saturate( 1.0 - abs( centerSignNoL - signNoL ) );

        SIGMA_TYPE s;
        #if( !defined SIGMA_FIRST_PASS || defined SIGMA_TRANSLUCENT )
            s = gIn_Shadow_Translucency.SampleLevel( gNearestClamp, uvScaled, 0 );
        #else
            s = float( h == NRD_FP16_MAX );
        #endif
        s = Denanify( w, s );

        #ifndef SIGMA_FIRST_PASS
            s = UnpackShadowSpecial( s );
        #endif

        // Weight for outer shadow ( to avoid blurring of ~umbra )
        w *= lerp( 1.0, s.x, centerWeight );

        // Accumulate
        sum += w;

        result += s * w;
        hitDist += h * float( s.x != 1.0 ) * w;
    }

    invSum = 1.0 / sum;
    result *= invSum;
    hitDist *= invSum;

    hitDist *= tileValue;
    hitDist *= centerSignNoL;

    // Output
    gOut_Shadow_Translucency[ pixelPos ] = PackShadow( result );
    gOut_Hit_ViewZ[ pixelPos ] = float2( hitDist, viewZ * NRD_FP16_VIEWZ_SCALE );
}
