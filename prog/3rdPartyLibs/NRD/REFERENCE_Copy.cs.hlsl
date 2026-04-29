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

#include "REFERENCE_Copy.resources.hlsli"

#include "Common.hlsli"

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( NRD_CS_MAIN_ARGS )
{
    NRD_CTA_ORDER_DEFAULT;

    float2 pixelUv = float2( pixelPos + 0.5 ) * gRectSizeInv;

    if( pixelUv.x > gSplitScreen )
        gOut_Output[ pixelPos ] = gIn_Input[ pixelPos ];
}
