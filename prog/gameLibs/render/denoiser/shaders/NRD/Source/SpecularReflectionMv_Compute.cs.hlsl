/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NRD.hlsli"
#include "STL.hlsli"

#include "SpecularReflectionMv_Compute.resources.hlsli"

#include "Common.hlsli"

groupshared float4 s_Normal_Roughness[ BUFFER_Y ][ BUFFER_X ];

void Preload( uint2 sharedPos, int2 globalPos )
{
    globalPos = clamp( globalPos, 0, gRectSize - 1.0 );

    s_Normal_Roughness[ sharedPos.y ][ sharedPos.x ] = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness[ WithRectOrigin( globalPos ) ] );
}

float3 GetViewVector( float3 X, bool isViewSpace = false )
{
    return gOrthoMode == 0.0 ? normalize( -X ) : ( isViewSpace ? float3( 0, 0, -1 ) : gViewVectorWorld.xyz );
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( int2 threadPos : SV_GroupThreadId, int2 pixelPos : SV_DispatchThreadId, uint threadIndex : SV_GroupIndex )
{
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;

    PRELOAD_INTO_SMEM;

    // Early out
    float viewZ = gIn_ViewZ[ WithRectOrigin( pixelPos ) ];

    [branch]
    if( viewZ > gDenoisingRange )
        return;

    // Normal and roughness
    int2 smemPos = threadPos + BORDER;
    float4 normalAndRoughness = s_Normal_Roughness[ smemPos.y ][ smemPos.x ];
    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    // Current position and view vectir
    float3 Xv = STL::Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode );
    float3 X = STL::Geometry::RotateVector( gViewToWorld, Xv );
    float3 V = GetViewVector( X );
    float NoV = abs( dot( N, V ) );

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

    // Modified roughness
    float3 Navg = N;

    [unroll]
    for( j = 0; j <= BORDER * 2; j++ )
    {
        [unroll]
        for( i = 0; i <= BORDER * 2; i++ )
        {
            if( i == BORDER && j == BORDER )
                continue;

            int2 pos = threadPos + int2( i, j );
            Navg += s_Normal_Roughness[ pos.y ][ pos.x ].xyz;
        }
    }

    Navg /= ( BORDER * 2 + 1 ) * ( BORDER * 2 + 1 ); // needs to be unnormalized!

    float roughnessModified = STL::Filtering::GetModifiedRoughnessFromNormalVariance( roughness, Navg );

    // Parallax
    float smbParallaxInPixels = ComputeParallaxInPixels( Xprev - gCameraDelta.xyz, gOrthoMode == 0.0 ? pixelUv : smbPixelUv, gWorldToClip, gRectSize );

    // Hit distance
    float hitDistForTracking = gIn_HitDist[ pixelPos ]; // TODO: min hitDist logic from REBLUR / RELAX needed

    // Curvature
    float curvature;
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
    }

    // Virtual motion
    float dominantFactor = STL::ImportanceSampling::GetSpecularDominantFactor( NoV, roughnessModified, STL_SPECULAR_DOMINANT_DIRECTION_G2 );

    float3 Xvirtual = GetXvirtual( NoV, hitDistForTracking, curvature, X, Xprev, V, dominantFactor );
    float2 vmbPixelUv = STL::Geometry::GetScreenUv( gWorldToClipPrev, Xvirtual );

    gOut_SpecularReflectionMv[ pixelPos ] = vmbPixelUv - pixelUv;
}
