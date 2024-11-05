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
    float smc = GetSpecMagicCurve( roughness );

#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
    if( gSpecPrepassBlurRadius != 0.0 )
    {
        STL::Rng::Hash::Initialize( pixelPos, gFrameIndex );

        float specNonLinearAccumSpeed = REBLUR_PRE_BLUR_NON_LINEAR_ACCUM_SPEED;
#endif

        float fractionScale = 1.0;
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        fractionScale = REBLUR_PRE_BLUR_FRACTION_SCALE;
    #elif( REBLUR_SPATIAL_MODE == REBLUR_BLUR )
        float radiusScale = 1.0;
        fractionScale = REBLUR_BLUR_FRACTION_SCALE;
    #elif( REBLUR_SPATIAL_MODE == REBLUR_POST_BLUR )
        float radiusScale = REBLUR_POST_BLUR_RADIUS_SCALE;
        fractionScale = REBLUR_POST_BLUR_FRACTION_SCALE;
    #endif

        float lobeAngleFractionScale = saturate( gLobeAngleFraction * fractionScale );
        float roughnessFractionScale = saturate( gRoughnessFraction * fractionScale );

        float hitDist = ExtractHitDist( spec ) * _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness );

        // Min blur radius
        float4 Dv = STL::ImportanceSampling::GetSpecularDominantDirection( Nv, Vv, roughness, STL_SPECULAR_DOMINANT_DIRECTION_G2 );
        float NoD = abs( dot( Nv, Dv.xyz ) );
        float lobeTanHalfAngle = STL::ImportanceSampling::GetSpecularLobeTanHalfAngle( roughness );
        float lobeRadius = hitDist * NoD * lobeTanHalfAngle;
        float minBlurRadius = lobeRadius / PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ + hitDist * Dv.w );

        // Hit distance factor ( tests 76, 95, 120 )
        // TODO: reduce "hitDistFactor" influence if reprojection confidence is low?
        // TODO: if luminance stoppers are used, blur radius should depend less on "hitDistFactor"
        float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
        float hitDistFactor = GetHitDistFactor( hitDist * NoD, frustumSize );

        // Blur radius
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        float minHitDist = hitDist == 0.0 ? NRD_INF : hitDist;

        // Blur radius - main
        float blurRadius = smc * gSpecPrepassBlurRadius;
        blurRadius *= hitDistFactor;
        blurRadius = min( blurRadius, minBlurRadius );
    #else
        // IMPORTANT: keep an eye on tests:
        // - 51 and 128: outlines without TAA
        // - 81 and 117: cleanness in disoccluded regions
        float boost = 1.0 - GetFadeBasedOnAccumulatedFrames( data1.z );
        boost *= 1.0 - STL::BRDF::Pow5( NoV );
        boost *= smc;

        float specNonLinearAccumSpeed = 1.0 / ( 1.0 + REBLUR_SAMPLES_PER_FRAME * ( 1.0 - boost ) * data1.z );

        // Tests 144, 145, 150, 153, 23e
        float hitDistFactorRelaxedByError = lerp( hitDistFactor, 1.0, data1.w );
        float hitDistFactorAdditionallyRelaxedByRoughness = lerp( 1.0, hitDistFactorRelaxedByError, roughness );

        // Blur radius - main
        float blurRadius = smc * gMaxBlurRadius * ( 1.0 + 2.0 * boost ) / 3.0;
        blurRadius *= lerp( hitDistFactorRelaxedByError, hitDistFactorAdditionallyRelaxedByRoughness, specNonLinearAccumSpeed );
        blurRadius = min( blurRadius, minBlurRadius );

        // Blur radius - addition to avoid underblurring
        blurRadius += gMinBlurRadius * smc; // TODO: a source of contact detail loss

        // Blur radius - scaling
        blurRadius *= radiusScale;
    #endif

        // Weights
        float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, specNonLinearAccumSpeed );
        float normalWeightParams = GetNormalWeightParams( specNonLinearAccumSpeed, lobeAngleFractionScale, roughness );
        float2 roughnessWeightParams = GetRoughnessWeightParams( roughness, roughnessFractionScale );
        float3 px = float3( geometryWeightParams.x, normalWeightParams, roughnessWeightParams.x );
        float3 py = float3( geometryWeightParams.y, 0.0, roughnessWeightParams.y );

        float2 hitDistanceWeightParams = GetHitDistanceWeightParams( ExtractHitDist( spec ), specNonLinearAccumSpeed, roughness );
        float minHitDistWeight = REBLUR_HIT_DIST_MIN_WEIGHT( smc ) * fractionScale;

        // Sampling
        float2x3 TvBv = GetKernelBasis( Dv.xyz, Nv, NoD, roughness, specNonLinearAccumSpeed );

        float worldRadius = PixelRadiusToWorld( gUnproject, gOrthoMode, blurRadius, viewZ );
        TvBv[ 0 ] *= worldRadius;
        TvBv[ 1 ] *= worldRadius;

        [unroll]
        for( uint n = 0; n < POISSON_SAMPLE_NUM; n++ )
        {
            float3 offset = POISSON_SAMPLES( n );

            // Sample coordinates
        #if( REBLUR_USE_SCREEN_SPACE_SAMPLING == 1 || REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            float2 uv = pixelUv + STL::Geometry::RotateVector( rotator, offset.xy ) * gRectSizeInv * blurRadius;
        #else
            float2 uv = GetKernelSampleCoordinates( gViewToClip, offset, Xv, TvBv[ 0 ], TvBv[ 1 ], rotator );
        #endif

            // Snap to the pixel center!
            uv = ( floor( uv * gRectSize ) + 0.5 ) * gRectSizeInv;

            // Apply checkerboard shift
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            uv = ApplyCheckerboardShift( uv * gRectSize, gSpecCheckerboard, n, gFrameIndex ) * gRectSizeInv;
        #endif

            // Texture coordinates
            float2 uvScaled = ClampUvToViewport( uv );
            float2 checkerboardUvScaled = uvScaled;
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            if( gSpecCheckerboard != 2 )
                checkerboardUvScaled.x *= 0.5;
        #endif

            // Fetch data
        #if( REBLUR_SPATIAL_MODE == REBLUR_POST_BLUR )
            float zs = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, uvScaled, 0 ) );
        #else
            float zs = abs( gIn_ViewZ.SampleLevel( gNearestClamp, WithRectOffset( uvScaled ), 0 ) );
        #endif

            float3 Xvs = STL::Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

            float materialIDs;
            float4 Ns = gIn_Normal_Roughness.SampleLevel( gNearestClamp, WithRectOffset( uvScaled ), 0 );
            Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

            // Weight
            float w = IsInScreenNearest( uv );
            w *= float( zs < gDenoisingRange );
            w *= CompareMaterials( materialID, materialIDs, gSpecMaterialMask );

            float3 x;
            x.x = dot( Nv, Xvs );
            x.y = STL::Math::AcosApprox( dot( N, Ns.xyz ) );
            x.z = Ns.w;
            x = ComputeWeight( x, px, py );
            w *= x.x * x.y * x.z;

            REBLUR_TYPE s = gIn_Spec.SampleLevel( gNearestClamp, checkerboardUvScaled, 0 );
            s = Denanify( w, s );

        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            // Min hit distance for tracking, ignoring 0 values ( which still can be produced by VNDF sampling )
            // TODO: if trimming is off min hitDist can come from samples with very low probabilities, it worsens reprojection
            float hs = ExtractHitDist( s ) * _REBLUR_GetHitDistanceNormalization( zs, gHitDistParams, Ns.w );
            float d = length( Xvs - Xv ) + NRD_EPS;
            float geometryWeight = w * saturate( hs / d );
            if( STL::Rng::Hash::GetFloat( ) < geometryWeight )
                minHitDist = min( minHitDist, hs );

            // In rare cases, when bright samples are so sparse that any bright neighbors can't be reached,
            // pre-pass transforms a standalone bright pixel into a standalone bright blob, worsening the
            // situation. Despite that it's a problem of sampling, the denoiser needs to handle it somehow
            // on its side too. Diffuse pre-pass can be just disabled, but for specular it's still needed
            // to find an optimal hit distance for tracking.
            w *= gUsePrepassNotOnlyForSpecularMotionEstimation; // TODO: is there a better solution?

            // Decrease weight for samples that most likely are very close to reflection contact which should not be blurred
            float t = hs / ( d + hitDist );
            w *= lerp( saturate( t ), 1.0, STL::Math::LinearStep( 0.5, 1.0, roughness ) );
        #endif
            w *= lerp( minHitDistWeight, 1.0, ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y ) );
            w *= GetGaussianWeight( offset.z );

            // Accumulate
            sum += w;

            spec += s * w;
            #ifdef REBLUR_SH
                float4 sh = gIn_SpecSh.SampleLevel( gNearestClamp, checkerboardUvScaled, 0 );
                sh = Denanify( w, sh );
                specSh.xyz += sh.xyz * w; // see TA
            #endif
        }

        float invSum = STL::Math::PositiveRcp( sum );
        spec *= invSum;
        #ifdef REBLUR_SH
            specSh.xyz *= invSum;
        #endif

