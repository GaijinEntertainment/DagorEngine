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
#include "REBLUR_TemporalStabilization.resources.hlsli"

#include "Common.hlsli"

#include "REBLUR_Common.hlsli"

groupshared float s_DiffLuma[ BUFFER_Y ][ BUFFER_X ];
groupshared float s_SpecLuma[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( globalPos ) ] );

    #if( NRD_DIFF )
        float diffLuma = GetLuma( gIn_Diff[ globalPos ] );
        s_DiffLuma[ sharedPos.y ][ sharedPos.x ] = viewZ > gDenoisingRange ? REBLUR_INVALID : diffLuma;
    #endif

    #if( NRD_SPEC )
        float specLuma = GetLuma( gIn_Spec[ globalPos ] );
        s_SpecLuma[ sharedPos.y ][ sharedPos.x ] = viewZ > gDenoisingRange ? REBLUR_INVALID : specLuma;
    #endif
}

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
        return; // IMPORTANT: no data output, must be rejected by the "viewZ" check!

    // Position
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 X = Geometry::RotateVector( gViewToWorld, Xv );

    // Previous position and surface motion uv
    // Gaijin change:
    // Our motion vector has no UAV, and as we are only reading it, it is bound as SRV instead of UAV in gInOut_Mv.
    float4 inMv = gIn_Mv[ WithRectOrigin( pixelPos ) ];
    float3 mv = inMv.xyz * gMvScale.xyz;
    float3 Xprev = X;
    float2 smbPixelUv = pixelUv + mv.xy;

    if( gMvScale.w == 0.0 )
    {
        if( gMvScale.z == 0.0 )
            mv.z = Geometry::AffineTransform( gWorldToViewPrev, X ).z - viewZ;

        float viewZprev = viewZ + mv.z;
        float3 Xvprevlocal = Geometry::ReconstructViewPosition( smbPixelUv, gFrustumPrev, viewZprev, gOrthoMode ); // TODO: use gOrthoModePrev

        Xprev = Geometry::RotateVectorInverse( gWorldToViewPrev, Xvprevlocal ) + gCameraDelta.xyz;
    }
    else
    {
        Xprev += mv;
        smbPixelUv = Geometry::GetScreenUv( gWorldToClipPrev, Xprev );
    }

    // Normal and roughness
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    // Shared data
    uint bits;
    bool smbAllowCatRom;
    REBLUR_DATA1_TYPE data1 = UnpackData1( gIn_Data1[ pixelPos ] );
    float2 data2 = UnpackData2( gIn_Data2[ pixelPos ], bits, smbAllowCatRom );

    // Surface motion footprint
    Filtering::Bilinear smbBilinearFilter = Filtering::GetBilinearFilter( smbPixelUv, gRectSizePrev );
    float4 smbOcclusion = float4( ( bits & uint4( 1, 2, 4, 8 ) ) != 0 );

    float4 smbOcclusionWeights = Filtering::GetBilinearCustomWeights( smbBilinearFilter, smbOcclusion );
    float smbFootprintQuality = Filtering::ApplyBilinearFilter( smbOcclusion.x, smbOcclusion.y, smbOcclusion.z, smbOcclusion.w, smbBilinearFilter );
    smbFootprintQuality = Math::Sqrt01( smbFootprintQuality );

    int2 smemPos = threadPos + NRD_BORDER;

    // Diffuse
    #if( NRD_DIFF )
        float diffLuma = s_DiffLuma[ smemPos.y ][ smemPos.x ];
        float diffLumaM1 = diffLuma;
        float diffLumaM2 = diffLuma * diffLuma;

        [unroll]
        for( j = 0; j <= NRD_BORDER * 2; j++ )
        {
            [unroll]
            for( i = 0; i <= NRD_BORDER * 2; i++ )
            {
                if( i == NRD_BORDER && j == NRD_BORDER )
                    continue;

                int2 pos = threadPos + int2( i, j );

                // Accumulate moments
                float d = s_DiffLuma[ pos.y ][ pos.x ];
                d = d == REBLUR_INVALID ? diffLuma : d;

                diffLumaM1 += d;
                diffLumaM2 += d * d;
            }
        }

        // Compute sigma
        diffLumaM1 /= ( NRD_BORDER * 2 + 1 ) * ( NRD_BORDER * 2 + 1 );
        diffLumaM2 /= ( NRD_BORDER * 2 + 1 ) * ( NRD_BORDER * 2 + 1 );

        float diffLumaSigma = GetStdDev( diffLumaM1, diffLumaM2 );

        // Clean-up fireflies if HistoryFix pass was in action
        if( data1.x < gHistoryFixFrameNum )
            diffLuma = min( diffLuma, diffLumaM1 * ( 1.2 + 1.0 / ( 1.0 + data1.x ) ) );

        // Sample history - surface motion
        float smbDiffLumaHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights1(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gHistory_DiffLumaStabilized, smbDiffLumaHistory
        );

        // Avoid negative values
        smbDiffLumaHistory = max( smbDiffLumaHistory, 0.0 );

        // Compute antilag
        float diffAntilag = ComputeAntilag( smbDiffLumaHistory, diffLumaM1, diffLumaSigma, smbFootprintQuality * data1.x );

        float diffMinAccumSpeed = min( data1.x, gHistoryFixFrameNum ) * REBLUR_USE_ANTILAG_NOT_INVOKING_HISTORY_FIX;
        data1.x = lerp( diffMinAccumSpeed, data1.x, diffAntilag );

        // Clamp history and combine with the current frame
        float2 diffTemporalAccumulationParams = GetTemporalAccumulationParams( smbFootprintQuality, data1.x, diffAntilag );

        float diffHistoryWeight = diffTemporalAccumulationParams.x;
        diffHistoryWeight *= float( pixelUv.x >= gSplitScreen );
        diffHistoryWeight *= float( smbPixelUv.x >= gSplitScreenPrev );

        smbDiffLumaHistory = Color::Clamp( diffLumaM1, diffLumaSigma * diffTemporalAccumulationParams.y, smbDiffLumaHistory );

        float diffLumaStabilized = lerp( diffLuma, smbDiffLumaHistory, min( diffHistoryWeight, gStabilizationStrength ) );

        REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
        diff = ChangeLuma( diff, diffLumaStabilized );
        #if( NRD_MODE == SH )
            REBLUR_SH_TYPE diffSh = gIn_DiffSh[ pixelPos ];
            diffSh.xyz *= GetLumaScale( length( diffSh.xyz ), diffLumaStabilized );
        #endif

        // Output
        diff.w = gReturnHistoryLengthInsteadOfOcclusion ? data1.x : diff.w;

        gOut_Diff[ pixelPos ] = diff;
        gOut_DiffLumaStabilized[ pixelPos ] = diffLumaStabilized;
        #if( NRD_MODE == SH )
            gOut_DiffSh[ pixelPos ] = diffSh;
        #endif
    #endif

    // Specular
    #if( NRD_SPEC )
        float specLuma = s_SpecLuma[ smemPos.y ][ smemPos.x ];
        float specLumaM1 = specLuma;
        float specLumaM2 = specLuma * specLuma;

        [unroll]
        for( j = 0; j <= NRD_BORDER * 2; j++ )
        {
            [unroll]
            for( i = 0; i <= NRD_BORDER * 2; i++ )
            {
                if( i == NRD_BORDER && j == NRD_BORDER )
                    continue;

                int2 pos = threadPos + int2( i, j );

                // Accumulate moments
                float s = s_SpecLuma[ pos.y ][ pos.x ];
                s = s == REBLUR_INVALID ? specLuma : s;

                specLumaM1 += s;
                specLumaM2 += s * s;
            }
        }

        // Compute sigma
        specLumaM1 /= ( NRD_BORDER * 2 + 1 ) * ( NRD_BORDER * 2 + 1 );
        specLumaM2 /= ( NRD_BORDER * 2 + 1 ) * ( NRD_BORDER * 2 + 1 );

        float specLumaSigma = GetStdDev( specLumaM1, specLumaM2 );

        // Clean-up fireflies if HistoryFix pass was in action
        if( data1.y < gHistoryFixFrameNum )
            specLuma = min( specLuma, specLumaM1 * ( 1.2 + 1.0 / ( 1.0 + data1.y ) ) );

        // Hit distance for tracking ( tests 6, 67, 155 )
        REBLUR_TYPE spec = gIn_Spec[ pixelPos ];
        float hitDistForTracking = ExtractHitDist( spec ) * _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistSettings, roughness ); // TODO: min in 3x3 seems to be not needed here

        // Needed to preserve contact ( test 3, 8 ), but adds pixelation in some cases ( test 149, 160 ), more fun if lobe trimming is off
        [flatten]
        if( gSpecPrepassBlurRadius != 0.0 )
            hitDistForTracking = min( hitDistForTracking, gIn_SpecHitDistForTracking[ pixelPos ] );

        // Virtual motion
        float virtualHistoryAmount = data2.x;
        float curvature = data2.y;

        float3 V = GetViewVector( X );
        float NoV = abs( dot( N, V ) );
        float3 Xvirtual = GetXvirtual( hitDistForTracking, curvature, X, Xprev, N, V, roughness );

        float2 vmbPixelUv = Geometry::GetScreenUv( gWorldToClipPrev, Xvirtual );
        // Gaijin change:
        // Use smbPixelUv for camera-attached reflections, to improve stability. (Like it's done in REBLUR_TemporalAccumulation.cs)
        vmbPixelUv = materialID == gCameraAttachedReflectionMaterialID ? smbPixelUv : vmbPixelUv;

        // Modify MVs if requested
        if( gSpecProbabilityThresholdsForMvModification.x < 1.0 && NRD_SUPPORTS_BASECOLOR_METALNESS )
        {
            float4 baseColorMetalness = gIn_BaseColor_Metalness[ WithRectOrigin( pixelPos ) ];

            float3 albedo, Rf0;
            BRDF::ConvertBaseColorMetalnessToAlbedoRf0( baseColorMetalness.xyz, baseColorMetalness.w, albedo, Rf0 );

            float3 Fenv = BRDF::EnvironmentTerm_Rtg( Rf0, NoV, roughness );

            float lumSpec = Color::Luminance( Fenv );
            float lumDiff = Color::Luminance( albedo * ( 1.0 - Fenv ) );
            float specProb = lumSpec / ( lumDiff + lumSpec + NRD_EPS );

            float f = Math::SmoothStep( gSpecProbabilityThresholdsForMvModification.x, gSpecProbabilityThresholdsForMvModification.y, specProb );
            f *= 1.0 - GetSpecMagicCurve( roughness );
            f *= 1.0 - Math::Sqrt01( abs( curvature ) );

            if( f != 0.0 )
            {
                float3 specMv = Xvirtual - X; // world-space delta fits badly into FP16! Prefer 2.5D motion!
                if( gMvScale.w == 0.0 )
                {
                    specMv.xy = vmbPixelUv - pixelUv;
                    specMv.z = Geometry::AffineTransform( gWorldToViewPrev, Xvirtual ).z - viewZ; // TODO: is it useful?
                }

                // Modify only .xy for 2D and .xyz for 2.5D and 3D MVs
                mv.xy = specMv.xy / gMvScale.xy;
                mv.z = gMvScale.z == 0.0 ? inMv.z : specMv.z / gMvScale.z;

                inMv.xyz = lerp( inMv.xyz, mv, f );

                // Gaijin change:
                // Only have gIn_Mv, and this branch is disabled
                // gInOut_Mv[ WithRectOrigin( pixelPos ) ] = inMv;
			}
        }

        // Sample history - surface motion
        float smbSpecLumaHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights1(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gHistory_SpecLumaStabilized, smbSpecLumaHistory
        );

        // Virtual motion footprint
        Filtering::Bilinear vmbBilinearFilter = Filtering::GetBilinearFilter( vmbPixelUv, gRectSizePrev );
        float4 vmbOcclusion = float4( ( bits & uint4( 16, 32, 64, 128 ) ) != 0 );
        float4 vmbOcclusionWeights = Filtering::GetBilinearCustomWeights( vmbBilinearFilter, vmbOcclusion );
        bool vmbAllowCatRom = dot( vmbOcclusion, 1.0 ) > 3.5 && REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TS;
        vmbAllowCatRom = vmbAllowCatRom && smbAllowCatRom; // helps to reduce over-sharpening in disoccluded areas
        float vmbFootprintQuality = Filtering::ApplyBilinearFilter( vmbOcclusion.x, vmbOcclusion.y, vmbOcclusion.z, vmbOcclusion.w, vmbBilinearFilter );
        vmbFootprintQuality = Math::Sqrt01( vmbFootprintQuality );

        // Sample history - virtual motion
        float vmbSpecLumaHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights1(
            saturate( vmbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            vmbOcclusionWeights, vmbAllowCatRom,
            gHistory_SpecLumaStabilized, vmbSpecLumaHistory
        );

        // Avoid negative values
        smbSpecLumaHistory = max( smbSpecLumaHistory, 0.0 );
        vmbSpecLumaHistory = max( vmbSpecLumaHistory, 0.0 );

        // Combine surface and virtual motion
        float specLumaHistory = lerp( smbSpecLumaHistory, vmbSpecLumaHistory, virtualHistoryAmount );

        // Compute antilag
        float footprintQuality = lerp( smbFootprintQuality, vmbFootprintQuality, virtualHistoryAmount );
        float specAntilag = ComputeAntilag( specLumaHistory, specLumaM1, specLumaSigma, footprintQuality * data1.y );

        float specMinAccumSpeed = min( data1.y, gHistoryFixFrameNum ) * REBLUR_USE_ANTILAG_NOT_INVOKING_HISTORY_FIX;
        data1.y = lerp( specMinAccumSpeed, data1.y, specAntilag );

        // Clamp history and combine with the current frame
        float2 specTemporalAccumulationParams = GetTemporalAccumulationParams( footprintQuality, data1.y, specAntilag );

        // TODO: roughness should affect stabilization:
        // - use "virtualHistoryRoughnessBasedConfidence" from TA
        // - compute moments for samples with similar roughness
        float specHistoryWeight = specTemporalAccumulationParams.x;
        specHistoryWeight *= float( pixelUv.x >= gSplitScreen );
        specHistoryWeight *= virtualHistoryAmount != 1.0 ? float( smbPixelUv.x >= gSplitScreenPrev ) : 1.0;
        specHistoryWeight *= virtualHistoryAmount != 0.0 ? float( vmbPixelUv.x >= gSplitScreenPrev ) : 1.0;

        float responsiveFactor = RemapRoughnessToResponsiveFactor( roughness );
        float smc = GetSpecMagicCurve( roughness );
        float acceleration = lerp( smc, 1.0, 0.5 + responsiveFactor * 0.5 );
        if( materialID == gStrandMaterialID )
            acceleration = min( acceleration, 0.5 );

        specHistoryWeight *= acceleration;

        specLumaHistory = Color::Clamp( specLumaM1, specLumaSigma * specTemporalAccumulationParams.y, specLumaHistory );

        float specLumaStabilized = lerp( specLuma, specLumaHistory, min( specHistoryWeight, gStabilizationStrength ) );

        spec = ChangeLuma( spec, specLumaStabilized );
        #if( NRD_MODE == SH )
            REBLUR_SH_TYPE specSh = gIn_SpecSh[ pixelPos ];
            specSh.xyz *= GetLumaScale( length( specSh.xyz ), specLumaStabilized );
        #endif

        // Output
        spec.w = gReturnHistoryLengthInsteadOfOcclusion ? data1.y : spec.w;

        gOut_Spec[ pixelPos ] = spec;
        gOut_SpecLumaStabilized[ pixelPos ] = specLumaStabilized;
        #if( NRD_MODE == SH )
            gOut_SpecSh[ pixelPos ] = specSh;
        #endif
    #endif

    gOut_InternalData[ pixelPos ] = PackInternalData( data1.x, data1.y, materialID );
}
