/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float4 s_Normal_Roughness[ BUFFER_Y ][ BUFFER_X ];
groupshared float3 s_HitDist_ViewZ[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( globalPos ) ] );

    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( globalPos ) ] );
    s_Normal_Roughness[ sharedPos.y ][ sharedPos.x ] = normalAndRoughness;

    float2 hitDist = 0.0;
    #ifdef REBLUR_DIFFUSE
        hitDist.x = ExtractHitDist( gIn_Diff[ globalPos ] );

        #if( REBLUR_USE_DECOMPRESSED_HIT_DIST_IN_RECONSTRUCTION == 1 )
            hitDist.x *= _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, 1.0 );
        #endif
    #endif

    #ifdef REBLUR_SPECULAR
        hitDist.y = ExtractHitDist( gIn_Spec[ globalPos ] );

        #if( REBLUR_USE_DECOMPRESSED_HIT_DIST_IN_RECONSTRUCTION == 1 )
            hitDist.y *= _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, normalAndRoughness.w );
        #endif
    #endif

    s_HitDist_ViewZ[ sharedPos.y ][ sharedPos.x ] = float3( hitDist, viewZ );
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Early out
    int2 smemPos = threadPos + BORDER;
    float3 center = s_HitDist_ViewZ[ smemPos.y ][ smemPos.x ];
    if( center.z > gDenoisingRange )
        return;

    // Center data
    float4 normalAndRoughness = s_Normal_Roughness[ smemPos.y ][ smemPos.x ];
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, center.z, gOrthoMode );
    float3 Nv = Geometry::RotateVectorInverse( gViewToWorld, N );
    float3 Vv = GetViewVector( Xv, true );

    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, center.z );

    float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, 1.0 );
    float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( roughness * roughness );
    float diffNormalWeightParam = GetNormalWeightParam( 1.0, 1.0 );
    float specNormalWeightParam = GetNormalWeightParam( 1.0, 1.0, roughness );

    // Hit distance reconstruction
    float2 sum = 1000.0 * float2( center.xy != 0.0 );
    center.xy *= sum;

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            float2 o = float2( i, j ) - BORDER;
            if( o.x == 0.0 && o.y == 0.0 )
                continue;

            int2 pos = threadPos + int2( i, j );
            float3 data = s_HitDist_ViewZ[ pos.y ][ pos.x ];

            // Weight
            float w = IsInScreenNearest( pixelUv + o * gRectSizeInv );
            w *= GetGaussianWeight( length( o ) * 0.5 );

            // This weight is strict ( non exponential ) because we need to avoid accessing data from other surfaces
            float2 uv = pixelUv + o * gRectSizeInv;
            float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, data.z, gOrthoMode );
            w *= ComputeWeight( dot( Nv, Xvs ), geometryWeightParams.x, geometryWeightParams.y );

            float2 ww = w;
            #ifndef REBLUR_PERFORMANCE_MODE
                float4 normalAndRoughness = s_Normal_Roughness[ pos.y ][ pos.x ];

                float cosa = dot( N, normalAndRoughness.xyz );
                float angle = Math::AcosApprox( cosa );

                // These weights have infinite exponential tails, because with strict weights we are reducing a chance to find a valid sample in 3x3 or 5x5 area
                ww.x *= ComputeExponentialWeight( angle, diffNormalWeightParam, 0.0 );
                ww.y *= ComputeExponentialWeight( angle, specNormalWeightParam, 0.0 );
                ww.y *= ComputeExponentialWeight( normalAndRoughness.w * normalAndRoughness.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y );
            #endif

            data.x = Denanify( ww.x, data.x );
            data.y = Denanify( ww.y, data.y );
            ww *= float2( data.xy != 0.0 );

            // Accumulate
            center.xy += data.xy * ww;
            sum += ww;
        }
    }

    // Normalize weighted sum
    center.xy /= max( sum, NRD_EPS ); // IMPORTANT: if all conditions are met, "sum" can't be 0

    // Return back to normalized hit distances
    #if( REBLUR_USE_DECOMPRESSED_HIT_DIST_IN_RECONSTRUCTION == 1 )
        center.x /= _REBLUR_GetHitDistanceNormalization( center.z, gHitDistParams, 1.0 );
        center.y /= _REBLUR_GetHitDistanceNormalization( center.z, gHitDistParams, roughness );
    #endif

    // Output
    #ifdef REBLUR_DIFFUSE
        #ifdef REBLUR_OCCLUSION
            gOut_Diff[ pixelPos ] = center.x;
        #else
            float3 diff = gIn_Diff[ pixelPos ].xyz;
            gOut_Diff[ pixelPos ] = float4( diff, center.x );
        #endif
    #endif

    #ifdef REBLUR_SPECULAR
        #ifdef REBLUR_OCCLUSION
            gOut_Spec[ pixelPos ] = center.y;
        #else
            float3 spec = gIn_Spec[ pixelPos ].xyz;
            gOut_Spec[ pixelPos ] = float4( spec, center.y );
        #endif
    #endif
}
