/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "defines.h"
#include "bake_cpu_impl.h"
#include "bake_kernels_cpu.h"
#include "texture_impl.h"

#include <shared/math.h>
#include <shared/bird.h>
#include <shared/cpu_raster.h>

#include <xxhash.h>

#include <random>
#include <array>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

namespace omm
{
namespace Cpu
{
    enum class BakeFlagsInternal
    {
        None                            = 0,
        EnableInternalThreads           = 1u << 0,
        DisableSpecialIndices           = 1u << 1,
        Force32BitIndices               = 1u << 2,
        DisableDuplicateDetection       = 1u << 3,
        EnableNearDuplicateDetection    = 1u << 4,
        EnableValidation                = 1u << 5,

        // Internal / not publicly exposed options.
        EnableAABBTesting               = 1u << 6,
        DisableRemovePoorQualityOMM     = 1u << 7,
        DisableLevelLineIntersection    = 1u << 8,
        DisableFineClassification       = 1u << 9,
        EnableNearDuplicateDetectionBruteForce = 1u << 10,
        EnableEdgeHeuristic             = 1u << 11,
    };

    constexpr void ValidateInternalBakeFlags()
    {
        static_assert((uint32_t)BakeFlagsInternal::None == (uint32_t)ommCpuBakeFlags_None);
        static_assert((uint32_t)BakeFlagsInternal::EnableInternalThreads == (uint32_t)ommCpuBakeFlags_EnableInternalThreads);
        static_assert((uint32_t)BakeFlagsInternal::DisableSpecialIndices == (uint32_t)ommCpuBakeFlags_DisableSpecialIndices);
        static_assert((uint32_t)BakeFlagsInternal::DisableDuplicateDetection == (uint32_t)ommCpuBakeFlags_DisableDuplicateDetection);
        static_assert((uint32_t)BakeFlagsInternal::EnableNearDuplicateDetection == (uint32_t)ommCpuBakeFlags_EnableNearDuplicateDetection);
        static_assert((uint32_t)BakeFlagsInternal::EnableValidation == (uint32_t)ommCpuBakeFlags_EnableValidation);
    }

    struct Options
    {
        Options(ommCpuBakeFlags flags) :
            enableInternalThreads(((uint32_t)flags& (uint32_t)BakeFlagsInternal::EnableInternalThreads) == (uint32_t)BakeFlagsInternal::EnableInternalThreads),
            disableSpecialIndices(((uint32_t)flags& (uint32_t)BakeFlagsInternal::DisableSpecialIndices) == (uint32_t)BakeFlagsInternal::DisableSpecialIndices),
            disableDuplicateDetection(((uint32_t)flags& (uint32_t)BakeFlagsInternal::DisableDuplicateDetection) == (uint32_t)BakeFlagsInternal::DisableDuplicateDetection),
            enableNearDuplicateDetection(((uint32_t)flags& (uint32_t)BakeFlagsInternal::EnableNearDuplicateDetection) == (uint32_t)BakeFlagsInternal::EnableNearDuplicateDetection),
            enableNearDuplicateDetectionBruteForce(((uint32_t)flags& (uint32_t)BakeFlagsInternal::EnableNearDuplicateDetectionBruteForce) == (uint32_t)BakeFlagsInternal::EnableNearDuplicateDetectionBruteForce),
            enableValidation(((uint32_t)flags& (uint32_t)BakeFlagsInternal::EnableValidation) == (uint32_t)BakeFlagsInternal::EnableValidation),
            enableAABBTesting(((uint32_t)flags& (uint32_t)BakeFlagsInternal::EnableAABBTesting) == (uint32_t)BakeFlagsInternal::EnableAABBTesting),
            disableRemovePoorQualityOMM(((uint32_t)flags& (uint32_t)BakeFlagsInternal::DisableRemovePoorQualityOMM) == (uint32_t)BakeFlagsInternal::DisableRemovePoorQualityOMM),
            disableLevelLineIntersection(((uint32_t)flags& (uint32_t)BakeFlagsInternal::DisableLevelLineIntersection) == (uint32_t)BakeFlagsInternal::DisableLevelLineIntersection),
            disableFineClassification(((uint32_t)flags& (uint32_t)BakeFlagsInternal::DisableFineClassification) == (uint32_t)BakeFlagsInternal::DisableFineClassification),
            enableEdgeHeuristic(((uint32_t)flags& (uint32_t)BakeFlagsInternal::EnableEdgeHeuristic) == (uint32_t)BakeFlagsInternal::EnableEdgeHeuristic)
        { }
        const bool enableInternalThreads;
        const bool disableSpecialIndices;
        const bool disableDuplicateDetection;
        const bool enableNearDuplicateDetection;
        const bool enableNearDuplicateDetectionBruteForce;
        const bool enableValidation;
        const bool enableAABBTesting;
        const bool disableRemovePoorQualityOMM;
        const bool disableLevelLineIntersection;
        const bool disableFineClassification;
        const bool enableEdgeHeuristic;
    };

    BakerImpl::~BakerImpl()
    {}

    ommResult BakerImpl::Create(const ommBakerCreationDesc& desc)
    {
        m_log = Logger(desc.messageInterface);

        return ommResult_SUCCESS;
    }

    ommResult BakerImpl::Validate(const ommCpuBakeInputDesc& desc) {
        if (desc.texture == 0)
        {
            return m_log.InvalidArg("[Invalid Argument] - ommCpuBakeInputDesc has no texture set");
        }
        return ommResult_SUCCESS;
    }

    ommResult BakerImpl::BakeOpacityMicromap(const ommCpuBakeInputDesc& bakeInputDesc, ommCpuBakeResult* outBakeommResult)
    {
        RETURN_STATUS_IF_FAILED(Validate(bakeInputDesc));
        BakeOutputImpl* implementation = Allocate<BakeOutputImpl>(m_stdAllocator, m_stdAllocator, m_log);
        ommResult result = implementation->Bake(bakeInputDesc);

        if (result == ommResult_SUCCESS)
        {
            *outBakeommResult = (ommCpuBakeResult)implementation;
            return ommResult_SUCCESS;
        }

        Deallocate(m_stdAllocator, implementation);
        return result;
    }

