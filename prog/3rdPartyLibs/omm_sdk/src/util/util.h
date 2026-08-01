/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "math.h"
#include <omm.h>
#include "assert.h"

namespace omm
{
    static bool IsUnknown(ommOpacityState state) {
        return state == ommOpacityState_UnknownOpaque || state == ommOpacityState_UnknownTransparent;
    };

    static bool IsKnown(ommOpacityState state) {
        return state == ommOpacityState_Opaque || state == ommOpacityState_Transparent;
    };

    static bool IsCompatible(ommOpacityState state, ommFormat format) {
        if (format == ommFormat_OC1_2_State)
        {
            return state == ommOpacityState_Opaque || state == ommOpacityState_Transparent;
        }
        OMM_ASSERT(format == ommFormat_OC1_4_State);
        return true;
    };

    static ommOpacityState GetUnknownVersionOf(ommOpacityState state)
    {
        return (ommOpacityState)(state | 2u);
    }

    static const char* GetOpacityStateAsString(ommOpacityState state)
    {
        switch (state)
        {
        case ommOpacityState_Transparent: return "Transparent";
        case ommOpacityState_Opaque: return "Opaque";
        case ommOpacityState_UnknownTransparent: return "UnknownTransparent";
        case ommOpacityState_UnknownOpaque: return "UnknownOpaque";
        default:
        {
            OMM_ASSERT(false);
            return "Unknown";
        }
        }
    }

    static const char* GetFormatAsString(ommFormat format)
    {
        switch (format)
        {
        case ommFormat_OC1_2_State: return "OC1_2_State";
        case ommFormat_OC1_4_State: return "OC1_4_State";
        default:
        {
            OMM_ASSERT(false);
            return "Unknown";
        }
        }
    }
} // namespace omm