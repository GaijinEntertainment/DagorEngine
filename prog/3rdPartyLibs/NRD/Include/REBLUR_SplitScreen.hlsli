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

    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    if( pixelUv.x > gSplitScreen || any( pixelPos > gRectSizeMinusOne ) )
        return;

    float viewZ = UnpackViewZ( gIn_ViewZ[ WithRectOrigin( pixelPos ) ] );
    uint2 checkerboardPos = pixelPos;

    #ifdef REBLUR_DIFFUSE
        checkerboardPos.x = pixelPos.x >> ( gDiffCheckerboard != 2 ? 1 : 0 );

        float4 diff = gIn_Diff[ checkerboardPos ];
        gOut_Diff[ pixelPos ] = diff * float( viewZ < gDenoisingRange );

        #ifdef REBLUR_SH
            float4 diffSh = gIn_DiffSh[ checkerboardPos ];
            gOut_DiffSh[ pixelPos ] = diffSh * float( viewZ < gDenoisingRange );
        #endif
    #endif

    #ifdef REBLUR_SPECULAR
        checkerboardPos.x = pixelPos.x >> ( gSpecCheckerboard != 2 ? 1 : 0 );

        float4 spec = gIn_Spec[ checkerboardPos ];
        gOut_Spec[ pixelPos ] = spec * float( viewZ < gDenoisingRange );

        #ifdef REBLUR_SH
            float4 specSh = gIn_SpecSh[ checkerboardPos ];
            gOut_SpecSh[ pixelPos ] = specSh * float( viewZ < gDenoisingRange );
        #endif
    #endif
}
