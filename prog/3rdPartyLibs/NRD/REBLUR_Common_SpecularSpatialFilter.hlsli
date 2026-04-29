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
        Rng::Hash::Initialize( pixelPos, gFrameIndex );
#endif
        float specNonLinearAccumSpeed = nonLinearAccumSpeed.y;

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

        // Hit distance factor ( tests 76, 95, 120 )
        float4 Dv = ImportanceSampling::GetSpecularDominantDirection( Nv, Vv, roughness, ML_SPECULAR_DOMINANT_DIRECTION_G2 );
        float NoD = abs( dot( Nv, Dv.xyz ) );
        float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistSettings, roughness );
        float hitDist = ExtractHitDist( spec ) * hitDistScale;
        float hitDistFactor = GetHitDistFactor( hitDist, frustumSize ); // using "hitDist * NoD" worsens denoising at glancing angles

        // Blur radius
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        float hitDistForTracking = hitDist == 0.0 ? NRD_INF : hitDist;

        float blurRadius = gSpecPrepassBlurRadius;
        float areaFactor = roughness * hitDistFactor;
    #else
        float blurRadius = gMaxBlurRadius;
        float areaFactor = roughness * hitDistFactor * specNonLinearAccumSpeed;
    #endif

        blurRadius *= Math::Sqrt01( areaFactor ); // "areaFactor" affects area, not radius

    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        // The "area factor" idea works well and offers linear progression of blur radiuses over time, but it makes
        // the blur radius depend on "sqrt( roughness )" not "roughness". It hurts the pre pass, which can be very wide.
        // The code below fixes completely wrong big blur radiuses.
        float lobeTanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle( roughness, REBLUR_MAX_PERCENT_OF_LOBE_VOLUME_FOR_PRE_PASS );
        float lobeRadius = hitDist * NoD * lobeTanHalfAngle;
        float minBlurRadius = lobeRadius / PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ + hitDist * Dv.w );

        blurRadius = min( blurRadius, minBlurRadius );
    #endif

        // Blur radius - scale
        blurRadius *= radiusScale;

        // Blur radius - addition to avoid underblurring
        blurRadius = max( blurRadius, gMinBlurRadius * smc );

        // Weights
        float roughnessFractionScaled = saturate( gRoughnessFraction * fractionScale );
        float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv );
        float normalWeightParam = GetNormalWeightParam( specNonLinearAccumSpeed, gLobeAngleFraction, roughness ) / fractionScale;
        float2 roughnessWeightParams = GetRoughnessWeightParams( roughness, roughnessFractionScaled );

        float2 hitDistanceWeightParams = GetHitDistanceWeightParams( ExtractHitDist( spec ), specNonLinearAccumSpeed, roughness ); // TODO: what if hitT == 0?
        float minHitDistWeight = gMinHitDistanceWeight * fractionScale * smc;

        // ( Optional ) Gradually reduce "minHitDistWeight" to preserve contact details
    #if( REBLUR_SPATIAL_MODE != REBLUR_PRE_BLUR && NRD_MODE != OCCLUSION )
        minHitDistWeight *= specNonLinearAccumSpeed;
    #endif

        // Screen-space settings
    #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR || REBLUR_USE_SCREEN_SPACE_SAMPLING_FOR_SPECULAR == 1 )
        float2 skew = 1.0; // TODO: even for rough specular diffuse-like behavior is undesired ( especially visible in performance mode )
        skew *= gRectSizeInv * blurRadius;

        float4 scaledRotator = Geometry::ScaleRotator( rotator, skew );
    #else
        // World-space settings
        float bentFactor = sqrt( hitDistFactor );

        float skewFactor = lerp( 0.25 + 0.75 * roughness, 1.0, NoD ); // TODO: tune max skewing factor?
        skewFactor = lerp( skewFactor, 1.0, specNonLinearAccumSpeed );
        skewFactor = lerp( 1.0, skewFactor, bentFactor );

        float3 bentDv = normalize( lerp( Nv.xyz, Dv.xyz, bentFactor ) );
        float2x3 TvBv = GetKernelBasis( bentDv, Nv );

        float worldRadius = PixelRadiusToWorld( gUnproject, gOrthoMode, blurRadius, viewZ );
        TvBv[ 0 ] *= worldRadius * skewFactor;
        TvBv[ 1 ] *= worldRadius / skewFactor;
    #endif

        // Sampling
        [unroll]
        for( uint n = 0; n < POISSON_SAMPLE_NUM; n++ )
        {
            float3 offset = POISSON_SAMPLES( n );

            // Sample coordinates
        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR || REBLUR_USE_SCREEN_SPACE_SAMPLING_FOR_SPECULAR == 1 )
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
            if( gSpecCheckerboard != 2 )
            {
                int shift = ( ( n & 0x1 ) == 0 ) ? -1 : 1;
                pos.x += Sequence::CheckerBoard( pos, gFrameIndex ) != gSpecCheckerboard ? shift : 0;
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
            w *= CompareMaterials( materialID, materialIDs, gSpecMinMaterial );
            w *= ComputeWeight( angle, normalWeightParam, 0.0 );
            w *= ComputeWeight( Ns.w, roughnessWeightParams.x, roughnessWeightParams.y );
            w = zs < gDenoisingRange ? w : 0.0; // |NoX| can be ~0 if "zs" is out of range

            REBLUR_TYPE s = gIn_Spec[ int2( checkerboardX, pos.y ) ];
            s = Denanify( w, s );

        #if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
            // Min hit distance for tracking, ignoring 0 values ( which still can be produced by VNDF sampling )
            // TODO: if trimming is off min hitDist can come from samples with very low probabilities, it worsens reprojection
            float hs = ExtractHitDist( s ) * _REBLUR_GetHitDistanceNormalization( zs, gHitDistSettings, Ns.w );
            float geometryWeight = w * NoV * float( hs != 0.0 );
            if( Rng::Hash::GetFloat( ) < geometryWeight )
                hitDistForTracking = min( hitDistForTracking, hs );

            // In rare cases, when bright samples are so sparse that any bright neighbors can't be reached,
            // pre-pass transforms a standalone bright pixel into a standalone bright blob, worsening the
            // situation. Despite that it's a problem of sampling, the denoiser needs to handle it somehow
            // on its side too. Diffuse pre-pass can be just disabled, but for specular it's still needed
            // to find an optimal hit distance for tracking.
            w *= gUsePrepassNotOnlyForSpecularMotionEstimation; // TODO: is there a better solution?

            // Decrease weight for samples that most likely are very close to reflection contact which should not be blurred
            float d = length( Xvs - Xv ) + NRD_EPS;
            float t = hs / ( d + hitDist );
            w *= lerp( saturate( t ), 1.0, Math::LinearStep( 0.5, 1.0, roughness ) );
        #endif

            w *= minHitDistWeight + ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y );

            // Accumulate
            sum += w;

            spec += s * w;
            #if( NRD_MODE == SH )
                float4 sh = gIn_SpecSh[ int2( checkerboardX, pos.y ) ];
                sh = Denanify( w, sh );
                specSh.xyz += sh.xyz * w;
            #endif
        }

        float invSum = Math::PositiveRcp( sum );
        spec *= invSum;
        #if( NRD_MODE == SH )
            specSh.xyz *= invSum;
        #endif

