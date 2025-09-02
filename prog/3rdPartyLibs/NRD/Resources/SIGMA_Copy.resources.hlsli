/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

NRD_CONSTANTS_START( SIGMA_CopyConstants )
    SIGMA_SHARED_CONSTANTS
NRD_CONSTANTS_END

NRD_INPUTS_START
    NRD_INPUT( Texture2D<float2>, gIn_Tiles, t, 0 )
    NRD_INPUT( Texture2D<float4>, gIn_History, t, 1 )
    NRD_INPUT( Texture2D<uint>, gIn_HistoryLength, t, 2 )
NRD_INPUTS_END

NRD_OUTPUTS_START
    NRD_OUTPUT( RWTexture2D<float4>, gOut_History, u, 0 )
    NRD_OUTPUT( RWTexture2D<uint>, gOut_HistoryLength, u, 1 )
NRD_OUTPUTS_END

// Macro magic
#define SIGMA_CopyGroupX 8
#define SIGMA_CopyGroupY 16

// Redirection
#undef GROUP_X
#undef GROUP_Y
#define GROUP_X SIGMA_CopyGroupX
#define GROUP_Y SIGMA_CopyGroupY
