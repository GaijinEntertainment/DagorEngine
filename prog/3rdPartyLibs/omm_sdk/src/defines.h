/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#define RETURN_STATUS_IF_FAILED(expr)           \
do {                                            \
    ommResult sts##__COUNTER__ = expr;          \
    if (sts##__COUNTER__ != ommResult_SUCCESS)  \
        return sts##__COUNTER__;                \
} while(false)

#include<stdint.h>

namespace omm
{
    ///< Defined by DX/VK spec.
    static constexpr uint32_t kMaxSubdivLevel         = 12;
    ///< Useful for statically allocated arrays.
    static constexpr uint32_t kMaxNumSubdivLevels     = kMaxSubdivLevel + 1;
}