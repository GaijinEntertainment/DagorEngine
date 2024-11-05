/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float2 s_FrameNum[ BUFFER_Y ][ BUFFER_X ];
groupshared float s_DiffLuma[ BUFFER_Y ][ BUFFER_X ];
groupshared float s_SpecLuma[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    s_FrameNum[ sharedPos.y ][ sharedPos.x ] = UnpackData1( gIn_Data1[ globalPos ] ).xz;

    #ifdef REBLUR_DIFFUSE
        s_DiffLuma[ sharedPos.y ][ sharedPos.x ] = gIn_DiffFast[ globalPos ];
    #endif

    #ifdef REBLUR_SPECULAR
        s_SpecLuma[ sharedPos.y ][ sharedPos.x ] = gIn_SpecFast[ globalPos ];
    #endif
}

// Tests 20, 23, 24, 27, 28, 54, 59, 65, 66, 76, 81, 98, 112, 117, 124, 126, 128, 134
// TODO: potentially do color clamping after reconstruction in a separate pass

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( int2 threadPos : SV_GroupThreadId, int2 pixelPos : SV_DispatchThreadId, uint threadIndex : SV_GroupIndex )
{
    // Preload
    float isSky = gIn_Tiles[ pixelPos >> 4 ];
    PRELOAD_INTO_SMEM_WITH_TILE_CHECK;

    // Tile-based early out
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Early out
    float viewZ = abs( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    if( viewZ > gDenoisingRange )
        return;

    // Center data
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 Nv = STL::Geometry::RotateVectorInverse( gViewToWorld, N );
    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );
    float slopeScale = 1.0 / max( NoV, 0.2 ); // TODO: bigger scale introduces ghosting

    // Smooth number of accumulated frames
    int2 smemPos = threadPos + BORDER;
    float invHistoryFixFrameNum = STL::Math::PositiveRcp( gHistoryFixFrameNum );
    float2 frameNum = s_FrameNum[ smemPos.y ][ smemPos.x ]; // unsmoothed
    float2 frameNumNorm = saturate( frameNum * invHistoryFixFrameNum ); // smoothed
    float2 c = frameNumNorm;
    float2 sum = 1.0;

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            // Skip center
            if( i == BORDER && j == BORDER )
                continue;

            // Only 3x3 needed
            float2 o = float2( i, j ) - BORDER;
            if( any( abs( o ) > 1 ) )
                continue;

            int2 pos = threadPos + int2( i, j );

            float2 f = s_FrameNum[ pos.y ][ pos.x ];
            float2 fn = saturate( f * invHistoryFixFrameNum );
            float2 w = step( c, fn ); // use only neighbors with longer history

            frameNumNorm += fn * w;
            sum += w;
        }
    }

    frameNumNorm /= sum;

    // IMPORTANT: progression is "{8, 4, 2, 1} + 1". "+1" is important to better break blobs
    float2 stride = exp2( gHistoryFixFrameNum - frameNumNorm * gHistoryFixFrameNum ) + 1.0;

    // Diffuse
    #ifdef REBLUR_DIFFUSE
        REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
        #ifdef REBLUR_SH
            float4 diffSh = gIn_DiffSh[ pixelPos ];
        #endif

        // Stride between taps
         float diffStride = stride.x * float( frameNum.x < gHistoryFixFrameNum );
         diffStride = floor( diffStride );

        // History reconstruction
        if( diffStride != 0.0 )
        {
            int diffStridei = int( diffStride + 0.5 );

            // Parameters
            float diffNonLinearAccumSpeed = 1.0 / ( 1.0 + frameNum.x );

            float diffNormalWeightParam = GetNormalWeightParams( diffNonLinearAccumSpeed, 1.0 );
            float2 diffGeometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity * slopeScale, frustumSize, Xv, Nv, diffNonLinearAccumSpeed );

            float sumd = 1.0 + frameNum.x;
            #ifdef REBLUR_PERFORMANCE_MODE
                sumd = 1.0 + 1.0 / ( 1.0 + gMaxAccumulatedFrameNum ) - diffNonLinearAccumSpeed;
            #endif
            diff *= sumd;

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
                    float z = abs( gIn_ViewZ[ WithRectOrigin( pos ) ] );

                    float materialIDs;
                    float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
                    Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

                    float3 Xvs = STL::Geometry::ReconstructViewPosition( uv, gFrustum, z, gOrthoMode );
                    float NoX = dot( Nv, Xvs );

                    float angle = STL::Math::AcosApprox( dot( Ns.xyz, N ) );

                    // Weight
                    float w = IsInScreenNearest( uv );
                    w *= float( z < gDenoisingRange );
                    w *= CompareMaterials( materialID, materialIDs, gDiffMaterialMask );
                    w *= ComputeWeight( NoX, diffGeometryWeightParams.x, diffGeometryWeightParams.y );
                    w *= ComputeExponentialWeight( angle, diffNormalWeightParam, 0.0 );

                    #ifndef REBLUR_PERFORMANCE_MODE
                        w *= 1.0 + UnpackData1( gIn_Data1[ pos ] ).x;
                    #endif

                    // Accumulate
                    sumd += w;

                    REBLUR_TYPE s = gIn_Diff[ pos ];
                    s = Denanify( w, s );
                    diff += s * w;
                    #ifdef REBLUR_SH
                        float4 sh = gIn_DiffSh[ pos ];
                        sh = Denanify( w, sh );
                        diffSh += sh * w;
                    #endif
                }
            }

            sumd = STL::Math::PositiveRcp( sumd );
            diff *= sumd;
            #ifdef REBLUR_SH
                diffSh *= sumd;
            #endif
        }

        // Local variance
        float diffCenter = s_DiffLuma[ smemPos.y ][ smemPos.x ];
        float diffM1 = diffCenter;
        float diffM2 = diffM1 * diffM1;

        diffCenter = lerp( GetLuma( diff ), diffCenter, saturate( frameNum.x / ( gHistoryFixFrameNum + NRD_EPS ) ) );
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
    #endif

    // Specular
    #ifdef REBLUR_SPECULAR
        REBLUR_TYPE spec = gIn_Spec[ pixelPos ];
        #ifdef REBLUR_SH
            float4 specSh = gIn_SpecSh[ pixelPos ];
        #endif

        // Stride between taps
        float smc = GetSpecMagicCurve( roughness );
        float specStride = stride.y * float( frameNum.y < gHistoryFixFrameNum );
        specStride *= lerp( 0.5, 1.0, smc ); // TODO: seems to work better than "minBlurRadius"
        specStride = floor( specStride );

        // History reconstruction
        if( specStride != 0 )
        {
            // TODO: introduce IN_SECONDARY_ROUGHNESS:
            //  - to allow blur on diffuse-like surfaces in reflection
            //  - use "hitDistanceWeight" only for very low primary roughness to avoid color bleeding from one surface to another

            int specStridei = int( specStride + 0.5 );

            // Parameters
            float specNonLinearAccumSpeed = 1.0 / ( 1.0 + frameNum.y );
            float hitDistNormAtCenter = ExtractHitDist( spec );

            float lobeEnergy = lerp( 0.75, 0.85, specNonLinearAccumSpeed );
            float lobeHalfAngle = STL::ImportanceSampling::GetSpecularLobeHalfAngle( roughness, lobeEnergy ); // up to 85% energy to depend less on normal weight
            lobeHalfAngle *= specNonLinearAccumSpeed;

            float specNormalWeightParam = 1.0 / max( lobeHalfAngle, NRD_NORMAL_ULP );
            float2 specGeometryWeightParams = GetGeometryWeightParams( gPlaneDistSensitivity * slopeScale, frustumSize, Xv, Nv, specNonLinearAccumSpeed );
            float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( roughness * roughness, sqrt( gRoughnessFraction ) );
            float2 hitDistanceWeightParams = GetHitDistanceWeightParams( hitDistNormAtCenter, specNonLinearAccumSpeed, roughness );

            float sums = 1.0 + frameNum.y;
            #ifdef REBLUR_PERFORMANCE_MODE
                sums = 1.0 + 1.0 / ( 1.0 + gMaxAccumulatedFrameNum ) - specNonLinearAccumSpeed;
            #endif
            spec *= sums;

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
                    float z = abs( gIn_ViewZ[ WithRectOrigin( pos ) ] );

                    float materialIDs;
                    float4 Ns = gIn_Normal_Roughness[ WithRectOrigin( pos ) ];
                    Ns = NRD_FrontEnd_UnpackNormalAndRoughness( Ns, materialIDs );

                    float3 Xvs = STL::Geometry::ReconstructViewPosition( uv, gFrustum, z, gOrthoMode );
                    float NoX = dot( Nv, Xvs );

                    float angle = STL::Math::AcosApprox( dot( Ns.xyz, N ) );

                    // Weight
                    float w = IsInScreenNearest( uv );
                    w *= float( z < gDenoisingRange );
                    w *= CompareMaterials( materialID, materialIDs, gSpecMaterialMask );
                    w *= ComputeWeight( NoX, specGeometryWeightParams.x, specGeometryWeightParams.y );
                    w *= ComputeExponentialWeight( angle, specNormalWeightParam, 0.0 );
                    w *= ComputeExponentialWeight( Ns.w * Ns.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y );

                    #ifndef REBLUR_PERFORMANCE_MODE
                        w *= 1.0 + UnpackData1( gIn_Data1[ pos ] ).z;
                    #endif

                    REBLUR_TYPE s = gIn_Spec[ pos ];
                    s = Denanify( w, s );

                    w *= ComputeExponentialWeight( ExtractHitDist( s ), hitDistanceWeightParams.x, hitDistanceWeightParams.y );

                    // Accumulate
                    sums += w;

                    spec += s * w;
                    #ifdef REBLUR_SH
                        float4 sh = gIn_SpecSh[ pos ];
                        sh = Denanify( w, sh );
                        specSh.xyz += sh.xyz * w; // see TA
                    #endif
                }
            }

            sums = STL::Math::PositiveRcp( sums );
            spec *= sums;
            #ifdef REBLUR_SH
                specSh *= sums;
            #endif
        }

        // Local variance
        float specCenter = s_SpecLuma[ smemPos.y ][ smemPos.x ];
        float specM1 = specCenter;
        float specM2 = specM1 * specM1;

        specCenter = lerp( GetLuma( spec ), specCenter, saturate( frameNum.y / ( gHistoryFixFrameNum + NRD_EPS ) ) );
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
    #endif
}
