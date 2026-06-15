/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef REBLUR_SPATIAL_PASS
    #error REBLUR_SPATIAL_PASS must be defined!
#endif

#ifndef REBLUR_SPATIAL_LOBE
    #error REBLUR_SPATIAL_LOBE must be defined!
#endif

#ifndef MAX_BLUR_RADIUS
    #define MAX_BLUR_RADIUS             gMaxBlurRadius
#endif

#if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
    #define POISSON_SAMPLE_NUM          REBLUR_PRE_PASS_POISSON_SAMPLE_NUM
    #define POISSON_SAMPLES( i )        REBLUR_PRE_PASS_POISSON_SAMPLES( i )
#else
    #define POISSON_SAMPLE_NUM          REBLUR_POISSON_SAMPLE_NUM
    #define POISSON_SAMPLES( i )        REBLUR_POISSON_SAMPLES( i )
#endif

#if( REBLUR_SPATIAL_LOBE == REBLUR_DIFF )
    #define USE_SCREEN_SPACE            REBLUR_USE_SCREEN_SPACE_SAMPLING_FOR_DIFFUSE
    #define NON_LINEAR_ACCUM_SPEED      nonLinearAccumSpeed.x
    #define ACCUM_SPEED                 data1.x
    #define CHECKERBOARD                gDiffCheckerboard
    #define MIN_MATERIAL                gDiffMinMaterial
    #define ROUGHNESS                   1.0
    #define INPUT                       gIn_Diff
    #define INPUT_SH                    gIn_DiffSh
    #define OUTPUT                      gOut_Diff
    #define OUTPUT_SH                   gOut_DiffSh
    #define OUTPUT_COPY                 gOut_DiffCopy
    #define OUTPUT_SH_COPY              gOut_DiffShCopy
#else
    #define USE_SCREEN_SPACE            REBLUR_USE_SCREEN_SPACE_SAMPLING_FOR_SPECULAR
    #define NON_LINEAR_ACCUM_SPEED      nonLinearAccumSpeed.y
    #define ACCUM_SPEED                 data1.y
    #define CHECKERBOARD                gSpecCheckerboard
    #define MIN_MATERIAL                gSpecMinMaterial
    #define ROUGHNESS                   roughness
    #define INPUT                       gIn_Spec
    #define INPUT_SH                    gIn_SpecSh
    #define OUTPUT                      gOut_Spec
    #define OUTPUT_SH                   gOut_SpecSh
    #define OUTPUT_COPY                 gOut_SpecCopy
    #define OUTPUT_SH_COPY              gOut_SpecShCopy
#endif

