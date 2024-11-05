/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( int2 threadPos : SV_GroupThreadId, int2 pixelPos : SV_DispatchThreadId, uint threadIndex : SV_GroupIndex )
{
    // Tile-based early out
    float isSky = gIn_Tiles[ pixelPos >> 4 ];
    if( isSky != 0.0 || any( pixelPos > gRectSizeMinusOne ) )
        return;

    // Early out
    float viewZ = abs( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    if( viewZ > gDenoisingRange )
        return;

    // Checkerboard resolve
    uint checkerboard = STL::Sequence::CheckerBoard( pixelPos, gFrameIndex );

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

    // Normal and roughness
    float materialID;
    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( pixelPos ) ], materialID );
    float3 N = normalAndRoughness.xyz;
    float3 Nv = STL::Geometry::RotateVectorInverse( gViewToWorld, N );
    float roughness = normalAndRoughness.w;

    // Shared data
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float4 rotator = GetBlurKernelRotation( REBLUR_PRE_BLUR_ROTATOR_MODE, pixelPos, gRotator, gFrameIndex );

    float3 Vv = GetViewVector( Xv, true );
    float NoV = abs( dot( Nv, Vv ) );

    #define REBLUR_SPATIAL_MODE REBLUR_PRE_BLUR

    #ifdef REBLUR_DIFFUSE
    {
        uint2 pos = pixelPos;
        pos.x >>= gDiffCheckerboard == 2 ? 0 : 1;

        float sum = 1.0;
        REBLUR_TYPE diff = gIn_Diff[ pos ];
        #ifdef REBLUR_SH
            float4 diffSh = gIn_DiffSh[ pos ];
        #endif

        if( gDiffCheckerboard != 2 && checkerboard != gDiffCheckerboard )
        {
            sum = 0;
            diff = 0;
            #ifdef REBLUR_SH
                diffSh = 0;
            #endif
        }

        #include "REBLUR_Common_DiffuseSpatialFilter.hlsli"
    }
    #endif

    #ifdef REBLUR_SPECULAR
    {
        uint2 pos = pixelPos;
        pos.x >>= gSpecCheckerboard == 2 ? 0 : 1;

        float sum = 1.0;
        REBLUR_TYPE spec = gIn_Spec[ pos ];
        #ifdef REBLUR_SH
            float4 specSh = gIn_SpecSh[ pos ];
        #endif

        if( gSpecCheckerboard != 2 && checkerboard != gSpecCheckerboard )
        {
            sum = 0;
            spec = 0;
            #ifdef REBLUR_SH
                specSh = 0;
            #endif
        }

        #include "REBLUR_Common_SpecularSpatialFilter.hlsli"
    }
    #endif
}