#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        // Output
        gOut_Spec_HitDistForTracking[ pixelPos ] = minHitDist == NRD_INF ? 0.0 : minHitDist; // TODO: lerp to hitT at center based on NoV or NoD?
    }

    // Checkerboard resolve ( if pre-pass failed )
    [branch]
    if( sum == 0.0 )
    {
        REBLUR_TYPE s0 = gIn_Spec[ checkerboardPos.xz ];
        REBLUR_TYPE s1 = gIn_Spec[ checkerboardPos.yz ];

        s0 = Denanify( wc.x, s0 );
        s1 = Denanify( wc.y, s1 );

        spec = s0 * wc.x + s1 * wc.y;

        #ifdef REBLUR_SH
            float4 sh0 = gIn_SpecSh[ checkerboardPos.xz ];
            float4 sh1 = gIn_SpecSh[ checkerboardPos.yz ];

            sh0 = Denanify( wc.x, sh0 );
            sh1 = Denanify( wc.y, sh1 );

            specSh = sh0 * wc.x + sh1 * wc.y;
        #endif
    }
#endif

    // Output
    gOut_Spec[ pixelPos ] = spec;
    #ifdef REBLUR_SH
        gOut_SpecSh[ pixelPos ] = specSh;
    #endif
}

#undef POISSON_SAMPLE_NUM
#undef POISSON_SAMPLES
