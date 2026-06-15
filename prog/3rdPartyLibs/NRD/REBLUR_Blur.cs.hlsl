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
#include "REBLUR_Blur.resources.hlsli"

#include "Common.hlsli"

#include "REBLUR_Common.hlsli"

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    // Copy viewZ ( including sky! ) for the next pass and frame ( potentially lower precision )
    float viewZpacked = gIn_ViewZ[ WithRectOrigin( pixelPos ) ];

    #if( REBLUR_COPY_GBUFFER == 1 )
        gOut_ViewZ[ pixelPos ] = viewZpacked;
    #endif

    // Tile-based early out ( quad uniform )
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    if( isSky != 0.0 )
        return;

    // Non-linear accum speed
    REBLUR_DATA1_TYPE data1 = UnpackData1( gIn_Data1[ pixelPos ] );

    REBLUR_DATA1_TYPE nonLinearAccumSpeed;
    nonLinearAccumSpeed.x = GetAdvancedNonLinearAccumSpeed( data1.x );
    nonLinearAccumSpeed.y = GetAdvancedNonLinearAccumSpeed( data1.y );

    float viewZ = UnpackViewZ( viewZpacked );
    nonLinearAccumSpeed = !IsInDenoisingRange( viewZ ) ? 0.0 : nonLinearAccumSpeed; // less blur on "SKY" edges

    #ifdef NRD_COMPILER_DXC
    {
        // IMPORTANT: the spec says: "these routines assume that flow control execution is uniform at least across the quad"
        // Adapt to neighbors if they are more stable
        REBLUR_DATA1_TYPE d10 = QuadReadAcrossX( nonLinearAccumSpeed );
        REBLUR_DATA1_TYPE d01 = QuadReadAcrossY( nonLinearAccumSpeed );

        REBLUR_DATA1_TYPE avg = ( d10 + d01 + nonLinearAccumSpeed ) / 3.0;
        nonLinearAccumSpeed = min( nonLinearAccumSpeed, avg );
    }
    #endif

    // Early out ( thread )
    if( !IsInDenoisingRange( viewZ ) || any( pixelPos > gRectSizeMinusOne ) )
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
    const float4 rotator = GetBlurKernelRotation( REBLUR_BLUR_ROTATOR_MODE, pixelPos, gRotator, gFrameIndex );

    // Spatial filtering
    #define REBLUR_SPATIAL_PASS REBLUR_BLUR

    #if( NRD_DIFF )
        #define REBLUR_SPATIAL_LOBE REBLUR_DIFF
        #include "REBLUR_Common_SpatialFilter.hlsli"
    #endif

    #if( NRD_SPEC )
        #define REBLUR_SPATIAL_LOBE REBLUR_SPEC
        #include "REBLUR_Common_SpatialFilter.hlsli"
    #endif
}
