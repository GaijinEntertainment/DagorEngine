/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN( uint2 pixelPos : SV_DispatchThreadId )
{
    // Tile-based early out
    float isSky = gIn_Tiles[ pixelPos >> 4 ];
    if( isSky != 0.0 && !gIsRectChanged )
        return;

    // TODO: is it possible to introduce "CopyResource" in NRD API?
    #ifdef REBLUR_DIFFUSE
        gOut_Diff[ pixelPos ] = gIn_Diff[ pixelPos ];
        #ifdef REBLUR_SH
            gOut_DiffSh[ pixelPos ] = gIn_DiffSh[ pixelPos ];
        #endif
    #endif

    #ifdef REBLUR_SPECULAR
        gOut_Spec[ pixelPos ] = gIn_Spec[ pixelPos ];
        #ifdef REBLUR_SH
            gOut_SpecSh[ pixelPos ] = gIn_SpecSh[ pixelPos ];
        #endif
    #endif
}