{
    uint2 pos = pixelPos;
#if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
    pos.x >>= CHECKERBOARD == 2 ? 0 : 1;
#endif

    float sum = 1.0;
    REBLUR_TYPE result = INPUT[ pos ];
    #if( NRD_MODE == SH )
        REBLUR_SH_TYPE resultSh = INPUT_SH[ pos ];
    #endif

#if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
    #if( NRD_SUPPORTS_CHECKERBOARD == 1 )
        if( CHECKERBOARD != 2 && checkerboard != CHECKERBOARD )
        {
            sum = 0;
            result = 0;
            #if( NRD_MODE == SH )
                resultSh = 0;
            #endif
        }
    #endif

    if( MAX_BLUR_RADIUS != 0.0 )
    {
    #if( REBLUR_SPATIAL_LOBE == REBLUR_SPEC )
        Rng::Hash::Initialize( pixelPos, gFrameIndex );
    #endif
#endif

        // Scales ( A-trous-like behavior )
    #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
        float radiusScale = REBLUR_PRE_PASS_RADIUS_SCALE;
        float fractionScale = REBLUR_PRE_PASS_FRACTION_SCALE;
    #elif( REBLUR_SPATIAL_PASS == REBLUR_BLUR )
        float radiusScale = REBLUR_BLUR_RADIUS_SCALE;
        float fractionScale = REBLUR_BLUR_FRACTION_SCALE;
    #elif( REBLUR_SPATIAL_PASS == REBLUR_POST_BLUR )
        float radiusScale = REBLUR_POST_BLUR_RADIUS_SCALE;
        float fractionScale = REBLUR_POST_BLUR_FRACTION_SCALE;
    #endif

        // Hit distance factor ( tests 53, 76, 95, 120 )
        float4 Dv = ImportanceSampling::GetSpecularDominantDirection( Nv, Vv, ROUGHNESS, ML_SPECULAR_DOMINANT_DIRECTION_G2 );
        float NoD = abs( dot( Nv, Dv.xyz ) );
        float smc = GetSpecMagicCurve( ROUGHNESS, 0.5 ); // 0.5 matches previously used "sqrt( ROUGHNESS )" and fixes "roughnes < 0.1"

        float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistSettings.xyz, ROUGHNESS );
        float hitDist = ExtractHitDist( result ) * hitDistScale;
        float hitDistFactor = GetHitDistFactor( hitDist, frustumSize ); // using "hitDist * NoD" worsens denoising at glancing angles

        // Blur radius
    #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
        float areaFactor = hitDistFactor;
    #else
        float areaFactor = hitDistFactor * NON_LINEAR_ACCUM_SPEED;
    #endif

        float blurRadius = radiusScale * Math::Sqrt01( areaFactor ); // "areaFactor" affects area, not radius
        blurRadius = saturate( blurRadius ) * MAX_BLUR_RADIUS * smc; // previously "sqrt( ROUGHNESS )" was used instead of "smc", it's wrong for "ROUGHNESS < 0.1"
        blurRadius = max( blurRadius, gMinBlurRadius * smc );

    #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS && REBLUR_SPATIAL_LOBE == REBLUR_SPEC )
        // For "pre pass" we want to stay "in-lobe" because "MAX_BLUR_RADIUS" can be large
        float lobeTanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle( ROUGHNESS, REBLUR_MAX_PERCENT_OF_LOBE_VOLUME_FOR_PRE_PASS );
        float worldLobeRadius = hitDist * NoD * lobeTanHalfAngle;
        float lobeRadius = worldLobeRadius / PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ + hitDist * Dv.w );

        blurRadius = min( blurRadius, lobeRadius );
    #endif

        // Weights
        float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv );
        float normalWeightParam = GetNormalWeightParam( NON_LINEAR_ACCUM_SPEED, gLobeAngleFraction, ROUGHNESS ) / fractionScale;
        float2 roughnessWeightParams = GetRoughnessWeightParams( ROUGHNESS, gRoughnessFraction * fractionScale );

        float2 hitDistanceWeightParams = GetHitDistanceWeightParams( ExtractHitDist( result ), NON_LINEAR_ACCUM_SPEED );
        float minHitDistWeight = gMinHitDistanceWeight * fractionScale * smc;

        // Gradually reduce "minHitDistWeight" to squeeze more shadow details
    #if( REBLUR_SPATIAL_PASS != REBLUR_PRE_PASS && NRD_MODE != OCCLUSION && NRD_MODE != DO )
        minHitDistWeight *= NON_LINEAR_ACCUM_SPEED; // this is valid only for non-occlusion modes!
    #endif

        // Screen-space settings
    #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS || USE_SCREEN_SPACE == 1 )
        #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
            float2 skew = 1.0; // seems that uniform screen space sampling in pre-pass breaks residual boiling better
        #else
            #if( REBLUR_SPATIAL_LOBE == REBLUR_DIFF )
                float2 skew = lerp( 1.0 - abs( Nv.xy ), 1.0, NoV );
                skew /= max( skew.x, skew.y );
            #else
                float2 skew = 1.0; // TODO: improve?
            #endif
        #endif
        skew *= gRectSizeInv * blurRadius;

        float4 scaledRotator = Geometry::ScaleRotator( rotator, skew );
    #else
        // World-space settings
        #if( REBLUR_SPATIAL_LOBE == REBLUR_DIFF )
            float skewFactor = 1.0;
            float3 bentDv = Nv;
        #else
            float bentFactor = sqrt( hitDistFactor );

            float skewFactor = lerp( 0.25 + 0.75 * ROUGHNESS, 1.0, NoD ); // TODO: tune max skewing factor?
            skewFactor = lerp( skewFactor, 1.0, NON_LINEAR_ACCUM_SPEED );
            skewFactor = lerp( 1.0, skewFactor, bentFactor );

            float3 bentDv = normalize( lerp( Nv.xyz, Dv.xyz, bentFactor ) );
        #endif

        float worldRadius = PixelRadiusToWorld( gUnproject, gOrthoMode, blurRadius, viewZ );

        float2x3 TvBv = GetKernelBasis( bentDv, Nv );
        TvBv[ 0 ] *= worldRadius * skewFactor;
        TvBv[ 1 ] *= worldRadius / skewFactor;
    #endif

        // Sampling
        float hitDistForTracking = hitDist == 0.0 ? NRD_INF : hitDist;

        [unroll]
        for( uint n = 0; n < POISSON_SAMPLE_NUM; n++ )
        {
            float3 offset = POISSON_SAMPLES( n );

            // Sample coordinates
        #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS || USE_SCREEN_SPACE == 1 )
            float2 uv = pixelUv + Geometry::RotateVector( scaledRotator, offset.xy );
        #else
            float2 uv = GetKernelSampleCoordinates( gViewToClip, offset, Xv, TvBv[ 0 ], TvBv[ 1 ], rotator );
        #endif

            // Apply "mirror" to not waste taps going outside of the screen
            float2 mirrorUv = MirrorUv( uv );
            float w = any( uv != mirrorUv ) ? 1.0 : GetGaussianWeight( offset.z );

            // "uv" to "pos"
            int2 pos = mirrorUv * gRectSize;

            // Move to a "valid" pixel in checkerboard mode
            int checkerboardX = pos.x;
            #if( NRD_SUPPORTS_CHECKERBOARD == 1 && REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
                if( CHECKERBOARD != 2 )
                {
                    int shift = ( ( n & 0x1 ) == 0 ) ? -1 : 1;
                    pos.x += Sequence::CheckerBoard( pos, gFrameIndex ) != CHECKERBOARD ? shift : 0;
                    checkerboardX = pos.x >> 1;
                    w = pos.x < 0.0 || pos.x > gRectSizeMinusOne.x ? 0.0 : w; // "pos.x" clamping can make the sample "invalid"
                }
            #endif

            // Fetch data
        #if( REBLUR_SPATIAL_PASS == REBLUR_POST_BLUR )
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

            w *= CompareMaterials( materialID, materialIDs, MIN_MATERIAL );
            w *= ComputeWeight( angle, normalWeightParam, 0.0 );
        #if( REBLUR_SPATIAL_LOBE == REBLUR_SPEC )
            w *= ComputeWeight( Ns.w, roughnessWeightParams.x, roughnessWeightParams.y );
        #endif
            w = ApplyGeometryWeightLast( w, zs, NoX, geometryWeightParams );

            REBLUR_TYPE s = INPUT[ int2( checkerboardX, pos.y ) ];
            s = Denanify( w, s );

        #if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS && REBLUR_SPATIAL_LOBE == REBLUR_SPEC )
            // Min hit distance for tracking, ignoring 0 values ( which still can be produced by VNDF sampling )
            // TODO: if trimming is off min hitDist can come from samples with very low probabilities, it worsens reprojection
            float hs = ExtractHitDist( s ) * _REBLUR_GetHitDistanceNormalization( zs, gHitDistSettings.xyz, Ns.w );
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
            w *= lerp( saturate( t ), 1.0, Math::LinearStep( 0.5, 1.0, ROUGHNESS ) );
        #endif

            w *= minHitDistWeight + ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y );

            // Accumulate
            sum += w;

            result += s * w;
            #if( NRD_MODE == SH )
                REBLUR_SH_TYPE sh = INPUT_SH[ int2( checkerboardX, pos.y ) ];
                sh = Denanify( w, sh );

                resultSh += sh * w;
            #endif
        }

        float invSum = Math::PositiveRcp( sum );
        result *= invSum;
        #if( NRD_MODE == SH )
            resultSh *= invSum;
        #endif

        // Keep hit distances unprocessed to avoid bias and self-inference
    #if( REBLUR_SPATIAL_PASS != REBLUR_PRE_PASS && NRD_MODE != OCCLUSION && NRD_MODE != DO )
        result.w = hitDist / hitDistScale;
    #endif

