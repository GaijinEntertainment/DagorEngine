/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( Clear_FloatConstants )
    NRD_CONSTANT( float, gDebug ) // only for availability in Common.hlsl
NRD_CONSTANTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<float4>, gOut, u, 0 )
NRD_OUTPUTS_END

// Macro magic
#define Clear_FloatGroupX 16
#define Clear_FloatGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X Clear_FloatGroupX
#define GROUP_Y Clear_FloatGroupY