    BakeOutputImpl::BakeOutputImpl(const StdAllocator<uint8_t>& stdAllocator, const Logger& log) :
        m_stdAllocator(stdAllocator),
        m_log(log),
        m_bakeInputDesc({}),
        m_bakeResult(stdAllocator),
        bakeDispatchTable(stdAllocator.GetInterface())
    {
        #define REGISTER_DISPATCH(x, y, z, w)                                                                                               \
        RegisterDispatch<decltype(x), decltype(y), decltype(z), decltype(w)>(x, y, z, w, [&](const ommCpuBakeInputDesc& desc)->ommResult {  \
            return BakeImpl<x, y, z, w>(desc);                                                                                              \
        });                                                                                                                                 \

        REGISTER_DISPATCH(ommCpuTextureFormat_FP32, TilingMode::Linear, ommTextureAddressMode_Wrap, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32, TilingMode::Linear, ommTextureAddressMode_Mirror, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32, TilingMode::Linear, ommTextureAddressMode_Clamp, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32, TilingMode::Linear, ommTextureAddressMode_Border, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32, TilingMode::Linear, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Linear);

        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Wrap, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Mirror, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Clamp, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Border, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Linear);

        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::Linear, ommTextureAddressMode_Wrap, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::Linear, ommTextureAddressMode_Mirror, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::Linear, ommTextureAddressMode_Clamp, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::Linear, ommTextureAddressMode_Border, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::Linear, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Nearest);

        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Wrap, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Mirror, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Clamp, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_Border, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_FP32,TilingMode::MortonZ, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Nearest);

        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8, TilingMode::Linear, ommTextureAddressMode_Wrap, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8, TilingMode::Linear, ommTextureAddressMode_Mirror, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8, TilingMode::Linear, ommTextureAddressMode_Clamp, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8, TilingMode::Linear, ommTextureAddressMode_Border, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8, TilingMode::Linear, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Linear);

        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Wrap, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Mirror, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Clamp, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Border, ommTextureFilterMode_Linear);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Linear);

        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::Linear, ommTextureAddressMode_Wrap, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::Linear, ommTextureAddressMode_Mirror, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::Linear, ommTextureAddressMode_Clamp, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::Linear, ommTextureAddressMode_Border, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::Linear, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Nearest);

        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Wrap, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Mirror, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Clamp, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_Border, ommTextureFilterMode_Nearest);
        REGISTER_DISPATCH(ommCpuTextureFormat_UNORM8,TilingMode::MortonZ, ommTextureAddressMode_MirrorOnce, ommTextureFilterMode_Nearest);
    }

    BakeOutputImpl::~BakeOutputImpl()
    {
    }

    ommResult BakeOutputImpl::ValidateDesc(const ommCpuBakeInputDesc& desc) const {
        const Options options(desc.bakeFlags);

        if (desc.texture == 0)
            return m_log.InvalidArg("[Invalid Argument] - texture is not set");
        if (!GetCheckedHandleImpl<TextureImpl>(desc.texture))
            return m_log.InvalidArg("[Invalid Argument] - desc.texture is of incorrect type");
        if (desc.alphaMode == ommAlphaMode_MAX_NUM)
            return m_log.InvalidArg("[Invalid Argument] - alphaMode is not set");
        if (desc.runtimeSamplerDesc.addressingMode == ommTextureAddressMode_MAX_NUM)
            return m_log.InvalidArg("[Invalid Argument] - runtimeSamplerDesc.addressingMode is not set");
        if (desc.runtimeSamplerDesc.filter == ommTextureFilterMode_MAX_NUM)
            return m_log.InvalidArg("[Invalid Argument] - runtimeSamplerDesc.filter is not set");
        if (desc.texCoordFormat == ommTexCoordFormat_MAX_NUM)
            return m_log.InvalidArg("[Invalid Argument] - texCoordFormat is not set");
        if (desc.texCoords == nullptr)
            return m_log.InvalidArg("[Invalid Argument] - texCoords is not set");
        if (desc.indexFormat == ommIndexFormat_MAX_NUM)
            return m_log.InvalidArg("[Invalid Argument] - indexFormat is not set");
        if (desc.indexBuffer == nullptr)
            return m_log.InvalidArg("[Invalid Argument] - indexBuffer is not set");
        if (desc.indexCount == 0)
            return m_log.InvalidArg("[Invalid Argument] - indexCount is not set");
        if (desc.maxSubdivisionLevel > kMaxSubdivLevel)
        {
            static_assert(kMaxSubdivLevel == 12, "");
            if (desc.maxSubdivisionLevel > kMaxSubdivLevel)
                return m_log.InvalidArgf("[Invalid Argument] - maxSubdivisionLevel (%d) is greater than maximum supported (%d)", desc.maxSubdivisionLevel, kMaxSubdivLevel);
        }
        if ((options.enableNearDuplicateDetection || options.enableNearDuplicateDetectionBruteForce) && options.disableDuplicateDetection)
        {
            return m_log.InvalidArg("[Invalid Argument] - EnableNearDuplicateDetection or EnableNearDuplicateDetectionBruteForce is used together with DisableDuplicateDetection");
        }
        if (options.enableValidation && !m_log.HasLogger())
            return m_log.InvalidArg("[Invalid Argument] - EnableValidation is set but no message callback was provided"); // this works more as documentation since it won't be logged

        if (TextureImpl* texture = GetHandleImpl<TextureImpl>(desc.texture))
        {
            if (texture->HasAlphaCutoff() && texture->GetAlphaCutoff() != desc.alphaCutoff)
            {
                return m_log.InvalidArgf("[Invalid Argument] - Texture object alpha cutoff threshold (%.6f) is different from alpha cutoff threshold in bake input (%.6f)", texture->GetAlphaCutoff(), desc.alphaCutoff);
            }
        }

        if (!IsCompatible(desc.alphaCutoffGreater, desc.format))
        {
            return m_log.InvalidArgf("[Invalid Argument] - alphaCutoffGreater=%s is not compatible with %s", GetOpacityStateAsString(desc.alphaCutoffGreater), GetFormatAsString(desc.format));
        }

        if (!IsCompatible(desc.alphaCutoffLessEqual, desc.format))
        {
            return m_log.InvalidArgf("[Invalid Argument] - alphaCutoffLessEqual=%s is not compatible with %s", GetOpacityStateAsString(desc.alphaCutoffLessEqual), GetFormatAsString(desc.format));
        }

        return ommResult_SUCCESS;
    }

    template<class... TArgs>
    void BakeOutputImpl::RegisterDispatch(TArgs... args, std::function < ommResult(const ommCpuBakeInputDesc& desc)> fn) {
        bakeDispatchTable[std::make_tuple(args...)] = fn;
    }

    ommResult BakeOutputImpl::InvokeDispatch(const ommCpuBakeInputDesc& desc) {
        TextureImpl* texture = GetHandleImpl<TextureImpl>(desc.texture);
        auto it = bakeDispatchTable.find(std::make_tuple(texture->GetTextureFormat(), texture->GetTilingMode(), desc.runtimeSamplerDesc.addressingMode, desc.runtimeSamplerDesc.filter));
        if (it == bakeDispatchTable.end())
            return ommResult_FAILURE;
        return it->second(desc);
    }

    ommResult BakeOutputImpl::Bake(const ommCpuBakeInputDesc& desc)
    {
        return InvokeDispatch(desc);
    }

    static constexpr uint32_t kCacheLineSize = 128;
    template<class T>
    struct alignas(kCacheLineSize) TAligned : public T {
        using T::T;
        uint8_t _padding[kCacheLineSize - sizeof(T)];
        void Validate() {
            static_assert(sizeof(TAligned) == kCacheLineSize);
        }
    };

    using Atomic32Aligned = TAligned<std::atomic<uint32_t>>;

    struct VisibilityMapUsageHistogram
    {
    private:
        Atomic32Aligned visibilityMapUsageStats[(uint16_t)ommFormat_MAX_NUM][kMaxNumSubdivLevels] = { 0, };

        static uint16_t _GetOmmIndex(ommFormat format) {
            OMM_ASSERT(format != ommFormat_INVALID);
            static_assert((uint16_t)ommFormat_MAX_NUM == 3);
            static_assert((uint16_t)ommFormat_OC1_2_State == 1);
            static_assert((uint16_t)ommFormat_OC1_4_State == 2);
            return ((uint16_t)format) - 1;
        }
    public:
        void Inc(ommFormat format, uint32_t subDivLvl, uint32_t count) {
            OMM_ASSERT(subDivLvl < kMaxNumSubdivLevels);
            visibilityMapUsageStats[_GetOmmIndex(format)][subDivLvl] += count;
        }

        uint32_t GetOmmCount(ommFormat format, uint32_t subDivLvl) const {
            OMM_ASSERT(subDivLvl < kMaxNumSubdivLevels);
            return visibilityMapUsageStats[_GetOmmIndex(format)][subDivLvl];
        }
    };

    class OmmArrayDataView
    {
        static void SetStateInternal(uint8_t* targetBuffer, uint32_t index, ommOpacityState state) {
            targetBuffer[index] = (uint8_t)state;
        }

        static ommOpacityState GetStateInternal(const uint8_t* targetBuffer, uint32_t index) {
            return (ommOpacityState)targetBuffer[index];
        }

    public:
        OmmArrayDataView() = delete;
        OmmArrayDataView(ommFormat format, uint8_t* data, uint8_t* data3state, size_t ommArrayDataSize)
            : _is2State(format == ommFormat_OC1_2_State),
             _ommArrayData4or2state(data),
             _ommArrayData3state(data3state),
             _ommArrayDataSize(ommArrayDataSize) 
        {
            OMM_ASSERT(format == ommFormat_OC1_2_State || format == ommFormat_OC1_4_State);
        }

        void SetData(uint8_t* data, uint8_t* data3state, size_t size) {
            _ommArrayData4or2state = data;
            _ommArrayData3state = data3state;
            _ommArrayDataSize = size;
        }

        void SetState(uint32_t index, ommOpacityState state) {
            SetStateInternal(_ommArrayData4or2state, index, state);
            SetStateInternal(_ommArrayData3state, index, state == ommOpacityState_UnknownTransparent ? ommOpacityState_UnknownOpaque : state);
        }

        ommOpacityState GetState(uint32_t index) const {
            return GetStateInternal(_ommArrayData4or2state, index);
        }

        ommOpacityState Get3State(uint32_t index) const {
            return GetStateInternal(_ommArrayData3state, index);
        }

        uint8_t* GetOmm3StateData() const { return _ommArrayData3state; }
        size_t GetOmm3StateDataSize() const { return _ommArrayDataSize;  }

    private:
        bool _is2State;
        uint8_t* _ommArrayData4or2state;
        uint8_t* _ommArrayData3state;
        size_t _ommArrayDataSize;
    };

    class OmmArrayDataVector final : public OmmArrayDataView
    {
    public:
        OmmArrayDataVector() = delete;
        OmmArrayDataVector(const StdAllocator<uint8_t>& stdAllocator, ommFormat format, uint32_t _subdivisionLevel)
            : OmmArrayDataView(format, nullptr, nullptr, 0)
            , data(stdAllocator.GetInterface())
            , data3state(stdAllocator.GetInterface())
        {
            const size_t maxSizeInBytes = (size_t)omm::bird::GetNumMicroTriangles(_subdivisionLevel);
            data.resize(maxSizeInBytes);
            data3state.resize(maxSizeInBytes);
            OmmArrayDataView::SetData((uint8_t*)data.data(), data3state.data(), maxSizeInBytes);
            Init();
        }

    private:

        void Init()
        {
            std::fill(data.begin(), data.end(), ommOpacityState_UnknownOpaque);
            std::fill(data3state.begin(), data3state.end(), ommOpacityState_UnknownOpaque);
        }

    private:
        vector<uint8_t> data;
        vector<uint8_t> data3state;
    };

    struct OmmWorkItem {
        uint32_t subdivisionLevel;
        ommFormat vmFormat;
        Triangle uvTri;
        vector<uint32_t> primitiveIndices; // source primitive and identical indices

        OmmWorkItem() = delete;

        OmmWorkItem(const StdAllocator<uint8_t>& stdAllocator, ommFormat _vmFormat, uint32_t _subdivisionLevel, uint32_t primitiveIndex, const Triangle& _uvTri)
            : primitiveIndices(stdAllocator)
            , subdivisionLevel(_subdivisionLevel)
            , vmFormat(_vmFormat)
            , uvTri(_uvTri)
            , vmStates(stdAllocator, _vmFormat, _subdivisionLevel)
        {
            primitiveIndices.push_back(primitiveIndex);
        }

        bool HasSpecialIndex() const { return vmSpecialIndex != kNoSpecialIndex; }

        static constexpr uint16_t kNoSpecialIndex = 0;

        // Outputs.
        uint32_t vmDescOffset = 0xFFFFFFFF;
        uint32_t vmSpecialIndex = kNoSpecialIndex;
        OmmArrayDataVector vmStates;
    };

    static float GetArea2D(const float2& p0, const float2& p1, const float2& p2) {
        const float2 v0 = p2 - p0;
        const float2 v1 = p1 - p0;
        return 0.5f * length(cross(float3(v0, 0), float3(v1, 0)));
    };

    static float GetArea2D(const Triangle& uvTri) {
        return GetArea2D(uvTri.p0, uvTri.p1, uvTri.p2);
    };

    static const uint32_t ComputeAreaHeuristic(const ommCpuBakeInputDesc& desc, const Triangle& uvTri, uint2 texSize)
    {
        auto GetNextPow2 = [](uint v)->uint
        {
            v--;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            v++;
            return v;
        };

        auto GetLog2 = [](uint v)->uint{ // V must be power of 2.
            const unsigned int b[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                             0xFF00FF00, 0xFFFF0000 };
            unsigned int r = (v & b[0]) != 0;
            for (uint i = 4; i > 0; i--) // unroll for speed...
            {
                r |= ((v & b[i]) != 0) << i;
            }
            return r;
        };

        const float2 texSizef = float2(texSize);
        const float pixelUvArea = GetArea2D(uvTri.p0 * texSizef, uvTri.p1 * texSizef, uvTri.p2 * texSizef);

        // Solves the following eqn:
        // targetPixelArea / (4^N) = pixelUvArea 

        const float targetPixelArea = desc.dynamicSubdivisionScale * desc.dynamicSubdivisionScale;
        const uint ratio = uint(pixelUvArea / targetPixelArea);
        const uint ratioNextPow2 = GetNextPow2(ratio);
        const uint log2_ratio = GetLog2(ratioNextPow2);

        const uint SubdivisionLevel = log2_ratio >> 1u; // log2(ratio) / log2(4)

        return std::min<uint>(SubdivisionLevel, desc.maxSubdivisionLevel);
    }

    static const uint32_t ComputeEdgeHeuristic(const ommCpuBakeInputDesc& desc, const Triangle& uvTri, uint2 texSize)
    {
        // Adapted from 3.1.1 https://fileadmin.cs.lth.se/graphics/research/papers/2024/succinct_opacity_micromaps/paper-author-version.pdf
        const float2 ve0 = (float2)texSize * (uvTri.p1 - uvTri.p0);
        const float2 ve1 = (float2)texSize * (uvTri.p2 - uvTri.p0);
        const float2 ve2 = (float2)texSize * (uvTri.p2 - uvTri.p1);

        const float le0 = glm::dot(ve0, ve0);
        const float le1 = glm::dot(ve1, ve1);
        const float le2 = glm::dot(ve2, ve2);

        const float eMax = std::max({ le0, le1, le2 });

        const float n = eMax < 1e-6 ? 0 : std::log2(eMax) / 2.f - std::log2(desc.dynamicSubdivisionScale);

        const int SubdivisionLevel = (int)std::ceil(n);
        return std::clamp<int>(SubdivisionLevel, 0, desc.maxSubdivisionLevel);
    }

    static const uint32_t CalculateSuitableSubdivisionLevel(const ommCpuBakeInputDesc& desc, const Options& options, const Triangle& uvTri, uint2 texSize)
    {
        if (uvTri.GetIsDegenerate() || options.enableEdgeHeuristic)
        {
            return ComputeEdgeHeuristic(desc, uvTri, texSize);
        }
        else
        {
            return ComputeAreaHeuristic(desc, uvTri, texSize);
        }
    }

    static int32_t GetSubdivisionLevelForPrimitive(const ommCpuBakeInputDesc& desc, const Options& options, uint32_t i, const Triangle& uvTri, uint2 texSize)
    {
        if (desc.subdivisionLevels && desc.subdivisionLevels[i] <= 12)
        {
            // We have per-primitive setting.
            return desc.subdivisionLevels[i];
        }

        const bool enableDynamicSubdivisionLevel = desc.dynamicSubdivisionScale > 0;

        if (enableDynamicSubdivisionLevel)
        {
            return CalculateSuitableSubdivisionLevel(desc, options, uvTri, texSize);
        }
        else
        {
            return desc.maxSubdivisionLevel;
        }
    }

    static bool GetIsInvalid(const Options& options, const Triangle& uvTriangle)
    {
        if (uvTriangle.GetIsInvalid())
        {
            return true;
        }

        if (options.disableLevelLineIntersection && uvTriangle.GetIsDegenerate())
        {
            return true; // we only support degen triangles in level line intersection mode.
        }

        return false;
    }

    namespace impl
    {
        static ommResult SetupWorkItems(
            const StdAllocator<uint8_t>& allocator, const Logger& log, const ommCpuBakeInputDesc& desc, const Options& options, 
            vector<OmmWorkItem>& vmWorkItems)
        {
            const TextureImpl* texture = GetHandleImpl<TextureImpl>(desc.texture);

            const int32_t triangleCount = desc.indexCount / 3u;


            // 1. Reserve memory.
            hash_map<size_t, uint32_t> triangleIDToWorkItem(allocator.GetInterface());
            vmWorkItems.reserve(triangleCount);

            const int32_t kDisabledPrimitive = 0xE;

            // 2. Reduce uv.
            {
                const uint32_t texCoordStrideInBytes = desc.texCoordStrideInBytes == 0 ? GetTexCoordFormatSize(desc.texCoordFormat) : desc.texCoordStrideInBytes;

                uint32_t numDisabledTri = 0;

                for (int32_t i = 0; i < triangleCount; ++i)
                {
                    uint32_t triangleIndices[3];
                    GetUInt32Indices(desc.indexFormat, desc.indexBuffer, 3ull * i, triangleIndices);

                    const Triangle uvTri = FetchUVTriangle(desc.texCoords, texCoordStrideInBytes, desc.texCoordFormat, triangleIndices);

                    const int32_t subdivisionLevel = GetSubdivisionLevelForPrimitive(desc, options, i, uvTri, texture->GetSize(0 /*always based on mip 0*/));

                    const bool bIsDisabled = subdivisionLevel == kDisabledPrimitive;
                    
                    if (bIsDisabled || GetIsInvalid(options, uvTri))
                    {
                        numDisabledTri++;
                        continue; // These indices will be set to special index unknown later.
                    }

                    const ommFormat ommFormat = !desc.formats || desc.formats[i] == ommFormat_INVALID ? desc.format : desc.formats[i];

                    // This is an early check to test for VM reuse.
                    // If subdivision level or format differs we can't reuse the VM.
                    std::size_t seed = 42;
                    hash_combine(seed, uvTri.p0);
                    hash_combine(seed, uvTri.p1);
                    hash_combine(seed, uvTri.p2);
                    hash_combine(seed, subdivisionLevel);
                    hash_combine(seed, ommFormat);

                    const uint64_t vmId = seed;

                    auto it = triangleIDToWorkItem.find(vmId);
                    if ((it == triangleIDToWorkItem.end() || options.disableDuplicateDetection))
                    {
                        if (kMaxSubdivLevel < subdivisionLevel)
                        {
                            return log.InvalidArg("[Invalid Argument] - subdivisionLevel for primitive (i) is (d) which exceeds kMaxSubdivLevel(12)");
                        }
                        uint32_t workItemIdx = (uint32_t)vmWorkItems.size();
                        // Temporarily set the triangle->vm desc mapping like this.
                        triangleIDToWorkItem.insert(std::make_pair(vmId, workItemIdx));
                        vmWorkItems.emplace_back(allocator, ommFormat, subdivisionLevel, i, uvTri);
                    }
                    else {
                        vmWorkItems[it->second].primitiveIndices.push_back(i);
                    }
                }

                if (options.enableValidation && numDisabledTri != 0)
                {
                    const char* specialIndex = ToString(desc.unresolvedTriState);
                    log.Infof("[Info] - The workload consists of %d unclassifiable triangles, these will be classified as unresolvedTriState = %s.",
                        numDisabledTri, specialIndex);
                }
            }
            return ommResult_SUCCESS;
        }

        static uint64_t ComputeWorkloadSize(const ommCpuBakeInputDesc& desc, vector<OmmWorkItem>& vmWorkItems)
        {
            const TextureImpl* texture = GetHandleImpl<TextureImpl>(desc.texture);

            // Approximate the workload size. 
            // The workload metric is the accumulated count of the number of texels in total that needs to be processed.
            // So where is the cutoff point? Hard to say. But if the workload 
            const uint64_t texelCount = (uint64_t)texture->GetSize(0 /*mip*/).x * (uint64_t)texture->GetSize(0 /*mip*/).y;
            const float2 sizef = (float2)texture->GetSize(0 /*mip*/);
            uint64_t workloadSize = 0;

            for (const OmmWorkItem& workItem : vmWorkItems)
            {
                const int2 aabb = int2((workItem.uvTri.aabb_e - workItem.uvTri.aabb_s) * sizef);
                workloadSize += uint64_t(aabb.x * aabb.y);
            }

            return workloadSize;
        }

        static ommResult ValidateWorkloadSize(
            const StdAllocator<uint8_t>& allocator, Logger log, const ommCpuBakeInputDesc& desc, const Options& options, vector<OmmWorkItem>& ommWorkItems)
        {
            const bool limitWorkloadSize = desc.maxWorkloadSize != 0xFFFFFFFFFFFFFFFF;

            if (!options.enableValidation && !limitWorkloadSize)
                return ommResult_SUCCESS;

            uint64_t workloadSize = ComputeWorkloadSize(desc, ommWorkItems);

            if (limitWorkloadSize)
            {
                if (workloadSize > desc.maxWorkloadSize)
                {
                    return ommResult_WORKLOAD_TOO_BIG;
                }
            }
            
            if (options.enableValidation)
            {
                const uint64_t warnSize = 1ull << 27ull;  // 128 * 1024x1024 texels.
                if (workloadSize > warnSize)
                {
                    const uint64_t num1Ktextures = workloadSize >> (20ull); // divide by 1024x1024

                    log.PerfWarnf("[Perf Warning] - The workload consists of %lld work items (number of texels to classify), which corresponds to roughly %lld 1024x1024 textures."
                        " This is unusually large and may result in long bake times.", workloadSize, num1Ktextures);
                }
            }

            return ommResult_SUCCESS;
        }

        template<ommCpuTextureFormat eFormat, TilingMode eTilingMode, ommTextureAddressMode eTextureAddressMode, ommTextureFilterMode eFilterMode>
        static ommResult ResampleCoarse(const ommCpuBakeInputDesc& desc, const Logger& log, const Options& options, vector<OmmWorkItem>& vmWorkItems)
        {
            if (options.enableAABBTesting && !options.disableLevelLineIntersection)
                return log.InvalidArg("[Invalid Arg] - EnableAABBTesting can't be used without also setting DisableLevelLineIntersection");

            const TextureImpl* texture = GetHandleImpl<TextureImpl>(desc.texture);

            if (!texture->HasSAT())
                return ommResult_SUCCESS;

            if (texture->GetMipCount() != 1)
                return ommResult_SUCCESS;

            // 3. Process the queue of unique triangles...
            {
                const int32_t numWorkItems = (int32_t)vmWorkItems.size();

                // 3.1 Rasterize...
                {
                    #pragma omp parallel for if(options.enableInternalThreads)
                    for (int32_t workItemIt = 0; workItemIt < numWorkItems; ++workItemIt) {

                        // 3.2 figure out the sub-states via rasterization...
                        {
                            // Subdivide the input triangle in to smaller triangles. They will be "bird-curve" ordered.
                            OmmWorkItem& workItem = vmWorkItems[workItemIt];

                            const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(workItem.subdivisionLevel);

                            // Perform rasterization of each individual VM.
                            if (eFilterMode == ommTextureFilterMode_Linear)
                            {
                                // Run conservative rasterization on the micro triangle
                                for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt)
                                {
                                    const Triangle subTri = omm::bird::GetMicroTriangle(workItem.uvTri, uTriIt, workItem.subdivisionLevel);

                                    const int32_t Sx = (int32_t)subTri.aabb_s.x;
                                    const int32_t Sy = (int32_t)subTri.aabb_s.y;

                                    const int32_t Ex = (int32_t)subTri.aabb_e.x;
                                    const int32_t Ey = (int32_t)subTri.aabb_e.y;

                                    if (Sx != Ex || Sy != Ey)
                                    {
                                        continue;
                                    }

                                    const uint32_t mip = 0;

                                    const float2 faabb_s = (subTri.aabb_s * (float2)texture->GetSize(mip)) - 0.5f;
                                    const float2 faabb_e = (subTri.aabb_e * (float2)texture->GetSize(mip)) - 0.5f;
                                    int2 iaabb_s[TexelOffset::MAX_NUM];
                                    omm::GatherTexCoord4<eTextureAddressMode>(glm::floor(faabb_s), texture->GetSize(mip), iaabb_s);

                                    int2 iaabb_e[TexelOffset::MAX_NUM];
                                    omm::GatherTexCoord4<eTextureAddressMode>(glm::floor(faabb_e), texture->GetSize(mip), iaabb_e);

                                    const int2 aabb_s = iaabb_s[TexelOffset::I0x0];
                                    const int2 aabb_e = iaabb_e[TexelOffset::I1x1];

                                    // This means the micro-triangle wraps over the image border.
                                    if (aabb_e.x < aabb_s.x || aabb_e.y < aabb_s.y)
                                        continue;

                                    if (!texture->InTexture(aabb_s, mip) || !texture->InTexture(aabb_e, mip))
                                    {
                                        continue;
                                    }

                                    const int2 aabb = (aabb_e - aabb_s);
                                    const uint32_t area = (aabb.x + 1) * (aabb.y + 1);

                                    const uint32_t sa = texture->SAT(aabb_s, aabb_e, mip);

                                    if (sa == 0)
                                    {
                                        // (Less than or equal to alpha threshold)
                                        workItem.vmStates.SetState(uTriIt, desc.alphaCutoffLessEqual);
                                    }
                                    else if (sa == area)
                                    {
                                        // (Greater than alpha threshold)
                                        workItem.vmStates.SetState(uTriIt, desc.alphaCutoffGreater);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return ommResult_SUCCESS;
        }

        enum TriangleClass
        {
            Normal,
            Degenerate
        };

        template<ommCpuTextureFormat eFormat, TilingMode eTilingMode, ommTextureAddressMode eTextureAddressMode, ommTextureFilterMode eFilterMode, TriangleClass eTriangleClass>
        static ommResult ResampleFine(const ommCpuBakeInputDesc& desc, const Logger& log, const Options& options, vector<OmmWorkItem>& vmWorkItems)
        {
            if (options.enableAABBTesting && !options.disableLevelLineIntersection)
                return log.InvalidArg("[Invalid Arg] - EnableAABBTesting can't be used without also setting DisableLevelLineIntersection");

            if (options.disableFineClassification)
                return ommResult_SUCCESS;

            const TextureImpl* texture = GetHandleImpl<TextureImpl>(desc.texture);

            // 3. Process the queue of unique triangles...
            {
                const int32_t numWorkItems = (int32_t)vmWorkItems.size();

                // 3.1 Rasterize...
                {
                    #pragma omp parallel for if(options.enableInternalThreads)
                    for (int32_t workItemIt = 0; workItemIt < numWorkItems; ++workItemIt) {

                        // 3.2 figure out the sub-states via rasterization...
                        {
                            // Subdivide the input triangle in to smaller triangles. They will be "bird-curve" ordered.
                            OmmWorkItem& workItem = vmWorkItems[workItemIt];
                            const bool isDegenerate = workItem.uvTri.GetIsDegenerate();

                            if (eTriangleClass == TriangleClass::Normal && isDegenerate)
                            {
                                continue;
                            }

                            if (eTriangleClass == TriangleClass::Degenerate && !isDegenerate)
                            {
                                continue;
                            }

                            const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(workItem.subdivisionLevel);

                            // Perform rasterization of each individual VM.
                            if (eFilterMode == ommTextureFilterMode_Linear)
                            {
                                // Run conservative rasterization on the micro triangle
                                for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt)
                                {
                                    if (workItem.vmStates.GetState(uTriIt) != ommOpacityState_UnknownOpaque)
                                    {
                                        continue;
                                    }

                                    const Triangle subTri = omm::bird::GetMicroTriangle(workItem.uvTri, uTriIt, workItem.subdivisionLevel);

                                    // Figure out base-state by sampling at the center of the triangle.
                                    if (!options.disableLevelLineIntersection) 
                                    {
                                        OmmCoverage vmCoverage = { 0, };
                                        for (uint32_t mipIt = 0; mipIt < texture->GetMipCount(); ++mipIt)
                                        {
                                            // Linear interpolation requires a conservative raster and checking all four interpolants.
                                            // The size of the raster grid must (at least) match the input alpha texture size
                                            // this way we get a single pixel kernel execution per alpha texture texel.
                                            const int2 rasterSize = texture->GetSize(mipIt);


                                            LevelLineIntersectionKernel::Params params = { &vmCoverage,  &subTri, texture->GetRcpSize(mipIt), rasterSize, texture, desc.alphaCutoff, desc.runtimeSamplerDesc.borderAlpha, mipIt };

                                            // This offset (in pixel units) will be applied to the triangle,
                                            // the effect is that the raster grid is being mapped such that bilinear interpolation region defined by
                                            // the interior of 4 alpha interpolants is being mapped to match raster grid.
                                            // This is only correct for bilinear version, nearest sampling should map exactly to the source alpha texture.
                                            float2 pixelOffset = -float2(0.5, 0.5);

                                            if (desc.alphaCutoff < texture->Bilinear(eTextureAddressMode, subTri.p0, mipIt))
                                                vmCoverage.numAboveAlpha++;
                                            else
                                                vmCoverage.numBelowAlpha++;


                                            if constexpr (eTriangleClass == TriangleClass::Normal)
                                            {
                                                auto kernel = &LevelLineIntersectionKernel::run<eFormat, eTextureAddressMode, eTilingMode, false /*degenerate*/>;
                                                RasterizeConservativeSerialWithOffsetCoverage(subTri, rasterSize, pixelOffset, kernel, &params);
                                            }
                                            else
                                            {
                                                auto kernel = &LevelLineIntersectionKernel::run<eFormat, eTextureAddressMode, eTilingMode, true /*degenerate*/>;
                                                Line l(subTri.aabb_s, subTri.aabb_e);
                                                RasterizeConservativeLineWithOffset(l, rasterSize, pixelOffset, kernel, &params);
                                            }

                                            OMM_ASSERT(vmCoverage.numAboveAlpha != 0 || vmCoverage.numBelowAlpha != 0);
                                            const ommOpacityState state = GetStateFromCoverage(desc.format, desc.unknownStatePromotion, desc.alphaCutoffGreater, desc.alphaCutoffLessEqual, vmCoverage);

                                            if (IsUnknown(state))
                                                break;
                                        }
                                        const ommOpacityState state = GetStateFromCoverage(desc.format, desc.unknownStatePromotion, desc.alphaCutoffGreater, desc.alphaCutoffLessEqual, vmCoverage);
                                        workItem.vmStates.SetState(uTriIt, state);
                                    }
                                    else if (options.enableAABBTesting)
                                    {
                                        // This offset (in pixel units) will be applied to the triangle,
                                        // the effect is that the raster grid is being mapped such that bilinear interpolation region defined by
                                        // the interior of 4 alpha interpolants is being mapped to match raster grid.
                                        // This is only correct for bilinear version, nearest sampling should map exactly to the source alpha texture.

                                        uint32_t mip = 0;
                                        OMM_ASSERT(texture->GetMipCount() == 0);
                                        const int2 rasterSize = texture->GetSize(mip);
                                        float2 pixelOffset = -float2(0.5, 0.5);

                                        OmmCoverage vmCoverage = { 0, };
                                        ConservativeBilinearKernel::Params params = { &vmCoverage,  texture->GetRcpSize(mip), rasterSize, texture, desc.alphaCutoff, desc.runtimeSamplerDesc.borderAlpha, mip };

                                        Triangle subTri0 = Triangle(subTri.aabb_s, float2(subTri.aabb_e.x, subTri.aabb_s.y), float2(subTri.aabb_s.x, subTri.aabb_e.y));
                                        Triangle subTri1 = Triangle(subTri.aabb_e, float2(subTri.aabb_e.x, subTri.aabb_s.y), float2(subTri.aabb_s.x, subTri.aabb_e.y));
                                        auto kernel = &ConservativeBilinearKernel::run<eFormat, eTextureAddressMode, eTilingMode>;
                                        RasterizeConservativeSerialWithOffsetCoverage(subTri0, rasterSize, pixelOffset, kernel, &params);
                                        RasterizeConservativeSerialWithOffsetCoverage(subTri1, rasterSize, pixelOffset, kernel, &params);

                                        OMM_ASSERT(vmCoverage.numAboveAlpha != 0 || vmCoverage.numBelowAlpha != 0);

                                        const ommOpacityState state = GetStateFromCoverage(desc.format, desc.unknownStatePromotion, desc.alphaCutoffGreater, desc.alphaCutoffLessEqual, vmCoverage);
                                        workItem.vmStates.SetState(uTriIt, state);
                                    }
                                    else
                                    {
                                        // This offset (in pixel units) will be applied to the triangle,
                                        // the effect is that the raster grid is being mapped such that bilinear interpolation region defined by
                                        // the interior of 4 alpha interpolants is being mapped to match raster grid.
                                        // This is only correct for bilinear version, nearest sampling should map exactly to the source alpha texture.

                                        uint32_t mip = 0;
                                        OMM_ASSERT(texture->GetMipCount() == 1);
                                        const int2 rasterSize = texture->GetSize(mip);

                                        float2 pixelOffset = -float2(0.5, 0.5);

                                        OmmCoverage vmCoverage = { 0, };
                                        ConservativeBilinearKernel::Params params = { &vmCoverage,  texture->GetRcpSize(mip), rasterSize, texture, desc.alphaCutoff, desc.runtimeSamplerDesc.borderAlpha, mip };

                                        auto kernel = &ConservativeBilinearKernel::run<eFormat, eTextureAddressMode, eTilingMode>;
                                        RasterizeConservativeSerialWithOffsetCoverage(subTri, rasterSize, pixelOffset, kernel, &params);

                                        OMM_ASSERT(vmCoverage.numBelowAlpha != 0 || vmCoverage.numAboveAlpha != 0);

                                        const ommOpacityState state = GetStateFromCoverage(desc.format, desc.unknownStatePromotion, desc.alphaCutoffGreater, desc.alphaCutoffLessEqual, vmCoverage);

                                        workItem.vmStates.SetState(uTriIt, state);
                                    }
                                }
                            }
                            else if (eFilterMode == ommTextureFilterMode_Nearest)
                            {
                                struct KernelParams {
                                    OmmCoverage*        vmState;
                                    float2              invSize;
                                    int2                size;
                                    ommSamplerDesc      runtimeSamplerDesc;
                                    const TextureImpl* texture;
                                    float               alphaCutoff;
                                    float               borderAlpha;
                                    uint32_t            mipIt;
                                };

                                for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt)
                                {
                                    OmmCoverage vmCoverage = { 0, };
                                    for (uint32_t mipIt = 0; mipIt < texture->GetMipCount(); ++mipIt)
                                    {
                                        const int2 rasterSize = texture->GetSize(mipIt);
                                        KernelParams params = { nullptr, texture->GetRcpSize(mipIt), rasterSize, desc.runtimeSamplerDesc, texture, desc.alphaCutoff, desc.runtimeSamplerDesc.borderAlpha, mipIt };

                                        params.vmState = &vmCoverage;

                                        auto kernel = [](int2 pixel, void* ctx)
                                        {
                                            KernelParams* p = (KernelParams*)ctx;

                                            const int2 coord = omm::GetTexCoord<eTextureAddressMode>(pixel, p->size);

                                            const bool isBorder = eTextureAddressMode == ommTextureAddressMode_Border && (coord.x == kTexCoordBorder || coord.y == kTexCoordBorder);
                                            const float alpha = isBorder ? p->borderAlpha : p->texture->template Load<eFormat, eTilingMode>(coord, p->mipIt);

                                            if (p->alphaCutoff < alpha) {
                                                p->vmState->numAboveAlpha++;
                                            }
                                            else {
                                                p->vmState->numBelowAlpha++;
                                            }
                                        };

                                        const Triangle subTri = omm::bird::GetMicroTriangle(workItem.uvTri, uTriIt, workItem.subdivisionLevel);

                                        RasterizeConservativeSerial(subTri, rasterSize, kernel, &params);
                                        OMM_ASSERT(vmCoverage.numAboveAlpha != 0 || vmCoverage.numBelowAlpha != 0);

                                        const ommOpacityState state = GetStateFromCoverage(desc.format, desc.unknownStatePromotion, desc.alphaCutoffGreater, desc.alphaCutoffLessEqual, vmCoverage);
                                        if (IsUnknown(state))
                                            break;
                                    }
                                    const ommOpacityState state = GetStateFromCoverage(desc.format, desc.unknownStatePromotion, desc.alphaCutoffGreater, desc.alphaCutoffLessEqual, vmCoverage);
                                    workItem.vmStates.SetState(uTriIt, state);
                                }
                            }
                        }
                    }
                }
            }
            return ommResult_SUCCESS;
        }

        static ommResult DeduplicateExact(const StdAllocator<uint8_t>& allocator, const Options& options, vector<OmmWorkItem>& vmWorkItems)
        {
            if (options.disableDuplicateDetection)
                return ommResult_SUCCESS;

            uint32_t dupesFound = 0;

            auto CalcDigest = [&allocator](const OmmWorkItem& workItem) {
                return XXH64((const void*)workItem.vmStates.GetOmm3StateData(), workItem.vmStates.GetOmm3StateDataSize(), 42/*seed*/);
            };

            hash_map<uint64_t, uint32_t> digestToWorkItemIndex(allocator.GetInterface());
            for (uint32_t i = 0; i < vmWorkItems.size(); ++i)
            {
                const OmmWorkItem& workItem = vmWorkItems[i];
                uint64_t digest = CalcDigest(workItem);
                auto it = digestToWorkItemIndex.find(digest);
                if (it == digestToWorkItemIndex.end())
                {
                    digestToWorkItemIndex.insert(std::make_pair(digest, i));
                }
                else
                {
                    // Transfer primitives to the new VM index...
                    OmmWorkItem& existingWorkItem = vmWorkItems[it->second];
                    existingWorkItem.primitiveIndices.insert(existingWorkItem.primitiveIndices.end(), workItem.primitiveIndices.begin(), workItem.primitiveIndices.end());

                    // Get rid if this work item. Forver.
                    vmWorkItems[i].primitiveIndices.clear();
                    vmWorkItems[i].vmSpecialIndex = -1;
                    dupesFound++;
                }
            }

            return ommResult_SUCCESS;
        }

        static float HammingDistance3State(const OmmWorkItem& workItemA, const OmmWorkItem& workItemB)
        {
            OMM_ASSERT(workItemA.subdivisionLevel == workItemB.subdivisionLevel);
            const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(workItemA.subdivisionLevel);
            uint32_t numDiff = 0;
            for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt) {

                ommOpacityState stateA = workItemA.vmStates.Get3State(uTriIt);
                ommOpacityState stateB = workItemB.vmStates.Get3State(uTriIt);

                if (stateA != stateB)
                    numDiff++;
            }

            return float(numDiff);
        };

        // Computes hamming distnace, returns false if sizes don't match.
        static float NormalizedHammingDistance3State(const OmmWorkItem& workItemA, const OmmWorkItem& workItemB)
        {
            OMM_ASSERT(workItemA.subdivisionLevel == workItemB.subdivisionLevel);
            const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(workItemA.subdivisionLevel);
            return HammingDistance3State(workItemA, workItemB) / numMicroTriangles;
        };

        static ommResult MergeWorkItems(OmmWorkItem& to, OmmWorkItem& from)
        {
            OMM_ASSERT(to.subdivisionLevel == from.subdivisionLevel);

            // Transfer primitives to the new VM index...
            to.primitiveIndices.insert(to.primitiveIndices.end(), from.primitiveIndices.begin(), from.primitiveIndices.end());

            // Get rid if this work item. Forver.
            from.primitiveIndices.clear();
            from.vmSpecialIndex = -1;

            // Merge states from A -> B.
            uint32_t numMatches = 0;
            const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(from.subdivisionLevel);

            for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt) {

                ommOpacityState toState = to.vmStates.GetState(uTriIt);
                ommOpacityState fromState = from.vmStates.GetState(uTriIt);

                if (toState != fromState)
                {
                    if (IsKnown(fromState) && IsKnown(toState))
                    {
                        to.vmStates.SetState(uTriIt, ommOpacityState_UnknownOpaque);
                    }
                    else if (IsKnown(toState) && IsUnknown(fromState))
                    {
                        // Use the unknown state A as our new unknown.
                        to.vmStates.SetState(uTriIt, fromState);
                    }
                    else // if (IsUnknown(toState) && IsUnknown(fromState))
                    {
                        // Both unknown, keep current state.
                    }
                }
            }

            return ommResult_SUCCESS;
        }

        static ommResult DeduplicateSimilarLSH(const StdAllocator<uint8_t>& allocator, const Options& options, vector<OmmWorkItem>& vmWorkItems, uint32_t iterations)
        {
            if (options.disableDuplicateDetection)
                return ommResult_SUCCESS;
            if (!options.enableNearDuplicateDetection || options.enableNearDuplicateDetectionBruteForce)
                return ommResult_SUCCESS;

            // LHS (locality sensitive hashing) implemented via hamming bit sampling 
            // ref1: https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.712.8703&rep=rep1&type=pdf
            // ref2: https://www.vldb.org/conf/1999/P49.pdf

            std::mt19937 mt(42);

            uint32_t match = 0;
            uint32_t trueMatch = 0;
            uint32_t falseMatch = 0;
            uint32_t noMatch = 0;

            for (uint32_t attempts = 0; attempts < iterations; ++attempts)
            {
                vector<uint32_t> batchWorkItems(allocator);
                batchWorkItems.reserve(vmWorkItems.size());

                struct HashTable
                {
                    vector<uint32_t> bitIndices; // random bit indices
                    vector<uint64_t> workItemHashes;
                    hash_map<uint64_t, vector<uint32_t>> layerHashToWorkItem;
                    HashTable(const StdAllocator<uint8_t>& allocator) :bitIndices(allocator), workItemHashes(allocator), layerHashToWorkItem(allocator)
                    { }
                };

                vector<HashTable> hashTables(allocator);
                vector<uint32_t> bitSamples(allocator);
                set<uint32_t> potentialMatches(allocator);

                for (uint32_t subdivisionLevel = 1; subdivisionLevel <= kMaxSubdivLevel; ++subdivisionLevel)
                {
                    batchWorkItems.clear();

                    for (uint32_t i = 0; i < vmWorkItems.size(); ++i)
                    {
                        OmmWorkItem& workItem = vmWorkItems[i];

                        if (workItem.vmSpecialIndex != OmmWorkItem::kNoSpecialIndex)
                            continue;

                        if (workItem.vmFormat != ommFormat_OC1_4_State)
                            continue;

                        if (workItem.subdivisionLevel != subdivisionLevel)
                            continue;

                        batchWorkItems.push_back(i);
                    }

                    if (batchWorkItems.size() == 0)
                        continue;

                    // # n - size of(randomly generated) data set
                    // # d - int bit size
                    // # r - range for close points
                    // # c - approximation factor

                    const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(subdivisionLevel);

                    const uint32_t n = (uint32_t)batchWorkItems.size();     // number of points.
                    const uint32_t d = numMicroTriangles;                   // dimensionality.

                    const float r = 0.15f * d;   // Distance must be at most 25%
                    const float c = 4.0f;        // Allow 2x deviation from this

                    const float p1 = 1 - r / d;         // Lower bound probability, for close two points
                    const float p2 = 1 - (c * r) / d;   // Upper bound probability, for far two points

                    const float p = 1.f / c;
                    const float Lf = glm::ceil(std::pow((float)n, p));
                    const uint32_t L = (uint32_t)Lf;
                    if (L == 0)
                        continue;

                    const uint32_t k = uint32_t(glm::ceil((std::log((float)n) * d) / (c * r)));

                    if (k == 0)
                        continue;

                    const uint32_t bigO = L * (d + k);  // O(L(d+k)), for instance c = 2 => O(sqrt(n)*(d + k))
                    const uint32_t bigO2 = n * n;       // O(N^2)

                    const float ratio = (float)bigO / bigO2;

                    hashTables.resize(L, allocator);

                    for (HashTable& hashTable : hashTables)
                    {
                        hashTable.workItemHashes.resize(vmWorkItems.size(), 0);
                        hashTable.bitIndices.resize(k);
                        hashTable.layerHashToWorkItem.clear();
                        for (uint32_t& bitIndex : hashTable.bitIndices)
                        {
                            // We're not using std::uniform_int_distribution, the output is not defined in spec and may differ between compilers
                            const uint32_t random = mt(); // between 0..uint32 max
                            bitIndex = random & (numMicroTriangles - 1); // random mod numMicroTriangles - 1
                        }
                    }

                    bitSamples.resize(k);
                    for (uint32_t workItemIndex : batchWorkItems)
                    {
                        const OmmWorkItem& workItem = vmWorkItems[workItemIndex];

                        for (HashTable& hashTable : hashTables)
                        {
                            for (uint32_t kIt = 0; kIt < k; ++kIt)
                            {
                                const uint32_t randomBitIndex = hashTable.bitIndices[kIt];
                                ommOpacityState state = workItem.vmStates.Get3State(randomBitIndex);
                                bitSamples[kIt] = (uint32_t)state;
                            }

                            uint64_t hash = XXH64((const void*)bitSamples.data(), sizeof(uint32_t) * bitSamples.size(), 42/*seed*/);

                            hashTable.workItemHashes[workItemIndex] = hash;

                            auto it = hashTable.layerHashToWorkItem.find(hash);
                            if (it != hashTable.layerHashToWorkItem.end())
                            {
                                it->second.push_back(workItemIndex);
                            }
                            else
                            {
                                vector<uint32_t> layerHashToWorkItem(allocator);
                                layerHashToWorkItem.push_back(workItemIndex);
                                hashTable.layerHashToWorkItem.insert(std::make_pair(hash, layerHashToWorkItem));
                            }
                        }
                    }

                    // Now we can do the merging.
                    for (uint32_t workItemIndex : batchWorkItems)
                    {
                        OmmWorkItem& workItem = vmWorkItems[workItemIndex];

                        if (workItem.HasSpecialIndex()) // This might happen if we have already merged this work item.
                            continue;

                        potentialMatches.clear();
                        for (const HashTable& hashTable : hashTables)
                        {
                            uint64_t hash = hashTable.workItemHashes[workItemIndex];

                            OMM_ASSERT(hash != 0);

                            auto it = hashTable.layerHashToWorkItem.find(hash);

                            OMM_ASSERT(it != hashTable.layerHashToWorkItem.end());

                            for (uint32_t potentialWorkItemIndex : it->second)
                            {
                                if (potentialWorkItemIndex == workItemIndex)
                                    continue;

                                const OmmWorkItem& potentialWorkItem = vmWorkItems[potentialWorkItemIndex];

                                if (potentialWorkItem.HasSpecialIndex())
                                    continue;
                                
                                if (potentialMatches.size() > 3 * L)
                                    break;

                                potentialMatches.insert(potentialWorkItemIndex);
                            }
                        }

                        // out of potential matches... pick best one.
                        float minDist = std::numeric_limits<float>::max();
                        int32_t nearestIndex = -1;
                        for (uint32_t potentialMatch : potentialMatches)
                        {
                            const OmmWorkItem& maybeSimilarWorkItem = vmWorkItems[potentialMatch];

                            const float dist = HammingDistance3State(workItem, maybeSimilarWorkItem);
                            if (dist < r && dist < minDist)
                            {
                                minDist = dist;
                                nearestIndex = potentialMatch;
                            }
                        }

                        if (nearestIndex >= 0)
                        {
                            OmmWorkItem& similarWorkItem = vmWorkItems[nearestIndex];

                            trueMatch++;
                            MergeWorkItems(workItem /*to*/, similarWorkItem /*from*/);
                            OMM_ASSERT(similarWorkItem.HasSpecialIndex());
                        }
                        else
                        {
                            falseMatch++;
                        }

                        if (potentialMatches.size() == 1)
                            match++;
                        else
                            noMatch++;

                        uint32_t dummy = (uint32_t)potentialMatches.size();
                        const uint32_t dummy2 = dummy;
                    }
                }
            }
            
           // const float ratio = float(match) / noMatch;
            volatile const float ratio = float(noMatch) / trueMatch;
            volatile const float ratio2 = float(noMatch) / trueMatch;

            return ommResult_SUCCESS;
        }

        static ommResult DeduplicateSimilarBruteForce(const StdAllocator<uint8_t>& allocator, const Options& options, vector<OmmWorkItem>& vmWorkItems)
        {
            if (options.disableDuplicateDetection)
                return ommResult_SUCCESS;

            if (!options.enableNearDuplicateDetection || !options.enableNearDuplicateDetectionBruteForce)
               return ommResult_SUCCESS;

            if (vmWorkItems.size() == 0)
                return ommResult_SUCCESS;

            // The purpose of this pass is to identify "similar" OMMs, and then merge those.
            // Unfortunatley the search is O(n^2) - Is this a problem? Yes. 
            // Possible solutions:
            // - If set is too large - don't do an exhaustive search, just sample. (Current solution).
            // - Look in to LSH (locality senstive hashing) approaches.

            static constexpr float kMergeThreshold = 0.1f; // If two OMMs differ less than kMergeThreshold % (treating all unknowns as equal) -> then we combine them.
            static constexpr uint32_t kMaxComparsions = 2048; // Covert the O(n^2) nature of the algorithm to a -> O(kN) version...

            set<uint32_t> mergedWorkItems(allocator);
            for (uint32_t itA = 0; itA < vmWorkItems.size() - 1; ++itA)
            {
                OmmWorkItem& workItemA = vmWorkItems[itA];

                if (workItemA.vmSpecialIndex != OmmWorkItem::kNoSpecialIndex)
                    continue;

                if (workItemA.vmFormat != ommFormat_OC1_4_State)
                    continue;

                const uint32_t searchOffsetBase = itA + 1;
                const uint32_t searchStart = searchOffsetBase;
                const uint32_t searchEnd = std::min<uint32_t>(kMaxComparsions + searchStart, (uint32_t)vmWorkItems.size());

                float minDist = std::numeric_limits<float>::max();
                int32_t nearestIndex = -1;
                for (uint32_t itB = searchStart; itB < searchEnd; ++itB)
                {
                    const OmmWorkItem& workItemB = vmWorkItems[itB];

                    if (workItemB.vmSpecialIndex != OmmWorkItem::kNoSpecialIndex)
                        continue;

                    if (workItemB.vmFormat != ommFormat_OC1_4_State)
                        continue;

                    if (workItemB.primitiveIndices.empty())
                        continue;

                    if (workItemA.subdivisionLevel != workItemB.subdivisionLevel)
                        continue;

                    if (mergedWorkItems.find(itB) != mergedWorkItems.end())
                        continue;

                    const float dist = NormalizedHammingDistance3State(workItemA, workItemB);

                    if (dist < kMergeThreshold && dist < minDist)
                    {
                        minDist = dist;
                        nearestIndex = (int32_t)itB;
                    }
                }

                if (nearestIndex >= 0)
                {
                    OmmWorkItem& workItemB = vmWorkItems[nearestIndex];

                    mergedWorkItems.insert(itA);
                    mergedWorkItems.insert(nearestIndex);
                    MergeWorkItems(workItemA /*to*/, workItemB /*from*/);
                }
            }

            return ommResult_SUCCESS;
        }

        static ommResult PromoteToSpecialIndices(const ommCpuBakeInputDesc& desc, const Options& options, vector<OmmWorkItem>& vmWorkItems)
        {
            // Collect raster output to a final VM state.
            for (int32_t workItemIt = 0; workItemIt < vmWorkItems.size(); ++workItemIt)
            {
                OmmWorkItem& workItem = vmWorkItems[workItemIt];

                const uint32_t numMicroTriangles = omm::bird::GetNumMicroTriangles(workItem.subdivisionLevel);

                bool allEqual = true;
                ommOpacityState commonState = workItem.vmStates.GetState(0);
                for (uint32_t uTriIt = 1; uTriIt < numMicroTriangles; ++uTriIt) {
                    allEqual &= commonState == workItem.vmStates.GetState(uTriIt);
                }

                if (!allEqual && desc.rejectionThreshold > 0.f)
                {
                    // Reject "poor" VMs:
                    uint32_t known = 0;
                    for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt) {
                        if (IsKnown(workItem.vmStates.GetState(uTriIt)))
                            known++;
                    }

                    const float knownFrac = known / (float)numMicroTriangles;
                    if (knownFrac < desc.rejectionThreshold)
                    {
                        allEqual = true;
                        commonState = ommOpacityState_UnknownTransparent;
                    }
                }

                if (allEqual && !options.disableSpecialIndices) {
                    workItem.vmSpecialIndex = -int32_t(commonState) - 1;
                }
            }
            return ommResult_SUCCESS;
        }

        static ommResult CreateUsageHistograms(vector<OmmWorkItem>& vmWorkItems, VisibilityMapUsageHistogram& arrayHistogram, VisibilityMapUsageHistogram& indexHistogram)
        {
            // Collect raster output to a final VM state.
            for (int32_t workItemIt = 0; workItemIt < vmWorkItems.size(); ++workItemIt)
            {
                OmmWorkItem& workItem = vmWorkItems[workItemIt];

                if (workItem.vmSpecialIndex == OmmWorkItem::kNoSpecialIndex)
                {
                    // Must allocate vm-
                    arrayHistogram.Inc(workItem.vmFormat, workItem.subdivisionLevel, 1 /*vm count*/);
                    indexHistogram.Inc(workItem.vmFormat, workItem.subdivisionLevel, (uint32_t)workItem.primitiveIndices.size() /*vm count*/);
                }
            }
            return ommResult_SUCCESS;
        }

        static ommResult MicromapSpatialSort(const StdAllocator<uint8_t>& allocator, const Options& options, const vector<OmmWorkItem>& vmWorkItems,
            vector<std::pair<uint64_t, uint32_t>>& sortKeys)
        {
            // The VMs should be sorted to respect the following rules:
            //  - Sorted by VM size (Largest first).
            //      - This produces aligned VMs.
            //  - Sorted by spatial location. A proxy for this is to use a quantized morton code of the UV coordinate
            //      - For large VMs, this aims to reduce TLD-trashing / page misses
            //      - For smaller VMs they can be spatially compacted.

            static constexpr uint32_t kTargetDeviceCacheLineSize = 128;

            sortKeys.resize(vmWorkItems.size());
            {
                #pragma omp parallel for if(options.enableInternalThreads)
                for (int32_t vmIndex = 0; vmIndex < vmWorkItems.size(); ++vmIndex) {

                    const OmmWorkItem& vm = vmWorkItems[vmIndex];
                    if (vm.vmSpecialIndex != OmmWorkItem::kNoSpecialIndex)
                    {
                        // For special indices, maintain original order.
                        uint64_t key = (1ull << 63) | (uint64_t)vmIndex;
                        sortKeys[vmIndex] = std::make_pair(key, vmIndex);
                    }
                    else {
                        // For regular VMs,  Sort on Sub-div lvl and 
                        // Order VMs in Morton-order in UV-space. 
                        constexpr const uint32_t k = 13;
                        const int2 qSize = int2(1u << k, 1u << k);
                        const int2 qUV = int2(float2(qSize) * ((vm.uvTri.p0 + vm.uvTri.p1 + vm.uvTri.p2) / 3.f));
                        const int2 qPosMirrored = GetTexCoord<ommTextureAddressMode_MirrorOnce>(qUV, qSize);
                        OMM_ASSERT(qPosMirrored.x >= 0 && qPosMirrored.y >= 0);
                        const uint64_t mCode = xy_to_morton(qPosMirrored.x, qPosMirrored.y);
                        OMM_ASSERT(mCode < (1ull << (k << 1ull)));
                        OMM_ASSERT(mCode < (1ull << 60ull));

                        // First sort on sub-div lvl.
                        uint64_t key = 0;
                        key |= (uint64_t)vm.subdivisionLevel << 60;
                        key |= mCode;
                        sortKeys[vmIndex] = std::make_pair(key, vmIndex);
                    }
                }

                std::sort(sortKeys.begin(), sortKeys.end(), std::greater<std::pair<uint64_t, uint32_t>>());
            }
            return ommResult_SUCCESS;
        }

        static ommResult Serialize(
            const StdAllocator<uint8_t>& allocator, 
            const ommCpuBakeInputDesc& desc, const Options& options, vector<OmmWorkItem>& vmWorkItems, const VisibilityMapUsageHistogram& ommArrayHistogram, const VisibilityMapUsageHistogram& ommIndexHistogram,
            const vector<std::pair<uint64_t, uint32_t>>& sortKeys,
            BakeResultImpl& res)
        {
            {
                const uint32_t ommBitCount = omm::bird::GetBitCount(desc.format);

                uint32_t ommDescArrayCount = 0;
                size_t ommArrayDataSize = 0;
                for (uint32_t i = 0; i < kMaxNumSubdivLevels; ++i) {
                    const uint32_t ommCount = ommArrayHistogram.GetOmmCount(desc.format, i);
                    ommDescArrayCount += ommCount;
                    const size_t numOmmForSubDivLvl = (size_t)omm::bird::GetNumMicroTriangles(i) * ommBitCount;
                    ommArrayDataSize += size_t(ommCount) * std::max<size_t>(numOmmForSubDivLvl >> 3ull, 1ull);
                }

                if (ommArrayDataSize > std::numeric_limits<uint32_t>::max()) // Array data > 4GB? ouch
                    return ommResult_FAILURE;

                OMM_ASSERT((ommDescArrayCount == 0 && ommArrayDataSize == 0) || (ommDescArrayCount != 0 && ommArrayDataSize != 0));

                if (ommDescArrayCount != 0)
                {
                    res.ommArrayData.resize(ommArrayDataSize);
                    std::memset(res.ommArrayData.data(), 0, res.ommArrayData.size());
                    res.ommDescArray.resize(ommDescArrayCount);

                    uint32_t ommArrayDataOffset = 0;
                    uint32_t prevSubDivLvl = 0;
                    uint32_t vmDescOffset = 0;
                    for (auto [_, vmIndex] : sortKeys) {
                        OmmWorkItem& vm = vmWorkItems[vmIndex];

                        if (vm.vmSpecialIndex == OmmWorkItem::kNoSpecialIndex)
                        {
                            if (ommArrayDataOffset >= ommArrayDataSize)
                                return ommResult_FAILURE;

                            // Fill Desc Info
                            res.ommDescArray[vmDescOffset].subdivisionLevel = vm.subdivisionLevel;
                            res.ommDescArray[vmDescOffset].format = (uint16_t)vm.vmFormat;
                            res.ommDescArray[vmDescOffset].offset = ommArrayDataOffset;
                            vm.vmDescOffset = vmDescOffset++;

                            const uint32_t numMicroTriangles = bird::GetNumMicroTriangles(vm.subdivisionLevel);

                            uint8_t* ommArrayDataPtr = res.ommArrayData.data() + ommArrayDataOffset;
                            const uint32_t is2State = vm.vmFormat == ommFormat_OC1_2_State;
                            for (uint32_t uTriIt = 0; uTriIt < numMicroTriangles; ++uTriIt)
                            {
                                uint32_t state = ((uint32_t)vm.vmStates.GetState(uTriIt));

                                uint8_t val;
                                if (is2State)   val = state << ((uTriIt & 7));
                                else            val = state << ((uTriIt & 3) << 1u);

                                uint32_t byteIndex = uTriIt >> (2 + is2State);
                                ommArrayDataPtr[byteIndex] |= val;
                            }

                            // Offsets must be at least 1B aligned.
                            ommArrayDataOffset += std::max((numMicroTriangles * ommBitCount) >> 3u, 1u);
                        }
                    }
                }
            }

            // Allocate the final ommArrayHistogram & ommIndexHistogram
            {
                static constexpr uint32_t kMaxFormats = 2;
                static_assert(kMaxFormats == (int)ommFormat_MAX_NUM - 1);
                res.ommArrayHistogram.reserve(kMaxFormats * kMaxNumSubdivLevels);
                res.ommIndexHistogram.reserve(kMaxFormats * kMaxNumSubdivLevels);
                uint32_t ommUsageDescCount = 0;
                {
                    for (ommFormat vmFormat : { ommFormat_OC1_2_State, ommFormat_OC1_4_State, }) {
                        for (uint32_t subDivLvl = 0; subDivLvl < kMaxNumSubdivLevels; ++subDivLvl) {

                            {
                                uint32_t vmCount = ommArrayHistogram.GetOmmCount(vmFormat, subDivLvl);
                                if (vmCount != 0) {
                                    res.ommArrayHistogram.push_back({ vmCount, (uint16_t)subDivLvl, (uint16_t)vmFormat });
                                }
                            }

                            {
                                uint32_t vmCount = ommIndexHistogram.GetOmmCount(vmFormat, subDivLvl);
                                if (vmCount != 0) {
                                    res.ommIndexHistogram.push_back({ vmCount, (uint16_t)subDivLvl, (uint16_t)vmFormat });
                                }
                            }
                        }
                    }
                }
            }

            const int32_t triangleCount = desc.indexCount / 3;

            // Set special indices...
            {
                res.ommIndexBuffer.resize(triangleCount);
                std::fill(res.ommIndexBuffer.begin(), res.ommIndexBuffer.end(), (int32_t)desc.unresolvedTriState);
                for (const OmmWorkItem& vm : vmWorkItems) 
				{
                    for (uint32_t primitiveIndex : vm.primitiveIndices)
                    {
                        if (vm.vmSpecialIndex != OmmWorkItem::kNoSpecialIndex)
                            res.ommIndexBuffer[primitiveIndex] = vm.vmSpecialIndex;
                        else
                            res.ommIndexBuffer[primitiveIndex] = vm.vmDescOffset;
                    }
                }
            }

            // Compress to 16 bit indices if possible & allowed.
            ommIndexFormat ommIndexFormat = ommIndexFormat_UINT_32;
            {
                const bool force32bit = ((int32_t)desc.bakeFlags & (int32_t)ommCpuBakeFlags_Force32BitIndices) == (int32_t)ommCpuBakeFlags_Force32BitIndices;
                const bool canCompressTo16Bit = triangleCount <= std::numeric_limits<int16_t>::max();

                if (canCompressTo16Bit && !force32bit)
                {
                    int16_t* ommIndexBuffer16 = (int16_t*)res.ommIndexBuffer.data();
                    for (int32_t i = 0; i < triangleCount; ++i) {
                        int32_t idx = res.ommIndexBuffer[i];
                        int16_t idx16 = (int16_t)idx;
                        ommIndexBuffer16[i] = idx16;
                    }

                    ommIndexFormat = ommIndexFormat_UINT_16;
                }
            }

            res.Finalize(ommIndexFormat);

            return ommResult_SUCCESS;
        }
    } // namespace impl

    template<ommCpuTextureFormat eFormat, TilingMode eTilingMode, ommTextureAddressMode eTextureAddressMode, ommTextureFilterMode eFilterMode>
    ommResult BakeOutputImpl::BakeImpl(const ommCpuBakeInputDesc& desc)
    {
        RETURN_STATUS_IF_FAILED(ValidateDesc(desc));

        Options options(desc.bakeFlags);

        m_bakeInputDesc = desc;

        auto impl__ResampleCoarse = [](const ommCpuBakeInputDesc& desc, const Logger& log, const Options& options, vector<OmmWorkItem>& vmWorkItems) {
            return impl::ResampleCoarse<eFormat, eTilingMode, eTextureAddressMode, eFilterMode>(desc, log, options, vmWorkItems);
        };

        auto impl__ResampleFineNormal = [](const ommCpuBakeInputDesc& desc, const Logger& log, const Options& options, vector<OmmWorkItem>& vmWorkItems) {
            return impl::ResampleFine<eFormat, eTilingMode, eTextureAddressMode, eFilterMode, impl::TriangleClass::Normal>(desc, log, options, vmWorkItems);
        };

        auto impl__ResampleFineDegen = [](const ommCpuBakeInputDesc& desc, const Logger& log, const Options& options, vector<OmmWorkItem>& vmWorkItems) {
            return impl::ResampleFine<eFormat, eTilingMode, eTextureAddressMode, eFilterMode, impl::TriangleClass::Degenerate>(desc, log, options, vmWorkItems);
        };

        {
            vector<OmmWorkItem> vmWorkItems(m_stdAllocator.GetInterface());

            RETURN_STATUS_IF_FAILED(impl::SetupWorkItems(m_stdAllocator, m_log, desc, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl::ValidateWorkloadSize(m_stdAllocator, m_log, desc, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl__ResampleCoarse(desc, m_log, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl__ResampleFineNormal(desc, m_log, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl__ResampleFineDegen(desc, m_log, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl::PromoteToSpecialIndices(desc, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl::DeduplicateExact(m_stdAllocator, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl::DeduplicateSimilarLSH(m_stdAllocator, options, vmWorkItems, 3 /*iterations*/));

            RETURN_STATUS_IF_FAILED(impl::DeduplicateSimilarBruteForce(m_stdAllocator, options, vmWorkItems));

            RETURN_STATUS_IF_FAILED(impl::PromoteToSpecialIndices(desc, options, vmWorkItems));

            VisibilityMapUsageHistogram arrayHistogram;
            VisibilityMapUsageHistogram indexHistogram;
            RETURN_STATUS_IF_FAILED(impl::CreateUsageHistograms(vmWorkItems, arrayHistogram, indexHistogram));

            vector<std::pair<uint64_t, uint32_t>> sortKeys(m_stdAllocator.GetInterface());
            RETURN_STATUS_IF_FAILED(impl::MicromapSpatialSort(m_stdAllocator, options, vmWorkItems, sortKeys));

            RETURN_STATUS_IF_FAILED(impl::Serialize(m_stdAllocator, desc, options, vmWorkItems, arrayHistogram, indexHistogram,
                sortKeys, m_bakeResult));
        }

        return ommResult_SUCCESS;
    }

} // namespace Cpu
} // namespace omm
