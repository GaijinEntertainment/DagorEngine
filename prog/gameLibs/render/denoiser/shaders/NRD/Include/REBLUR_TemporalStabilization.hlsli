/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// TODO: add REBLUR_OCCLUSION support to TemporalStabilization?

groupshared float4 s_Diff[ BUFFER_Y ][ BUFFER_X ];
groupshared float4 s_Spec[ BUFFER_Y ][ BUFFER_X ];

#ifdef REBLUR_SH
    groupshared float4 s_DiffSh[ BUFFER_Y ][ BUFFER_X ];
    groupshared float4 s_SpecSh[ BUFFER_Y ][ BUFFER_X ];
#endif

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    #ifdef REBLUR_DIFFUSE
        s_Diff[ sharedPos.y ][ sharedPos.x ] = gIn_Diff[ globalPos ];
        #ifdef REBLUR_SH
            s_DiffSh[ sharedPos.y ][ sharedPos.x ] = gIn_DiffSh[ globalPos ];
        #endif
    #endif

    #ifdef REBLUR_SPECULAR
        s_Spec[ sharedPos.y ][ sharedPos.x ] = gIn_Spec[ globalPos ];
        #ifdef REBLUR_SH
            s_SpecSh[ sharedPos.y ][ sharedPos.x ] = gIn_SpecSh[ globalPos ];
        #endif
    #endif
}

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
    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    if( viewZ > gDenoisingRange )
        return; // IMPORTANT: no data output, must be rejected by the "viewZ" check!

    // Normal and roughness
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    // Position
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 X = STL::Geometry::RotateVector( gViewToWorld, Xv );

    // Local variance
    int2 smemPos = threadPos + BORDER;

    #ifdef REBLUR_DIFFUSE
        float4 diff = s_Diff[ smemPos.y ][ smemPos.x ];
        float4 diffM1 = diff;
        float4 diffM2 = diff * diff;

        #ifdef REBLUR_SH
            float4 diffSh = s_DiffSh[ smemPos.y ][ smemPos.x ];
            float4 diffShM1 = diffSh;
            float4 diffShM2 = diffSh * diffSh;
        #endif

        float2 diffMin = NRD_INF;
        float2 diffMax = -NRD_INF;
    #endif

    #ifdef REBLUR_SPECULAR
        float4 spec = s_Spec[ smemPos.y ][ smemPos.x ];
        float4 specM1 = spec;
        float4 specM2 = spec * spec;

        #ifdef REBLUR_SH
            float4 specSh = s_SpecSh[ smemPos.y ][ smemPos.x ];
            float4 specShM1 = specSh;
            float4 specShM2 = specSh * specSh;
        #endif

        float2 specMin = NRD_INF;
        float2 specMax = -NRD_INF;
    #endif

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            if( i == BORDER && j == BORDER )
                continue;

            int2 pos = threadPos + int2( i, j );

            #ifdef REBLUR_DIFFUSE
                // Accumulate moments
                float4 d = s_Diff[ pos.y ][ pos.x ];
                diffM1 += d;
                diffM2 += d * d;

                #ifdef REBLUR_SH
                    float4 dh = s_DiffSh[ pos.y ][ pos.x ];
                    diffShM1 += dh;
                    diffShM2 += dh * dh;
                #endif

                // RCRS
                float diffLuma = GetLuma( d );
                diffMin = min( diffMin, float2( diffLuma, d.w ) );
                diffMax = max( diffMax, float2( diffLuma, d.w ) );
            #endif

            #ifdef REBLUR_SPECULAR
                // Accumulate moments
                float4 s = s_Spec[ pos.y ][ pos.x ];
                specM1 += s;
                specM2 += s * s;

                #ifdef REBLUR_SH
                    float4 sh = s_SpecSh[ pos.y ][ pos.x ];
                    specShM1 += sh;
                    specShM2 += sh * sh;
                #endif

                // RCRS
                float specLuma = GetLuma( s );
                specMin = min( specMin, float2( specLuma, s.w ) );
                specMax = max( specMax, float2( specLuma, s.w ) );
            #endif
        }
    }

    float invSum = 1.0 / ( ( BORDER * 2 + 1 ) * ( BORDER * 2 + 1 ) );

    #ifdef REBLUR_DIFFUSE
        // Compute sigma
        diffM1 *= invSum;
        diffM2 *= invSum;
        float4 diffSigma = GetStdDev( diffM1, diffM2 );

        #ifdef REBLUR_SH
            diffShM1 *= invSum;
            diffShM2 *= invSum;
            float4 diffShSigma = GetStdDev( diffShM1, diffShM2 );
        #endif

        // RCRS
        float diffLuma = GetLuma( diff );
        float diffLumaClamped = clamp( diffLuma, diffMin.x, diffMax.x );

        [flatten]
        if( gMaxBlurRadius != 0 )
        {
            diff = ChangeLuma( diff, diffLumaClamped );
            diff.w = clamp( diff.w, diffMin.y, diffMax.y );
        }
    #endif

    #ifdef REBLUR_SPECULAR
        // Compute sigma
        specM1 *= invSum;
        specM2 *= invSum;
        float4 specSigma = GetStdDev( specM1, specM2 );

        #ifdef REBLUR_SH
            specShM1 *= invSum;
            specShM2 *= invSum;
            float4 specShSigma = GetStdDev( specShM1, specShM2 );
        #endif

        // RCRS
        float specLuma = GetLuma( spec );
        float specLumaClamped = clamp( specLuma, specMin.x, specMax.x );

        [flatten]
        if( gMaxBlurRadius != 0 )
        {
            spec = ChangeLuma( spec, specLumaClamped );
            spec.w = clamp( spec.w, specMin.y, specMax.y );
        }
    #endif

    // Previous position and surface motion uv
    // Gaijin change:
    // Our motion vector has no UAV, and as we are only reading it, it is bound as SRV instead of UAV in gInOut_Mv.
    float4 inMv = gIn_Mv[ WithRectOrigin( pixelPos ) ];
    float3 mv = inMv.xyz * gMvScale.xyz;
    float3 Xprev = X;
    float2 smbPixelUv = pixelUv + mv.xy;

    if( gMvScale.w != 0.0 )
    {
        Xprev += mv;
        smbPixelUv = STL::Geometry::GetScreenUv( gWorldToClipPrev, Xprev );
    }
    else
    {
        if( gMvScale.z == 0.0 )
            mv.z = STL::Geometry::AffineTransform( gWorldToViewPrev, X ).z - viewZ;

        float viewZprev = viewZ + mv.z;
        float3 Xvprevlocal = STL::Geometry::ReconstructViewPosition( smbPixelUv, gFrustumPrev, viewZprev, gOrthoMode ); // TODO: use gOrthoModePrev

        Xprev = STL::Geometry::RotateVectorInverse( gWorldToViewPrev, Xvprevlocal ) + gCameraDelta.xyz;
    }

    // Shared data
    uint bits;
    float4 data1 = UnpackData1( gIn_Data1[ pixelPos ] );
    float2 data2 = UnpackData2( gIn_Data2[ pixelPos ], bits );

    float stabilizationStrength = gStabilizationStrength * float( pixelUv.x >= gSplitScreen );

    // Surface motion footprint
    STL::Filtering::Bilinear smbBilinearFilter = STL::Filtering::GetBilinearFilter( smbPixelUv, gRectSizePrev );
    float4 smbOcclusion = float4( ( bits & uint4( 1, 2, 4, 8 ) ) != 0 );
    float4 smbOcclusionWeights = STL::Filtering::GetBilinearCustomWeights( smbBilinearFilter, smbOcclusion );
    bool smbAllowCatRom = dot( smbOcclusion, 1.0 ) > 3.5 && REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TS;
    float smbFootprintQuality = STL::Filtering::ApplyBilinearFilter( smbOcclusion.x, smbOcclusion.y, smbOcclusion.z, smbOcclusion.w, smbBilinearFilter );
    smbFootprintQuality = STL::Math::Sqrt01( smbFootprintQuality );

    // Diffuse
    #ifdef REBLUR_DIFFUSE
        // Sample history - surface motion
        REBLUR_TYPE smbDiffHistory;
        REBLUR_SH_TYPE smbDiffShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gIn_Diff_StabilizedHistory, smbDiffHistory
            #ifdef REBLUR_SH
                , gIn_DiffSh_StabilizedHistory, smbDiffShHistory
            #endif
        );

        // Avoid negative values
        smbDiffHistory = ClampNegativeToZero( smbDiffHistory );

        // Compute antilag
        float diffStabilizationStrength = stabilizationStrength * float( smbPixelUv.x >= gSplitScreen );
        float diffAntilag = ComputeAntilag( smbDiffHistory, diff, diffSigma, gAntilagParams, smbFootprintQuality * data1.x );

        // Clamp history and combine with the current frame
        float2 diffTemporalAccumulationParams = GetTemporalAccumulationParams( smbFootprintQuality, data1.x );

        float diffHistoryWeight = diffTemporalAccumulationParams.x;
        diffHistoryWeight *= diffAntilag; // this is important
        diffHistoryWeight *= diffStabilizationStrength;

        smbDiffHistory = STL::Color::Clamp( diffM1, diffSigma * diffTemporalAccumulationParams.y, smbDiffHistory );
        float4 diffResult = lerp( diff, smbDiffHistory, diffHistoryWeight );
        #ifdef REBLUR_SH
            smbDiffShHistory = STL::Color::Clamp( diffShM1, diffShSigma * diffTemporalAccumulationParams.y, smbDiffShHistory );
            float4 diffShResult = lerp( diffSh, smbDiffShHistory, diffHistoryWeight );
        #endif

        // Output
        gOut_Diff[ pixelPos ] = diffResult;
        #ifdef REBLUR_SH
            gOut_DiffSh[ pixelPos ] = diffShResult;
        #endif

        // Increment history length
        data1.x += 1.0;

        // Apply anti-lag
        float diffMinAccumSpeed = min( data1.x, gHistoryFixFrameNum ) * REBLUR_USE_ANTILAG_NOT_INVOKING_HISTORY_FIX;
        data1.x = lerp( diffMinAccumSpeed, data1.x, diffAntilag );
    #endif

    // Specular
    #ifdef REBLUR_SPECULAR
        float virtualHistoryAmount = data2.x;
        float curvature = data2.y;

        // Hit distance for tracking ( tests 6, 67, 155 )
        float hitDistForTracking = min( spec.w, specMin.y ) * _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness );

        // Needed to preserve contact ( test 3, 8 ), but adds pixelation in some cases ( test 160 ). More fun if lobe trimming is off.
        [flatten]
        if( gSpecPrepassBlurRadius != 0.0 )
            hitDistForTracking = min( hitDistForTracking, gIn_Spec_HitDistForTracking[ pixelPos ] );

        // Virtual motion
        float3 V = GetViewVector( X );
        float NoV = abs( dot( N, V ) );
        float dominantFactor = STL::ImportanceSampling::GetSpecularDominantFactor( NoV, roughness, STL_SPECULAR_DOMINANT_DIRECTION_G2 );
        float3 Xvirtual = GetXvirtual( NoV, hitDistForTracking, curvature, X, Xprev, V, dominantFactor );
        float2 vmbPixelUv = STL::Geometry::GetScreenUv( gWorldToClipPrev, Xvirtual );

        // Modify MVs if requested
        if( gSpecProbabilityThresholdsForMvModification.x < 1.0 )
        {
            float4 baseColorMetalness = gIn_BaseColor_Metalness[ WithRectOrigin( pixelPos ) ];

            float3 albedo, Rf0;
            STL::BRDF::ConvertBaseColorMetalnessToAlbedoRf0( baseColorMetalness.xyz, baseColorMetalness.w, albedo, Rf0 );

            float3 Fenv = STL::BRDF::EnvironmentTerm_Rtg( Rf0, NoV, roughness );

            float lumSpec = STL::Color::Luminance( Fenv ) * virtualHistoryAmount;
            float lumDiff = STL::Color::Luminance( albedo * ( 1.0 - Fenv ) );
            float specProb = lumSpec / ( lumDiff + lumSpec + NRD_EPS );

            STL::Rng::Hash::Initialize( pixelPos, gFrameIndex );
            float f = STL::Math::SmoothStep( gSpecProbabilityThresholdsForMvModification.x, gSpecProbabilityThresholdsForMvModification.y, specProb );
            if( STL::Rng::Hash::GetFloat( ) < f )
            {
                float3 specMv = Xvirtual - X; // TODO: world-space delta fits badly into FP16
                if( gMvScale.w == 0.0 )
                {
                    specMv.xy = vmbPixelUv - pixelUv;
                    specMv.z = STL::Geometry::AffineTransform( gWorldToViewPrev, Xvirtual ).z - viewZ; // TODO: is it useful?
                }

                mv = specMv;

                // Modify only .xy for 2D and .xyz for 2.5D and 3D MVs
                inMv.xy = mv.xy / gMvScale.xy;
                inMv.z = gMvScale.z == 0.0 ? inMv.z : mv.z / gMvScale.z;

                gInOut_Mv[ WithRectOrigin( pixelPos ) ] = inMv;
            }
        }

        // Sample history - surface motion
        REBLUR_TYPE smbSpecHistory;
        REBLUR_SH_TYPE smbSpecShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gIn_Spec_StabilizedHistory, smbSpecHistory
            #ifdef REBLUR_SH
                , gIn_SpecSh_StabilizedHistory, smbSpecShHistory
            #endif
        );

        // Virtual motion footprint
        STL::Filtering::Bilinear vmbBilinearFilter = STL::Filtering::GetBilinearFilter( vmbPixelUv, gRectSizePrev );
        float4 vmbOcclusion = float4( ( bits & uint4( 16, 32, 64, 128 ) ) != 0 );
        float4 vmbOcclusionWeights = STL::Filtering::GetBilinearCustomWeights( vmbBilinearFilter, vmbOcclusion );
        bool vmbAllowCatRom = dot( vmbOcclusion, 1.0 ) > 3.5 && REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TS;
        float vmbFootprintQuality = STL::Filtering::ApplyBilinearFilter( vmbOcclusion.x, vmbOcclusion.y, vmbOcclusion.z, vmbOcclusion.w, vmbBilinearFilter );
        vmbFootprintQuality = STL::Math::Sqrt01( vmbFootprintQuality );

        // Sample history - virtual motion
        REBLUR_TYPE vmbSpecHistory;
        REBLUR_SH_TYPE vmbSpecShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( vmbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            vmbOcclusionWeights, vmbAllowCatRom,
            gIn_Spec_StabilizedHistory, vmbSpecHistory
            #ifdef REBLUR_SH
                , gIn_SpecSh_StabilizedHistory, vmbSpecShHistory
            #endif
        );

        // Avoid negative values
        smbSpecHistory = ClampNegativeToZero( smbSpecHistory );
        vmbSpecHistory = ClampNegativeToZero( vmbSpecHistory );

        // Combine surface and virtual motion
        float4 specHistory = lerp( smbSpecHistory, vmbSpecHistory, virtualHistoryAmount );
        #ifdef REBLUR_SH
            float4 specShHistory = lerp( smbSpecShHistory, vmbSpecShHistory, virtualHistoryAmount );
        #endif

        // Compute antilag
        float specStabilizationStrength = stabilizationStrength;
        [flatten]
        if( virtualHistoryAmount != 1.0 )
            specStabilizationStrength *= float( smbPixelUv.x >= gSplitScreen );
        [flatten]
        if( virtualHistoryAmount != 0.0 )
            specStabilizationStrength *= float( vmbPixelUv.x >= gSplitScreen );

        float footprintQuality = lerp( smbFootprintQuality, vmbFootprintQuality, virtualHistoryAmount );
        float specAntilag = ComputeAntilag( specHistory, spec, specSigma, gAntilagParams, footprintQuality * data1.z );

        // Gaijin change:
        // Specular history confidence should be used here as well.
        if( gHasHistoryConfidence )
          specAntilag *= gIn_Spec_Confidence[ WithRectOrigin( pixelPos ) ];

        // Clamp history and combine with the current frame
        float2 specTemporalAccumulationParams = GetTemporalAccumulationParams( footprintQuality, data1.z );

        // TODO: roughness should affect stabilization:
        // - use "virtualHistoryRoughnessBasedConfidence" from TA
        // - compute moments for samples with similar roughness
        float specHistoryWeight = specTemporalAccumulationParams.x;
        specHistoryWeight *= specAntilag; // this is important
        specHistoryWeight *= specStabilizationStrength;

        specHistory = STL::Color::Clamp( specM1, specSigma * specTemporalAccumulationParams.y, specHistory );
        float4 specResult = lerp( spec, specHistory, specHistoryWeight );
        #ifdef REBLUR_SH
            specShHistory = STL::Color::Clamp( specShM1, specShSigma * specTemporalAccumulationParams.y, specShHistory );
            float4 specShResult = lerp( specSh, specShHistory, specHistoryWeight );

            // ( Optional ) Output modified roughness to assist AA during SG resolve
            specShResult.w = specSh.w;
        #endif

        // Output
        gOut_Spec[ pixelPos ] = specResult;
        #ifdef REBLUR_SH
            gOut_SpecSh[ pixelPos ] = specShResult;
        #endif

        // Increment history length
        data1.z += 1.0;

        // Apply anti-lag
        float specMinAccumSpeed = min( data1.z, gHistoryFixFrameNum ) * REBLUR_USE_ANTILAG_NOT_INVOKING_HISTORY_FIX;
        data1.z = lerp( specMinAccumSpeed, data1.z, specAntilag );
    #endif

    gOut_InternalData[ pixelPos ] = PackInternalData( data1.x, data1.z, materialID );
}
