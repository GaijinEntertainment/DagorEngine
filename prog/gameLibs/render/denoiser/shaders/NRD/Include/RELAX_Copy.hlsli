/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

[numthreads( GROUP_X, GROUP_Y, 1 )]
NRD_EXPORT void NRD_CS_MAIN(uint2 pixelPos : SV_DispatchThreadId)
{
#ifdef RELAX_SPECULAR
    gOutSpecularIllumination[pixelPos.xy] = gSpecIllumination[pixelPos.xy];
#endif

#ifdef RELAX_DIFFUSE
    gOutDiffuseIllumination[pixelPos.xy] = gDiffIllumination[pixelPos.xy];
#endif
}