#if( REBLUR_SPATIAL_MODE == REBLUR_PRE_BLUR )
        // Output
        gOut_SpecHitDistForTracking[ pixelPos ] = hitDistForTracking == NRD_INF ? 0.0 : hitDistForTracking;
    }

#if( NRD_SUPPORTS_CHECKERBOARD == 1 )
    // Checkerboard resolve ( if pre-pass failed )
    [branch]
    if( sum == 0.0 )
    {
        REBLUR_TYPE s0 = gIn_Spec[ checkerboardPos.xz ];
        REBLUR_TYPE s1 = gIn_Spec[ checkerboardPos.yz ];

        s0 = Denanify( wc.x, s0 );
        s1 = Denanify( wc.y, s1 );

        spec = s0 * wc.x + s1 * wc.y;

        #if( NRD_MODE == SH )
            float4 sh0 = gIn_SpecSh[ checkerboardPos.xz ];
            float4 sh1 = gIn_SpecSh[ checkerboardPos.yz ];

            sh0 = Denanify( wc.x, sh0 );
            sh1 = Denanify( wc.y, sh1 );

            specSh = sh0 * wc.x + sh1 * wc.y;
        #endif
    }
#endif
#endif

    // Output
    gOut_Spec[ pixelPos ] = spec;
    #if( NRD_MODE == SH )
        gOut_SpecSh[ pixelPos ] = specSh;
    #endif
}

#undef POISSON_SAMPLE_NUM
#undef POISSON_SAMPLES
