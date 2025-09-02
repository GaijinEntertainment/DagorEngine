/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

groupshared float4 s_Normal_Roughness[ BUFFER_Y ][ BUFFER_X ];
groupshared float s_HitDistForTracking[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSizeMinusOne );

    s_Normal_Roughness[ sharedPos.y ][ sharedPos.x ] = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( globalPos ) ] );

    #ifdef REBLUR_SPECULAR
        #ifdef REBLUR_OCCLUSION
            uint shift = gSpecCheckerboard != 2 ? 1 : 0;
            uint2 pos = uint2( globalPos.x >> shift, globalPos.y );
        #else
            uint2 pos = globalPos;
        #endif

        REBLUR_TYPE spec = gIn_Spec[ pos ];
        #ifdef REBLUR_OCCLUSION
            float hitDist = ExtractHitDist( spec );
        #else
            float hitDist = gSpecPrepassBlurRadius == 0.0 ? ExtractHitDist( spec ) : gIn_SpecHitDistForTracking[ globalPos ];
        #endif

        s_HitDistForTracking[ sharedPos.y ][ sharedPos.x ] = hitDist == 0.0 ? NRD_INF : hitDist;
    #endif
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

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

    // Current position
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 X = Geometry::RotateVector( gViewToWorld, Xv );

    // Find hit distance for tracking, averaged normal and roughness variance
    float3 Navg = 0.0;
    #ifdef REBLUR_SPECULAR
        float hitDistForTracking = NRD_INF;
        float roughnessM1 = 0.0;
        float roughnessM2 = 0.0;
    #endif

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            int2 pos = threadPos + int2( i, j );
            float4 normalAndRoughness = s_Normal_Roughness[ pos.y ][ pos.x ];

            // Average normal
            if( i < 2 && j < 2 )
                Navg += normalAndRoughness.xyz;

            #ifdef REBLUR_SPECULAR
                // Min hit distance for tracking, ignoring 0 values ( which still can be produced by VNDF sampling )
                float h = s_HitDistForTracking[ pos.y ][ pos.x ];
                hitDistForTracking = min( hitDistForTracking, h );

                // Roughness variance
                // IMPORTANT: squared because the test uses "roughness ^ 2"
                float roughnessSq = normalAndRoughness.w * normalAndRoughness.w;
                roughnessM1 += roughnessSq;
                roughnessM2 += roughnessSq * roughnessSq;
            #endif
        }
    }

    Navg /= 4.0; // needs to be unnormalized!

    // Normal and roughness
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    #ifdef REBLUR_SPECULAR
        float roughnessModified = Filtering::GetModifiedRoughnessFromNormalVariance( roughness, Navg ); // TODO: needed?

        roughnessM1 /= ( 1 + BORDER * 2 ) * ( 1 + BORDER * 2 );
        roughnessM2 /= ( 1 + BORDER * 2 ) * ( 1 + BORDER * 2 );
        float roughnessSigma = GetStdDev( roughnessM1, roughnessM2 );
    #endif

    // Hit distance for tracking ( tests 8, 110, 139, e3, e9 without normal map, e24 )
    #ifdef REBLUR_SPECULAR
        #if( REBLUR_USE_STF == 1 && NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
            Rng::Hash::Initialize( pixelPos, gFrameIndex );
        #endif

        hitDistForTracking = hitDistForTracking == NRD_INF ? 0.0 : hitDistForTracking;

        float hitDistNormalization = _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness );
        #ifdef REBLUR_OCCLUSION
            hitDistForTracking *= hitDistNormalization;
        #else
            hitDistForTracking *= gSpecPrepassBlurRadius == 0.0 ? hitDistNormalization : 1.0;
        #endif

        gOut_SpecHitDistForTracking[ pixelPos ] = hitDistForTracking;
    #endif

    // Previous position and surface motion uv
    float3 mv = gIn_Mv[ WithRectOrigin( pixelPos ) ] * gMvScale.xyz;
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

    // Previous viewZ ( 4x4, surface motion )
    /*
          Gather      => CatRom12    => Bilinear
        0x 0y 1x 1y       0y 1x
        0z 0w 1z 1w    0z 0w 1z 1w       0w 1z
        2x 2y 3x 3y    2x 2y 3x 3y       2y 3x
        2z 2w 3z 3w       2w 3z

         CatRom12     => Bilinear
           0x 1x
        0y 0z 1y 1z       0z 1y
        2x 2y 3x 3y       2y 3x
           2z 3z
    */
    Filtering::CatmullRom smbCatromFilter = Filtering::GetCatmullRomFilter( smbPixelUv, gRectSizePrev );
    float2 smbCatromGatherUv = smbCatromFilter.origin * gResourceSizeInvPrev;
    float4 smbViewZ0 = gPrev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 1 ) ).wzxy;
    float4 smbViewZ1 = gPrev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 1 ) ).wzxy;
    float4 smbViewZ2 = gPrev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 3 ) ).wzxy;
    float4 smbViewZ3 = gPrev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 3 ) ).wzxy;

    float3 prevViewZ0 = UnpackViewZ( smbViewZ0.yzw );
    float3 prevViewZ1 = UnpackViewZ( smbViewZ1.xzw );
    float3 prevViewZ2 = UnpackViewZ( smbViewZ2.xyw );
    float3 prevViewZ3 = UnpackViewZ( smbViewZ3.xyz );

    // Previous normal averaged for all pixels in 2x2 footprint
    // IMPORTANT: bilinear filter can touch sky pixels, due to this reason "Post Blur" writes special values into sky-pixels
    Filtering::Bilinear smbBilinearFilter = Filtering::GetBilinearFilter( smbPixelUv, gRectSizePrev );
    float3 smbNavg = 0; // TODO: the sum works well, but probably there is a potential to use just "1 smart sample" (see test 168)
    {
        uint2 p = uint2( smbBilinearFilter.origin );
        float sum = 0.0;

        float w = float( prevViewZ0.z < gDenoisingRange );
        smbNavg = NRD_FrontEnd_UnpackNormalAndRoughness( gPrev_Normal_Roughness[ p ] ).xyz * w;
        sum += w;

        w = float( prevViewZ1.y < gDenoisingRange );
        smbNavg += NRD_FrontEnd_UnpackNormalAndRoughness( gPrev_Normal_Roughness[ p + uint2( 1, 0 ) ] ).xyz * w;
        sum += w;

        w = float( prevViewZ2.y < gDenoisingRange );
        smbNavg += NRD_FrontEnd_UnpackNormalAndRoughness( gPrev_Normal_Roughness[ p + uint2( 0, 1 ) ] ).xyz * w;
        sum += w;

        w = float( prevViewZ3.x < gDenoisingRange );
        smbNavg += NRD_FrontEnd_UnpackNormalAndRoughness( gPrev_Normal_Roughness[ p + uint2( 1, 1 ) ] ).xyz * w;
        sum += w;

        smbNavg /= sum == 0.0 ? 1.0 : sum;
    }
    smbNavg = Geometry::RotateVector( gWorldPrevToWorld, smbNavg );

    // Parallax
    float smbParallaxInPixels1 = ComputeParallaxInPixels( Xprev + gCameraDelta.xyz, gOrthoMode == 0.0 ? smbPixelUv : pixelUv, gWorldToClipPrev, gRectSize );
    float smbParallaxInPixels2 = ComputeParallaxInPixels( Xprev - gCameraDelta.xyz, gOrthoMode == 0.0 ? pixelUv : smbPixelUv, gWorldToClip, gRectSize );

    float smbParallaxInPixelsMax = max( smbParallaxInPixels1, smbParallaxInPixels2 );
    float smbParallaxInPixelsMin = min( smbParallaxInPixels1, smbParallaxInPixels2 );

    // Disocclusion: threshold
    float pixelSize = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ );
    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );

    float disocclusionThresholdMix = 0;
    if( materialID == gStrandMaterialID )
        disocclusionThresholdMix = NRD_GetNormalizedStrandThickness( gStrandThickness, pixelSize );
    if( gHasDisocclusionThresholdMix && NRD_USE_DISOCCLUSION_THRESHOLD_MIX )
        disocclusionThresholdMix = gIn_DisocclusionThresholdMix[ WithRectOrigin( pixelPos ) ];

    float disocclusionThreshold = lerp( gDisocclusionThreshold, gDisocclusionThresholdAlternate, disocclusionThresholdMix );

    // TODO: small parallax ( very slow motion ) could be used to increase disocclusion threshold, but:
    // - MVs should be dilated first
    // - Problem: a static pixel ( with relaxed threshold ) can touch a moving pixel, leading to reprojection artefacts
    float smallParallax = Math::LinearStep( 0.25, 0.0, smbParallaxInPixelsMax );
    float thresholdAngle = REBLUR_ALMOST_ZERO_ANGLE - 0.25 * smallParallax;

    float3 V = GetViewVector( X );
    float NoV = abs( dot( N, V ) );
    float NoVstrict = lerp( NoV, 1.0, saturate( smbParallaxInPixelsMax / 30.0 ) );
    float4 smbDisocclusionThreshold = GetDisocclusionThreshold( disocclusionThreshold, frustumSize, NoVstrict );
    smbDisocclusionThreshold *= float( dot( smbNavg, Navg ) > thresholdAngle ); // good for smb
    smbDisocclusionThreshold *= IsInScreenBilinear( smbBilinearFilter.origin, gRectSizePrev );
    smbDisocclusionThreshold -= NRD_EPS;

    // Disocclusion: plane distance
    float3 Xvprev = Geometry::AffineTransform( gWorldToViewPrev, Xprev );
    float3 smbPlaneDist0 = abs( prevViewZ0 - Xvprev.z );
    float3 smbPlaneDist1 = abs( prevViewZ1 - Xvprev.z );
    float3 smbPlaneDist2 = abs( prevViewZ2 - Xvprev.z );
    float3 smbPlaneDist3 = abs( prevViewZ3 - Xvprev.z );
    float3 smbOcclusion0 = step( smbPlaneDist0, smbDisocclusionThreshold.x );
    float3 smbOcclusion1 = step( smbPlaneDist1, smbDisocclusionThreshold.y );
    float3 smbOcclusion2 = step( smbPlaneDist2, smbDisocclusionThreshold.z );
    float3 smbOcclusion3 = step( smbPlaneDist3, smbDisocclusionThreshold.w );

    // Disocclusion: materialID
    #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        uint4 smbInternalData0 = gPrev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 1 ) ).wzxy;
        uint4 smbInternalData1 = gPrev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 1 ) ).wzxy;
        uint4 smbInternalData2 = gPrev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 3 ) ).wzxy;
        uint4 smbInternalData3 = gPrev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 3 ) ).wzxy;

        float3 smbMaterialID0 = float3( UnpackInternalData( smbInternalData0.y ).z, UnpackInternalData( smbInternalData0.z ).z, UnpackInternalData( smbInternalData0.w ).z );
        float3 smbMaterialID1 = float3( UnpackInternalData( smbInternalData1.x ).z, UnpackInternalData( smbInternalData1.z ).z, UnpackInternalData( smbInternalData1.w ).z );
        float3 smbMaterialID2 = float3( UnpackInternalData( smbInternalData2.x ).z, UnpackInternalData( smbInternalData2.y ).z, UnpackInternalData( smbInternalData2.w ).z );
        float3 smbMaterialID3 = float3( UnpackInternalData( smbInternalData3.x ).z, UnpackInternalData( smbInternalData3.y ).z, UnpackInternalData( smbInternalData3.z ).z );

        float minMaterialID = min( gSpecMinMaterial, gDiffMinMaterial ); // TODO: separation is expensive
        smbOcclusion0 *= CompareMaterials( materialID, smbMaterialID0, minMaterialID );
        smbOcclusion1 *= CompareMaterials( materialID, smbMaterialID1, minMaterialID );
        smbOcclusion2 *= CompareMaterials( materialID, smbMaterialID2, minMaterialID );
        smbOcclusion3 *= CompareMaterials( materialID, smbMaterialID3, minMaterialID );

        uint4 smbInternalData = uint4( smbInternalData0.w, smbInternalData1.z, smbInternalData2.y, smbInternalData3.x );
    #else
        float2 smbBilinearGatherUv = ( smbBilinearFilter.origin + 1.0 ) * gResourceSizeInvPrev;
        uint4 smbInternalData = gPrev_InternalData.GatherRed( gNearestClamp, smbBilinearGatherUv ).wzxy;
    #endif

    // 2x2 occlusion weights
    float4 smbOcclusionWeights = Filtering::GetBilinearCustomWeights( smbBilinearFilter, float4( smbOcclusion0.z, smbOcclusion1.y, smbOcclusion2.y, smbOcclusion3.x ) );
    bool smbAllowCatRom = dot( smbOcclusion0 + smbOcclusion1 + smbOcclusion2 + smbOcclusion3, 1.0 ) > 11.5 && REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TA;

    float fbits = smbOcclusion0.z * 1.0;
    fbits += smbOcclusion1.y * 2.0;
    fbits += smbOcclusion2.y * 4.0;
    fbits += smbOcclusion3.x * 8.0;

    // Accumulation speed
    float2 internalData00 = UnpackInternalData( smbInternalData.x ).xy;
    float2 internalData10 = UnpackInternalData( smbInternalData.y ).xy;
    float2 internalData01 = UnpackInternalData( smbInternalData.z ).xy;
    float2 internalData11 = UnpackInternalData( smbInternalData.w ).xy;

    #ifdef REBLUR_DIFFUSE
        float4 diffAccumSpeeds = float4( internalData00.x, internalData10.x, internalData01.x, internalData11.x );
        float diffAccumSpeed = Filtering::ApplyBilinearCustomWeights( diffAccumSpeeds.x, diffAccumSpeeds.y, diffAccumSpeeds.z, diffAccumSpeeds.w, smbOcclusionWeights );
    #endif

    #ifdef REBLUR_SPECULAR
        float4 specAccumSpeeds = float4( internalData00.y, internalData10.y, internalData01.y, internalData11.y );
        float smbSpecAccumSpeed = Filtering::ApplyBilinearCustomWeights( specAccumSpeeds.x, specAccumSpeeds.y, specAccumSpeeds.z, specAccumSpeeds.w, smbOcclusionWeights );
    #endif

    // Footprint quality
    float3 smbVprev = GetViewVectorPrev( Xprev, gCameraDelta.xyz );
    float NoVprev = abs( dot( N, smbVprev ) ); // TODO: should be smbNavg ( normalized? ), but jittering breaks logic
    float sizeQuality = ( NoVprev + 1e-3 ) / ( NoV + 1e-3 ); // this order because we need to fix stretching only, shrinking is OK
    sizeQuality *= sizeQuality;
    sizeQuality = lerp( 0.1, 1.0, saturate( sizeQuality ) );

    float smbFootprintQuality = Filtering::ApplyBilinearFilter( smbOcclusion0.z, smbOcclusion1.y, smbOcclusion2.y, smbOcclusion3.x, smbBilinearFilter );
    smbFootprintQuality = Math::Sqrt01( smbFootprintQuality );
    smbFootprintQuality *= sizeQuality; // avoid footprint momentary stretching due to changed viewing angle

    // Checkerboard resolve
    uint checkerboard = Sequence::CheckerBoard( pixelPos, gFrameIndex );
    #ifdef REBLUR_OCCLUSION
        int3 checkerboardPos = pixelPos.xxy + int3( -1, 1, 0 );
        checkerboardPos.x = max( checkerboardPos.x, 0 );
        checkerboardPos.y = min( checkerboardPos.y, gRectSizeMinusOne.x );
        float viewZ0 = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( checkerboardPos.xz ) ] );
        float viewZ1 = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( checkerboardPos.yz ) ] );
        float disocclusionThresholdCheckerboard = GetDisocclusionThreshold( NRD_DISOCCLUSION_THRESHOLD, frustumSize, NoV );
        float2 wc = GetDisocclusionWeight( float2( viewZ0, viewZ1 ), viewZ, disocclusionThresholdCheckerboard );
        wc.x = ( viewZ0 > gDenoisingRange || pixelPos.x < 1 ) ? 0.0 : wc.x;
        wc.y = ( viewZ1 > gDenoisingRange || pixelPos.x >= gRectSizeMinusOne.x ) ? 0.0 : wc.y;
        wc *= Math::PositiveRcp( wc.x + wc.y );
        checkerboardPos.xy >>= 1;
    #endif

    // Specular
    #ifdef REBLUR_SPECULAR
        // Accumulation speed
        float specHistoryConfidence = smbFootprintQuality;
        if( gHasHistoryConfidence && NRD_USE_HISTORY_CONFIDENCE )
            specHistoryConfidence *= gIn_SpecConfidence[ WithRectOrigin( pixelPos ) ];

        smbSpecAccumSpeed *= lerp( specHistoryConfidence, 1.0, 1.0 / ( 1.0 + smbSpecAccumSpeed ) );
        smbSpecAccumSpeed = min( smbSpecAccumSpeed, gMaxAccumulatedFrameNum );

        // Current
        bool specHasData = gSpecCheckerboard == 2 || checkerboard == gSpecCheckerboard;
        uint2 specPos = pixelPos;
        #ifdef REBLUR_OCCLUSION
            specPos.x >>= gSpecCheckerboard == 2 ? 0 : 1;
        #endif

        REBLUR_TYPE spec = gIn_Spec[ specPos ];

        // Checkerboard resolve // TODO: materialID support?
        #ifdef REBLUR_OCCLUSION
            if( !specHasData )
            {
                float s0 = gIn_Spec[ checkerboardPos.xz ];
                float s1 = gIn_Spec[ checkerboardPos.yz ];

                s0 = Denanify( wc.x, s0 );
                s1 = Denanify( wc.y, s1 );

                spec = s0 * wc.x + s1 * wc.y;
            }
        #endif

        // Curvature estimation along predicted motion ( tests 15, 40, 76, 133, 146, 147, 148 )
        /*
        TODO: curvature! (-_-)
         - by design: curvature = 0 on static objects if camera is static
         - quantization errors hurt
         - curvature on bumpy surfaces is just wrong, pulling virtual positions into a surface and introducing lags
         - suboptimal reprojection if curvature changes signs under motion
        */
        float curvature = 0.0;
        {
            // IMPORTANT: non-zero parallax on objects attached to the camera is needed
            // IMPORTANT: the direction of "deltaUv" is important ( test 1 )
            float2 uvForZeroParallax = gOrthoMode == 0.0 ? smbPixelUv : pixelUv;
            float2 deltaUv = uvForZeroParallax - Geometry::GetScreenUv( gWorldToClipPrev, Xprev + gCameraDelta.xyz ); // TODO: repeats code for "smbParallaxInPixels1" with "-" sign
            deltaUv *= gRectSize;
            deltaUv /= max( smbParallaxInPixels1, 1.0 / 256.0 );

            // 10 edge
            float3 n10, x10;
            {
                float3 xv = Geometry::ReconstructViewPosition( pixelUv + float2( 1, 0 ) * gRectSizeInv, gFrustum, 1.0, gOrthoMode );
                float3 x = Geometry::RotateVector( gViewToWorld, xv );
                float3 v = GetViewVector( x );
                float3 o = gOrthoMode == 0.0 ? 0 : x;

                x10 = o + v * dot( X - o, N ) / dot( N, v ); // line-plane intersection
                n10 = s_Normal_Roughness[ threadPos.y + BORDER ][ threadPos.x + BORDER + 1 ].xyz;
            }

            // 01 edge
            float3 n01, x01;
            {
                float3 xv = Geometry::ReconstructViewPosition( pixelUv + float2( 0, 1 ) * gRectSizeInv, gFrustum, 1.0, gOrthoMode );
                float3 x = Geometry::RotateVector( gViewToWorld, xv );
                float3 v = GetViewVector( x );
                float3 o = gOrthoMode == 0.0 ? 0 : x;

                x01 = o + v * dot( X - o, N ) / dot( N, v ); // line-plane intersection
                n01 = s_Normal_Roughness[ threadPos.y + BORDER + 1 ][ threadPos.x + BORDER ].xyz;
            }

            // Mix
            float2 w = abs( deltaUv ) + 1.0 / 256.0;
            w /= w.x + w.y;

            float3 x = x10 * w.x + x01 * w.y;
            float3 n = normalize( n10 * w.x + n01 * w.y );

            // High parallax - flattens surface on high motion ( test 132, 172, 173, 174, 190, 201, 202, 203, e9 )
            // IMPORTANT: a must for 8-bit and 10-bit normals ( tests b7, b10, b33, 202 )
            float deltaUvLenFixed = smbParallaxInPixelsMin; // "min" because not needed for objects attached to the camera!
            deltaUvLenFixed *= NRD_USE_HIGH_PARALLAX_CURVATURE_SILHOUETTE_FIX ? NoV : 1.0; // it fixes silhouettes, but leads to less flattening
            deltaUvLenFixed *= 1.0 + gFramerateScale * Sequence::Bayer4x4( pixelPos, gFrameIndex ); // improves behavior if FPS is high, dithering is needed to avoid artefacts in test 1

            float2 motionUvHigh = pixelUv + deltaUvLenFixed * deltaUv * gRectSizeInv;
            motionUvHigh = ( floor( motionUvHigh * gRectSize ) + 0.5 ) * gRectSizeInv; // Snap to the pixel center!

            if( NRD_USE_HIGH_PARALLAX_CURVATURE && deltaUvLenFixed > 1.0 && IsInScreenNearest( motionUvHigh ) )
            {
                float2 uvScaled = WithRectOffset( ClampUvToViewport( motionUvHigh ) );

                float zHigh = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, uvScaled, 0 ) );
                float3 xHigh = Geometry::ReconstructViewPosition( motionUvHigh, gFrustum, zHigh, gOrthoMode );
                xHigh = Geometry::RotateVector( gViewToWorld, xHigh );

                float3 nHigh = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness.SampleLevel( gNearestClamp, uvScaled, 0 ) ).xyz;

                // Replace if same surface
                float zError = abs( zHigh - viewZ ) * rcp( max( zHigh, viewZ ) );
                bool cmp = zError < NRD_CURVATURE_Z_THRESHOLD; // TODO: use common disocclusion logic?

                n = cmp ? nHigh : n;
                x = cmp ? xHigh : x;
            }

            // Estimate curvature for the edge { x; X }
            float3 edge = x - X;
            float edgeLenSq = Math::LengthSquared( edge );
            curvature = dot( n - N, edge ) * Math::PositiveRcp( edgeLenSq );

        #if( NRD_USE_SPECULAR_MOTION_V2 == 0 ) // needed only for the old version
            // Correction #1 - this is needed if camera is "inside" a concave mirror ( tests 133, 164, 171 - 176 )
            if( length( X ) < -1.0 / curvature ) // TODO: test 78
                curvature *= NoV;

            // Correction #2 - very negative inconsistent with previous frame curvature blows up reprojection ( tests 164, 171 - 176 )
            float2 uv1 = Geometry::GetScreenUv( gWorldToClipPrev, X - V * ApplyThinLensEquation( hitDistForTracking, curvature ) );
            float2 uv2 = Geometry::GetScreenUv( gWorldToClipPrev, X );
            float a = length( ( uv1 - uv2 ) * gRectSize );
            curvature *= float( a < NRD_MAX_ALLOWED_VIRTUAL_MOTION_ACCELERATION * smbParallaxInPixelsMax + gRectSizeInv.x );
        #endif
        }

        // Virtual motion - coordinates
        float3 Xvirtual = GetXvirtual( hitDistForTracking, curvature, X, Xprev, N, V, roughness );
        float XvirtualLength = length( Xvirtual );

        float2 vmbPixelUv = Geometry::GetScreenUv( gWorldToClipPrev, Xvirtual );
        vmbPixelUv = materialID == gCameraAttachedReflectionMaterialID ? smbPixelUv : vmbPixelUv;

        float2 vmbDelta = vmbPixelUv - smbPixelUv;
        float vmbPixelsTraveled = length( vmbDelta * gRectSize );

        // Virtual motion - roughness
        Filtering::Bilinear vmbBilinearFilter = Filtering::GetBilinearFilter( vmbPixelUv, gRectSizePrev );
        float2 vmbBilinearGatherUv = ( vmbBilinearFilter.origin + 1.0 ) * gResourceSizeInvPrev;
        float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( roughness * roughness, gRoughnessFraction, REBLUR_ROUGHNESS_SENSITIVITY_IN_TA ); // TODO: GetRoughnessWeightParams with 0.05 sensitivity?
    #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        float4 vmbRoughness = gPrev_Normal_Roughness.GatherBlue( gNearestClamp, vmbBilinearGatherUv ).wzxy;
    #else
        float4 vmbRoughness = gPrev_Normal_Roughness.GatherAlpha( gNearestClamp, vmbBilinearGatherUv ).wzxy;
    #endif
        float4 roughnessWeight = ComputeNonExponentialWeightWithSigma( vmbRoughness * vmbRoughness, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y, roughnessSigma );
        roughnessWeight = lerp( Math::SmoothStep( 1.0, 0.0, smbParallaxInPixelsMax ), 1.0, roughnessWeight ); // jitter friendly
        float virtualHistoryRoughnessBasedConfidence = Filtering::ApplyBilinearFilter( roughnessWeight.x, roughnessWeight.y, roughnessWeight.z, roughnessWeight.w, vmbBilinearFilter );

        // Virtual motion - normal: parallax ( test 132 )
        float4 vmbNormalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gPrev_Normal_Roughness.SampleLevel( STOCHASTIC_BILINEAR_FILTER, StochasticBilinear( vmbPixelUv, gRectSizePrev ) * gResolutionScalePrev, 0 ) );
        float3 vmbN = Geometry::RotateVector( gWorldPrevToWorld, vmbNormalAndRoughness.xyz );
        float Dfactor = ImportanceSampling::GetSpecularDominantFactor( NoV, roughness, ML_SPECULAR_DOMINANT_DIRECTION_G2 );
        float virtualHistoryNormalBasedConfidence = 1.0 / ( 1.0 + 0.5 * Dfactor * saturate( length( N - vmbN ) - REBLUR_NORMAL_ULP ) * vmbPixelsTraveled );

        // Patch "smbNavg" if "smb" motion is invalid ( make relative tests a NOP )
        smbNavg = smbFootprintQuality == 0.0 ? vmbN : smbNavg;

        // Virtual motion - disocclusion: plane distance and roughness
        float4 vmbOcclusion;
        {
            float4 vmbOcclusionThreshold = disocclusionThreshold * frustumSize;
            vmbOcclusionThreshold *= lerp( 0.25, 1.0, NoV ); // yes, "*" not "/" // TODO: it's from commit "fixed suboptimal "vmb" reprojection behavior in disocclusions", but is it really needed?
            vmbOcclusionThreshold *= float( dot( vmbN, N ) > thresholdAngle ); // good for vmb
            vmbOcclusionThreshold *= float( dot( vmbN, smbNavg ) > thresholdAngle ); // bonus check for test 168
            vmbOcclusionThreshold *= IsInScreenBilinear( vmbBilinearFilter.origin, gRectSizePrev );
            vmbOcclusionThreshold -= NRD_EPS;

            float4 vmbViewZ = UnpackViewZ( gPrev_ViewZ.GatherRed( gNearestClamp, vmbBilinearGatherUv ).wzxy );
            float3 vmbVv = Geometry::ReconstructViewPosition( vmbPixelUv, gFrustumPrev, 1.0 ); // unnormalized, orthoMode = 0
            float3 vmbV = Geometry::RotateVectorInverse( gWorldToViewPrev, vmbVv );
            float NoXcurr = dot( N, Xprev - gCameraDelta.xyz );
            float4 NoXprev = ( N.x * vmbV.x + N.y * vmbV.y ) * ( gOrthoMode == 0 ? vmbViewZ : gOrthoMode ) + N.z * vmbV.z * vmbViewZ;
            float4 vmbPlaneDist = abs( NoXprev - NoXcurr );

            vmbOcclusion = step( vmbPlaneDist, vmbOcclusionThreshold );
            vmbOcclusion *= step( 0.5, roughnessWeight );
        }

        // Virtual motion - disocclusion: materialID
        uint4 vmbInternalData = gPrev_InternalData.GatherRed( gNearestClamp, vmbBilinearGatherUv ).wzxy;

        float3 vmbInternalData00 = UnpackInternalData( vmbInternalData.x );
        float3 vmbInternalData10 = UnpackInternalData( vmbInternalData.y );
        float3 vmbInternalData01 = UnpackInternalData( vmbInternalData.z );
        float3 vmbInternalData11 = UnpackInternalData( vmbInternalData.w );

        #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
            float4 vmbMaterialID = float4( vmbInternalData00.z, vmbInternalData10.z, vmbInternalData01.z, vmbInternalData11.z  );
            vmbOcclusion *= CompareMaterials( materialID, vmbMaterialID, gSpecMinMaterial );
        #endif

        // Bits
        fbits += vmbOcclusion.x * 16.0;
        fbits += vmbOcclusion.y * 32.0;
        fbits += vmbOcclusion.z * 64.0;
        fbits += vmbOcclusion.w * 128.0;

        // Virtual motion - accumulation speed
        float4 vmbOcclusionWeights = Filtering::GetBilinearCustomWeights( vmbBilinearFilter, vmbOcclusion );
        float vmbSpecAccumSpeed = Filtering::ApplyBilinearCustomWeights( vmbInternalData00.y, vmbInternalData10.y, vmbInternalData01.y, vmbInternalData11.y, vmbOcclusionWeights );

        float vmbFootprintQuality = Filtering::ApplyBilinearFilter( vmbOcclusion.x, vmbOcclusion.y, vmbOcclusion.z, vmbOcclusion.w, vmbBilinearFilter );
        vmbFootprintQuality = Math::Sqrt01( vmbFootprintQuality );
        vmbSpecAccumSpeed *= lerp( vmbFootprintQuality, 1.0, 1.0 / ( 1.0 + vmbSpecAccumSpeed ) );

        bool vmbAllowCatRom = dot( vmbOcclusion, 1.0 ) > 3.5 && REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA;
        vmbAllowCatRom = vmbAllowCatRom && smbAllowCatRom; // helps to reduce over-sharpening in disoccluded areas

        // Estimate how many pixels are traveled by virtual motion - how many radians can it be?
        // IMPORTANT: if curvature angle is multiplied by path length then we can get an angle exceeding 2 * PI, what is impossible. The max
        // angle is PI ( most left and most right points on a hemisphere ), it can be achieved by using "tan" instead of angle.
        float curvatureAngleTan = pixelSize * abs( curvature ); // tana = pixelSize / curvatureRadius = pixelSize * curvature
        curvatureAngleTan *= max( vmbPixelsTraveled / max( NoV, 0.01 ), 1.0 ); // path length
        curvatureAngleTan *= 2.0; // TODO: why it's here? it's needed to allow REBLUR_NORMAL_ULP values < "1.5 / 255.0"
        float curvatureAngle = atan( curvatureAngleTan );

        // Copied from "GetNormalWeightParam" but doesn't use "lobeAngleFraction"
        float percentOfVolume = NRD_MAX_PERCENT_OF_LOBE_VOLUME / ( 1.0 + vmbSpecAccumSpeed );
        float lobeTanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle( roughnessModified, percentOfVolume );

        // TODO: use old code and sync with "GetNormalWeightParam"?
        //float lobeTanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle( roughnessModified, NRD_MAX_PERCENT_OF_LOBE_VOLUME );
        //lobeTanHalfAngle /= 1.0 + vmbSpecAccumSpeed;

        float lobeHalfAngle = atan( lobeTanHalfAngle );
        lobeHalfAngle = max( lobeHalfAngle, NRD_NORMAL_ENCODING_ERROR );

        // Virtual motion - normal: lobe overlapping ( test 107 )
        float normalWeight = GetEncodingAwareNormalWeight( N, vmbN, lobeHalfAngle, curvatureAngle, REBLUR_NORMAL_ULP );
        normalWeight = lerp( Math::SmoothStep( 1.0, 0.0, vmbPixelsTraveled ), 1.0, normalWeight ); // jitter friendly
        virtualHistoryNormalBasedConfidence = min( virtualHistoryNormalBasedConfidence, normalWeight );

        // Virtual history amount
        // Tests 65, 66, 103, 107, 111, 132, e9, e11
        float virtualHistoryAmount = Math::SmoothStep( 0.05, 0.95, Dfactor ); // TODO: too much for high roughness under glancing angles ( test 81 )
        virtualHistoryAmount *= virtualHistoryNormalBasedConfidence; // helps on bumpy surfaces, because virtual motion gets ruined by big curvature

        // Virtual motion - virtual parallax difference
        // Tests 3, 6, 8, 11, 14, 100, 103, 104, 106, 109, 110, 114, 120, 127, 130, 131, 132, 138, 139 and 9e
        float virtualHistoryParallaxBasedConfidence;
        {
            float hitDistForTrackingPrev = gPrev_SpecHitDistForTracking.SampleLevel( gLinearClamp, vmbPixelUv * gResolutionScalePrev, 0 );
            float3 XvirtualPrev = GetXvirtual( hitDistForTrackingPrev, curvature, X, Xprev, N, V, roughness );

            float2 vmbPixelUvPrev = Geometry::GetScreenUv( gWorldToClipPrev, XvirtualPrev );
            vmbPixelUvPrev = materialID == gCameraAttachedReflectionMaterialID ? smbPixelUv : vmbPixelUvPrev;

            float pixelSizeAtXvirtual = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, XvirtualLength );
            float r = ( lobeTanHalfAngle + curvatureAngle ) * min( hitDistForTracking, hitDistForTrackingPrev ) / pixelSizeAtXvirtual; // "pixelSize" at "XvirtualPrev" seems to be not needed
            float d = length( ( vmbPixelUvPrev - vmbPixelUv ) * gRectSize );

            r = max( r, 0.1 ); // important, especially if "curvatureAngle" is not used
            virtualHistoryParallaxBasedConfidence = Math::LinearStep( r, 0.0, d );
        }

        // Virtual motion - normal & roughness prev-prev tests
        // IMPORTANT: 2 is needed because:
        // - line *** allows fallback to laggy surface motion, which can be wrongly redistributed by virtual motion
        // - we use at least linear filters, as the result a wider initial offset is needed
        float stepBetweenTaps = min( vmbPixelsTraveled * gFramerateScale, 2.0 ) + vmbPixelsTraveled / REBLUR_VIRTUAL_MOTION_PREV_PREV_WEIGHT_ITERATION_NUM;
        vmbDelta *= Math::Rsqrt( Math::LengthSquared( vmbDelta ) );
        vmbDelta /= gRectSizePrev;

        relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( vmbNormalAndRoughness.w * vmbNormalAndRoughness.w, gRoughnessFraction, REBLUR_ROUGHNESS_SENSITIVITY_IN_TA ); // TODO: GetRoughnessWeightParams with 0.05 sensitivity?

        [unroll]
        for( i = 1; i <= REBLUR_VIRTUAL_MOTION_PREV_PREV_WEIGHT_ITERATION_NUM; i++ )
        {
            float2 vmbPixelUvPrev = vmbPixelUv + vmbDelta * i * stepBetweenTaps;
            float4 vmbNormalAndRoughnessPrev = NRD_FrontEnd_UnpackNormalAndRoughness( gPrev_Normal_Roughness.SampleLevel( STOCHASTIC_BILINEAR_FILTER, StochasticBilinear( vmbPixelUvPrev, gRectSizePrev ) * gResolutionScalePrev, 0 ) );

            float2 w;
            w.x = GetEncodingAwareNormalWeight( vmbNormalAndRoughness.xyz, vmbNormalAndRoughnessPrev.xyz, lobeHalfAngle, curvatureAngle * ( 1.0 + i * stepBetweenTaps ), REBLUR_NORMAL_ULP );
            w.y = ComputeNonExponentialWeightWithSigma( vmbNormalAndRoughnessPrev.w * vmbNormalAndRoughnessPrev.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y, roughnessSigma );

            #if( REBLUR_USE_STF == 1 && NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
                // Cures issues of "StochasticBilinear", produces closer look to the linear filter
                w = lerp( 1.0, w, saturate( stepBetweenTaps ) );
            #endif

            w = IsInScreenNearest( vmbPixelUvPrev ) ? w : 1.0;

            virtualHistoryNormalBasedConfidence = min( virtualHistoryNormalBasedConfidence, w.x );
            virtualHistoryRoughnessBasedConfidence = min( virtualHistoryRoughnessBasedConfidence, w.y );
        }

        // Virtual history confidence
        float virtualHistoryConfidenceForSmbRelaxation = virtualHistoryNormalBasedConfidence * virtualHistoryRoughnessBasedConfidence;
        float virtualHistoryConfidence = virtualHistoryNormalBasedConfidence * virtualHistoryRoughnessBasedConfidence * virtualHistoryParallaxBasedConfidence;

        // TODO: more fallback to "smb" needed in many cases if roughness changes
        virtualHistoryAmount *= virtualHistoryRoughnessBasedConfidence; // helps to preserve roughness details, which lies on surfaces

        // Sample surface history
        REBLUR_TYPE smbSpecHistory;
        REBLUR_FAST_TYPE smbSpecFastHistory;
        REBLUR_SH_TYPE smbSpecShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gHistory_Spec, smbSpecHistory,
            gHistory_SpecFast, smbSpecFastHistory
            #ifdef REBLUR_SH
                , gHistory_SpecSh, smbSpecShHistory
            #endif
        );

        // Surface motion ( test 9, 9e )
        // IMPORTANT: needs to be responsive, because "vmb" fails on bumpy surfaces for the following reasons:
        //  - normal and prev-prev tests fail
        //  - curvature is so high that "vmb" regresses to "smb" and starts to lag
        float surfaceHistoryConfidence = 1.0;
        {
            float a = atan( smbParallaxInPixelsMax * pixelSize / length( X ) );
            //a = acos( saturate( dot( V, smbVprev ) ) ); // numerically unstable

            float nonLinearAccumSpeed = 1.0 / ( 1.0 + smbSpecAccumSpeed );
            float h = lerp( ExtractHitDist( smbSpecHistory ), ExtractHitDist( spec ), nonLinearAccumSpeed ) * hitDistNormalization;

            float tana0 = ImportanceSampling::GetSpecularLobeTanHalfAngle( roughnessModified, NRD_MAX_PERCENT_OF_LOBE_VOLUME ); // base lobe angle
            tana0 *= lerp( NoV, 1.0, roughnessModified ); // make more strict if NoV is low and lobe is very V-dependent
            tana0 *= nonLinearAccumSpeed; // make more strict if history is long
            tana0 /= GetHitDistFactor( h, frustumSize ) + NRD_EPS; // make relaxed "in corners", where reflection is close to the surface

            float a0 = atan( tana0 );
            a0 = max( a0, NRD_NORMAL_ENCODING_ERROR );

            float f = Math::LinearStep( a0, 0.0, a );
            surfaceHistoryConfidence = Math::Pow01( f, 4.0 );
        }

        // Responsive accumulation
        float2 maxResponsiveFrameNum = gMaxAccumulatedFrameNum;
        {
            float responsiveFactor = RemapRoughnessToResponsiveFactor( roughness );
            float smc = GetSpecMagicCurve( roughnessModified );

            float2 f;
            f.x = dot( N, normalize( smbNavg ) );
            f.y = dot( N, vmbN );
            f = lerp( smc, 1.0, responsiveFactor ) * Math::Pow01( f, lerp( 32.0, 1.0, smc ) * ( 1.0 - responsiveFactor ) );

            maxResponsiveFrameNum = max( gMaxAccumulatedFrameNum * f, gHistoryFixFrameNum );
        }

        // Surface motion: max allowed frames
        float smbMaxFrameNum = gMaxAccumulatedFrameNum;
        smbMaxFrameNum *= surfaceHistoryConfidence;
        smbMaxFrameNum = min( smbMaxFrameNum, maxResponsiveFrameNum.x );

        // Ensure that HistoryFix pass doesn't pop up without a disocclusion in some critical cases
        float smbBoostedMaxFrameNum = max( smbMaxFrameNum, gHistoryFixFrameNum * ( 1.0 - virtualHistoryConfidenceForSmbRelaxation ) );
        float smbSpecAccumSpeedBoosted = min( smbSpecAccumSpeed, smbBoostedMaxFrameNum );

        // Virtual motion: max allowed frames
        float vmbMaxFrameNum = gMaxAccumulatedFrameNum;
        vmbMaxFrameNum *= virtualHistoryConfidence; // previously was just "virtualHistoryParallaxBasedConfidence * virtualHistoryNormalBasedConfidence"
        vmbMaxFrameNum = min( vmbMaxFrameNum, maxResponsiveFrameNum.y );

        // Limit number of accumulated frames
        smbSpecAccumSpeed = min( smbSpecAccumSpeed, smbMaxFrameNum );
        vmbSpecAccumSpeed = min( vmbSpecAccumSpeed, vmbMaxFrameNum );

        // Fallback to "smb" if "vmb" history is short // ***
    #if( REBLUR_USE_OLD_SMB_FALLBACK_LOGIC == 1 )
        // Interactive comparison of two methods: https://www.desmos.com/calculator/syocjyk9wc
        // TODO: the amount gets shifted heavily towards "smb" if "smb" > "vmb" even by 5 frames
        float vmbToSmbRatio = saturate( vmbSpecAccumSpeed / ( smbSpecAccumSpeed + NRD_EPS ) );
        float smbBonusFrames = max( smbSpecAccumSpeed - vmbSpecAccumSpeed, 0.0 );
        virtualHistoryAmount *= vmbToSmbRatio;
        virtualHistoryAmount /= 1.0 + smbBonusFrames * ( 1.0 - vmbToSmbRatio );
    #else
        // This version works in both directions: towards "smb" and towards "vmb"
        // TODO: "magic" is tuned to ~match old logic and also offer non-aggressive fallback to "vmb" on top ( test 218 )
        float magic = vmbSpecAccumSpeed > smbSpecAccumSpeed ? 8.0 : 0.5;
        virtualHistoryAmount *= 1.0 + ( vmbSpecAccumSpeed - smbSpecAccumSpeed ) / ( magic * max( vmbSpecAccumSpeed, smbSpecAccumSpeed ) + 1.0 );
        virtualHistoryAmount = saturate( virtualHistoryAmount );
    #endif

        #if( REBLUR_VIRTUAL_HISTORY_AMOUNT != 2 )
            virtualHistoryAmount = REBLUR_VIRTUAL_HISTORY_AMOUNT;
        #endif

        // Sample virtual history
        REBLUR_TYPE vmbSpecHistory;
        REBLUR_FAST_TYPE vmbSpecFastHistory;
        REBLUR_SH_TYPE vmbSpecShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( vmbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            vmbOcclusionWeights, vmbAllowCatRom,
            gHistory_Spec, vmbSpecHistory,
            gHistory_SpecFast, vmbSpecFastHistory
            #ifdef REBLUR_SH
                , gHistory_SpecSh, vmbSpecShHistory
            #endif
        );

        // Avoid negative values
        smbSpecHistory = ClampNegativeToZero( smbSpecHistory );
        vmbSpecHistory = ClampNegativeToZero( vmbSpecHistory );

        // Accumulation
        float smbSpecNonLinearAccumSpeed = 1.0 / ( 1.0 + smbSpecAccumSpeed );
        float vmbSpecNonLinearAccumSpeed = 1.0 / ( 1.0 + vmbSpecAccumSpeed );

        if( !specHasData )
        {
            smbSpecNonLinearAccumSpeed *= lerp( 1.0 - gCheckerboardResolveAccumSpeed, 1.0, smbSpecNonLinearAccumSpeed );
            vmbSpecNonLinearAccumSpeed *= lerp( 1.0 - gCheckerboardResolveAccumSpeed, 1.0, vmbSpecNonLinearAccumSpeed );
        }

        REBLUR_TYPE smbSpec = MixHistoryAndCurrent( smbSpecHistory, spec, smbSpecNonLinearAccumSpeed, roughnessModified );
        REBLUR_TYPE vmbSpec = MixHistoryAndCurrent( vmbSpecHistory, spec, vmbSpecNonLinearAccumSpeed, roughnessModified );

        REBLUR_TYPE specResult = lerp( smbSpec, vmbSpec, virtualHistoryAmount );

        #ifdef REBLUR_SH
            float4 specSh = gIn_SpecSh[ specPos ];
            float4 smbShSpec = lerp( smbSpecShHistory, specSh, smbSpecNonLinearAccumSpeed );
            float4 vmbShSpec = lerp( vmbSpecShHistory, specSh, vmbSpecNonLinearAccumSpeed );

            float4 specShResult = lerp( smbShSpec, vmbShSpec, virtualHistoryAmount );

            // ( Optional ) Output modified roughness to assist AA during SG resolve
            specShResult.w = roughnessModified; // IMPORTANT: must not be blurred! this is why "specSh.xyz" is used ~everywhere
        #endif

        float specAccumSpeed = lerp( smbSpecAccumSpeedBoosted, vmbSpecAccumSpeed, virtualHistoryAmount );
        REBLUR_TYPE specHistory = lerp( smbSpecHistory, vmbSpecHistory, virtualHistoryAmount );

        // Firefly suppressor
        #if( !defined REBLUR_OCCLUSION && !defined REBLUR_DIRECTIONAL_OCCLUSION )
            float specMaxRelativeIntensity = gFireflySuppressorMinRelativeScale + REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY / ( specAccumSpeed + 1.0 );

            float specAntifireflyFactor = specAccumSpeed * gMaxBlurRadius * REBLUR_FIREFLY_SUPPRESSOR_RADIUS_SCALE;
            specAntifireflyFactor /= 1.0 + specAntifireflyFactor;

            float specLumaResult = GetLuma( specResult );
            float specLumaClamped = min( specLumaResult, GetLuma( specHistory ) * specMaxRelativeIntensity );
            specLumaClamped = lerp( specLumaResult, specLumaClamped, specAntifireflyFactor );

            specResult = ChangeLuma( specResult, specLumaClamped );
            #ifdef REBLUR_SH
                specShResult.xyz *= GetLumaScale( length( specShResult.xyz ), specLumaClamped );
            #endif
        #endif

        // Output
        gOut_Spec[ pixelPos ] = specResult;
        #ifdef REBLUR_SH
            gOut_SpecSh[ pixelPos ] = specShResult;
        #endif

        // Fast history
        float smbSpecFastNonLinearAccumSpeed = GetNonLinearAccumSpeed( smbSpecAccumSpeed, gMaxFastAccumulatedFrameNum, surfaceHistoryConfidence, specHasData );
        float vmbSpecFastNonLinearAccumSpeed = GetNonLinearAccumSpeed( vmbSpecAccumSpeed, gMaxFastAccumulatedFrameNum, virtualHistoryConfidence, specHasData );

        float smbSpecFast = lerp( smbSpecFastHistory, GetLuma( spec ), smbSpecFastNonLinearAccumSpeed );
        float vmbSpecFast = lerp( vmbSpecFastHistory, GetLuma( spec ), vmbSpecFastNonLinearAccumSpeed );

        float specFastResult = lerp( smbSpecFast, vmbSpecFast, virtualHistoryAmount );

        #if( !defined REBLUR_OCCLUSION && !defined REBLUR_DIRECTIONAL_OCCLUSION )
            // Firefly suppressor ( fixes heavy crawling under camera rotation: test 95, 120 )
            float specFastClamped = min( specFastResult, GetLuma( specHistory ) * specMaxRelativeIntensity * REBLUR_FIREFLY_SUPPRESSOR_FAST_RELATIVE_INTENSITY );
            specFastResult = lerp( specFastResult, specFastClamped, specAntifireflyFactor );
        #endif

        gOut_SpecFast[ pixelPos ] = specFastResult;

        // Debug
        #if( REBLUR_SHOW == REBLUR_SHOW_CURVATURE )
            virtualHistoryAmount = abs( curvature ) * pixelSize * 30.0;
        #elif( REBLUR_SHOW == REBLUR_SHOW_CURVATURE_SIGN )
            virtualHistoryAmount = sign( curvature ) * 0.5 + 0.5;
        #elif( REBLUR_SHOW == REBLUR_SHOW_SURFACE_HISTORY_CONFIDENCE )
            virtualHistoryAmount = surfaceHistoryConfidence;
        #elif( REBLUR_SHOW == REBLUR_SHOW_VIRTUAL_HISTORY_CONFIDENCE )
            virtualHistoryAmount = virtualHistoryConfidence;
        #elif( REBLUR_SHOW == REBLUR_SHOW_VIRTUAL_HISTORY_NORMAL_CONFIDENCE )
            virtualHistoryAmount = virtualHistoryNormalBasedConfidence;
        #elif( REBLUR_SHOW == REBLUR_SHOW_VIRTUAL_HISTORY_ROUGHNESS_CONFIDENCE )
            virtualHistoryAmount = virtualHistoryRoughnessBasedConfidence;
        #elif( REBLUR_SHOW == REBLUR_SHOW_VIRTUAL_HISTORY_PARALLAX_CONFIDENCE )
            virtualHistoryAmount = virtualHistoryParallaxBasedConfidence;
        #elif( REBLUR_SHOW == REBLUR_SHOW_HIT_DIST_FOR_TRACKING )
            float smc = GetSpecMagicCurve( roughness );
            virtualHistoryAmount = hitDistForTracking * lerp( 1.0, 5.0, smc ) / ( 1.0 + hitDistForTracking * lerp( 1.0, 5.0, smc ) );
        #endif
    #else
        float specAccumSpeed = 0;
        float curvature = 0;
        float virtualHistoryAmount = 0;
    #endif

    // Output
    #ifndef REBLUR_OCCLUSION
        // TODO: "PackData2" can be inlined into the code ( right after a variable gets ready for use ) to utilize the only
        // one "uint" for the intermediate storage. But it looks like the compiler does good job by rearranging the code for us
        gOut_Data2[ pixelPos ] = PackData2( fbits, curvature, virtualHistoryAmount, smbAllowCatRom );
    #endif

    // Diffuse
    #ifdef REBLUR_DIFFUSE
        // Accumulation speed
        float diffHistoryConfidence = smbFootprintQuality;
        if( gHasHistoryConfidence && NRD_USE_HISTORY_CONFIDENCE )
            diffHistoryConfidence *= gIn_DiffConfidence[ WithRectOrigin( pixelPos ) ];

        diffAccumSpeed *= lerp( diffHistoryConfidence, 1.0, 1.0 / ( 1.0 + diffAccumSpeed ) );
        diffAccumSpeed = min( diffAccumSpeed, gMaxAccumulatedFrameNum );

        // Current
        bool diffHasData = gDiffCheckerboard == 2 || checkerboard == gDiffCheckerboard;
        uint2 diffPos = pixelPos;
        #ifdef REBLUR_OCCLUSION
            diffPos.x >>= gDiffCheckerboard == 2 ? 0 : 1;
        #endif

        REBLUR_TYPE diff = gIn_Diff[ diffPos ];

        // Checkerboard resolve // TODO: materialID support?
        #ifdef REBLUR_OCCLUSION
            if( !diffHasData )
            {
                float d0 = gIn_Diff[ checkerboardPos.xz ];
                float d1 = gIn_Diff[ checkerboardPos.yz ];

                d0 = Denanify( wc.x, d0 );
                d1 = Denanify( wc.y, d1 );

                diff = d0 * wc.x + d1 * wc.y;
            }
        #endif

        // Sample history - surface motion
        REBLUR_TYPE smbDiffHistory;
        REBLUR_FAST_TYPE smbDiffFastHistory;
        REBLUR_SH_TYPE smbDiffShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gHistory_Diff, smbDiffHistory,
            gHistory_DiffFast, smbDiffFastHistory
            #ifdef REBLUR_SH
                , gHistory_DiffSh, smbDiffShHistory
            #endif
        );

        // Avoid negative values
        smbDiffHistory = ClampNegativeToZero( smbDiffHistory );

        // Accumulation
        float diffNonLinearAccumSpeed = 1.0 / ( 1.0 + diffAccumSpeed );
        if( !diffHasData )
            diffNonLinearAccumSpeed *= lerp( 1.0 - gCheckerboardResolveAccumSpeed, 1.0, diffNonLinearAccumSpeed );

        REBLUR_TYPE diffResult = MixHistoryAndCurrent( smbDiffHistory, diff, diffNonLinearAccumSpeed );
        #ifdef REBLUR_SH
            float4 diffSh = gIn_DiffSh[ diffPos ];
            float4 diffShResult = MixHistoryAndCurrent( smbDiffShHistory, diffSh, diffNonLinearAccumSpeed );
        #endif

        // Firefly suppressor
        #if( !defined REBLUR_OCCLUSION && !defined REBLUR_DIRECTIONAL_OCCLUSION )
            float diffMaxRelativeIntensity = gFireflySuppressorMinRelativeScale + REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY / ( diffAccumSpeed + 1.0 );

            float diffAntifireflyFactor = diffAccumSpeed * gMaxBlurRadius * REBLUR_FIREFLY_SUPPRESSOR_RADIUS_SCALE;
            diffAntifireflyFactor /= 1.0 + diffAntifireflyFactor;

            float diffLumaResult = GetLuma( diffResult );
            float diffLumaClamped = min( diffLumaResult, GetLuma( smbDiffHistory ) * diffMaxRelativeIntensity );
            diffLumaClamped = lerp( diffLumaResult, diffLumaClamped, diffAntifireflyFactor );

            diffResult = ChangeLuma( diffResult, diffLumaClamped );
            #ifdef REBLUR_SH
                diffShResult.xyz *= GetLumaScale( length( diffShResult.xyz ), diffLumaClamped );
            #endif
        #endif

        // Output
        gOut_Diff[ pixelPos ] = diffResult;
        #ifdef REBLUR_SH
            gOut_DiffSh[ pixelPos ] = diffShResult;
        #endif

        // Fast history
        float diffFastAccumSpeed = min( diffAccumSpeed, gMaxFastAccumulatedFrameNum );
        float diffFastNonLinearAccumSpeed = 1.0 / ( 1.0 + diffFastAccumSpeed );
        if( !diffHasData )
            diffFastNonLinearAccumSpeed *= lerp( 1.0 - gCheckerboardResolveAccumSpeed, 1.0, diffFastNonLinearAccumSpeed );

        float diffFastResult = lerp( smbDiffFastHistory.x, GetLuma( diff ), diffFastNonLinearAccumSpeed );
        #if( !defined REBLUR_OCCLUSION && !defined REBLUR_DIRECTIONAL_OCCLUSION )
            // Firefly suppressor ( fixes heavy crawling under camera rotation, test 99 )
            float diffFastClamped = min( diffFastResult, GetLuma( smbDiffHistory ) * diffMaxRelativeIntensity * REBLUR_FIREFLY_SUPPRESSOR_FAST_RELATIVE_INTENSITY );
            diffFastResult = lerp( diffFastResult, diffFastClamped, diffAntifireflyFactor );
        #endif

        gOut_DiffFast[ pixelPos ] = diffFastResult;
    #else
        float diffAccumSpeed = 0;
    #endif

    // Output
    gOut_Data1[ pixelPos ] = PackData1( diffAccumSpeed, specAccumSpeed );
}
