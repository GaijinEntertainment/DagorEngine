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

    float4 temp = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( globalPos ) ] );
    s_Normal_Roughness[ sharedPos.y ][ sharedPos.x ] = temp;

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
            float hitDist = gSpecPrepassBlurRadius == 0.0 ? ExtractHitDist( spec ) : gIn_Spec_HitDistForTracking[ globalPos ];
        #endif

        s_HitDistForTracking[ sharedPos.y ][ sharedPos.x ] = hitDist;
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
    float viewZ = abs( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    if( viewZ > gDenoisingRange )
        return;

    // Current position
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 X = STL::Geometry::RotateVector( gViewToWorld, Xv );

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
            if( i < 2 && j < 2 ) // TODO: is backward 2x2 OK?
                Navg += normalAndRoughness.xyz;

            #ifdef REBLUR_SPECULAR
                // Min hit distance for tracking, ignoring 0 values ( which still can be produced by VNDF sampling )
                float h = s_HitDistForTracking[ pos.y ][ pos.x ];
                hitDistForTracking = min( hitDistForTracking, h == 0.0 ? NRD_INF : h );

                // Roughness variance
                // IMPORTANT: squared because the test uses "roughness ^ 2"
                roughnessM1 += normalAndRoughness.w * normalAndRoughness.w;
                roughnessM2 += normalAndRoughness.w * normalAndRoughness.w * normalAndRoughness.w * normalAndRoughness.w;
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
        float roughnessModified = STL::Filtering::GetModifiedRoughnessFromNormalVariance( roughness, Navg ); // TODO: needed?

        roughnessM1 /= ( 1 + BORDER * 2 ) * ( 1 + BORDER * 2 );
        roughnessM2 /= ( 1 + BORDER * 2 ) * ( 1 + BORDER * 2 );
        float roughnessSigma = GetStdDev( roughnessM1, roughnessM2 );
    #endif

    // Hit distance for tracking ( tests 8, 110, 139, e3, e9 without normal map, e24 )
    #ifdef REBLUR_SPECULAR
        hitDistForTracking = hitDistForTracking == NRD_INF ? 0.0 : hitDistForTracking;

        #ifdef REBLUR_OCCLUSION
            hitDistForTracking *= _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness );
        #else
            hitDistForTracking *= gSpecPrepassBlurRadius == 0.0 ? _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness ) : 1.0;
        #endif

        gOut_Spec_HitDistForTracking[ pixelPos ] = hitDistForTracking;
    #endif

    // Previous position and surface motion uv
    float3 mv = gIn_Mv[ WithRectOrigin( pixelPos ) ] * gMvScale.xyz;
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

    // Parallax
    float smbParallaxInPixels = ComputeParallaxInPixels( Xprev + gCameraDelta.xyz, gOrthoMode == 0.0 ? smbPixelUv : pixelUv, gWorldToClipPrev, gRectSize );

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
    STL::Filtering::CatmullRom smbCatromFilter = STL::Filtering::GetCatmullRomFilter( smbPixelUv, gRectSizePrev );
    float2 smbCatromGatherUv = smbCatromFilter.origin * gResourceSizeInvPrev;
    float4 smbViewZ0 = gIn_Prev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 1 ) ).wzxy;
    float4 smbViewZ1 = gIn_Prev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 1 ) ).wzxy;
    float4 smbViewZ2 = gIn_Prev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 3 ) ).wzxy;
    float4 smbViewZ3 = gIn_Prev_ViewZ.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 3 ) ).wzxy;

    float3 prevViewZ0 = UnpackViewZ( smbViewZ0.yzw );
    float3 prevViewZ1 = UnpackViewZ( smbViewZ1.xzw );
    float3 prevViewZ2 = UnpackViewZ( smbViewZ2.xyw );
    float3 prevViewZ3 = UnpackViewZ( smbViewZ3.xyz );

    // Previous normal averaged for all pixels in 2x2 footprint
    // IMPORTANT: bilinear filter can touch sky pixels, due to this reason "Post Blur" writes special values into sky-pixels
    STL::Filtering::Bilinear smbBilinearFilter = STL::Filtering::GetBilinearFilter( smbPixelUv, gRectSizePrev );
    float2 smbBilinearGatherUv = ( smbBilinearFilter.origin + 1.0 ) * gResourceSizeInvPrev;
    float3 prevNavg = UnpackNormalAndRoughness( gIn_Prev_Normal_Roughness.SampleLevel( gLinearClamp, smbBilinearGatherUv, 0 ), false ).xyz;
    prevNavg = STL::Geometry::RotateVector( gWorldPrevToWorld, prevNavg );

    // Disocclusion: threshold
    float disocclusionThresholdMulFrustumSize = gDisocclusionThreshold;
    if( gHasDisocclusionThresholdMix )
        disocclusionThresholdMulFrustumSize = lerp( gDisocclusionThreshold, gDisocclusionThresholdAlternate, gIn_DisocclusionThresholdMix[ WithRectOrigin( pixelPos ) ] );

    float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    disocclusionThresholdMulFrustumSize *= frustumSize;

    float3 V = GetViewVector( X );
    float NoV = abs( dot( N, V ) );
    float mvLengthFactor = STL::Math::LinearStep( 0.5, 1.0, smbParallaxInPixels );
    float frontFacing = lerp( cos( STL::Math::DegToRad( 135.0 ) ), cos( STL::Math::DegToRad( 91.0 ) ), mvLengthFactor );
    float4 smbDisocclusionThreshold = disocclusionThresholdMulFrustumSize / lerp( lerp( 0.05, 1.0, NoV ), 1.0, saturate( smbParallaxInPixels / 30.0 ) );
    smbDisocclusionThreshold *= float( dot( prevNavg, Navg ) > frontFacing );
    smbDisocclusionThreshold *= IsInScreenBilinear( smbBilinearFilter.origin, gRectSizePrev );
    smbDisocclusionThreshold -= NRD_EPS;

    // Disocclusion: plane distance
    float3 Xvprev = STL::Geometry::AffineTransform( gWorldToViewPrev, Xprev );
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
        uint4 smbInternalData0 = gIn_Prev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 1 ) ).wzxy;
        uint4 smbInternalData1 = gIn_Prev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 1 ) ).wzxy;
        uint4 smbInternalData2 = gIn_Prev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 1, 3 ) ).wzxy;
        uint4 smbInternalData3 = gIn_Prev_InternalData.GatherRed( gNearestClamp, smbCatromGatherUv, float2( 3, 3 ) ).wzxy;

        float3 smbMaterialID0 = float3( UnpackInternalData( smbInternalData0.y ).z, UnpackInternalData( smbInternalData0.z ).z, UnpackInternalData( smbInternalData0.w ).z );
        float3 smbMaterialID1 = float3( UnpackInternalData( smbInternalData1.x ).z, UnpackInternalData( smbInternalData1.z ).z, UnpackInternalData( smbInternalData1.w ).z );
        float3 smbMaterialID2 = float3( UnpackInternalData( smbInternalData2.x ).z, UnpackInternalData( smbInternalData2.y ).z, UnpackInternalData( smbInternalData2.w ).z );
        float3 smbMaterialID3 = float3( UnpackInternalData( smbInternalData3.x ).z, UnpackInternalData( smbInternalData3.y ).z, UnpackInternalData( smbInternalData3.z ).z );

        uint mask = gSpecMaterialMask | gDiffMaterialMask; // TODO: separation is expensive
        smbOcclusion0 *= CompareMaterials( materialID, smbMaterialID0, mask );
        smbOcclusion1 *= CompareMaterials( materialID, smbMaterialID1, mask );
        smbOcclusion2 *= CompareMaterials( materialID, smbMaterialID2, mask );
        smbOcclusion3 *= CompareMaterials( materialID, smbMaterialID3, mask );

        uint4 smbInternalData = uint4( smbInternalData0.w, smbInternalData1.z, smbInternalData2.y, smbInternalData3.x );
    #else
        uint4 smbInternalData = gIn_Prev_InternalData.GatherRed( gNearestClamp, smbBilinearGatherUv ).wzxy;
    #endif

    // 2x2 occlusion weights
    float4 smbOcclusionWeights = STL::Filtering::GetBilinearCustomWeights( smbBilinearFilter, float4( smbOcclusion0.z, smbOcclusion1.y, smbOcclusion2.y, smbOcclusion3.x ) );
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
        float diffAccumSpeed = STL::Filtering::ApplyBilinearCustomWeights( diffAccumSpeeds.x, diffAccumSpeeds.y, diffAccumSpeeds.z, diffAccumSpeeds.w, smbOcclusionWeights );
    #endif

    #ifdef REBLUR_SPECULAR
        float4 specAccumSpeeds = float4( internalData00.y, internalData10.y, internalData01.y, internalData11.y );
        float smbSpecAccumSpeed = STL::Filtering::ApplyBilinearCustomWeights( specAccumSpeeds.x, specAccumSpeeds.y, specAccumSpeeds.z, specAccumSpeeds.w, smbOcclusionWeights );
    #endif

    // Footprint quality
    float3 smbVprev = GetViewVectorPrev( Xprev, gCameraDelta.xyz );
    float NoVprev = abs( dot( N, smbVprev ) ); // TODO: should be prevNavg ( normalized? ), but jittering breaks logic
    float sizeQuality = ( NoVprev + 1e-3 ) / ( NoV + 1e-3 ); // this order because we need to fix stretching only, shrinking is OK
    sizeQuality *= sizeQuality;
    sizeQuality = lerp( 0.1, 1.0, saturate( sizeQuality ) );

    float smbFootprintQuality = STL::Filtering::ApplyBilinearFilter( smbOcclusion0.z, smbOcclusion1.y, smbOcclusion2.y, smbOcclusion3.x, smbBilinearFilter );
    smbFootprintQuality = STL::Math::Sqrt01( smbFootprintQuality );
    smbFootprintQuality *= sizeQuality; // avoid footprint momentary stretching due to changed viewing angle

    // Checkerboard resolve
    uint checkerboard = STL::Sequence::CheckerBoard( pixelPos, gFrameIndex );
    #ifdef REBLUR_OCCLUSION
        int3 checkerboardPos = pixelPos.xxy + int3( -1, 1, 0 );
        checkerboardPos.x = max( checkerboardPos.x, 0 );
        checkerboardPos.y = min( checkerboardPos.y, gRectSizeMinusOne.x );
        float viewZ0 = abs( gIn_ViewZ[ WithRectOrigin( checkerboardPos.xz ) ] );
        float viewZ1 = abs( gIn_ViewZ[ WithRectOrigin( checkerboardPos.yz ) ] );
        float2 wc = GetBilateralWeight( float2( viewZ0, viewZ1 ), viewZ );
        wc.x = ( viewZ0 > gDenoisingRange || pixelPos.x < 1 ) ? 0.0 : wc.x;
        wc.y = ( viewZ1 > gDenoisingRange || pixelPos.x >= gRectSizeMinusOne.x ) ? 0.0 : wc.y;
        wc *= STL::Math::PositiveRcp( wc.x + wc.y );
        checkerboardPos.xy >>= 1;
    #endif

    // Specular
    #ifdef REBLUR_SPECULAR
        // Accumulation speed
        float specHistoryConfidence = smbFootprintQuality;
        if( gHasHistoryConfidence )
            specHistoryConfidence *= gIn_Spec_Confidence[ WithRectOrigin( pixelPos ) ];

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

        float Dfactor = STL::ImportanceSampling::GetSpecularDominantFactor( NoV, roughness, STL_SPECULAR_DOMINANT_DIRECTION_G2 );

        // Curvature estimation along predicted motion ( tests 15, 40, 76, 133, 146, 147, 148 )
        /*
        TODO: curvature! (-_-)
         - by design: curvature = 0 on static objects if camera is static
         - quantization errors hurt
         - curvature on bumpy surfaces is just wrong, pulling virtual positions into a surface and introducing lags
         - bad reprojection if curvature changes signs under motion
         - code below works better on smooth curved surfaces, but not in tests: 174, 175 ( without normal map )
            BEFORE: hitDistFocused = hitDist / ( 2.0 * curvature * hitDist * NoV + 1.0 );
            AFTER:  hitDistFocused = hitDist / ( 2.0 * curvature * hitDist / NoV + 1.0 );
        */
        float curvature;
        float2 vmbDelta;
        {
            // IMPORTANT: this code allows to get non-zero parallax on objects attached to the camera
            float2 uvForZeroParallax = gOrthoMode == 0.0 ? smbPixelUv : pixelUv;
            float2 deltaUv = STL::Geometry::GetScreenUv( gWorldToClipPrev, Xprev - gCameraDelta.xyz ) - uvForZeroParallax;
            deltaUv *= gRectSize;
            float deltaUvLen = length( deltaUv );
            deltaUv /= max( deltaUvLen, 1.0 / 256.0 );
            float2 motionUv = pixelUv + 0.99 * deltaUv * gRectSizeInv; // stay in SMEM

            // Construct the other edge point "x"
            float z = abs( gIn_ViewZ.SampleLevel( gLinearClamp, WithRectOffset( motionUv * gResolutionScale ), 0 ) );
            float3 x = STL::Geometry::ReconstructViewPosition( motionUv, gFrustum, z, gOrthoMode );
            x = STL::Geometry::RotateVector( gViewToWorld, x );

            // Interpolate normal at "x"
            STL::Filtering::Bilinear f = STL::Filtering::GetBilinearFilter( motionUv, gRectSize );

            int2 pos = threadPos + BORDER + int2( f.origin ) - pixelPos;
            pos = clamp( pos, 0, int2( BUFFER_X, BUFFER_Y ) - 2 ); // just in case?

            float3 n00 = s_Normal_Roughness[ pos.y ][ pos.x ].xyz;
            float3 n10 = s_Normal_Roughness[ pos.y ][ pos.x + 1 ].xyz;
            float3 n01 = s_Normal_Roughness[ pos.y + 1 ][ pos.x ].xyz;
            float3 n11 = s_Normal_Roughness[ pos.y + 1 ][ pos.x + 1 ].xyz;

            float3 n = _NRD_SafeNormalize( STL::Filtering::ApplyBilinearFilter( n00, n10, n01, n11, f ) );

            // ( Optional ) High parallax - flattens surface on high motion ( test 132, e9 )
            // IMPORTANT: a must for 8-bit and 10-bit normals ( tests b7, b10, b33 )
            float deltaUvLenFixed = deltaUvLen * ( NRD_USE_HIGH_PARALLAX_CURVATURE_SILHOUETTE_FIX ? NoV : 1.0 ); // it fixes silhouettes, but leads to less flattening
            float2 motionUvHigh = pixelUv + deltaUvLenFixed * deltaUv * gRectSizeInv;
            if( NRD_USE_HIGH_PARALLAX_CURVATURE && deltaUvLenFixed > 1.0 && IsInScreenNearest( motionUvHigh ) )
            {
                // Construct the other edge point "xHigh"
                float zHigh = abs( gIn_ViewZ.SampleLevel( gLinearClamp, WithRectOffset( motionUvHigh * gResolutionScale ), 0 ) );
                float3 xHigh = STL::Geometry::ReconstructViewPosition( motionUvHigh, gFrustum, zHigh, gOrthoMode );
                xHigh = STL::Geometry::RotateVector( gViewToWorld, xHigh );

                // Interpolate normal at "xHigh"
                #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
                    f = STL::Filtering::GetBilinearFilter( motionUvHigh, gRectSize );

                    f.origin = clamp( f.origin, 0, gRectSize - 2.0 );
                    pos = gRectOrigin + int2( f.origin );

                    n00 = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ pos ] ).xyz;
                    n10 = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ pos + int2( 1, 0 ) ] ).xyz;
                    n01 = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ pos + int2( 0, 1 ) ] ).xyz;
                    n11 = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ pos + int2( 1, 1 ) ] ).xyz;

                    float3 nHigh = _NRD_SafeNormalize( STL::Filtering::ApplyBilinearFilter( n00, n10, n01, n11, f ) );
                #else
                    float3 nHigh = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness.SampleLevel( gLinearClamp, WithRectOffset( motionUvHigh * gResolutionScale ), 0 ) ).xyz;
                #endif

                // Replace if same surface
                float zError = abs( zHigh - viewZ ) * rcp( max( zHigh, viewZ ) );
                bool cmp = zError < NRD_CURVATURE_Z_THRESHOLD;

                n = cmp ? nHigh : n;
                x = cmp ? xHigh : x;
            }

            // Estimate curvature for the edge { x; X }
            float3 edge = x - X;
            float edgeLenSq = STL::Math::LengthSquared( edge );
            curvature = dot( n - N, edge ) * STL::Math::PositiveRcp( edgeLenSq );

            // Correction #1 - values below this threshold get turned into garbage due to numerical imprecision
            float d = STL::Math::ManhattanDistance( N, n );
            float s = STL::Math::LinearStep( NRD_NORMAL_ENCODING_ERROR, 2.0 * NRD_NORMAL_ENCODING_ERROR, d );
            curvature *= s;

            // Correction #2 - very negative inconsistent with previous frame curvature blows up reprojection ( tests 164, 171 - 176 )
            float2 uv1 = STL::Geometry::GetScreenUv( gWorldToClipPrev, X - V * ApplyThinLensEquation( NoV, hitDistForTracking, curvature ) );
            float2 uv2 = STL::Geometry::GetScreenUv( gWorldToClipPrev, X );
            float a = length( ( uv1 - uv2 ) * gRectSize );
            curvature *= float( a < 3.0 * deltaUvLen + gRectSizeInv.x ); // TODO:it's a hack, incompatible with concave mirrors ( tests 22b, 23b, 25b )

            // Smooth virtual motion delta ( omitting huge values if curvature is negative and curvature radius is very small )
            float3 Xvirtual = GetXvirtual( NoV, hitDistForTracking, max( curvature, 0.0 ), X, Xprev, V, Dfactor );
            float2 vmbPixelUv = STL::Geometry::GetScreenUv( gWorldToClipPrev, Xvirtual );
            vmbDelta = vmbPixelUv - smbPixelUv;
        }

        // Virtual motion - coordinates
        float3 Xvirtual = GetXvirtual( NoV, hitDistForTracking, curvature, X, Xprev, V, Dfactor );
        float2 vmbPixelUv = STL::Geometry::GetScreenUv( gWorldToClipPrev, Xvirtual );

        float vmbPixelsTraveled = length( vmbDelta * gRectSize );
        float XvirtualLength = length( Xvirtual );

        // Estimate how many pixels are traveled by virtual motion - how many radians can it be?
        // IMPORTANT: if curvature angle is multiplied by path length then we can get an angle exceeding 2 * PI, what is impossible. The max
        // angle is PI ( most left and most right points on a hemisphere ), it can be achieved by using "tan" instead of angle.
        float pixelSize = PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, viewZ );
        float curvatureAngleTan = pixelSize * abs( curvature ); // tana = pixelSize / curvatureRadius = pixelSize * curvature
        curvatureAngleTan *= max( vmbPixelsTraveled / max( NoV, 0.01 ), 1.0 ); // path length

        float lobeHalfAngle = max( STL::ImportanceSampling::GetSpecularLobeHalfAngle( roughnessModified ), NRD_NORMAL_ULP );
        float curvatureAngle = atan( curvatureAngleTan );

        // IMPORTANT: increase roughness sensitivity at high FPS
        float roughnessSensitivity = NRD_ROUGHNESS_SENSITIVITY * lerp( 1.0, 0.5, STL::Math::SmoothStep( 1.0, 4.0, gFramerateScale ) );

        // Virtual motion - roughness
        STL::Filtering::Bilinear vmbBilinearFilter = STL::Filtering::GetBilinearFilter( vmbPixelUv, gRectSizePrev );
        float2 vmbBilinearGatherUv = ( vmbBilinearFilter.origin + 1.0 ) * gResourceSizeInvPrev;
        float2 relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( roughness * roughness, gRoughnessFraction, roughnessSensitivity );
        float4 vmbRoughness = gIn_Prev_Normal_Roughness.GatherAlpha( gNearestClamp, vmbBilinearGatherUv ).wzxy;
        float4 roughnessWeight = ComputeNonExponentialWeightWithSigma( vmbRoughness * vmbRoughness, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y, roughnessSigma );
        roughnessWeight = lerp( STL::Math::SmoothStep( 1.0, 0.0, smbParallaxInPixels ), 1.0, roughnessWeight ); // jitter friendly
        float virtualHistoryRoughnessBasedConfidence = STL::Filtering::ApplyBilinearFilter( roughnessWeight.x, roughnessWeight.y, roughnessWeight.z, roughnessWeight.w, vmbBilinearFilter );

        // Virtual motion - normal: parallax ( test 132 )
        float4 vmbNormalAndRoughness = UnpackNormalAndRoughness( gIn_Prev_Normal_Roughness.SampleLevel( gLinearClamp, vmbPixelUv * gResolutionScalePrev, 0 ) );
        float3 vmbN = STL::Geometry::RotateVector( gWorldPrevToWorld, vmbNormalAndRoughness.xyz );

        float hitDist = ExtractHitDist( spec ) * _REBLUR_GetHitDistanceNormalization( viewZ, gHitDistParams, roughness );
        float parallaxEstimation = smbParallaxInPixels * GetHitDistFactor( hitDist, frustumSize );
        float virtualHistoryNormalBasedConfidence = 1.0 / ( 1.0 + 0.5 * Dfactor * saturate( length( N - vmbN ) - NRD_NORMAL_ULP ) * max( parallaxEstimation, vmbPixelsTraveled ) );

        // Virtual motion - disocclusion: plane distance and roughness
        float4 vmbOcclusionThreshold = disocclusionThresholdMulFrustumSize;
        vmbOcclusionThreshold *= lerp( 0.05, 1.0, NoV ); // yes, "*" not "/"
        vmbOcclusionThreshold *= float( dot( vmbN, N ) > 0.0 ); // TODO: Navg?
        vmbOcclusionThreshold *= IsInScreenBilinear( vmbBilinearFilter.origin, gRectSizePrev );
        vmbOcclusionThreshold -= NRD_EPS;

        float4 vmbViewZ = UnpackViewZ( gIn_Prev_ViewZ.GatherRed( gNearestClamp, vmbBilinearGatherUv ).wzxy );
        float3 vmbVv = STL::Geometry::ReconstructViewPosition( vmbPixelUv, gFrustumPrev, 1.0 ); // unnormalized, orthoMode = 0
        float3 vmbV = STL::Geometry::RotateVectorInverse( gWorldToViewPrev, vmbVv );
        float NoXcurr = dot( N, X - gCameraDelta.xyz );
        float4 NoXprev = ( N.x * vmbV.x + N.y * vmbV.y ) * ( gOrthoMode == 0 ? vmbViewZ : gOrthoMode ) + N.z * vmbV.z * vmbViewZ;
        float4 vmbPlaneDist = abs( NoXprev - NoXcurr );
        float4 vmbOcclusion = step( vmbPlaneDist, vmbOcclusionThreshold );
        vmbOcclusion *= step( 0.5, roughnessWeight );

        // Virtual motion - disocclusion: materialID
        uint4 vmbInternalData = gIn_Prev_InternalData.GatherRed( gNearestClamp, vmbBilinearGatherUv ).wzxy;

        float3 vmbInternalData00 = UnpackInternalData( vmbInternalData.x );
        float3 vmbInternalData10 = UnpackInternalData( vmbInternalData.y );
        float3 vmbInternalData01 = UnpackInternalData( vmbInternalData.z );
        float3 vmbInternalData11 = UnpackInternalData( vmbInternalData.w );

        #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
            float4 vmbMaterialID = float4( vmbInternalData00.z, vmbInternalData10.z, vmbInternalData01.z, vmbInternalData11.z  );
            vmbOcclusion *= CompareMaterials( materialID, vmbMaterialID, gSpecMaterialMask );
        #endif

        // Bits
        fbits += vmbOcclusion.x * 16.0;
        fbits += vmbOcclusion.y * 32.0;
        fbits += vmbOcclusion.z * 64.0;
        fbits += vmbOcclusion.w * 128.0;

        // Virtual motion - accumulation speed
        float4 vmbOcclusionWeights = STL::Filtering::GetBilinearCustomWeights( vmbBilinearFilter, vmbOcclusion );
        float vmbSpecAccumSpeed = STL::Filtering::ApplyBilinearCustomWeights( vmbInternalData00.y, vmbInternalData10.y, vmbInternalData01.y, vmbInternalData11.y, vmbOcclusionWeights );

        float vmbFootprintQuality = STL::Filtering::ApplyBilinearFilter( vmbOcclusion.x, vmbOcclusion.y, vmbOcclusion.z, vmbOcclusion.w, vmbBilinearFilter );
        vmbFootprintQuality = STL::Math::Sqrt01( vmbFootprintQuality );
        vmbSpecAccumSpeed *= lerp( vmbFootprintQuality, 1.0, 1.0 / ( 1.0 + vmbSpecAccumSpeed ) );

        bool vmbAllowCatRom = dot( vmbOcclusion, 1.0 ) > 3.5 && REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA;
        vmbAllowCatRom = vmbAllowCatRom && smbAllowCatRom; // helps to reduce over-sharpening in disoccluded areas

        // Virtual motion - normal: lobe overlapping ( test 107 )
        float normalWeight = GetEncodingAwareNormalWeight( N, vmbN, lobeHalfAngle, curvatureAngle );
        normalWeight = lerp( STL::Math::SmoothStep( 1.0, 0.0, vmbPixelsTraveled ), 1.0, normalWeight ); // jitter friendly
        virtualHistoryNormalBasedConfidence = min( virtualHistoryNormalBasedConfidence, normalWeight );

        // Virtual history amount - normal confidence ( tests 9e, 65, 66, 107, 111, 132 )
        // IMPORTANT: this is currently needed for bumpy surfaces, because virtual motion gets ruined by big curvature
        float virtualHistoryAmount = virtualHistoryNormalBasedConfidence;

        // Virtual motion - virtual parallax difference
        // Tests 3, 6, 8, 11, 14, 100, 103, 104, 106, 109, 110, 114, 120, 127, 130, 131, 132, 138, 139 and 9e
        float hitDistForTrackingPrev = gIn_Prev_Spec_HitDistForTracking.SampleLevel( gLinearClamp, vmbPixelUv * gResolutionScalePrev, 0 );
        float3 XvirtualPrev = GetXvirtual( NoV, hitDistForTrackingPrev, curvature, X, Xprev, V, Dfactor );
        float XvirtualLengthPrev = length( XvirtualPrev );
        float2 vmbPixelUvPrev = STL::Geometry::GetScreenUv( gWorldToClipPrev, XvirtualPrev );

        float percentOfVolume = 0.6; // TODO: why 60%? should be smaller for high FPS?
        float lobeTanHalfAngle = STL::ImportanceSampling::GetSpecularLobeTanHalfAngle( roughness, percentOfVolume );

        #if( REBLUR_USE_MORE_STRICT_PARALLAX_BASED_CHECK == 1 )
            float unproj1 = min( hitDistForTracking, hitDistForTrackingPrev ) / PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, max( XvirtualLength, XvirtualLengthPrev ) );
            float lobeRadiusInPixels = lobeTanHalfAngle * unproj1;
        #else
            // Works better if "percentOfVolume" is 0.3-0.6
            float unproj1 = hitDistForTracking / PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, XvirtualLength );
            float unproj2 = hitDistForTrackingPrev / PixelRadiusToWorld( gUnproject, gOrthoMode, 1.0, XvirtualLengthPrev );
            float lobeRadiusInPixels = lobeTanHalfAngle * min( unproj1, unproj2 );
        #endif

        float deltaParallaxInPixels = length( ( vmbPixelUvPrev - vmbPixelUv ) * gRectSize );
        float virtualHistoryParallaxBasedConfidence = STL::Math::SmoothStep( lobeRadiusInPixels + 0.25, 0.0, deltaParallaxInPixels );

        // Virtual motion - normal & roughness prev-prev tests
        // IMPORTANT: 2 is needed because:
        // - line *** allows fallback to laggy surface motion, which can be wrongly redistributed by virtual motion
        // - we use at least linear filters, as the result a wider initial offset is needed
        float stepBetweenTaps = min( vmbPixelsTraveled * gFramerateScale, 2.0 ) + vmbPixelsTraveled / REBLUR_VIRTUAL_MOTION_PREV_PREV_WEIGHT_ITERATION_NUM;
        vmbDelta *= STL::Math::Rsqrt( STL::Math::LengthSquared( vmbDelta ) );
        vmbDelta /= gRectSizePrev;

        relaxedRoughnessWeightParams = GetRelaxedRoughnessWeightParams( vmbNormalAndRoughness.w * vmbNormalAndRoughness.w, gRoughnessFraction, roughnessSensitivity );

        [unroll]
        for( i = 1; i <= REBLUR_VIRTUAL_MOTION_PREV_PREV_WEIGHT_ITERATION_NUM; i++ )
        {
            float2 vmbPixelUvPrev = vmbPixelUv + vmbDelta * i * stepBetweenTaps;
            float4 vmbNormalAndRoughnessPrev = UnpackNormalAndRoughness( gIn_Prev_Normal_Roughness.SampleLevel( gLinearClamp, vmbPixelUvPrev * gResolutionScalePrev, 0 ) );

            float2 w;
            w.x = GetEncodingAwareNormalWeight( vmbNormalAndRoughness.xyz, vmbNormalAndRoughnessPrev.xyz, lobeHalfAngle, curvatureAngle * ( 1.0 + i * stepBetweenTaps ) );
            w.y = ComputeNonExponentialWeightWithSigma( vmbNormalAndRoughnessPrev.w * vmbNormalAndRoughnessPrev.w, relaxedRoughnessWeightParams.x, relaxedRoughnessWeightParams.y, roughnessSigma );

            w = IsInScreenNearest( vmbPixelUvPrev ) ? w : 1.0;

            virtualHistoryNormalBasedConfidence = min( virtualHistoryNormalBasedConfidence, w.x );
            virtualHistoryRoughnessBasedConfidence = min( virtualHistoryRoughnessBasedConfidence, w.y );
        }

        // Virtual history confidence
        float virtualHistoryConfidenceForSmbRelaxation = virtualHistoryNormalBasedConfidence * virtualHistoryRoughnessBasedConfidence;
        float virtualHistoryConfidence = virtualHistoryNormalBasedConfidence * virtualHistoryRoughnessBasedConfidence * virtualHistoryParallaxBasedConfidence;

        // Surface motion ( test 9, 9e )
        // IMPORTANT: needs to be responsive, because "vmb" fails on bumpy surfaces for the following reasons:
        //  - normal and prev-prev tests fail
        //  - curvature is so high that "vmb" regresses to "smb" and starts to lag
        float smc = GetSpecMagicCurve( roughnessModified );
        float surfaceHistoryConfidence = 1.0;
        {
            // Main part
            float f = lerp( 6.0, 0.0, smc ); // TODO: use Dfactor somehow?
            f *= lerp( 0.5, 1.0, virtualHistoryConfidenceForSmbRelaxation ); // TODO: ( optional ) visually looks good, but adds temporal lag

            // IMPORTANT: we must not use any "vmb" data for parallax estimation, because curvature can be wrong.
            // Estimation below is visually close to "vmbPixelsTraveled" computed for "vmbPixelUv" produced by 0 curvature
            surfaceHistoryConfidence /= 1.0 + f * parallaxEstimation;

            // Eliminate trailing if parallax is out of lobe ( test 142 )
            float ta = PixelRadiusToWorld( gUnproject, gOrthoMode, vmbPixelsTraveled, viewZ ) / viewZ;
            float ca = STL::Math::Rsqrt( 1.0 + ta * ta );
            float a = STL::Math::AcosApprox( ca ) - curvatureAngle;

            surfaceHistoryConfidence *= STL::Math::SmoothStep( lobeHalfAngle, 0.0, a );
        }

        // Surface motion: max allowed frames
        float smbMaxFrameNumNoBoost = gMaxAccumulatedFrameNum * surfaceHistoryConfidence;

        // Ensure that HistoryFix pass doesn't pop up without a disocclusion in critical cases
        float smbMaxFrameNum = max( smbMaxFrameNumNoBoost, gHistoryFixFrameNum * ( 1.0 - virtualHistoryConfidenceForSmbRelaxation ) );

        // Virtual motion: max allowed frames
        float responsiveAccumulationAmount = GetResponsiveAccumulationAmount( roughness );
        responsiveAccumulationAmount = lerp( 1.0, smc, responsiveAccumulationAmount );

        float vmbMaxFrameNum = gMaxAccumulatedFrameNum * responsiveAccumulationAmount;
        vmbMaxFrameNum *= virtualHistoryParallaxBasedConfidence;
        vmbMaxFrameNum *= virtualHistoryNormalBasedConfidence;

        // Limit number of accumulated frames
        float smbSpecAccumSpeedNoBoost = min( smbSpecAccumSpeed, smbMaxFrameNumNoBoost );
        smbSpecAccumSpeed = min( smbSpecAccumSpeed, smbMaxFrameNum );
        vmbSpecAccumSpeed = min( vmbSpecAccumSpeed, vmbMaxFrameNum );

        // Virtual history amount - other ( tests 65, 66, 103, 111, 132, e9, e11 )
        virtualHistoryAmount *= STL::Math::SmoothStep( 0.05, 0.95, Dfactor );
        virtualHistoryAmount *= virtualHistoryRoughnessBasedConfidence;
        virtualHistoryAmount *= saturate( vmbSpecAccumSpeed / ( smbSpecAccumSpeed + NRD_EPS ) ); // ***

        #if( REBLUR_VIRTUAL_HISTORY_AMOUNT != 2 )
            virtualHistoryAmount = REBLUR_VIRTUAL_HISTORY_AMOUNT;
        #endif

        // Sample history
        REBLUR_TYPE smbSpecHistory;
        REBLUR_FAST_TYPE smbSpecFastHistory;
        REBLUR_SH_TYPE smbSpecShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( smbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            smbOcclusionWeights, smbAllowCatRom,
            gIn_Spec_History, smbSpecHistory,
            gIn_SpecFast_History, smbSpecFastHistory
            #ifdef REBLUR_SH
                , gIn_SpecSh_History, smbSpecShHistory
            #endif
        );

        REBLUR_TYPE vmbSpecHistory;
        REBLUR_FAST_TYPE vmbSpecFastHistory;
        REBLUR_SH_TYPE vmbSpecShHistory;

        BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
            saturate( vmbPixelUv ) * gRectSizePrev, gResourceSizeInvPrev,
            vmbOcclusionWeights, vmbAllowCatRom,
            gIn_Spec_History, vmbSpecHistory,
            gIn_SpecFast_History, vmbSpecFastHistory
            #ifdef REBLUR_SH
                , gIn_SpecSh_History, vmbSpecShHistory
            #endif
        );

        // Avoid negative values
        smbSpecHistory = ClampNegativeToZero( smbSpecHistory );
        vmbSpecHistory = ClampNegativeToZero( vmbSpecHistory );

        // Accumulation
        float smbSpecNonLinearAccumSpeed = 1.0 / ( 1.0 + smbSpecAccumSpeedNoBoost );
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
            specShResult.w = roughnessModified; // IMPORTANT: should not be blurred
        #endif

        float specAccumSpeed = lerp( smbSpecAccumSpeed, vmbSpecAccumSpeed, virtualHistoryAmount );
        REBLUR_TYPE specHistory = lerp( smbSpecHistory, vmbSpecHistory, virtualHistoryAmount );

        // Anti-firefly suppressor
        float specAntifireflyFactor = specAccumSpeed * ( gMinBlurRadius + gMaxBlurRadius ) * REBLUR_FIREFLY_SUPPRESSOR_RADIUS_SCALE * smc;
        specAntifireflyFactor /= 1.0 + specAntifireflyFactor;

        float specHitDistResult = ExtractHitDist( specResult );
        float specHitDistClamped = min( specHitDistResult, ExtractHitDist( specHistory ) * REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY.y );
        specHitDistClamped = lerp( specHitDistResult, specHitDistClamped, specAntifireflyFactor );

        #if( defined REBLUR_OCCLUSION || defined REBLUR_DIRECTIONAL_OCCLUSION )
            specResult = ChangeLuma( specResult, specHitDistClamped );
        #else
            float specLumaResult = GetLuma( specResult );
            float specLumaClamped = min( specLumaResult, GetLuma( specHistory ) * REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY.x );
            specLumaClamped = lerp( specLumaResult, specLumaClamped, specAntifireflyFactor );

            specResult = ChangeLuma( specResult, specLumaClamped );
            specResult.w = specHitDistClamped;

            #ifdef REBLUR_SH
                specShResult.xyz *= GetLumaScale( length( specShResult.xyz ), specLumaClamped );
            #endif
        #endif

        // Output
        float specError = GetColorErrorForAdaptiveRadiusScale( specResult, specHistory, specAccumSpeed, roughness );

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
            virtualHistoryAmount = hitDistForTracking * lerp( 1.0, 5.0, smc ) / ( 1.0 + hitDistForTracking * lerp( 1.0, 5.0, smc ) );
        #endif
    #else
        float specAccumSpeed = 0;
        float curvature = 0;
        float virtualHistoryAmount = 0;
        float specError = 0;
    #endif

    // Output
    #ifndef REBLUR_OCCLUSION
        gOut_Data2[ pixelPos ] = PackData2( fbits, curvature, virtualHistoryAmount );
    #endif

    // Diffuse
    #ifdef REBLUR_DIFFUSE
        // Accumulation speed
        float diffHistoryConfidence = smbFootprintQuality;
        if( gHasHistoryConfidence )
            diffHistoryConfidence *= gIn_Diff_Confidence[ WithRectOrigin( pixelPos ) ];

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
            gIn_Diff_History, smbDiffHistory,
            gIn_DiffFast_History, smbDiffFastHistory
            #ifdef REBLUR_SH
                , gIn_DiffSh_History, smbDiffShHistory
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

        // Anti-firefly suppressor
        float diffAntifireflyFactor = diffAccumSpeed * ( gMinBlurRadius + gMaxBlurRadius ) * REBLUR_FIREFLY_SUPPRESSOR_RADIUS_SCALE;
        diffAntifireflyFactor /= 1.0 + diffAntifireflyFactor;

        float diffHitDistResult = ExtractHitDist( diffResult );
        float diffHitDistClamped = min( diffHitDistResult, ExtractHitDist( smbDiffHistory ) * REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY.y );
        diffHitDistClamped = lerp( diffHitDistResult, diffHitDistClamped, diffAntifireflyFactor );

        #if( defined REBLUR_OCCLUSION || defined REBLUR_DIRECTIONAL_OCCLUSION )
            diffResult = ChangeLuma( diffResult, diffHitDistClamped );
        #else
            float diffLumaResult = GetLuma( diffResult );
            float diffLumaClamped = min( diffLumaResult, GetLuma( smbDiffHistory ) * REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY.x );
            diffLumaClamped = lerp( diffLumaResult, diffLumaClamped, diffAntifireflyFactor );

            diffResult = ChangeLuma( diffResult, diffLumaClamped );
            diffResult.w = diffHitDistClamped;

            #ifdef REBLUR_SH
                diffShResult.xyz *= GetLumaScale( length( diffShResult.xyz ), diffLumaClamped );
            #endif
        #endif

        // Output
        float diffError = GetColorErrorForAdaptiveRadiusScale( diffResult, smbDiffHistory, diffAccumSpeed );

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

        gOut_DiffFast[ pixelPos ] = diffFastResult;
    #else
        float diffAccumSpeed = 0;
        float diffError = 0;
    #endif

    // Output
    gOut_Data1[ pixelPos ] = PackData1( diffAccumSpeed, diffError, specAccumSpeed, specError );
}
