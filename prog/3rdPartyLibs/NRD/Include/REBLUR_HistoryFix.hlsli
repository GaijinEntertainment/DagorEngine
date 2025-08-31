/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float s_DiffLuma[ BUFFER_Y ][ BUFFER_X ];
groupshared float s_SpecLuma[ BUFFER_Y ][ BUFFER_X ];
groupshared float2 s_FrameNum[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    #ifdef REBLUR_DIFFUSE
        s_DiffLuma[ sharedPos.y ][ sharedPos.x ] = gIn_DiffFast[ globalPos ];
    #endif

    #ifdef REBLUR_SPECULAR
        s_SpecLuma[ sharedPos.y ][ sharedPos.x ] = gIn_SpecFast[ globalPos ];
    #endif

    s_FrameNum[ sharedPos.y ][ sharedPos.x ] = UnpackData1( gIn_Data1[ globalPos ] );
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

    // Tile-based early out
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Early out
    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    if( viewZ > gDenoisingRange )
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

    int2 smemPos = threadPos + BORDER;
    float2 frameNum = s_FrameNum[ smemPos.y ][ smemPos.x ];

    // Use smaller strides if neighborhood pixels have longer history to minimize chances of ruining contact details // TODO: it would be good to apply the same logic in spatial passes
    float2 stride;
    {
        float2 frameNumAvg = frameNum;
        float2 sum = 1.0;
        float invHistoryFixFrameNum = 1.0 / ( gHistoryFixFrameNum + NRD_EPS );

        [unroll]
        for( i = -1; i <= 1; i++ )
        {
            [unroll]
            for( j = -1; j <= 1; j++ )
            {
                if( i == 0 && j == 0 )
                    continue;

                float2 f = s_FrameNum[ smemPos.y + j ][ smemPos.x + i ];
                float2 w = step( frameNum, f ); // use only neighbors with longer history

                frameNumAvg += saturate( f * invHistoryFixFrameNum ) * w;
                sum += w;
            }
        }

        frameNumAvg /= sum;

        stride = gHistoryFixBasePixelStride / ( 2.0 + frameNumAvg * gHistoryFixFrameNum ); // +1 to match RELAX
        stride *= float2( frameNum < gHistoryFixFrameNum );
    }

    // Diffuse
    #ifdef REBLUR_DIFFUSE
    {
        REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
        #ifdef REBLUR_SH
            float4 diffSh = gIn_DiffSh[ pixelPos ];
        #endif

        // Stride between taps
         float diffStride = stride.x;
         diffStride = floor( diffStride );

        // History reconstruction
        if( diffStride != 0.0 )
        {
            int diffStridei = int( diffStride + 0.5 );

            // Parameters
            float diffNonLinearAccumSpeed = 1.0 / ( 1.0 + frameNum.x );

            float normalWeightParam = GetNormalWeightParam( diffNonLinearAccumSpeed, gLobeAngleFraction );
            float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, diffNonLinearAccumSpeed );

            float sumd = 1.0 + frameNum.x;
            #ifdef REBLUR_PERFORMANCE_MODE
                sumd = 1.0 + 1.0 / ( 1.0 + gMaxAccumulatedFrameNum ) - diffNonLinearAccumSpeed;
            #endif

            // TODO: non-standard use of "hitDistFactor" as "hitDist", but works well
            float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, 1.0 );
            float hitDist = ExtractHitDist( diff ) * hitDistScale;
            float hitDistFactor = GetHitDistFactor( hitDist, frustumSize );
            float2 hitDistanceWeightParams = GetHitDistanceWeightParams( hitDistFactor, diffNonLinearAccumSpeed, 1.0 );

            diff *= sumd;
            #ifdef REBLUR_SH
                diffSh *= sumd;
            #endif

            [unroll]
            for( j = -2; j <= 2; j++ )
            {
                [unroll]
                for( i = -2; i <= 2; i++ )
                {
                    // Skip center
                    if( i == 0 && j == 0 )
                        continue;

                    // Skip corners
                    if( abs( i ) + abs( j ) == 4 )
                        continue;

                    // Sample uv
                    float2 uv = pixelUv + float2( i, j ) * diffStride * gRectSizeInv;

                    int2 pos = pixelPos + int2( i, j ) * diffStridei;
                    pos = clamp( pos, 0, gRectSizeMinusOne );

                    // Fetch data
                    float zs = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pos ) ] );

                    float materialIDs;
                    float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
                    Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

                    // Weight
                    float angle = Math::AcosApprox( dot( Ns.xyz, N ) );
                    float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

                    float w = IsInScreenNearest( uv );
                    w *= ComputeWeight( dot( Nv, Xvs ), geometryWeightParams.x, geometryWeightParams.y );
                    w *= CompareMaterials( materialID, materialIDs, gDiffMinMaterial );
                    w *= ComputeExponentialWeight( angle, normalWeightParam, 0.0 );

                    #ifndef REBLUR_PERFORMANCE_MODE
                        w *= 1.0 + UnpackData1( gIn_Data1[ pos ] ).x;
                    #endif

                    REBLUR_TYPE s = gIn_Diff[ pos ];
                    s = Denanify( w, s );

                    float hs = ExtractHitDist( s ) * hitDistScale;
                    float hsFactor = GetHitDistFactor( hs, frustumSize );
                    w *= ComputeExponentialWeight( hsFactor, hitDistanceWeightParams.x, hitDistanceWeightParams.y );

                    // Accumulate
                    sumd += w;

                    diff += s * w;
                    #ifdef REBLUR_SH
                        float4 sh = gIn_DiffSh[ pos ];
                        sh = Denanify( w, sh );
                        diffSh += sh * w;
                    #endif
                }
            }

            sumd = Math::PositiveRcp( sumd );
            diff *= sumd;
            #ifdef REBLUR_SH
                diffSh *= sumd;
            #endif
        }

        // Local variance
        float diffCenter = s_DiffLuma[ threadPos.y + BORDER ][ threadPos.x + BORDER ];
        float diffM1 = diffCenter;
        float diffM2 = diffM1 * diffM1;

        float f = saturate( frameNum.x / ( gHistoryFixFrameNum + NRD_EPS ) );
        diffCenter = lerp( GetLuma( diff ), diffCenter, f );
        gOut_DiffFast[ pixelPos ] = diffCenter;

        [unroll]
        for( j = 0; j <= BORDER * 2; j++ )
        {
            [unroll]
            for( i = 0; i <= BORDER * 2; i++ )
            {
                // Skip center
                if( i == BORDER && j == BORDER )
                    continue;

                int2 pos = threadPos + int2( i, j );

                float d = s_DiffLuma[ pos.y ][ pos.x ];
                diffM1 += d;
                diffM2 += d * d;
            }
        }

        float diffLuma = GetLuma( diff );

        // Anti-firefly
        if( gAntiFirefly && REBLUR_USE_ANTIFIREFLY == 1 )
        {
            float m1 = 0;
            float m2 = 0;

            [unroll]
            for( j = -REBLUR_ANTI_FIREFLY_FILTER_RADIUS; j <= REBLUR_ANTI_FIREFLY_FILTER_RADIUS; j++ )
            {
                [unroll]
                for( i = -REBLUR_ANTI_FIREFLY_FILTER_RADIUS; i <= REBLUR_ANTI_FIREFLY_FILTER_RADIUS; i++ )
                {
                    // Skip central 3x3 area
                    if( abs( i ) <= 1 && abs( j ) <= 1 )
                        continue;

                    int2 pos = clamp( pixelPos + int2( i, j ), 0, gRectSizeMinusOne );
                    float d = gIn_DiffFast[ pos ].x;

                    m1 += d;
                    m2 += d * d;
                }
            }

            float invNorm = 1.0 / ( ( REBLUR_ANTI_FIREFLY_FILTER_RADIUS * 2 + 1 ) * ( REBLUR_ANTI_FIREFLY_FILTER_RADIUS * 2 + 1 ) - 3 * 3 );
            m1 *= invNorm;
            m2 *= invNorm;

            float sigma = GetStdDev( m1, m2 ) * REBLUR_ANTI_FIREFLY_SIGMA_SCALE;
            diffLuma = clamp( diffLuma, m1 - sigma, m1 + sigma );
        }

        // Fast history
        diffM1 /= ( BORDER * 2 + 1 ) * ( BORDER * 2 + 1 );
        diffM2 /= ( BORDER * 2 + 1 ) * ( BORDER * 2 + 1 );
        float diffSigma = GetStdDev( diffM1, diffM2 ) * REBLUR_COLOR_CLAMPING_SIGMA_SCALE;

        float diffMin = diffM1 - diffSigma;
        float diffMax = diffM1 + diffSigma;

        float diffLumaClamped = clamp( diffLuma, diffMin, diffMax );
        diffLuma = lerp( diffLumaClamped, diffLuma, 1.0 / ( 1.0 + float( gMaxFastAccumulatedFrameNum < gMaxAccumulatedFrameNum ) * frameNum.x * 2.0 ) );

        // Change luma
        #if( REBLUR_SHOW == REBLUR_SHOW_FAST_HISTORY )
            diffLuma = diffCenter;
        #endif

        diff = ChangeLuma( diff, diffLuma );
        #ifdef REBLUR_SH
            diffSh.xyz *= GetLumaScale( length( diffSh.xyz ), diffLuma );
        #endif

        // Output
        gOut_Diff[ pixelPos ] = diff;
        #ifdef REBLUR_SH
            gOut_DiffSh[ pixelPos ] = diffSh;
        #endif
    }
    #endif

    // Specular
    #ifdef REBLUR_SPECULAR
    {
        REBLUR_TYPE spec = gIn_Spec[ pixelPos ];
        #ifdef REBLUR_SH
            float4 specSh = gIn_SpecSh[ pixelPos ];
        #endif

        // Stride between taps
        float smc = GetSpecMagicCurve( roughness );
        float specStride = stride.y;
        specStride *= lerp( 0.5, 1.0, smc ); // hand tuned
        specStride = floor( specStride );

        // History reconstruction
        if( specStride != 0 )
        {
            int specStridei = int( specStride + 0.5 );

            // Parameters
            float specNonLinearAccumSpeed = 1.0 / ( 1.0 + frameNum.y );

            float normalWeightParam = GetNormalWeightParam( specNonLinearAccumSpeed, gLobeAngleFraction, roughness );
            float2 geometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity, frustumSize, Xv, Nv, specNonLinearAccumSpeed );
            float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( roughness * roughness, sqrt( gRoughnessFraction ) );

            // TODO: non-standard use of "hitDistFactor" as "hitDist", but works well
            float hitDistScale = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness );
            float hitDist = ExtractHitDist( spec ) * hitDistScale;
            float hitDistFactor = GetHitDistFactor( hitDist, frustumSize );
            float2 hitDistanceWeightParams = GetHitDistanceWeightParams( hitDistFactor, specNonLinearAccumSpeed, roughness );

            float sums = 1.0 + frameNum.y;
            #ifdef REBLUR_PERFORMANCE_MODE
                sums = 1.0 + 1.0 / ( 1.0 + gMaxAccumulatedFrameNum ) - specNonLinearAccumSpeed;
            #endif

            spec *= sums;
            #ifdef REBLUR_SH
                specSh.xyz *= sums;
            #endif

            [unroll]
            for( j = -2; j <= 2; j++ )
            {
                [unroll]
                for( i = -2; i <= 2; i++ )
                {
                    // Skip center
                    if( i == 0 && j == 0 )
                        continue;

                    // Skip corners
                    if( abs( i ) + abs( j ) == 4 )
                        continue;

                    // Sample uv
                    float2 uv = pixelUv + float2( i, j ) * specStride * gRectSizeInv;

                    int2 pos = pixelPos + int2( i, j ) * specStridei;
                    pos = clamp( pos, 0, gRectSizeMinusOne );

                    // Fetch data
                    float zs = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pos ) ] );

                    float materialIDs;
                    float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
                    Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

                    float angle = Math::AcosApprox( dot( Ns.xyz, N ) );
                    float3 Xvs = Geometry::ReconstructViewPosition( uv, gFrustum, zs, gOrthoMode );

                    // Weight
                    float w = IsInScreenNearest( uv );
                    w *= ComputeWeight( dot( Nv, Xvs ), geometryWeightParams.x, geometryWeightParams.y );
                    w *= CompareMaterials( materialID, materialIDs, gSpecMinMaterial );
                    w *= ComputeExponentialWeight( angle, normalWeightParam, 0.0 );
                    w *= ComputeExponentialWeight( Ns.w * Ns.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y );

                    #ifndef REBLUR_PERFORMANCE_MODE
                        w *= 1.0 + UnpackData1( gIn_Data1[ pos ] ).y;
                    #endif

                    REBLUR_TYPE s = gIn_Spec[ pos ];
                    s = Denanify( w, s );

                    float hs = ExtractHitDist( s ) * hitDistScale;
                    float hsFactor = GetHitDistFactor( hs, frustumSize );
                    w *= ComputeExponentialWeight( hsFactor, hitDistanceWeightParams.x, hitDistanceWeightParams.y );

                    // Special case for low roughness ( hit distances work as a non-noisy guide )
                    float d = abs( hitDist - hs ) / ( max( hitDist, hs ) + 0.001 );
                    float b = Math::LinearStep( 0.03, 0.05, roughness );
                    w *= Math::SmoothStep( 0.2 + b, 0.05 + b, d );

                    // Accumulate
                    sums += w;

                    spec += s * w;
                    #ifdef REBLUR_SH
                        float4 sh = gIn_SpecSh[ pos ];
                        sh = Denanify( w, sh );
                        specSh.xyz += sh.xyz * w;
                    #endif
                }
            }

            sums = Math::PositiveRcp( sums );
            spec *= sums;
            #ifdef REBLUR_SH
                specSh.xyz *= sums;
            #endif
        }

        // Local variance
        float specCenter = s_SpecLuma[ threadPos.y + BORDER ][ threadPos.x + BORDER ];
        float specM1 = specCenter;
        float specM2 = specM1 * specM1;

        float f = saturate( frameNum.y / ( gHistoryFixFrameNum + NRD_EPS ) );
        f = lerp( 1.0, f, smc ); // HistoryFix-ed data is undesired in fast history for low roughness ( test 115 )
        specCenter = lerp( GetLuma( spec ), specCenter, f );
        gOut_SpecFast[ pixelPos ] = specCenter;

        [unroll]
        for( j = 0; j <= BORDER * 2; j++ )
        {
            [unroll]
            for( i = 0; i <= BORDER * 2; i++ )
            {
                // Skip center
                if( i == BORDER && j == BORDER )
                    continue;

                int2 pos = threadPos + int2( i, j );

                float s = s_SpecLuma[ pos.y ][ pos.x ];
                specM1 += s;
                specM2 += s * s;
            }
        }

        float specLuma = GetLuma( spec );

        // Anti-firefly
        if( gAntiFirefly && REBLUR_USE_ANTIFIREFLY == 1 )
        {
            float m1 = 0;
            float m2 = 0;

            [unroll]
            for( j = -REBLUR_ANTI_FIREFLY_FILTER_RADIUS; j <= REBLUR_ANTI_FIREFLY_FILTER_RADIUS; j++ )
            {
                [unroll]
                for( i = -REBLUR_ANTI_FIREFLY_FILTER_RADIUS; i <= REBLUR_ANTI_FIREFLY_FILTER_RADIUS; i++ )
                {
                    // Skip central 3x3 area
                    if( abs( i ) <= 1 && abs( j ) <= 1 )
                        continue;

                    int2 pos = clamp( pixelPos + int2( i, j ), 0, gRectSizeMinusOne );
                    float s = gIn_SpecFast[ pos ].x;

                    m1 += s;
                    m2 += s * s;
                }
            }

            float invNorm = 1.0 / ( ( REBLUR_ANTI_FIREFLY_FILTER_RADIUS * 2 + 1 ) * ( REBLUR_ANTI_FIREFLY_FILTER_RADIUS * 2 + 1 ) - 3 * 3 );
            m1 *= invNorm;
            m2 *= invNorm;

            float sigma = GetStdDev( m1, m2 ) * REBLUR_ANTI_FIREFLY_SIGMA_SCALE;
            specLuma = clamp( specLuma, m1 - sigma, m1 + sigma );
        }

        // Fast history
        specM1 /= ( BORDER * 2 + 1 ) * ( BORDER * 2 + 1 );
        specM2 /= ( BORDER * 2 + 1 ) * ( BORDER * 2 + 1 );
        float specSigma = GetStdDev( specM1, specM2 ) * REBLUR_COLOR_CLAMPING_SIGMA_SCALE;

        float specMin = specM1 - specSigma;
        float specMax = specM1 + specSigma;

        float specLumaClamped = clamp( specLuma, specMin, specMax );
        specLuma = lerp( specLumaClamped, specLuma, 1.0 / ( 1.0 + float( gMaxFastAccumulatedFrameNum < gMaxAccumulatedFrameNum ) * frameNum.y * 2.0 ) );

        // Change luma
        #if( REBLUR_SHOW == REBLUR_SHOW_FAST_HISTORY )
            specLuma = specCenter;
        #endif

        spec = ChangeLuma( spec, specLuma );
        #ifdef REBLUR_SH
            specSh.xyz *= GetLumaScale( length( specSh.xyz ), specLuma );
        #endif

        // Output
        gOut_Spec[ pixelPos ] = spec;
        #ifdef REBLUR_SH
            gOut_SpecSh[ pixelPos ] = specSh;
        #endif
    }
    #endif
}
