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
#include "omm_handle.h"
#include "defines.h"
#include "std_containers.h"
#include "texture_impl.h"
#include "log.h"

#include <shared/math.h>
#include <shared/texture.h>

#include <map>
#include <set>

#include "std_allocator.h"

namespace omm
{
namespace Cpu
{
    class BakerImpl
    {
    // Internal
    public:

        static inline constexpr HandleType kHandleType = HandleType::CpuBaker;
        
        inline BakerImpl(const StdAllocator<uint8_t>& stdAllocator) :
            m_stdAllocator(stdAllocator)
        {}

        ~BakerImpl();

        inline StdAllocator<uint8_t>& GetStdAllocator()
        { return m_stdAllocator; }

        inline const Logger& GetLog() const
        { return m_log; }

        ommResult Create(const ommBakerCreationDesc& bakeCreationDesc);
        ommResult BakeOpacityMicromap(const ommCpuBakeInputDesc& bakeInputDesc, ommCpuBakeResult* bakeOutput);

    private:
        ommResult Validate(const ommCpuBakeInputDesc& desc);
    private:
        StdAllocator<uint8_t> m_stdAllocator;
        Logger m_log;
    };

    struct BakeResultImpl
    {
        vector<int32_t> ommIndexBuffer;
        vector<ommCpuOpacityMicromapDesc> ommDescArray;
        vector<uint8_t> ommArrayData;
        vector<ommCpuOpacityMicromapUsageCount> ommArrayHistogram;
        vector<ommCpuOpacityMicromapUsageCount> ommIndexHistogram;
        ommCpuBakeResultDesc bakeOutputDesc = {0,};

        BakeResultImpl(const StdAllocator<uint8_t>& stdAllocator) :
            ommIndexBuffer(stdAllocator),
            ommDescArray(stdAllocator),
            ommArrayData(stdAllocator),
            ommArrayHistogram(stdAllocator),
            ommIndexHistogram(stdAllocator)
        {
        }

        void Finalize(ommIndexFormat ommIndexFormat)
        {
            bakeOutputDesc.arrayData                 = ommArrayData.data();
            bakeOutputDesc.arrayDataSize             = (uint32_t)ommArrayData.size();
            bakeOutputDesc.descArray                 = ommDescArray.data();
            bakeOutputDesc.descArrayCount            = (uint32_t)ommDescArray.size();
            bakeOutputDesc.descArrayHistogram        = ommArrayHistogram.data();
            bakeOutputDesc.descArrayHistogramCount   = (uint32_t)ommArrayHistogram.size();
            bakeOutputDesc.indexBuffer               = ommIndexBuffer.data();
            bakeOutputDesc.indexCount                = (uint32_t)ommIndexBuffer.size();
            bakeOutputDesc.indexFormat               = ommIndexFormat;
            bakeOutputDesc.indexHistogram            = ommIndexHistogram.data();
            bakeOutputDesc.indexHistogramCount       = (uint32_t)ommIndexHistogram.size();
        }
    };

    class BakeOutputImpl
    {
    public:
        BakeOutputImpl(const StdAllocator<uint8_t>& stdAllocator, const Logger& log);
        ~BakeOutputImpl();

        inline const StdAllocator<uint8_t>& GetStdAllocator() const
        {
            return m_stdAllocator;
        }

        inline const ommCpuBakeResultDesc& GetBakeOutputDesc() const
        {
            return m_bakeResult.bakeOutputDesc;
        }

        inline ommResult GetBakeResultDesc(const ommCpuBakeResultDesc** desc)
        {
            if (desc == nullptr)
                return m_log.InvalidArg("[Invalid Arg] - No BakeResultDesc provided");

            *desc = &m_bakeResult.bakeOutputDesc;
            return ommResult_SUCCESS;
        }

        ommResult Bake(const ommCpuBakeInputDesc& desc);

    private:
        ommResult ValidateDesc(const ommCpuBakeInputDesc& desc) const;

        template<ommCpuTextureFormat format, TilingMode eTextureFormat, ommTextureAddressMode eTextureAddressMode, ommTextureFilterMode eFilterMode>
        ommResult BakeImpl(const ommCpuBakeInputDesc& desc);

        template<class... TArgs>
        void RegisterDispatch(TArgs... args, std::function < ommResult(const ommCpuBakeInputDesc& desc)> fn);
        map<std::tuple<ommCpuTextureFormat, TilingMode, ommTextureAddressMode, ommTextureFilterMode>, std::function<ommResult(const ommCpuBakeInputDesc& desc)>> bakeDispatchTable;
        ommResult InvokeDispatch(const ommCpuBakeInputDesc& desc);
    private:
        StdAllocator<uint8_t> m_stdAllocator;
        const Logger& m_log;
        ommCpuBakeInputDesc m_bakeInputDesc;
        BakeResultImpl m_bakeResult;
    };
} // namespace Cpu
} // namespace omm
