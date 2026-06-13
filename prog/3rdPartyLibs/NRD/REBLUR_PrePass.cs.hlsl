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
#include "REBLUR_PrePass.resources.hlsli"

#include "Common.hlsli"

#include "REBLUR_Common.hlsli"

// IMPORTANT Gaijin modification
// It's very important to use sample, instead of operator[] to fetch first 3 values from gIn_ViewZ,
// frequent gpu crash on nVidia gpus will happen in other case.
float SampleViewZ(int2 coord)
{
    return gIn_ViewZ.SampleLevel(gNearestClamp, (coord.xy + 0.5f) * gRectSizeInv, 0);
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_REVERSED;

    // Tile-based early out
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Early out
    float viewZ = UnpackViewZ( SampleViewZ( WithRectOrigin( pixelPos ) ) ); // Modified by Gaijin
    if( !IsInDenoisingRange( viewZ ) )
        return;

    // Center data
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float3 Nv = Geometry::RotateVectorInverse( gViewToWorld, N );
    float roughness = normalAndRoughness.w;

    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );

    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );

    const float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    const float4 rotator = GetBlurKernelRotation( REBLUR_PRE_PASS_ROTATOR_MODE, pixelPos, gRotatorPre, gFrameIndex );

    // Checkerboard resolve
#if( NRD_SUPPORTS_CHECKERBOARD == 1 )
    uint checkerboard = Sequence::CheckerBoard( pixelPos, gFrameIndex );

    int3 checkerboardPos = pixelPos.xxy + int3( -1, 1, 0 );
    checkerboardPos.x = max( checkerboardPos.x, 0 );
    checkerboardPos.y = min( checkerboardPos.y, gRectSizeMinusOne.x );
    float viewZ0 = UnpackViewZ( SampleViewZ( WithRectOrigin( checkerboardPos.xz ) ) ); // Modified by Gaijin
    float viewZ1 = UnpackViewZ( SampleViewZ( WithRectOrigin( checkerboardPos.yz ) ) ); // Modified by Gaijin
    float disocclusionThresholdCheckerboard = GetDisocclusionThreshold( NRD_DISOCCLUSION_THRESHOLD, frustumSize, NoV );
    float2 wc = GetDisocclusionWeight( float2( viewZ0, viewZ1 ), viewZ, disocclusionThresholdCheckerboard );
    wc.x = ( !IsInDenoisingRange( viewZ0 ) || pixelPos.x < 1 ) ? 0.0 : wc.x;
    wc.y = ( !IsInDenoisingRange( viewZ1 ) || pixelPos.x >= gRectSizeMinusOne.x ) ? 0.0 : wc.y;
    wc *= Math::PositiveRcp( wc.x + wc.y );
    checkerboardPos.xy >>= 1;
#endif

    // Non-linear accum speed
    float2 nonLinearAccumSpeed = REBLUR_PRE_PASS_NON_LINEAR_ACCUM_SPEED;

    // Spatial filtering
    #define REBLUR_SPATIAL_PASS REBLUR_PRE_PASS

    #if( NRD_DIFF )
        #define REBLUR_SPATIAL_LOBE REBLUR_DIFF
        #define MAX_BLUR_RADIUS gDiffPrepassBlurRadius
        #include "REBLUR_Common_SpatialFilter.hlsli"
    #endif

    #if( NRD_SPEC )
        #define REBLUR_SPATIAL_LOBE REBLUR_SPEC
        #define MAX_BLUR_RADIUS gSpecPrepassBlurRadius
        #include "REBLUR_Common_SpatialFilter.hlsli"
    #endif
}
