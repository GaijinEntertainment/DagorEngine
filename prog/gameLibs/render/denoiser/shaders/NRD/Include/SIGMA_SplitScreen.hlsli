/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

[numthreads( GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN( int2 pixelPos : SV_DispatchThreadId)
{
    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;
    if( pixelUv.x > gSplitScreen || any( pixelPos > gRectSizeMinusOne ) )
        return;

    float2 data = gIn_Hit_ViewZ[ pixelPos ];
    float viewZ = abs( data.y ) / NRD_FP16_VIEWZ_SCALE;

    SIGMA_TYPE s;
    #ifdef SIGMA_TRANSLUCENT
        s = gIn_Shadow_Translucency[ pixelPos ];
    #else
        s = float( data.x == NRD_FP16_MAX );
    #endif

    gOut_Shadow_Translucency[ pixelPos ] = s * float( viewZ < gDenoisingRange );
}
