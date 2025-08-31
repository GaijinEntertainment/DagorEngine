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
    NRD_CTA_ORDER_REVERSED;

    // TODO: introduce "CopyResource" in NRD API?
#ifdef RELAX_SPECULAR
    gOut_Spec[pixelPos.xy] = gIn_Spec[pixelPos.xy];
#endif

#ifdef RELAX_DIFFUSE
    gOut_Diff[pixelPos.xy] = gIn_Diff[pixelPos.xy];
#endif
}
