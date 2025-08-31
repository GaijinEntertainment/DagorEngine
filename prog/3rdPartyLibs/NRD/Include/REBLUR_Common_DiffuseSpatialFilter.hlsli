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
        float diffNonLinearAccumSpeed = REBLUR_PRE_BLUR_NON_LINEAR_ACCUM_SPEED;
#endif

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
        float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, 1.0 );
        float hitDist = ExtractHitDist( diff ) * hitDistScale;
        float hitDistFactor = GetHitDistFactor( hitDist, frustumSize );

        // Blur radius
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        float blurRadius = gDiffPrepassBlurRadius;
        float areaFactor = hitDistFactor;
    #else
        float boost = 1.0 - GetFadeBasedOnAccumulatedFrames( data1.x ); // TODO: still needed?
        boost *= 1.0 - BRDF::Pow5( NoV );

        float diffNonLinearAccumSpeed = 1.0 / ( 1.0 + REBLUR_SAMPLES_PER_FRAME * ( 1.0 - boost ) * data1.x );

        float blurRadius = gMaxBlurRadius;
        float areaFactor = hitDistFactor * diffNonLinearAccumSpeed;
    #endif

        blurRadius *= Math::Sqrt01( areaFactor ); // "areaFactor" affects area, not radius

        // Blur radius - scale
        blurRadius *= radiusScale;

        // Blur radius - addition to avoid underblurring
        blurRadius = max( blurRadius, gMinBlurRadius );

        // Weights
        float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, diffNonLinearAccumSpeed );
        float normalWeightParam = GetNormalWeightParam( diffNonLinearAccumSpeed, gLobeAngleFraction ) / fractionScale;

        float2 hitDistanceWeightParams = GetHitDistanceWeightParams( ExtractHitDist( diff ), diffNonLinearAccumSpeed ); // TODO: what if hitT == 0?
        float minHitDistWeight = gMinHitDistanceWeight * fractionScale;

        // ( Optional ) Gradually reduce "minHitDistWeight" to preserve contact details
    #if( REBLUR_SPATIAL_MODE != REBLUR_PRE_BLUR && !defined( REBLUR_OCCLUSION ) )
        minHitDistWeight *= sqrt( diffNonLinearAccumSpeed );
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

            // Snap to the pixel center!
            uv = floor( uv * gRectSize ) + 0.5;

            // Apply checkerboard shift
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            uv = ApplyCheckerboardShift( uv, gDiffCheckerboard, n, gFrameIndex );
        #endif

            // Texture coordinates
            uv *= gRectSizeInv;

            float2 uvScaled = ClampUvToViewport( uv );
            float2 checkerboardUvScaled = uvScaled;
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            if( gDiffCheckerboard != 2 )
                checkerboardUvScaled.x *= 0.5;
        #endif

            // Fetch data
        #if( REBLUR_SPATIAL_MODE == REBLUR_POST_BLUR )
            float zs = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, uvScaled, 0 ) );
        #else
            float zs = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, WithRectOffset( uvScaled ), 0 ) );
        #endif

            float materialIDs;
            float4 Ns = gIn_Normal_Roughness.SampleLevel( gNearestClamp, WithRectOffset( uvScaled ), 0 );
            Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

            // Weight
            float angle = Math::AcosApprox( dot( N, Ns.xyz ) );
            float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

            float w = IsInScreenNearest( uv );
            w *= ComputeWeight( dot( Nv, Xvs ), geometryWeightParams.x, geometryWeightParams.y );
            w *= CompareMaterials( materialID, materialIDs, gDiffMinMaterial );
            w *= ComputeWeight( angle, normalWeightParam, 0.0 );

            // Gaijin change:
            // The sample can be on a sky tile. See https://github.com/NVIDIA-RTX/NRD/issues/88
            float sampleIsSky = gIn_Tiles.SampleLevel( gNearestClamp , uvScaled , 0 ); 
            w *= 1 - sampleIsSky;

            REBLUR_TYPE s = gIn_Diff.SampleLevel( gNearestClamp, checkerboardUvScaled, 0 );
            s = Denanify( w, s );

            w *= lerp( minHitDistWeight, 1.0, ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y ) );
            w *= GetGaussianWeight( offset.z );

            // Accumulate
            sum += w;

            diff += s * w;
            #ifdef REBLUR_SH
                float4 sh = gIn_DiffSh.SampleLevel( gNearestClamp, checkerboardUvScaled, 0 );
                sh = Denanify( w, sh );
                diffSh += sh * w;
            #endif
        }

        float invSum = Math::PositiveRcp( sum );
        diff *= invSum;
    #ifdef REBLUR_SH
        diffSh *= invSum;
    #endif

#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
    }

    // Checkerboard resolve ( if pre-pass failed )
    [branch]
    if( sum == 0.0 )
    {
        REBLUR_TYPE s0 = gIn_Diff[ checkerboardPos.xz ];
        REBLUR_TYPE s1 = gIn_Diff[ checkerboardPos.yz ];

        s0 = Denanify( wc.x, s0 );
        s1 = Denanify( wc.y, s1 );

        diff = s0 * wc.x + s1 * wc.y;

        #ifdef REBLUR_SH
            float4 sh0 = gIn_DiffSh[ checkerboardPos.xz ];
            float4 sh1 = gIn_DiffSh[ checkerboardPos.yz ];

            sh0 = Denanify( wc.x, sh0 );
            sh1 = Denanify( wc.y, sh1 );

            diffSh = sh0 * wc.x + sh1 * wc.y;
        #endif
    }
#endif

    // Output
    gOut_Diff[ pixelPos ] = diff;
    #ifdef REBLUR_SH
        gOut_DiffSh[ pixelPos ] = diffSh;
    #endif

    #if( defined( REBLUR_NO_TEMPORAL_STABILIZATION ) && !defined( REBLUR_OCCLUSION ) )
        gOut_DiffCopy[ pixelPos ] = diff;
        #ifdef REBLUR_SH
            gOut_DiffShCopy[ pixelPos ] = diffSh;
        #endif
    #endif
}

#undef POISSON_SAMPLE_NUM
#undef POISSON_SAMPLES
