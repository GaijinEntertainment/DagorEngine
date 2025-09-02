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
#include "REBLUR_Validation.resources.hlsli"

#include "Common.hlsli"
#include "REBLUR_Common.hlsli"

#define VIEWPORT_SIZE   0.25
#define OFFSET          5

/*
Viewport layout:
 0   1   2   3
 4   5   6   7
 8   9  10  11
12  13  14  15
*/

[numthreads( 16, 16, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    if( gResetHistory != 0 )
    {
        gOut_Validation[ pixelPos ] = 0;
        return;
    }

    float2 pixelUv = float2( pixelPos + 0.5 ) / gResourceSize;

    float2 viewportUv = frac( pixelUv / VIEWPORT_SIZE );
    float2 viewportId = floor( pixelUv / VIEWPORT_SIZE );
    float viewportIndex = viewportId.y / VIEWPORT_SIZE + viewportId.x;

    float2 viewportUvScaled = viewportUv * gResolutionScale;

    float4 normalAndRoughness = NRD_FrontEnd_UnpackNormalAndRoughness( gIn_Normal_Roughness.SampleLevel( gNearestClamp, WithRectOffset( viewportUvScaled ), 0 ) );
    float viewZ = UnpackViewZ( gIn_ViewZ.SampleLevel( gNearestClamp, WithRectOffset( viewportUvScaled ), 0 ) );
    float3 mv = gIn_Mv.SampleLevel( gNearestClamp, WithRectOffset( viewportUvScaled ), 0 ) * gMvScale.xyz;
    float4 diff = gIn_Diff.SampleLevel( gNearestClamp, viewportUvScaled * float2( gDiffCheckerboard != 2 ? 0.5 : 1.0, 1.0 ), 0 );
    float4 spec = gIn_Spec.SampleLevel( gNearestClamp, viewportUvScaled * float2( gSpecCheckerboard != 2 ? 0.5 : 1.0, 1.0 ), 0 );

    // See "UnpackData1"
    REBLUR_DATA1_TYPE data1 = gIn_Data1.SampleLevel( gNearestClamp, viewportUvScaled, 0 );
    if( !gHasDiffuse )
        data1.y = data1.x;
    data1 *= REBLUR_MAX_ACCUM_FRAME_NUM;

    uint bits;
    bool smbAllowCatRom;
    float2 data2 = UnpackData2( gIn_Data2[ uint2( viewportUvScaled * gResourceSize ) ], bits, smbAllowCatRom );

    float3 N = normalAndRoughness.xyz;
    float roughness = normalAndRoughness.w;

    float3 Xv = Geometry::ReconstructViewPosition( viewportUv, gFrustum, abs( viewZ ), gOrthoMode );
    float3 X = Geometry::RotateVector( gViewToWorld, Xv );

    bool isInf = abs( viewZ ) > gDenoisingRange;
    bool checkerboard = Sequence::CheckerBoard( pixelPos >> 2, 0 );

    uint4 textState = Text::Init( pixelPos, viewportId * gResourceSize * VIEWPORT_SIZE + OFFSET, 1 );

    float4 result = gOut_Validation[ pixelPos ];

    // World-space normal
    if( viewportIndex == 0 )
    {
        Text::Print_ch( 'N', textState );
        Text::Print_ch( 'O', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'M', textState );
        Text::Print_ch( 'A', textState );
        Text::Print_ch( 'L', textState );
        Text::Print_ch( 'S', textState );

        result.xyz = N * 0.5 + 0.5;
        result.w = 1.0;
    }
    // Linear roughness
    else if( viewportIndex == 1 )
    {
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'O', textState );
        Text::Print_ch( 'U', textState );
        Text::Print_ch( 'G', textState );
        Text::Print_ch( 'H', textState );
        Text::Print_ch( 'N', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'S', textState );
        Text::Print_ch( 'S', textState );

        result.xyz = normalAndRoughness.w;
        result.w = 1.0;
    }
    // View Z
    else if( viewportIndex == 2 )
    {
        Text::Print_ch( 'Z', textState );
        if( viewZ < 0 )
            Text::Print_ch( Text::Char_Minus, textState );

        float f = 0.1 * abs( viewZ ) / ( 1.0 + 0.1 * abs( viewZ ) ); // TODO: tuned for meters
        float3 color = viewZ < 0.0 ? float3( 0, 0, 1 ) : float3( 0, 1, 0 );

        result.xyz = isInf ? float3( 1, 0, 0 ) : color * f;
        result.w = 1.0;
    }
    // MV
    else if( viewportIndex == 3 )
    {
        Text::Print_ch( 'M', textState );
        Text::Print_ch( 'V', textState );

        float2 viewportUvPrevExpected = Geometry::GetScreenUv( gWorldToClipPrev, X );

        float2 viewportUvPrev = viewportUv + mv.xy;
        if( gMvScale.w != 0.0 )
            viewportUvPrev = Geometry::GetScreenUv( gWorldToClipPrev, X + mv );

        float2 uvDelta = ( viewportUvPrev - viewportUvPrevExpected ) * gRectSize;

        result.xyz = IsInScreenNearest( viewportUvPrev ) ? float3( abs( uvDelta ), 0 ) : float3( 0, 0, 1 );
        result.w = 1.0;
    }
    // World units, jitter and rotators
    else if( viewportIndex == 4 )
    {
        Text::Print_ch( 'U', textState );
        Text::Print_ch( 'N', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'S', textState );
        Text::Print_ch( '-', textState );
        Text::Print_ch( 'J', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( '-', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'A', textState );
        Text::Print_ch( 'M', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'N', textState );
        Text::Print_ch( 'D', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'X', textState );

        float2 dim = float2( 0.5 * gResourceSize.y / gResourceSize.x, 0.5 );
        float2 dimInPixels = gResourceSize * VIEWPORT_SIZE * dim;

        float2 remappedUv = ( viewportUv - ( 1.0 - dim ) ) / dim;
        float2 remappedUv2 = ( viewportUv - float2( 1.0 - dim.x, 0.0 ) ) / dim;

        if( all( remappedUv > 0.0 ) )
        {
            // Jitter
            float2 uv = gJitter + 0.5;
            bool isValid = all( saturate( uv ) == uv );
            int2 a = int2( saturate( uv ) * dimInPixels );
            int2 b = int2( remappedUv * dimInPixels );

            if( all( abs( a - b ) <= 1 ) && isValid )
                result.xyz = 0.66;

            if( all( abs( a - b ) <= 3 ) && !isValid )
                result.xyz = float3( 1.0, 0.0, 0.0 );
        }
        else if( all( remappedUv2 > 0.0 ) )
        {
            // Rotators
            float scale = 0.5;
            //scale *= sqrt( 1.0 / ( 1.0 + ( gFrameIndex % 16 ) ) );
            scale *= float( Math::ReverseBits4( gFrameIndex ) ) / 16.0;

            float4 rotatorPre = gRotatorPre;
            float4 rotator = gRotator;
            float4 rotatorPost = gRotatorPost;

            int2 b = int2( remappedUv2 * dimInPixels );

            [unroll]
            for( uint n = 0; n < 8; n++ )
            {
                float3 offset = g_Special8[ n ];
                offset *= scale;

                {
                    float2 uv = 0.5 + Geometry::RotateVector( rotatorPre, offset.xy );
                    int2 a = int2( saturate( uv ) * dimInPixels );

                    result.x += all( abs( a - b ) <= 1 );
                }

                {
                    float2 uv = 0.5 + Geometry::RotateVector( rotator, offset.xy );
                    int2 a = int2( saturate( uv ) * dimInPixels );

                    result.y += all( abs( a - b ) <= 1 );
                }

                {
                    float2 uv = 0.5 + Geometry::RotateVector( rotatorPost, offset.xy );
                    int2 a = int2( saturate( uv ) * dimInPixels );

                    result.z += all( abs( a - b ) <= 1 );
                }
            }

            result = gFrameIndex % 256 == 0 ? 0 : saturate( result );
        }
        else
        {
            // Units
            float roundingErrorCorrection = abs( viewZ ) * 0.001;

            result.xyz = frac( X + roundingErrorCorrection ) * float( !isInf );
        }

        result.w = 1.0;
    }
    // Virtual history
    else if( viewportIndex == 7 && gHasSpecular )
    {
        Text::Print_ch( 'V', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'U', textState );
        Text::Print_ch( 'A', textState );
        Text::Print_ch( 'L', textState );
        Text::Print_ch( ' ', textState );
        Text::Print_ch( 'H', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'S', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'O', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'Y', textState );

        result.xyz = data2.x * float( !isInf );
        result.w = 1.0;
    }
    // Diffuse frames
    else if( viewportIndex == 8 && gHasDiffuse )
    {
        Text::Print_ch( 'D', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( ' ', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'A', textState );
        Text::Print_ch( 'M', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'S', textState );

        float f = 1.0 - saturate( data1.x / max( gMaxAccumulatedFrameNum, 1.0 ) );
        f = checkerboard && data1.x < 1.0 ? 0.75 : f;

        result.xyz = Color::ColorizeZucconi( viewportUv.y > 0.95 ? 1.0 - viewportUv.x : f * float( !isInf ) );
        result.w = 1.0;
    }
    // Specular frames
    else if( viewportIndex == 11 && gHasSpecular )
    {
        Text::Print_ch( 'S', textState );
        Text::Print_ch( 'P', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'C', textState );
        Text::Print_ch( ' ', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( 'R', textState );
        Text::Print_ch( 'A', textState );
        Text::Print_ch( 'M', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'S', textState );

        float f = 1.0 - saturate( data1.y / max( gMaxAccumulatedFrameNum, 1.0 ) );
        f = checkerboard && data1.y < 1.0 ? 0.75 : f;

        result.xyz = Color::ColorizeZucconi( viewportUv.y > 0.95 ? 1.0 - viewportUv.x : f * float( !isInf ) );
        result.w = 1.0;
    }
    // Diff hitT
    else if( viewportIndex == 12 && gHasDiffuse )
    {
        Text::Print_ch( 'D', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( 'F', textState );
        Text::Print_ch( ' ', textState );
        Text::Print_ch( 'H', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'T', textState );

        if( diff.w == 0.0 )
            result.xyz = float3( 1, 0, 0 );
        else
            result.xyz = diff.w != saturate( diff.w ) ? float3( 1, 0, 1 ) : diff.w;

        result.xyz *= float( !isInf );
        result.w = 1.0;
    }
    // Spec hitT
    else if( viewportIndex == 15 && gHasSpecular )
    {
        Text::Print_ch( 'S', textState );
        Text::Print_ch( 'P', textState );
        Text::Print_ch( 'E', textState );
        Text::Print_ch( 'C', textState );
        Text::Print_ch( ' ', textState );
        Text::Print_ch( 'H', textState );
        Text::Print_ch( 'I', textState );
        Text::Print_ch( 'T', textState );
        Text::Print_ch( 'T', textState );

        if( spec.w == 0.0 )
            result.xyz = float3( 1, 0, 0 );
        else
            result.xyz = spec.w != saturate( spec.w ) ? float3( 1, 0, 1 ) : spec.w;

        result.xyz *= float( !isInf );
        result.w = 1.0;
    }

    // Text
    if( Text::IsForeground( textState ) )
    {
        float lum = Color::Luminance( result.xyz );
        result.xyz = lerp( 0.0, 1.0 - result.xyz, saturate( abs( lum - 0.5 ) / 0.25 ) ) ;

        if( viewportIndex == 12 || viewportIndex == 15 )
            result.xyz = 0.5;
    }

    // Output
    gOut_Validation[ pixelPos ] = result;
}
