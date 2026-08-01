/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NRD.hlsli"
#include "ml.hlsli"

#include "REBLUR_Config.hlsli"
#include "REBLUR_HistoryFix.resources.hlsli"

#include "Common.hlsli"

#include "REBLUR_Common.hlsli"

groupshared float s_DiffLuma[ BUFFER_Y ][ BUFFER_X ];
groupshared float s_SpecLuma[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( globalPos ) ] );

    #if( NRD_DIFF )
        float diffFast = gIn_DiffFast[ globalPos ];
        s_DiffLuma[ sharedPos.y ][ sharedPos.x ] = !IsInDenoisingRange( viewZ ) ? REBLUR_INVALID : diffFast;
    #endif

    #if( NRD_SPEC )
        float specFast = gIn_SpecFast[ globalPos ];
        s_SpecLuma[ sharedPos.y ][ sharedPos.x ] = !IsInDenoisingRange( viewZ ) ? REBLUR_INVALID : specFast;
    #endif
}

// Tests 20, 23, 24, 27, 28, 54, 59, 65, 66, 76, 81, 98, 112, 117, 124, 126, 128, 134
// TODO: potentially do color clamping after reconstruction in a separate pass

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_REVERSED;

    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out ( quad uniform )
    if( isSky != 0.0 )
        return;

    // Blur stride
    REBLUR_DATA1_TYPE frameNum = UnpackData1( gIn_Data1[ pixelPos ] );

    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    frameNum = !IsInDenoisingRange( viewZ ) ? REBLUR_MAX_ACCUM_FRAME_NUM : frameNum; // less blur on "SKY" edges

    float2 stride = float2( frameNum < gHistoryFixFrameNum );
    #if( NRD_SUPPORTS_WAVE_INTRINSICS == 1 )
    {
        // IMPORTANT: the spec says: "these routines assume that flow control execution is uniform at least across the quad"
        // Adapt to neighbors if they are more stable
        float2 d10 = QuadReadAcrossX( stride );
        float2 d01 = QuadReadAcrossY( stride );

        float2 avg = ( d10 + d01 + stride ) / 3.0;
        stride = min( stride, avg );
    }
    #endif

    // Early out ( thread )
    if( !IsInDenoisingRange( viewZ ) || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Center data
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 Nv = Geometry::RotateVectorInverse( gViewToWorld, N );
    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );

    int2 smemPos = threadPos + NRD_BORDER;

    float invHistoryFixFrameNum = 1.0 / max( gHistoryFixFrameNum, NRD_EPS );
    float2 frameNumAvgNorm = saturate( frameNum * invHistoryFixFrameNum );

    stride /= 1.0 + 1.0; // to match RELAX, where "frameNum" after "TemporalAccumulation" is "1", not "0"
    stride *= 2.0 / REBLUR_HISTORY_FIX_FILTER_RADIUS; // preserve blur radius in pixels ( default blur radius is 2 taps )
    stride *= materialID == gHistoryFixAlternatePixelStrideMaterialID ? gHistoryFixAlternatePixelStride : gHistoryFixBasePixelStride;

    // Diffuse
    #if( NRD_DIFF )
    {
        REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
        #if( NRD_MODE == SH )
            REBLUR_SH_TYPE diffSh = gIn_DiffSh[ pixelPos ];
        #endif

        float diffNonLinearAccumSpeed = 1.0 / ( 1.0 + frameNum.x );

        float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistSettings.xyz, 1.0 );
        float hitDist = ExtractHitDist( diff ) * hitDistScale;
        float hitDistFactor = GetHitDistFactor( hitDist, frustumSize );
        hitDist = ExtractHitDist( diff );

        // Stride between taps
        stride.x *= lerp( 0.25 + 0.75 * Math::Sqrt01( hitDistFactor ), 1.0, diffNonLinearAccumSpeed ); // "hitDistFactor" is very noisy and breaks nice patterns
        stride.x = round( stride.x );

        // History reconstruction
        if( stride.x != 0.0 )
        {
            // Parameters
            float normalWeightParam = GetNormalWeightParam( diffNonLinearAccumSpeed, gLobeAngleFraction );
            float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv );
            float2 hitDistanceWeightParams = GetHitDistanceWeightParams( hitDist, diffNonLinearAccumSpeed );

            float sumd = 1.0 + frameNum.x;
            #if( REBLUR_PERFORMANCE_MODE == 1 )
                sumd = 1.0 + 1.0 / ( 1.0 + gMaxAccumulatedFrameNum ) - diffNonLinearAccumSpeed;
            #endif

            diff *= sumd;
            #if( NRD_MODE == SH )
                diffSh *= sumd;
            #endif

            [unroll]
            for( j = -REBLUR_HISTORY_FIX_FILTER_RADIUS; j <= REBLUR_HISTORY_FIX_FILTER_RADIUS; j++ )
            {
                [unroll]
                for( i = -REBLUR_HISTORY_FIX_FILTER_RADIUS; i <= REBLUR_HISTORY_FIX_FILTER_RADIUS; i++ )
                {
                    // Skip center
                    if( i == 0 && j == 0 )
                        continue;

                    // Skip corners
                    if( abs( i ) + abs( j ) == REBLUR_HISTORY_FIX_FILTER_RADIUS * 2 )
                        continue;

                    // Sample uv ( at the pixel center )
                    float2 uv = pixelUv + float2( i, j ) * stride.x * gRectSizeInv;

                    // Apply "mirror" to not waste taps going outside of the screen
                    uv = MirrorUv( uv );

                    // "uv" to "pos"
                    int2 pos = uv * gRectSize;

                    // Fetch data
                    float zs = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pos ) ] );
                    float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

                    float materialIDs;
                    float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
                    Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

                    // Weight
                    float angle = Math::AcosApprox( dot( Ns.xyz, N ) );
                    float NoX = dot( Nv, Xvs );

                    float w = CompareMaterials( materialID, materialIDs, gDiffMinMaterial );
                    w *= ComputeExponentialWeight( angle, normalWeightParam, 0.0 );
                    // gaussian weight is not needed

                    #if( REBLUR_PERFORMANCE_MODE == 0 )
                        w *= 1.0 + UnpackData1( gIn_Data1[ pos ] ).x;
                    #endif

                    w = ApplyGeometryWeightLast( w, zs, NoX, geometryWeightParams );

                    REBLUR_TYPE s = gIn_Diff[ pos ];
                    s = Denanify( w, s );

                    // A-trous weight
                    w *= ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y );

                    // Accumulate
                    sumd += w;

                    diff += s * w;
                    #if( NRD_MODE == SH )
                        REBLUR_SH_TYPE sh = gIn_DiffSh[ pos ];
                        sh = Denanify( w, sh );

                        diffSh += sh * w;
                    #endif
                }
            }

            sumd = Math::PositiveRcp( sumd );
            diff *= sumd;
            #if( NRD_MODE == SH )
                diffSh *= sumd;
            #endif
        }

        float diffLuma = GetLuma( diff );

        // Mix fast history with "fixed" normal history if normal history is not longer than fast
        float f = frameNumAvgNorm.x;

        float diffFastCenter = s_DiffLuma[ smemPos.y ][ smemPos.x ];
        diffFastCenter = lerp( diffLuma, diffFastCenter, f );

        gOut_DiffFast[ pixelPos ] = diffFastCenter;

        // Local variance
        float diffFastM1 = diffFastCenter;
        float diffFastM2 = diffFastCenter * diffFastCenter;
        float diffAntiFireflyM1 = 0;
        float diffAntiFireflyM2 = 0;

        [unroll]
        for( j = -NRD_BORDER; j <= NRD_BORDER; j++ )
        {
            [unroll]
            for( i = -NRD_BORDER; i <= NRD_BORDER; i++ )
            {
                if( i == 0 && j == 0 )
                    continue;

                int2 pos = smemPos + int2( i, j );

                float d = s_DiffLuma[ pos.y ][ pos.x ];
                d = d == REBLUR_INVALID ? diffFastCenter : d;

                // Variance in 5x5 for fast history
                if( abs( i ) <= REBLUR_FAST_HISTORY_CLAMPING_RADIUS && abs( j ) <= REBLUR_FAST_HISTORY_CLAMPING_RADIUS )
                {
                    diffFastM1 += d;
                    diffFastM2 += d * d;
                }

                // Variance in "NRD_BORDER x NRD_BORDER" skipping central 3x3 for anti-firefly
                if( NRD_SUPPORTS_ANTIFIREFLY && !( abs( i ) <= 1 && abs( j ) <= 1 ) )
                {
                    diffAntiFireflyM1 += d;
                    diffAntiFireflyM2 += d * d;
                }
            }
        }

        // Anti-firefly
        if( NRD_SUPPORTS_ANTIFIREFLY && gAntiFirefly )
        {
            float invNorm = 1.0 / ( ( NRD_BORDER * 2 + 1 ) * ( NRD_BORDER * 2 + 1 ) - 3 * 3 ); // -9 samples
            diffAntiFireflyM1 *= invNorm;
            diffAntiFireflyM2 *= invNorm;

            float diffAntiFireflySigma = GetStdDev( diffAntiFireflyM1, diffAntiFireflyM2 ) * REBLUR_ANTI_FIREFLY_SIGMA_SCALE;

            diffLuma = clamp( diffLuma, diffAntiFireflyM1 - diffAntiFireflySigma, diffAntiFireflyM1 + diffAntiFireflySigma );
        }

        // Clamp to fast history
        {
            float invNorm = 1.0 / ( ( REBLUR_FAST_HISTORY_CLAMPING_RADIUS * 2 + 1 ) * ( REBLUR_FAST_HISTORY_CLAMPING_RADIUS * 2 + 1 ) );
            diffFastM1 *= invNorm;
            diffFastM2 *= invNorm;

            float diffFastSigma = GetStdDev( diffFastM1, diffFastM2 ) * gFastHistoryClampingSigmaScale;
            float diffLumaClamped = clamp( diffLuma, diffFastM1 - diffFastSigma, diffFastM1 + diffFastSigma );

            diffLuma = lerp( diffLumaClamped, diffLuma, 1.0 / ( 1.0 + float( gMaxFastAccumulatedFrameNum < gMaxAccumulatedFrameNum ) * frameNum.x * 2.0 ) );
        }

        // Change luma
        #if( REBLUR_SHOW == REBLUR_SHOW_FAST_HISTORY )
            diffLuma = diffFastCenter;
        #endif

        diff = ChangeLuma( diff, diffLuma );
        #if( NRD_MODE == SH )
            diffSh *= GetLumaScale( length( diffSh ), diffLuma );
        #endif

        // Output
        gOut_Diff[ pixelPos ] = diff;
        #if( NRD_MODE == SH )
            gOut_DiffSh[ pixelPos ] = diffSh;
        #endif
    }
    #endif

    // Specular
    #if( NRD_SPEC )
    {
        REBLUR_TYPE spec = gIn_Spec[ pixelPos ];
        #if( NRD_MODE == SH )
            REBLUR_SH_TYPE specSh = gIn_SpecSh[ pixelPos ];
        #endif

        float smc = GetSpecMagicCurve( roughness );
        float specNonLinearAccumSpeed = 1.0 / ( 1.0 + frameNum.y );

        float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistSettings.xyz, roughness );
        float hitDist = ExtractHitDist( spec ) * hitDistScale;
        #if( NRD_MODE != OCCLUSION )
            // "gIn_SpecHitDistForTracking" is better for low roughness, but doesn't suit for high roughness ( because it's min )
            hitDist = lerp( gIn_SpecHitDistForTracking[ pixelPos ], hitDist, smc );
        #endif
        float hitDistFactor = GetHitDistFactor( hitDist, frustumSize );
        hitDist = saturate( hitDist / hitDistScale );

        // Stride between taps
        stride.y *= lerp( 0.25 + 0.75 * Math::Sqrt01( hitDistFactor ), 1.0, specNonLinearAccumSpeed ); // "hitDistFactor" is very noisy and breaks nice patterns
        stride.y *= lerp( 0.25, 1.0, smc ); // hand tuned // TODO: use "lobeRadius"?
        stride.y = round( stride.y );

        // History reconstruction
        if( stride.y != 0 )
        {
            // Parameters
            float normalWeightParam = GetNormalWeightParam( specNonLinearAccumSpeed, gLobeAngleFraction, roughness );
            float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv );
            float2 hitDistanceWeightParams = GetHitDistanceWeightParams( hitDist, specNonLinearAccumSpeed );
            float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( roughness * roughness, sqrt( gRoughnessFraction ) );

            float sums = 1.0 + frameNum.y;
            #if( REBLUR_PERFORMANCE_MODE == 1 )
                sums = 1.0 + 1.0 / ( 1.0 + gMaxAccumulatedFrameNum ) - specNonLinearAccumSpeed;
            #endif

            spec *= sums;
            #if( NRD_MODE == SH )
                specSh *= sums;
            #endif

            [unroll]
            for( j = -REBLUR_HISTORY_FIX_FILTER_RADIUS; j <= REBLUR_HISTORY_FIX_FILTER_RADIUS; j++ )
            {
                [unroll]
                for( i = -REBLUR_HISTORY_FIX_FILTER_RADIUS; i <= REBLUR_HISTORY_FIX_FILTER_RADIUS; i++ )
                {
                    // Skip center
                    if( i == 0 && j == 0 )
                        continue;

                    // Skip corners
                    if( abs( i ) + abs( j ) == REBLUR_HISTORY_FIX_FILTER_RADIUS * 2 )
                        continue;

                    // Sample uv ( at the pixel center )
                    float2 uv = pixelUv + float2( i, j ) * stride.y * gRectSizeInv;

                    // Apply "mirror" to not waste taps going outside of the screen
                    uv = MirrorUv( uv );

                    // "uv" to "pos"
                    int2 pos = uv * gRectSize;

                    // Fetch data
                    float zs = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pos ) ] );
                    float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

                    float materialIDs;
                    float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
                    Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

                    // Weight
                    float angle = Math::AcosApprox( dot( Ns.xyz, N ) );
                    float NoX = dot( Nv, Xvs );

                    float w = CompareMaterials( materialID, materialIDs, gSpecMinMaterial );
                    w *= ComputeExponentialWeight( angle, normalWeightParam, 0.0 );
                    w *= ComputeExponentialWeight( Ns.w * Ns.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y );
                    // gaussian weight is not needed

                    #if( REBLUR_PERFORMANCE_MODE == 0 )
                        w *= 1.0 + UnpackData1( gIn_Data1[ pos ] ).y;
                    #endif

                    w = ApplyGeometryWeightLast( w, zs, NoX, geometryWeightParams );

                    REBLUR_TYPE s = gIn_Spec[ pos ];
                    s = Denanify( w, s );

                    // A-trous weight
                    w *= ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y );

                    // Accumulate
                    sums += w;

                    spec += s * w;
                    #if( NRD_MODE == SH )
                        REBLUR_SH_TYPE sh = gIn_SpecSh[ pos ];
                        sh = Denanify( w, sh );

                        specSh += sh * w;
                    #endif
                }
            }

            sums = Math::PositiveRcp( sums );
            spec *= sums;
            #if( NRD_MODE == SH )
                specSh *= sums;
            #endif
        }

        float specLuma = GetLuma( spec );

        // Mix fast history with "fixed" normal history if normal history is not longer than fast
        float f = frameNumAvgNorm.y;
        f = lerp( 1.0, f, smc ); // HistoryFix-ed data is undesired in fast history for low roughness ( test 115 )

        float specFastCenter = s_SpecLuma[ smemPos.y ][ smemPos.x ];
        specFastCenter = lerp( specLuma, specFastCenter, f );

        gOut_SpecFast[ pixelPos ] = specFastCenter;

        // Local variance
        float specFastM1 = specFastCenter;
        float specFastM2 = specFastCenter * specFastCenter;
        float specAntiFireflyM1 = 0;
        float specAntiFireflyM2 = 0;

        [unroll]
        for( j = -NRD_BORDER; j <= NRD_BORDER; j++ )
        {
            [unroll]
            for( i = -NRD_BORDER; i <= NRD_BORDER; i++ )
            {
                if( i == 0 && j == 0 )
                    continue;

                int2 pos = smemPos + int2( i, j );

                float s = s_SpecLuma[ pos.y ][ pos.x ];
                s = s == REBLUR_INVALID ? specFastCenter : s;

                // Variance in 5x5 for fast history
                if( abs( i ) <= REBLUR_FAST_HISTORY_CLAMPING_RADIUS && abs( j ) <= REBLUR_FAST_HISTORY_CLAMPING_RADIUS )
                {
                    specFastM1 += s;
                    specFastM2 += s * s;
                }

                // Variance in "NRD_BORDER x NRD_BORDER" skipping central 3x3 for anti-firefly
                if( NRD_SUPPORTS_ANTIFIREFLY && !( abs( i ) <= 1 && abs( j ) <= 1 ) )
                {
                    specAntiFireflyM1 += s;
                    specAntiFireflyM2 += s * s;
                }
            }
        }

        // Anti-firefly
        if( NRD_SUPPORTS_ANTIFIREFLY && gAntiFirefly )
        {
            float invNorm = 1.0 / ( ( NRD_BORDER * 2 + 1 ) * ( NRD_BORDER * 2 + 1 ) - 3 * 3 ); // -9 samples
            specAntiFireflyM1 *= invNorm;
            specAntiFireflyM2 *= invNorm;

            float specAntiFireflySigma = GetStdDev( specAntiFireflyM1, specAntiFireflyM2 ) * REBLUR_ANTI_FIREFLY_SIGMA_SCALE;

            specLuma = clamp( specLuma, specAntiFireflyM1 - specAntiFireflySigma, specAntiFireflyM1 + specAntiFireflySigma );
        }

        // Clamp to fast history
        {
            float invNorm = 1.0 / ( ( REBLUR_FAST_HISTORY_CLAMPING_RADIUS * 2 + 1 ) * ( REBLUR_FAST_HISTORY_CLAMPING_RADIUS * 2 + 1 ) );
            specFastM1 *= invNorm;
            specFastM2 *= invNorm;

            float fastHistoryClampingSigmaScale = gFastHistoryClampingSigmaScale;
            if( materialID == gStrandMaterialID )
                fastHistoryClampingSigmaScale = max( fastHistoryClampingSigmaScale, 3.0 );

            float specFastSigma = GetStdDev( specFastM1, specFastM2 ) * fastHistoryClampingSigmaScale;
            float specLumaClamped = clamp( specLuma, specFastM1 - specFastSigma, specFastM1 + specFastSigma );

            specLuma = lerp( specLumaClamped, specLuma, 1.0 / ( 1.0 + float( gMaxFastAccumulatedFrameNum < gMaxAccumulatedFrameNum ) * frameNum.y * 2.0 ) );
        }

        // Change luma
        #if( REBLUR_SHOW == REBLUR_SHOW_FAST_HISTORY )
            specLuma = specFastCenter;
        #endif

        spec = ChangeLuma( spec, specLuma );
        #if( NRD_MODE == SH )
            specSh *= GetLumaScale( length( specSh ), specLuma );
        #endif

        // Output
        gOut_Spec[ pixelPos ] = spec;
        #if( NRD_MODE == SH )
            gOut_SpecSh[ pixelPos ] = specSh;
        #endif
    }
    #endif
}
