/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef OMM_HLSLI
#define OMM_HLSLI

enum class OMMFormat : uint
{
	OC1_2 = 1,
	OC1_4 = 2,
};

enum class TextureFilterMode : uint {
	Nearest,
	Linear,

	MAX_NUM
};

enum class OpacityState : uint {
	Transparent         = 0,
	Opaque              = 1,
	UnknownTransparent  = 2,
	UnknownOpaque       = 3,
};

enum class SpecialIndex : int {
	FullyTransparent         = -1,
	FullyOpaque              = -2,
	FullyUnknownTransparent  = -3,
	FullyUnknownOpaque       = -4,
};

enum class TexCoordFormat : int {
   UV16_UNORM,
   UV16_FLOAT,
   UV32_FLOAT,
	
   MAX_NUM
};

bool IsKnown(OpacityState state)
{
    return ((uint) state & 1u) == (uint) state;
}

bool IsUnknown(OpacityState state)
{
    return !IsKnown(state);
}

OpacityState GetUnknownVersionOf(OpacityState state)
{
    return (OpacityState)((uint)state | 2u);
}

OpacityState _getOpacityStateInternal(uint numAboveAlpha, uint numBelowAlpha, OpacityState alphaCutoffGT, OpacityState alphaCutoffLE)
{
    if (numAboveAlpha == 0)
	{
        return alphaCutoffLE;
    }
    else if (numBelowAlpha == 0)
	{
        return alphaCutoffGT;
    }
    else if (numBelowAlpha > numAboveAlpha)
	{
        return GetUnknownVersionOf(alphaCutoffLE);
    }
    else // if (numBelowAlpha <= numAboveAlpha)
	{
        return GetUnknownVersionOf(alphaCutoffGT);
    }
}

OpacityState GetOpacityState(uint numAboveAlpha, uint numBelowAlpha, uint /*OpacityState*/ alphaCutoffGT, uint /*OpacityState*/ alphaCutoffLE, OMMFormat ommFormat)
{
    OpacityState opacityState = _getOpacityStateInternal(numAboveAlpha, numBelowAlpha, (OpacityState) alphaCutoffGT, (OpacityState) alphaCutoffLE);
	if (ommFormat == OMMFormat::OC1_2)
		return (OpacityState)((uint)opacityState & 1u);
	return opacityState;
}

float3 GetDebugColorForState(OpacityState state)
{
	switch (state) {
		case OpacityState::Transparent:			return float3(0, 0, 1);
		case OpacityState::Opaque:				return float3(0, 1, 0);
		case OpacityState::UnknownTransparent:	return float3(1, 0, 1);
		case OpacityState::UnknownOpaque:		return float3(1, 1, 0);
		// undefined
		default: return 0;
	}
}
#endif