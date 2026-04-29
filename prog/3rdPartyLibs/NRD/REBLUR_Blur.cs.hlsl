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

    // Tile-based early out and viewZ-based early out
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    float viewZ = UnpackViewZ( viewZpacked );
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) || viewZ > gDenoisingRange )
        return;

    // Center data
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float3 Nv = Geometry::RotateVectorInverse( gViewToWorld, N );
    float roughness = normalAndRoughness.w;

    REBLUR_DATA1_TYPE data1 = UnpackData1( gIn_Data1[ pixelPos ] );

    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );

    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );

    const float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    const float4 rotator = GetBlurKernelRotation( REBLUR_BLUR_ROTATOR_MODE, pixelPos, gRotator, gFrameIndex );

    // Non-linear accum speed
    float2 nonLinearAccumSpeed;
    nonLinearAccumSpeed.x = GetAdvancedNonLinearAccumSpeed( data1.x );
    nonLinearAccumSpeed.y = GetAdvancedNonLinearAccumSpeed( data1.y );

    #ifdef NRD_COMPILER_DXC
        // Adapt to neighbors if they are more stable
        REBLUR_DATA1_TYPE d10 = QuadReadAcrossX( nonLinearAccumSpeed );
        REBLUR_DATA1_TYPE d01 = QuadReadAcrossY( nonLinearAccumSpeed );

        REBLUR_DATA1_TYPE avg = ( d10 + d01 + nonLinearAccumSpeed ) / 3.0;
        nonLinearAccumSpeed = min( nonLinearAccumSpeed, avg );
    #endif

    // Spatial filtering
    #define REBLUR_SPATIAL_MODE REBLUR_BLUR

    #if( NRD_DIFF )
    {
        float sum = 1.0;
        REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
        #if( NRD_MODE == SH )
            float4 diffSh = gIn_DiffSh[ pixelPos ];
        #endif

        #include "REBLUR_Common_DiffuseSpatialFilter.hlsli"
    }
    #endif

    #if( NRD_SPEC )
    {
        float sum = 1.0;
        REBLUR_TYPE spec = gIn_Spec[ pixelPos ];
        #if( NRD_MODE == SH )
            float4 specSh = gIn_SpecSh[ pixelPos ];
        #endif

        #include "REBLUR_Common_SpecularSpatialFilter.hlsli"
    }
    #endif
}
