#include "NRD.hlsli"
#include "ml.hlsli"

#include "REBLUR_Config.hlsli"

NRD_CONSTANTS_START( REBLUR_BehindHistoryConstants )
    REBLUR_SHARED_CONSTANTS
    NRD_CONSTANT( uint, gHasDiffuse )
    NRD_CONSTANT( uint, gHasSpecular )
    NRD_CONSTANT( uint, gHasBehindHistory )
    NRD_CONSTANT( uint, gSecondChanceDebug )
NRD_CONSTANTS_END

NRD_SAMPLERS_START
    NRD_SAMPLER( SamplerState, gNearestClamp, s, 0 )
    NRD_SAMPLER( SamplerState, gLinearClamp, s, 1 )
NRD_SAMPLERS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D, float, gIn_ViewZ, t, 0 )
    NRD_INPUT( Texture2D, uint, gIn_InternalData, t, 1 )
    NRD_INPUT( Texture2D, REBLUR_TYPE, gIn_Diff, t, 2 )
    NRD_INPUT( Texture2D, REBLUR_FAST_TYPE, gIn_DiffFast, t, 3 )
    NRD_INPUT( Texture2D, float, gBehind_ViewZ, t, 4 )
    NRD_INPUT( Texture2D, uint, gBehind_InternalData, t, 5 )
    NRD_INPUT( Texture2D, REBLUR_TYPE, gBehind_Diff, t, 6 )
    NRD_INPUT( Texture2D, REBLUR_FAST_TYPE, gBehind_DiffFast, t, 7 )
    NRD_INPUT( Texture2D, float, gPrev_ViewZ, t, 8 )
    NRD_INPUT( Texture2D, uint, gPrev_InternalData, t, 9 )
    NRD_INPUT( Texture2D, REBLUR_TYPE, gHistory_Diff, t, 10 )
    NRD_INPUT( Texture2D, REBLUR_FAST_TYPE, gHistory_DiffFast, t, 11 )
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D, float, gOut_ViewZ, u, 0 )
    NRD_OUTPUT( RWTexture2D, uint, gOut_InternalData, u, 1 )
    NRD_OUTPUT( RWTexture2D, REBLUR_TYPE, gOut_Diff, u, 2 )
    NRD_OUTPUT( RWTexture2D, REBLUR_FAST_TYPE, gOut_DiffFast, u, 3 )
NRD_OUTPUTS_END

#include "Common.hlsli"
#include "REBLUR_Common.hlsli"

#define BEHIND_OCCLUDER_DEPTH_RATIO 0.9

int2 PrevPos( float2 uv )
{
    return clamp( int2( uv * gRectSizePrev ), 0, int2( gRectSizePrev ) - 1 );
}

bool FindHiddenTexel( Texture2D<float> viewZTex, float2 pixelUv, float visibleViewZ, out int2 texelPos, out float texelViewZ )
{
    float viewZ = UnpackViewZ( viewZTex[ PrevPos( pixelUv ) ] );
    float2 uvPrev = pixelUv;

    [unroll]
    for( uint i = 0; i < 3; i++ )
    {
        float3 X = Geometry::RotateVector( gViewToWorld, Geometry::ReconstructViewPosition( pixelUv, gFrustum, viewZ, gOrthoMode ) );
        uvPrev = Geometry::GetScreenUv( gWorldToClipPrev, X );
        viewZ += UnpackViewZ( viewZTex[ PrevPos( uvPrev ) ] ) - Geometry::AffineTransform( gWorldToViewPrev, X ).z;
    }

    texelPos = PrevPos( uvPrev );
    float texelViewZPrev = UnpackViewZ( viewZTex[ texelPos ] );

    float2 texelUv = ( float2( texelPos ) + 0.5 ) / gRectSizePrev;
    float3 Xvprev = Geometry::ReconstructViewPosition( texelUv, gFrustumPrev, texelViewZPrev, gOrthoMode );
    float3 X = Geometry::RotateVectorInverse( gWorldToViewPrev, Xvprev ) + gCameraDelta.xyz;
    float3 Xv = Geometry::RotateVectorInverse( gViewToWorld, X );
    texelViewZ = Xv.z;

    float2 texelUvNow = ( Xv.xy / Xv.z - gFrustum.xy ) / gFrustum.zw;
    bool landsOnPixel = all( abs( texelUvNow - pixelUv ) * gRectSize < 1.0 );

    return landsOnPixel && IsInScreenNearest( uvPrev ) && IsInDenoisingRange( texelViewZPrev ) && visibleViewZ < texelViewZ * BEHIND_OCCLUDER_DEPTH_RATIO;
}

uint AgeInternalData( uint internalData )
{
    float3 data = UnpackInternalData( internalData );

    return PackInternalData( max( data.x - 2.0, 0.0 ), max( data.y - 2.0, 0.0 ), data.z );
}

[numthreads( 8, 8, 1 )]
void main( uint2 pixelPos : SV_DispatchThreadID )
{
    if( any( int2( pixelPos ) > gRectSizeMinusOne ) )
        return;

    float viewZpacked = gIn_ViewZ[ pixelPos ];
    uint internalData = gIn_InternalData[ pixelPos ];
    REBLUR_TYPE diff = gIn_Diff[ pixelPos ];
    REBLUR_FAST_TYPE diffFast = gIn_DiffFast[ pixelPos ];

    [branch]
    if( gResetHistory == 0 )
    {
        float viewZ = UnpackViewZ( viewZpacked );
        float2 pixelUv = ( float2( pixelPos ) + 0.5 ) * gRectSizeInv;

        int2 texelPos;
        float texelViewZ;
        if( FindHiddenTexel( gPrev_ViewZ, pixelUv, viewZ, texelPos, texelViewZ ) )
        {
            viewZpacked = texelViewZ / gViewZScale;
            internalData = AgeInternalData( gPrev_InternalData[ texelPos ] );
            diff = gHistory_Diff[ texelPos ];
            diffFast = gHistory_DiffFast[ texelPos ];
        }
        else if( gHasBehindHistory != 0 && FindHiddenTexel( gBehind_ViewZ, pixelUv, viewZ, texelPos, texelViewZ ) )
        {
            viewZpacked = texelViewZ / gViewZScale;
            internalData = AgeInternalData( gBehind_InternalData[ texelPos ] );
            diff = gBehind_Diff[ texelPos ];
            diffFast = gBehind_DiffFast[ texelPos ];
        }
    }

    gOut_ViewZ[ pixelPos ] = viewZpacked;
    gOut_InternalData[ pixelPos ] = internalData;
    gOut_Diff[ pixelPos ] = diff;
    gOut_DiffFast[ pixelPos ] = diffFast;
}
