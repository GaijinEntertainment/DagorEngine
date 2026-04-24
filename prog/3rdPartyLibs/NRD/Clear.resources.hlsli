/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( Clear_Constants )
    NRD_CONSTANT( float, gDebug ) // only for availability in Common.hlsl
    NRD_CONSTANT( float, gViewZScale ) // only for availability in Common.hlsl
NRD_CONSTANTS_END

NRD_OUTPUTS_START
    #if( FLOAT == 1 )
        NRD_OUTPUT( RWTexture2D, float4, gOut, u, 0 )
    #else
        NRD_OUTPUT( RWTexture2D, uint4, gOut, u, 0 )
    #endif
NRD_OUTPUTS_END

// Macro magic
#define ClearGroupX 16
#define ClearGroupY 16

// Shader only
#ifndef __cplusplus

#define GROUP_X ClearGroupX
#define GROUP_Y ClearGroupY

#endif
