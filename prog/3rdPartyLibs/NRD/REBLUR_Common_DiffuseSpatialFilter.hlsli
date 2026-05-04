/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef REBLUR_SPATIAL_MODE
    #error REBLUR_SPATIAL_MODE must be defined!
#endif

#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
    #define POISSON_SAMPLE_NUM      REBLUR_PRE_BLUR_POISSON_SAMPLE_NUM
    #define POISSON_SAMPLES( i )    REBLUR_PRE_BLUR_POISSON_SAMPLES( i )
#else
    #define POISSON_SAMPLE_NUM      REBLUR_POISSON_SAMPLE_NUM
    #define POISSON_SAMPLES( i )    REBLUR_POISSON_SAMPLES( i )
#endif

{
#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
    if( gDiffPrepassBlurRadius != 0.0 )
    {
#endif
        float diffNonLinearAccumSpeed = nonLinearAccumSpeed.x;

        float fractionScale = 1.0;
        float radiusScale = 1.0;
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        fractionScale = REBLUR_PRE_BLUR_FRACTION_SCALE;
    #elif( REBLUR_SPATIAL_MODE == REBLUR_BLUR )
        fractionScale = REBLUR_BLUR_FRACTION_SCALE;
    #elif( REBLUR_SPATIAL_MODE == REBLUR_POST_BLUR )
        radiusScale = REBLUR_POST_BLUR_RADIUS_SCALE;
        fractionScale = REBLUR_POST_BLUR_FRACTION_SCALE;
    #endif

        // Hit distance factor ( tests 53, 76, 95, 120 )
        float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistSettings, 1.0 );
        float hitDist = ExtractHitDist( diff ) * hitDistScale;
        float hitDistFactor = GetHitDistFactor( hitDist, frustumSize );

        // Blur radius
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        float blurRadius = gDiffPrepassBlurRadius;
        float areaFactor = hitDistFactor;
    #else
        float blurRadius = gMaxBlurRadius;
        float areaFactor = hitDistFactor * diffNonLinearAccumSpeed;
    #endif

        blurRadius *= Math::Sqrt01( areaFactor ); // "areaFactor" affects area, not radius

        // Blur radius - scale
        blurRadius *= radiusScale;

        // Blur radius - addition to avoid underblurring
        blurRadius = max( blurRadius, gMinBlurRadius );

        // Weights
        float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv );
        float normalWeightParam = GetNormalWeightParam( diffNonLinearAccumSpeed, gLobeAngleFraction ) / fractionScale;

        float2 hitDistanceWeightParams = GetHitDistanceWeightParams( ExtractHitDist( diff ), diffNonLinearAccumSpeed ); // TODO: what if hitT == 0?
        float minHitDistWeight = gMinHitDistanceWeight * fractionScale;

        // ( Optional ) Gradually reduce "minHitDistWeight" to preserve contact details
    #if( REBLUR_SPATIAL_MODE != REBLUR_PRE_BLUR && NRD_MODE != OCCLUSION )
        minHitDistWeight *= diffNonLinearAccumSpeed;
    #endif

        // Screen-space settings
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR || REBLUR_USE_SCREEN_SPACE_SAMPLING_FOR_DIFFUSE == 1 )
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            float2 skew = 1.0; // TODO: seems that uniform screen space sampling in pre-pass breaks residual boiling better
        #else
            float2 skew = lerp( 1.0 - abs( Nv.xy ), 1.0, NoV );
            skew /= max( skew.x, skew.y );
        #endif
        skew *= gRectSizeInv * blurRadius;

        float4 scaledRotator = Geometry::ScaleRotator( rotator, skew );
    #else
        // World-space settings
        float2x3 TvBv = GetKernelBasis( Nv, Nv ); // D = N

        float worldRadius = PixelRadiusToWorld( gUnproject, gOrthoMode, blurRadius, viewZ );
        TvBv[ 0 ] *= worldRadius;
        TvBv[ 1 ] *= worldRadius;
    #endif

        // Sampling
        [unroll]
        for( uint n = 0; n < POISSON_SAMPLE_NUM; n++ )
        {
            float3 offset = POISSON_SAMPLES( n );

            // Sample coordinates
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR || REBLUR_USE_SCREEN_SPACE_SAMPLING_FOR_DIFFUSE == 1 )
            float2 uv = pixelUv + Geometry::RotateVector( scaledRotator, offset.xy );
        #else
            float2 uv = GetKernelSampleCoordinates( gViewToClip, offset, Xv, TvBv[ 0 ], TvBv[ 1 ], rotator );
        #endif

            // Apply "mirror" to not waste taps going outside of the screen
            float2 uv01 = saturate( uv );
            float w = GetGaussianWeight( offset.z );
            if( any( uv != uv01 ) ) // TODO: this branch saves a bit of perf
            {
                uv = MirrorUv( uv );
                w = 1.0; // offset.z is not valid after mirroring
            }

            // "uv" to "pos"
            int2 pos = uv * gRectSize; // "uv" can't be "1"

            // Move to a "valid" pixel in checkerboard mode
            int checkerboardX = pos.x;
        #if( NRD_SUPPORTS_CHECKERBOARD == 1 && REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            if( gDiffCheckerboard != 2 )
            {
                int shift = ( ( n & 0x1 ) == 0 ) ? -1 : 1;
                pos.x += Sequence::CheckerBoard( pos, gFrameIndex ) != gDiffCheckerboard ? shift : 0;
                checkerboardX = pos.x >> 1;
                w = pos.x < 0.0 || pos.x > gRectSizeMinusOne.x ? 0.0 : w; // "pos.x" clamping can make the sample "invalid"
            }
        #endif

            // Fetch data
        #if( REBLUR_SPATIAL_MODE == REBLUR_POST_BLUR )
            float zs = UnpackViewZ( gIn_ViewZ[ pos ] );
        #else
            float zs = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pos ) ] );
        #endif
            float3 Xvs = Geometry::ReconstructViewPosition( float2( pos + 0.5 ) * gRectSizeInv, gFrustum, zs, gOrthoMode );

            float materialIDs;
            float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
            Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

            // Weight
            float angle = Math::AcosApprox( dot( N, Ns.xyz ) );
            float NoX = dot( Nv, Xvs );

            w *= ComputeWeight( NoX, geometryWeightParams.x, geometryWeightParams.y );
            w *= CompareMaterials( materialID, materialIDs, gDiffMinMaterial );
            w *= ComputeWeight( angle, normalWeightParam, 0.0 );
            w = zs < gDenoisingRange ? w : 0.0; // |NoX| can be ~0 if "zs" is out of range

            REBLUR_TYPE s = gIn_Diff[ int2( checkerboardX, pos.y ) ];
            s = Denanify( w, s );

            w *= minHitDistWeight + ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y );

            // Accumulate
            sum += w;

            diff += s * w;
            #if( NRD_MODE == SH )
                float4 sh = gIn_DiffSh[ int2( checkerboardX, pos.y ) ];
                sh = Denanify( w, sh );
                diffSh += sh * w;
            #endif
        }

        float invSum = Math::PositiveRcp( sum );
        diff *= invSum;
    #if( NRD_MODE == SH )
        diffSh *= invSum;
    #endif

#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
    }

#if( NRD_SUPPORTS_CHECKERBOARD == 1 )
    // Checkerboard resolve ( if pre-pass failed )
    [branch]
    if( sum == 0.0 )
    {
        REBLUR_TYPE s0 = gIn_Diff[ checkerboardPos.xz ];
        REBLUR_TYPE s1 = gIn_Diff[ checkerboardPos.yz ];

        s0 = Denanify( wc.x, s0 );
        s1 = Denanify( wc.y, s1 );

        diff = s0 * wc.x + s1 * wc.y;

        #if( NRD_MODE == SH )
            float4 sh0 = gIn_DiffSh[ checkerboardPos.xz ];
            float4 sh1 = gIn_DiffSh[ checkerboardPos.yz ];

            sh0 = Denanify( wc.x, sh0 );
            sh1 = Denanify( wc.y, sh1 );

            diffSh = sh0 * wc.x + sh1 * wc.y;
        #endif
    }
#endif
#endif

    // Output
    gOut_Diff[ pixelPos ] = diff;
    #if( NRD_MODE == SH )
        gOut_DiffSh[ pixelPos ] = diffSh;
    #endif
}

#undef POISSON_SAMPLE_NUM
#undef POISSON_SAMPLES
