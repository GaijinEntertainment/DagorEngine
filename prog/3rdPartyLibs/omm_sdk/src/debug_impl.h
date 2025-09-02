/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "omm.h"
#include "std_allocator.h"
#include "log.h"

namespace omm
{
    OMM_API ommResult SaveAsImagesImpl(StdAllocator<uint8_t>& memoryAllocator, const ommCpuBakeInputDesc& bakeInputDesc, const ommCpuBakeResultDesc* res, const ommDebugSaveImagesDesc& desc);

    OMM_API ommResult GetStatsImpl(StdAllocator<uint8_t>& memoryAllocator, const ommCpuBakeResultDesc* res, ommDebugStats* out);

    OMM_API ommResult SaveBinaryToDiskImpl(const Logger& log, const ommCpuBlobDesc& data, const char* path);
}
