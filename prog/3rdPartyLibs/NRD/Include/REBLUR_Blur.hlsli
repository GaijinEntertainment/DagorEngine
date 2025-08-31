/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    // Tile-based early out
    float isSky = gIn_Tiles[ pixelPos >> 4 ].x;
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Early out
    float viewZpacked = gIn_ViewZ[ WithRectOrigin( pixelPos ) ];
    gOut_ViewZ[ pixelPos ] = viewZpacked;

    float viewZ = UnpackViewZ( viewZpacked );
    if( viewZ > gDenoisingRange )
        return;

    // Normal and roughness
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float3 Nv = Geometry::RotateVectorInverse( gViewToWorld, N );
    float roughness = normalAndRoughness.w;

    // Shared data
    REBLUR_DATA1_TYPE data1 = UnpackData1( gIn_Data1[ pixelPos ] );

    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );

    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );

    const float frustumSize = GetFrustumSize( gMinRectDimMulUnproject, gOrthoMode, viewZ );
    const float4 rotator = GetBlurKernelRotation( REBLUR_BLUR_ROTATOR_MODE, pixelPos, gRotator, gFrameIndex );

    // Spatial filtering
    #define REBLUR_SPATIAL_MODE REBLUR_BLUR

    #ifdef REBLUR_DIFFUSE
    {
        float sum = 1.0;
        REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
        #ifdef REBLUR_SH
            float4 diffSh = gIn_DiffSh[ pixelPos ];
        #endif

        #include "REBLUR_Common_DiffuseSpatialFilter.hlsli"
    }
    #endif

    #ifdef REBLUR_SPECULAR
    {
        float sum = 1.0;
        REBLUR_TYPE spec = gIn_Spec[ pixelPos ];
        #ifdef REBLUR_SH
            float4 specSh = gIn_SpecSh[ pixelPos ];
        #endif

        #include "REBLUR_Common_SpecularSpatialFilter.hlsli"
    }
    #endif
}