#if( REBLUR_SPATIAL_PASS == REBLUR_PRE_PASS )
    #if( REBLUR_SPATIAL_LOBE == REBLUR_SPEC )
        // Output
        gOut_SpecHitDistForTracking[ pixelPos ] = hitDistForTracking == NRD_INF ? 0.0 : hitDistForTracking;
    #endif
    }

    #if( NRD_SUPPORTS_CHECKERBOARD == 1 )
        // Checkerboard resolve ( if pre-pass failed )
        [branch]
        if( sum == 0.0 )
        {
            REBLUR_TYPE s0 = INPUT[ checkerboardPos.xz ];
            REBLUR_TYPE s1 = INPUT[ checkerboardPos.yz ];

            s0 = Denanify( wc.x, s0 );
            s1 = Denanify( wc.y, s1 );

            result = s0 * wc.x + s1 * wc.y;

            #if( NRD_MODE == SH )
                REBLUR_SH_TYPE sh0 = INPUT_SH[ checkerboardPos.xz ];
                REBLUR_SH_TYPE sh1 = INPUT_SH[ checkerboardPos.yz ];

                sh0 = Denanify( wc.x, sh0 );
                sh1 = Denanify( wc.y, sh1 );

                resultSh = sh0 * wc.x + sh1 * wc.y;
            #endif
        }
    #endif
#endif

    // Output
    OUTPUT[ pixelPos ] = result;
    #if( NRD_MODE == SH )
        OUTPUT_SH[ pixelPos ] = resultSh;
    #endif

#if( REBLUR_SPATIAL_PASS == REBLUR_POST_BLUR && TEMPORAL_STABILIZATION == 0 )
    #if( NRD_MODE != OCCLUSION && NRD_MODE != DO )
        result.w = gReturnHistoryLengthInsteadOfOcclusion ? ACCUM_SPEED : result.w;
    #endif

    OUTPUT_COPY[ pixelPos ] = result;
    #if( NRD_MODE == SH )
        OUTPUT_SH_COPY[ pixelPos ] = resultSh;
    #endif
#endif
}

#undef POISSON_SAMPLE_NUM
#undef POISSON_SAMPLES
#undef USE_SCREEN_SPACE
#undef NON_LINEAR_ACCUM_SPEED
#undef ACCUM_SPEED
#undef CHECKERBOARD
#undef MIN_MATERIAL
#undef ROUGHNESS
#undef INPUT
#undef INPUT_SH
#undef OUTPUT
#undef OUTPUT_SH
#undef OUTPUT_COPY
#undef OUTPUT_SH_COPY

#undef REBLUR_SPATIAL_LOBE
#undef MAX_BLUR_RADIUS
