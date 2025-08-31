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
#include "std_containers.h"
#include "shader_bindings.h"
#include "std_allocator.h"
#include "log.h"

#include <shared/math.h>
#include <shared/texture.h>

#include <map>
#include <array>
#include <cstring>

namespace omm
{
namespace Gpu
{
    struct CBufferWriter
    {
        CBufferWriter(uint8_t* data, size_t size) :_data(data), _offset(0), _maxSize(size)
        { }

        template<class T>
        void WriteDW(T data) {
            static_assert(sizeof(T) == sizeof(uint32_t));
            *((T*)(_data + _offset)) = data;
            _offset += sizeof(uint32_t);
            OMM_ASSERT(_offset <= _maxSize);
        }

        size_t GetSize() const
        {
            return _offset;
        }

    private:
        uint8_t* _data;
        size_t _offset;
        size_t _maxSize;
    };

    struct BufferHeapAlloc
    {
        struct Handle
        {
            uint32_t offset;
            uint32_t size;
        };
        static inline const Handle kInvalidHandle = { 0xFFFFFFFu, 0xFFFFFFFu };

        ommResult Allocate(size_t size, uint32_t alignment, Handle& handle)
        {
            OMM_ASSERT(size < std::numeric_limits<uint32_t>::max());
            uint32_t size32 = math::Align<uint32_t>((uint32_t)size, alignment);

            handle.offset = m_offset;
            handle.size = size32;
            OMM_ASSERT((uint64_t)m_offset + size < std::numeric_limits<uint32_t>::max());
            m_offset += size32;
            return ommResult_SUCCESS;
        }

        uint32_t GetBufferOffset(Handle handle) const
        {
            return handle.offset;
        }

        uint32_t GetSize(Handle handle) const
        {
            return handle.size;
        }

        uint32_t GetCurrentReservation() const
        {
            return m_offset;
        }

    private:
        uint32_t m_offset = 0;
    };

    struct BufferResource {
        const char* debugName;
        ommGpuResourceType type;
        uint32_t indexInPool;
        BufferHeapAlloc allocator;

        struct SubRange
        {
            BufferHeapAlloc::Handle slot = BufferHeapAlloc::kInvalidHandle;
            const BufferResource* resourceRef = nullptr;

            uint32_t GetBufferOffset() const
            {
                return resourceRef->allocator.GetBufferOffset(slot);
            }

            uint32_t GetSize() const
            {
                return resourceRef->allocator.GetSize(slot);
            }

            bool IsValid() const 
            {
                return resourceRef != nullptr;
            }
        };

        ommResult Allocate(size_t size, uint32_t alignment, SubRange& out)
        {
            BufferHeapAlloc::Handle handle;
            RETURN_STATUS_IF_FAILED(allocator.Allocate(size, alignment, handle));
            out = { handle, this };
            return ommResult_SUCCESS;
        }
    };

    struct PipelineBuilder
    {
        struct ByteCode {
            const char* name;
            const char* entryPoint;
            const unsigned char* dxil;
            size_t dxilSize;
            const unsigned char* spirv;
            size_t spirvSize;

            const unsigned char* GetByteCode(ommGpuRenderAPI api) const { return api == ommGpuRenderAPI_DX12 ? dxil : spirv; }
            size_t GetByteCodeSize(ommGpuRenderAPI api) const { return api == ommGpuRenderAPI_DX12 ? dxilSize : spirvSize; }
        };

        PipelineBuilder(const StdAllocator<uint8_t>& stdAllocator)
            : _pipelines(stdAllocator)
            , _ranges(stdAllocator)
            , _staticSamplers(stdAllocator)
        {
            _desc.pipelineNum = 0;
            _desc.pipelines = nullptr;
            _desc.spirvBindingOffsets.samplerOffset = OMM_VK_S_SHIFT;
            _desc.spirvBindingOffsets.textureOffset = OMM_VK_T_SHIFT;
            _desc.spirvBindingOffsets.constantBufferOffset = OMM_VK_B_SHIFT;
            _desc.spirvBindingOffsets.storageTextureAndBufferOffset = OMM_VK_U_SHIFT;
            _desc.globalConstantBufferDesc.maxDataSize = 0;
            _desc.globalConstantBufferDesc.registerIndex = 0;
            _desc.localConstantBufferDesc.maxDataSize = 0;
            _desc.localConstantBufferDesc.registerIndex = 0;
            _desc.descriptorSetDesc.constantBufferMaxNum = 0;
            _desc.descriptorSetDesc.storageBufferMaxNum = 0;
            _desc.descriptorSetDesc.descriptorRangeMaxNumPerPipeline = 0;
        }

        void SetAPI(ommGpuRenderAPI api) {
            _API = api;
        }

        const ommGpuPipelineDesc& GetPipeline(uint32_t pipelineIndex) const
        {
            return _pipelines[pipelineIndex];
        }

        void AddStaticSamplerDesc(
            std::initializer_list<ommGpuStaticSamplerDesc> ranges)
        {
            _staticSamplers.insert(_staticSamplers.end(), ranges.begin(), ranges.end());
        }

        uint32_t GetStaticSamplerIndex(const ommSamplerDesc& desc) const
        {
            OMM_ASSERT(_staticSamplers.size() < 16); // Keep linear search small.

            for (int32_t i = 0; i < _staticSamplers.size(); ++i) {
                if (_staticSamplers[i].desc.addressingMode == desc.addressingMode &&
                    _staticSamplers[i].desc.borderAlpha == desc.borderAlpha &&
                    _staticSamplers[i].desc.filter == desc.filter)  {
                    return i;
                }
            }

            OMM_ASSERT(false && "Sampler not found.");
            return (uint32_t)0;
        }

        uint32_t AddComputePipeline(const ByteCode& bytecode,
            const ommGpuDescriptorRangeDesc* ranges, uint32_t numRanges)
        {
            ommGpuPipelineDesc desc;
            desc.type = ommGpuPipelineType_Compute;
            desc.compute.computeShader.data = bytecode.GetByteCode(_API);
            desc.compute.computeShader.size = bytecode.GetByteCodeSize(_API);
            desc.compute.shaderFileName = bytecode.name;
            desc.compute.shaderEntryPointName = bytecode.entryPoint;
            desc.compute.descriptorRanges = (const ommGpuDescriptorRangeDesc*)_ranges.size();
            desc.compute.descriptorRangeNum = (uint32_t)numRanges;
            _pipelines.push_back(desc);

            _ranges.insert(_ranges.end(), ranges, ranges + numRanges);

            return (uint32_t)(_pipelines.size() - 1);
        }

        uint32_t AddGraphicsPipeline(
            const ByteCode& vs,
            const ByteCode& gs,
            const ByteCode& ps,
            bool ConservativeRasterization,
            uint32_t NumRenderTargets,
            const ommGpuDescriptorRangeDesc* ranges, uint32_t numRanges)
        {
            ommGpuPipelineDesc desc;
            desc.type = ommGpuPipelineType_Graphics;
            desc.graphics.vertexShaderFileName = vs.name;
            desc.graphics.vertexShaderEntryPointName = vs.entryPoint;
            desc.graphics.vertexShader.data = vs.GetByteCode(_API);
            desc.graphics.vertexShader.size = vs.GetByteCodeSize(_API);
            desc.graphics.geometryShaderFileName = gs.name;
            desc.graphics.geometryShaderEntryPointName = gs.entryPoint;
            desc.graphics.geometryShader.data = gs.GetByteCode(_API);
            desc.graphics.geometryShader.size = gs.GetByteCodeSize(_API);
            desc.graphics.pixelShader.data = ps.GetByteCode(_API);
            desc.graphics.pixelShader.size = ps.GetByteCodeSize(_API);
            desc.graphics.pixelShaderFileName = ps.name;
            desc.graphics.pixelShaderEntryPointName = ps.entryPoint;
            desc.graphics.conservativeRasterization = ConservativeRasterization;
            desc.graphics.descriptorRanges = (const ommGpuDescriptorRangeDesc*)_ranges.size();
            desc.graphics.descriptorRangeNum = (uint32_t)numRanges;
            desc.graphics.numRenderTargets = NumRenderTargets;
            _pipelines.push_back(desc);

            _ranges.insert(_ranges.end(), ranges, ranges + numRanges);

            return (uint32_t)(_pipelines.size() - 1);
        }

        void Finalize()
        {
            for (ommGpuPipelineDesc& desc : _pipelines)
            {
                switch (desc.type)
                {
                case ommGpuPipelineType_Compute:
                {
                    desc.compute.descriptorRanges = &_ranges[(uint64_t)desc.compute.descriptorRanges];
                    break;
                }
                case ommGpuPipelineType_Graphics:
                {
                    desc.graphics.descriptorRanges = &_ranges[(uint64_t)desc.graphics.descriptorRanges];
                    break;
                }
                default:
                    OMM_ASSERT(false);
                    break;
                }
            }
            _desc.pipelineNum = (uint32_t)_pipelines.size();
            _desc.pipelines = _pipelines.data();
            _desc.staticSamplers = _staticSamplers.data();
            _desc.staticSamplersNum = (uint32_t)_staticSamplers.size();
            _desc.globalConstantBufferDesc.registerIndex = 0;
            _desc.globalConstantBufferDesc.maxDataSize = 2048;
            _desc.localConstantBufferDesc.registerIndex = 1;
            _desc.localConstantBufferDesc.maxDataSize = sizeof(uint32_t) * 8; // OOOH Hardcoded. Bad.
            _desc.descriptorSetDesc.constantBufferMaxNum = 1;
            _desc.descriptorSetDesc.storageBufferMaxNum = 10;
            _desc.descriptorSetDesc.descriptorRangeMaxNumPerPipeline = 3;
        }

        ommGpuRenderAPI _API = ommGpuRenderAPI_MAX_NUM;
        vector<ommGpuPipelineDesc> _pipelines;
        vector<ommGpuDescriptorRangeDesc> _ranges;
        vector<ommGpuStaticSamplerDesc> _staticSamplers;
        ommGpuPipelineInfoDesc _desc;
    };

    struct StringCache
    {
        static constexpr uint64_t ByteSize = 1024 << 10;

        StringCache(const StdAllocator<uint8_t>& stdAllocator)
            : _data(stdAllocator)
            , _cache(stdAllocator)
            , _offset(0)
        {
            _data.resize(ByteSize);
        }

        const char* GetString(const char* str)
        {
            uint64_t hash = hash_cstr(str);
            auto it = _cache.find(hash);

            if (it == _cache.end())
            {
                const char* cpy = AllocateAndCloneString(hash, str);
                it = _cache.insert(std::make_pair(hash, cpy)).first;
            }

            return it->second;
        }

    private:

        static uint64_t hash_cstr(const char* s)
        {
            return std::hash<std::string_view>()(std::string_view(s, std::strlen(s)));
        }

        char* AllocateString(size_t length)
        {
            OMM_ASSERT(_offset + length < _data.size());
            char* dst = (char*)_data.data() + _offset;
            _offset += (uint32_t)length;
            return dst;
        }

        const char* AllocateAndCloneString(uint64_t hash, const char* src)
        {
            size_t len = strlen(src) + 1;
            char* dst = AllocateString(len);
            return std::strcpy(dst, src);
        }

        hash_map<uint64_t, const char*> _cache;
        vector<uint8_t> _data;
        uint32_t _offset;
    };

    struct RuntimeBindingSet
    {
        RuntimeBindingSet(const StdAllocator<uint8_t>& stdAllocator, const ShaderBindings& binding, bool enableValidation)
        : _stdAllocator(stdAllocator)
        , _flattenedBindings(stdAllocator)
        , _flattenedResources(stdAllocator)
        , _bufferMappings(stdAllocator)
        , _resourceMappings(stdAllocator)
        , _mappings(stdAllocator)
        , _binding(binding)
        , _enableValidation(enableValidation)
        {
        }

        void Bind(const char* identifier, const BufferResource::SubRange& subRange)
        {
            if (_enableValidation && !_binding.HasSubResourceBinding(identifier))
            {
                OMM_ASSERT(!"*identifier* is not referenced by the shader");
            }

            _bufferMappings.insert(std::make_pair(std::make_pair(const_hash(identifier), identifier), subRange.resourceRef));
        }

        void Bind(const char* identifier, ommGpuResourceType type, uint32_t indexInPool)
        {
            if (_enableValidation && !_binding.HasResourceBinding(identifier))
            {
                OMM_ASSERT(!"*identifier* is not referenced by the shader");
            }

            _resourceMappings.insert(std::make_pair(std::make_pair(const_hash(identifier), identifier), std::make_pair(type, indexInPool)));
        }

        void Finalize()
        {
            if (_enableValidation)
            {
                for (uint i = 0; i < _binding.GetNumSubResources(); ++i)
                {
                    const SubResourceBinding& binding = _binding.GetSubResources()[i];

                    auto it = _bufferMappings.find(std::make_pair(binding.nameHash, binding.name));
                    if (it == _bufferMappings.end())
                    {
                        OMM_ASSERT(it != _bufferMappings.end() && "shader expects subresource *binding.name* to be bound.");
                    }
                } 
            }
            

            map<uint32_t, const BufferResource*> resourceMapping(_stdAllocator);

            {
                for (auto& it : _bufferMappings)
                {
                    ResourceBinding binding = _binding.GetSubResourceBinding(it.first.first);

                    auto resourceIt = resourceMapping.find(binding.registerIndex);
                    if (resourceIt == resourceMapping.end())
                    {
                        _flattenedBindings.push_back(std::make_pair(binding, it.second));
                        resourceMapping.insert(std::make_pair(binding.registerIndex, it.second));
                    }
                    else
                    {
                        // Validate that no subresources are conflicting in HLSL vs Cpp side.
                        if (_enableValidation && resourceIt->second != it.second)
                        {
                            std::string msg = "Subresource " + std::string(it.first.second) + " is allocated on resource " + std::string(it.second->debugName) +
                                " which is bound to shader resource " + std::string(binding.name) + ". However the resource " + resourceIt->second->debugName + " is already bound to " +
                                std::string(binding.name) + ". Resolve this inconsistency by changing the mapping in the .resources.hlsli";

                            OMM_ASSERT(resourceIt->second == it.second && !msg.c_str());
                        }
                    }
                }
            }

            {
                for (auto& it : _resourceMappings)
                {
                    if (_enableValidation && !_binding.HasResourceBinding(it.first.first))
                    {
                        std::string msg = "Resource " + std::string(it.first.second) + " is being bound but is not declared in .resources.hlsli.";
                        OMM_ASSERT(_binding.HasResourceBinding(it.first.first));
                    }

                    ResourceBinding binding = _binding.GetResourceBinding(it.first.first);
                    _flattenedResources.push_back(std::make_pair(binding, it.second));

                    resourceMapping.insert(std::make_pair(binding.registerIndex, nullptr /**/));
                }
            }

            OMM_ASSERT(_binding.GetNumResources() == (_flattenedResources.size() + _flattenedBindings.size()) &&
                "Shader has unbound resources!");
        }

        const std::pair<ResourceBinding, const BufferResource*>* GetBuffers() const { return _flattenedBindings.data(); }
        const uint32_t GetNumBuffers() const { return (uint32_t)_flattenedBindings.size(); }

        const std::pair<ResourceBinding, std::pair<ommGpuResourceType, uint32_t>>* GetResources() const { return _flattenedResources.data(); }
        const uint32_t GetNumResources() const { return (uint32_t)_flattenedResources.size(); }

    private:
        const StdAllocator<uint8_t>& _stdAllocator;
        vector<std::pair<ResourceBinding, const BufferResource*>> _flattenedBindings;
        vector<std::pair<ResourceBinding, std::pair<ommGpuResourceType, uint32_t>>> _flattenedResources;
        map<std::pair<uint32_t, const char*>, const BufferResource*> _bufferMappings;
        map<std::pair<uint32_t, const char*>, std::pair<ommGpuResourceType, uint32_t>> _resourceMappings;
        map<std::pair<uint32_t, const char*>, ommGpuResourceType> _mappings;
        const ShaderBindings& _binding;
        bool _enableValidation;
    };

    struct PassBuilder
    {
        struct PassConfig
        {
            PassConfig(const StdAllocator<uint8_t>& stdAllocator, ommGpuDispatchType type, const ShaderBindings& binding, bool enableValidation)
                : m_bindingSet(stdAllocator, binding, enableValidation)
                , m_subRange(stdAllocator)
                , m_localCb(m_cbuffer.data()
                , m_cbuffer.max_size())
            {
                std::memset(&m_desc, 0x0, sizeof(m_desc));
                m_desc.type = type;
            }

            void SetLabel(const char* label)
            {
                OMM_ASSERT(m_desc.type == ommGpuDispatchType_BeginLabel);
                m_desc.beginLabel.debugName = label;
            }

            bool IsGraphics() const {
                return m_desc.type == ommGpuDispatchType_DrawIndexedIndirect;
            }

            bool IsDrawIndirect() const {
                return m_desc.type == ommGpuDispatchType_DrawIndexedIndirect;
            }

            bool IsCompute() const {
                return m_desc.type == ommGpuDispatchType_Compute || m_desc.type == ommGpuDispatchType_ComputeIndirect;
            }

            bool IsComputeDirect() const {
                return m_desc.type == ommGpuDispatchType_Compute;
            }

            bool IsComputeIndirect() const {
                return m_desc.type == ommGpuDispatchType_ComputeIndirect;
            }

            void UseGlobalCbuffer() {
                m_useGlobalIndexBuffer = true;
            }

            void BindSubRange(const char* identifier, const BufferResource::SubRange& subRange)
            {
                OMM_ASSERT(subRange.IsValid());
                m_bindingSet.Bind(identifier, subRange);
            }

            void BindResource(const char* identifier, ommGpuResourceType type, uint32_t indexInPool = 0)
            {
                m_bindingSet.Bind(identifier, type, indexInPool);
            }

            void BindRawBufferWrite(ommGpuResourceType type, uint16_t indexInPool, uint32_t bindingSlot) {
                m_rawBufferWrite[m_rawBufferWriteCount++] = { {ommGpuDescriptorType_RawBufferWrite, type, (uint16_t)indexInPool, 0, 0}, bindingSlot };
            }

            void BindRawBufferWrite(BufferResource resource, uint32_t bindingSlot) {
                m_rawBufferWrite[m_rawBufferWriteCount++] = { {ommGpuDescriptorType_RawBufferWrite, resource.type, (uint16_t)resource.indexInPool, 0, 0}, bindingSlot };
            }

            void BindRawBufferRead(ommGpuResourceType type, uint16_t indexInPool, uint32_t bindingSlot) {
                m_rawBufferRead[m_rawBufferReadCount++] = { {ommGpuDescriptorType_RawBufferRead, type, (uint16_t)indexInPool, 0, 0}, bindingSlot };
            }

            void BindRawBufferRead(BufferResource resource, uint32_t bindingSlot) {
                m_rawBufferRead[m_rawBufferReadCount++] = { {ommGpuDescriptorType_RawBufferRead, resource.type, (uint16_t)resource.indexInPool, 0, 0}, bindingSlot };
            }

            void BindBufferRead(BufferResource resource, uint32_t bindingSlot) {
                m_bufferRead[m_bufferReadCount++] = { {ommGpuDescriptorType_BufferRead, resource.type, (uint16_t)resource.indexInPool, 0, 0}, bindingSlot };
            }

            void BindBufferRead(ommGpuResourceType type, uint16_t indexInPool, uint32_t bindingSlot) {
                m_bufferRead[m_bufferReadCount++] = { {ommGpuDescriptorType_BufferRead, type, (uint16_t)indexInPool, 0, 0}, bindingSlot };
            }

            void BindTextureRead(ommGpuResourceType type, uint16_t indexInPool, uint32_t bindingSlot) {
                m_textureRead[m_textureReadCount++] = { {ommGpuDescriptorType_TextureRead, type, (uint16_t)indexInPool, 0, 1}, bindingSlot };
            }

            void BindIB(ommGpuResourceType type, uint32_t offsetInBytes) {
                OMM_ASSERT(IsGraphics());
                m_desc.drawIndexedIndirect.indexBuffer.type = type;
                m_desc.drawIndexedIndirect.indexBuffer.indexInPool = 0;
                m_desc.drawIndexedIndirect.indexBufferOffset = offsetInBytes;
            }

            void BindVB(ommGpuResourceType type, uint32_t offsetInBytes) {
                OMM_ASSERT(IsGraphics());
                m_desc.drawIndexedIndirect.vertexBuffer.type = type;
                m_desc.drawIndexedIndirect.vertexBuffer.indexInPool = 0;
                m_desc.drawIndexedIndirect.vertexBufferOffset = offsetInBytes;
            }

            void SetViewport(float minWidth, float minHeight, float maxWidth, float maxHeight)
            {
                OMM_ASSERT(IsDrawIndirect());
                m_desc.drawIndexedIndirect.viewport = { minWidth, minHeight, maxWidth, maxHeight };
            }

            CBufferWriter& AddComputeDispatch(
                uint32_t pipelineIndex,
                uint32_t threadGroupCountX,
                uint32_t threadGroupCountY)
            {
                OMM_ASSERT(IsComputeDirect());
                m_desc.compute.pipelineIndex = pipelineIndex;
                m_desc.compute.gridWidth = threadGroupCountX;
                m_desc.compute.gridHeight = threadGroupCountY;
                return m_localCb;
            }

            CBufferWriter& AddComputeIndirect(
                uint32_t pipelineIndex,
                BufferResource::SubRange indirectArgBuffer,
                uint32_t indirectArgOffset)
            {
                OMM_ASSERT(IsComputeIndirect());
                m_desc.computeIndirect.pipelineIndex = pipelineIndex;
                m_desc.computeIndirect.indirectArg.stateNeeded = ommGpuDescriptorType_MAX_NUM;
                m_desc.computeIndirect.indirectArg.indexInPool = indirectArgBuffer.resourceRef->indexInPool;
                m_desc.computeIndirect.indirectArg.type = indirectArgBuffer.resourceRef->type;
                m_desc.computeIndirect.indirectArg.mipNum = 0;
                m_desc.computeIndirect.indirectArg.mipOffset = 0;
                m_desc.computeIndirect.indirectArgByteOffset = indirectArgBuffer.GetBufferOffset() + indirectArgOffset;
                return m_localCb;
            }

            CBufferWriter& AddDrawIndirect(
                uint32_t pipelineIndex,
                BufferResource::SubRange indirectArgBuffer,
                uint32_t indirectArgOffset)
            {
                OMM_ASSERT(IsDrawIndirect());
                m_desc.drawIndexedIndirect.pipelineIndex = pipelineIndex;
                m_desc.drawIndexedIndirect.indirectArg.stateNeeded = ommGpuDescriptorType_MAX_NUM;
                m_desc.drawIndexedIndirect.indirectArg.indexInPool = indirectArgBuffer.resourceRef->indexInPool;
                m_desc.drawIndexedIndirect.indirectArg.type = indirectArgBuffer.resourceRef->type;
                m_desc.drawIndexedIndirect.indirectArg.mipNum = 0;
                m_desc.drawIndexedIndirect.indirectArg.mipOffset = 0;
                m_desc.drawIndexedIndirect.indirectArgByteOffset = indirectArgBuffer.GetBufferOffset() + indirectArgOffset;
                return m_localCb;
            }

            vector<std::pair<uint32_t, BufferResource::SubRange>> m_subRange;

            std::array<std::pair<ommGpuResource, uint32_t>, 4> m_textureRead;
            uint32_t m_textureReadCount = 0;
            std::array<std::pair<ommGpuResource, uint32_t>, 32> m_rawBufferRead;
            uint32_t m_rawBufferReadCount = 0;
            std::array<std::pair<ommGpuResource, uint32_t>, 32> m_rawBufferWrite;
            uint32_t m_rawBufferWriteCount = 0;
            std::array<std::pair<ommGpuResource, uint32_t>, 32> m_bufferRead;
            uint32_t m_bufferReadCount = 0;

            RuntimeBindingSet m_bindingSet;
            std::array<uint8_t, 32> m_cbuffer;
            CBufferWriter m_localCb;

            bool m_useGlobalIndexBuffer = false;
            ommGpuDispatchDesc m_desc;
        };

        void ValidateDescRanges(const PassConfig& cfg, uint32_t pipelineIndex)
        {
            const ommGpuPipelineDesc& pipeline = _pipelines.GetPipeline(pipelineIndex);

            const ommGpuDescriptorRangeDesc* descriptorRanges = nullptr;
            uint32_t descriptorRangeNum = 0;
            switch (pipeline.type)
            {
            case ommGpuPipelineType_Compute:
            {
                descriptorRanges = pipeline.compute.descriptorRanges;
                descriptorRangeNum = pipeline.compute.descriptorRangeNum;
                break;
            }
            case ommGpuPipelineType_Graphics:
            {
                descriptorRanges = pipeline.graphics.descriptorRanges;
                descriptorRangeNum = pipeline.graphics.descriptorRangeNum;
                break;
            }
            default:
            {
                OMM_ASSERT(false);
                break;
            }
            }

            /*
                Don't worry! Thes asserts are likely to assert when adding or removing inputs to render passes!
                It only means you need to update the pipeline ranges accordingly :) 
            */

            for (uint32_t i = 0; i < descriptorRangeNum; ++i)
            {
                const ommGpuDescriptorRangeDesc& range = descriptorRanges[i];
                if (range.descriptorType == ommGpuDescriptorType_TextureRead)
                    OMM_ASSERT(range.descriptorNum == cfg.m_textureReadCount);
                if (range.descriptorType == ommGpuDescriptorType_RawBufferRead)
                    OMM_ASSERT(range.descriptorNum == cfg.m_rawBufferReadCount);
                if (range.descriptorType == ommGpuDescriptorType_RawBufferWrite)
                    OMM_ASSERT(range.descriptorNum == cfg.m_rawBufferWriteCount);
                if (range.descriptorType == ommGpuDescriptorType_BufferRead)
                    OMM_ASSERT(range.descriptorNum == cfg.m_bufferReadCount);
            }
        }

        PassBuilder(const StdAllocator<uint8_t>& stdAllocator, const PipelineBuilder& pipelines, StringCache& strings)
            : _stdAllocator(stdAllocator)
            , _dispatches(stdAllocator)
            , _resources(stdAllocator)
            , _localCbufferData(stdAllocator)
            , _globalCbufferData(stdAllocator)
            , _pipelines(pipelines)
            , _strings(strings)
        { }

        void SetGlobalCbuffer(const uint8_t* cbuffer, size_t size)
        {
            OMM_ASSERT(_globalCbufferData.size() == 0);
            _globalCbufferData.insert(_globalCbufferData.end(), cbuffer, cbuffer + size);
        }

        void PushPass(const char* pass, ommGpuDispatchType type, const ShaderBindings& binding, std::function<void(PassConfig&)> fillConfigCb, bool enableValidation) {
            PassConfig dt(_stdAllocator, type, binding, enableValidation);
            fillConfigCb(dt);
            // Copy to shared memory structs etc.
            FinalizePass(pass, dt);
        }

        void PushPass(const char* pass, PassConfig& dt) {
            // Copy to shared memory structs etc.
            FinalizePass(pass, dt);
        }

        template<typename... TArgs>
        void PushLabel(const char* fmt, TArgs&&... args) {
            const uint32_t kBufferSize = 128;
            char buffer[kBufferSize];
            snprintf(buffer, kBufferSize, fmt, std::forward<TArgs>(args)...);
            PushLabel(buffer);
        }

        void PushLabel(const char* label) {
            ommGpuDispatchDesc desc;
            desc.type = ommGpuDispatchType_BeginLabel;
            desc.beginLabel.debugName = _strings.GetString(label);
            _dispatches.push_back(desc);
        }

        void PopLabel() {
            ommGpuDispatchDesc desc;
            desc.type = ommGpuDispatchType_EndLabel;
            _dispatches.push_back(desc);
        }

        void Reset()
        {
            _dispatches.clear();
            _resources.clear();
            _localCbufferData.clear();
            _globalCbufferData.clear();
            _result.dispatches = nullptr;
            _result.numDispatches = 0;
            _result.globalCBufferData = nullptr;
            _result.globalCBufferDataSize = 0;
        }

        void FinalizePass(const char* pass, PassConfig& cfg)
        {
            cfg.m_bindingSet.Finalize();

            for (uint32_t i = 0; i < cfg.m_bindingSet.GetNumBuffers(); ++i)
            {
                const auto& [binding, resource] = cfg.m_bindingSet.GetBuffers()[i];

                switch (binding.type) {
                case HLSLResourceType::Buffer: {
                    cfg.BindBufferRead(*resource, binding.registerIndex);
                    break;
                }
                case HLSLResourceType::ByteAddressBuffer: {
                    cfg.BindRawBufferRead(*resource, binding.registerIndex);
                    break;
                }
                case HLSLResourceType::RWByteAddressBuffer: {
                    cfg.BindRawBufferWrite(*resource, binding.registerIndex);
                    break;
                }
                case HLSLResourceType::Texture2D: {
                    OMM_ASSERT(false);
                    break;
                }
                }
            }

            for (uint32_t i = 0; i < cfg.m_bindingSet.GetNumResources(); ++i)
            {
                const auto& [binding, resource] = cfg.m_bindingSet.GetResources()[i];

                switch (binding.type) {
                case HLSLResourceType::Buffer: {
                    cfg.BindBufferRead(resource.first, resource.second, binding.registerIndex);
                    break;
                }
                case HLSLResourceType::ByteAddressBuffer: {
                    cfg.BindRawBufferRead(resource.first, resource.second, binding.registerIndex);
                    break;
                }
                case HLSLResourceType::RWByteAddressBuffer: {
                    cfg.BindRawBufferWrite(resource.first, resource.second, binding.registerIndex);
                    break;
                }
                case HLSLResourceType::Texture2D: {
                    cfg.BindTextureRead(resource.first, resource.second, binding.registerIndex);
                    break;
                }
                }
            }

            ommGpuDispatchDesc dispatch = cfg.m_desc;

            size_t resourcesStart = _resources.size();
            size_t resourceNum = (size_t)cfg.m_textureReadCount + cfg.m_rawBufferReadCount + cfg.m_rawBufferWriteCount + cfg.m_bufferReadCount;

            auto SortFn = [](const std::pair<ommGpuResource, uint32_t>& a, const std::pair<ommGpuResource, uint32_t>& b) {
                return a.second < b.second;
            };

            std::sort(cfg.m_textureRead.begin(), cfg.m_textureRead.begin() + cfg.m_textureReadCount, SortFn);
            std::sort(cfg.m_bufferRead.begin(), cfg.m_bufferRead.begin() + cfg.m_bufferReadCount, SortFn);
            std::sort(cfg.m_rawBufferRead.begin(), cfg.m_rawBufferRead.begin() + cfg.m_rawBufferReadCount, SortFn);
            std::sort(cfg.m_rawBufferWrite.begin(), cfg.m_rawBufferWrite.begin() + cfg.m_rawBufferWriteCount, SortFn);

            for (uint32_t i = 0; i < cfg.m_textureReadCount; ++i)
                _resources.push_back(cfg.m_textureRead[i].first);
            for (uint32_t i = 0; i < cfg.m_bufferReadCount; ++i)
                _resources.push_back(cfg.m_bufferRead[i].first);
            for (uint32_t i = 0; i < cfg.m_rawBufferReadCount; ++i)
                _resources.push_back(cfg.m_rawBufferRead[i].first);
            for (uint32_t i = 0; i < cfg.m_rawBufferWriteCount; ++i)
                _resources.push_back(cfg.m_rawBufferWrite[i].first);

            size_t localCbStart = 0;
            if (cfg.m_localCb.GetSize() != 0)
            {
                localCbStart = _localCbufferData.size();
                _localCbufferData.insert(std::end(_localCbufferData), cfg.m_cbuffer.data(), cfg.m_cbuffer.data() + cfg.m_localCb.GetSize());
            }

            switch (dispatch.type)
            {
            case ommGpuDispatchType_Compute:
            {
                ommGpuComputeDesc& compute = dispatch.compute;
                compute.name = pass;
                compute.resources = (const ommGpuResource*)resourcesStart;
                compute.resourceNum = (uint32_t)resourceNum;
                compute.localConstantBufferData = (const uint8_t*)localCbStart;
                compute.localConstantBufferDataSize = (uint32_t)cfg.m_localCb.GetSize();

                ValidateDescRanges(cfg, compute.pipelineIndex);

                break;
            }
            case ommGpuDispatchType_ComputeIndirect:
            {
                ommGpuComputeIndirectDesc& compute = dispatch.computeIndirect;
                compute.name = pass;
                compute.resources = (const ommGpuResource*)resourcesStart;
                compute.resourceNum = (uint32_t)resourceNum;
                compute.localConstantBufferData = (const uint8_t*)localCbStart;
                compute.localConstantBufferDataSize = (uint32_t)cfg.m_localCb.GetSize();

                ValidateDescRanges(cfg, compute.pipelineIndex);

                break;
            }
            case ommGpuDispatchType_DrawIndexedIndirect:
            {
                ommGpuDrawIndexedIndirectDesc& draw = dispatch.drawIndexedIndirect;
                draw.name = pass;
                draw.resources = (const ommGpuResource*)resourcesStart;
                draw.resourceNum = (uint32_t)resourceNum;
                draw.localConstantBufferData = (const uint8_t*)localCbStart;
                draw.localConstantBufferDataSize = (uint32_t)cfg.m_localCb.GetSize();

                ValidateDescRanges(cfg, draw.pipelineIndex);

                break;
            }
            }

            _dispatches.push_back(dispatch);
        }

        void Finalize()
        {
            for (ommGpuDispatchDesc& desc : _dispatches)
            {
                switch (desc.type)
                {
                case ommGpuDispatchType_Compute:
                {
                    ommGpuComputeDesc& compute = desc.compute;
                    compute.resources = &_resources[reinterpret_cast<uint64_t>(compute.resources)];
                    compute.localConstantBufferData = &_localCbufferData[reinterpret_cast<uint64_t>(compute.localConstantBufferData)];
                    break;
                }
                case ommGpuDispatchType_ComputeIndirect:
                {
                    ommGpuComputeIndirectDesc& compute = desc.computeIndirect;
                    compute.resources = &_resources[reinterpret_cast<uint64_t>(compute.resources)];
                    compute.localConstantBufferData = &_localCbufferData[reinterpret_cast<uint64_t>(compute.localConstantBufferData)];
                    break;
                }
                case ommGpuDispatchType_DrawIndexedIndirect:
                {
                    ommGpuDrawIndexedIndirectDesc& draw = desc.drawIndexedIndirect;
                    draw.resources = &_resources[reinterpret_cast<uint64_t>(draw.resources)];
                    draw.localConstantBufferData = &_localCbufferData[reinterpret_cast<uint64_t>(draw.localConstantBufferData)];
                    break;
                }
                }
            }

            _result.dispatches = _dispatches.data();
            _result.numDispatches = (uint32_t)_dispatches.size();
            _result.globalCBufferData = _globalCbufferData.data();
            _result.globalCBufferDataSize = (uint32_t)_globalCbufferData.size();
        }

        const StdAllocator<uint8_t>& _stdAllocator;
        const PipelineBuilder& _pipelines;
        vector<ommGpuDispatchDesc> _dispatches;
        vector<ommGpuResource> _resources;
        vector<uint8_t> _localCbufferData;
        vector<uint8_t> _globalCbufferData;
        ommGpuDispatchChain _result;
        StringCache& _strings;
    };

    struct OmmStaticBuffers
    {
        static ommResult GetStaticResourceData(ommGpuResourceType resource, uint8_t* data, size_t* byteSize);
    };

    class PipelineImpl
    {
        // Internal
    public:

        static constexpr HandleType kHandleType = HandleType::Pipeline;

        PipelineImpl(const StdAllocator<uint8_t>& stdAllocator, const Logger& log)
            : m_stdAllocator(stdAllocator)
            , m_log(log)
            , m_scratchBufferDescs(stdAllocator)
            , m_pipelineBuilder(stdAllocator)
            , m_strings(stdAllocator)
            , m_passBuilder(stdAllocator, m_pipelineBuilder, m_strings)
            , m_pipelines(stdAllocator)
        {}

        ~PipelineImpl();

        inline StdAllocator<uint8_t>& GetStdAllocator()
        {
            return m_stdAllocator;
        }
        static ommResult Validate(const ommGpuPipelineConfigDesc& config);
        ommResult Validate(const ommGpuDispatchConfigDesc& config) const;
        ommResult Create(const ommGpuPipelineConfigDesc& config);
        ommResult GetPipelineDesc(const ommGpuPipelineInfoDesc** outPipelineDesc);
        ommResult GetPreDispatchInfo(const ommGpuDispatchConfigDesc& config, ommGpuPreDispatchInfo* outPreBuildInfo) const;
        ommResult GetDispatchDesc(const ommGpuDispatchConfigDesc& dispatchConfig, const ommGpuDispatchChain** outDispatchDesc);

    private:

        static constexpr uint32_t kHashTableEntrySize = sizeof(uint32_t) * 2;
        static constexpr int32_t kViewportScale = 5; // Increasing this to 6 fails... TODO: investigate why!
        static constexpr int32_t kIndirectDispatchEntryStride = 64; // The stride of consecutive dispatch indirect buffer entries

        ommGpuPipelineConfigDesc m_config;
        StdAllocator<uint8_t> m_stdAllocator;
        const Logger& m_log;
        vector<ommGpuBufferDesc> m_scratchBufferDescs;
        PipelineBuilder m_pipelineBuilder;
        StringCache m_strings;
        PassBuilder m_passBuilder;

        struct Pipelines
        {
            omm_clear_buffer_cs_bindings ommClearBufferBindings;
            uint32_t ommClearBufferIdx = -1;

            omm_render_target_clear_bindings ommRenderTargetClearDebugBindings;
            uint32_t ommRenderTargetClearDebugIdx = -1;

            omm_init_buffers_cs_cs_bindings ommInitBuffersCsBindings;
            uint32_t ommInitBuffersCsIdx = -1;

            omm_init_buffers_gfx_cs_bindings ommInitBuffersGfxBindings;
            uint32_t ommInitBuffersGfxIdx = -1;

            omm_post_build_info_cs_bindings ommPostBuildInfoBindings;
            uint32_t ommPostBuildInfoBuffersIdx = -1;

            omm_work_setup_cs_cs_bindings ommWorkSetupCsBindings;
            uint32_t ommWorkSetupCsIdx = -1;

            omm_work_setup_bake_only_cs_cs_bindings ommWorkSetupBakeOnlyCsBindings;
            uint32_t ommWorkSetupBakeOnlyCsIdx = -1;

            omm_work_setup_gfx_cs_bindings ommWorkSetupGfxBindings;
            uint32_t ommWorkSetupGfxIdx = -1;

            omm_work_setup_bake_only_gfx_cs_bindings ommWorkSetupBakeOnlyGfxBindings;
            uint32_t ommWorkSetupBakeOnlyGfxIdx = -1;

            omm_rasterize_bindings ommRasterizeBindings;
            uint32_t ommRasterizeRIdx = -1;
            uint32_t ommRasterizeGIdx = -1;
            uint32_t ommRasterizeBIdx = -1;
            uint32_t ommRasterizeAIdx = -1;

            omm_rasterize_cs_bindings ommRasterizeCsBindings;
            uint32_t ommRasterizeRCsIdx = -1;
            uint32_t ommRasterizeGCsIdx = -1;
            uint32_t ommRasterizeBCsIdx = -1;
            uint32_t ommRasterizeACsIdx = -1;

            omm_compress_cs_bindings ommCompressBindings;
            uint32_t ommCompressIdx = -1;

            omm_desc_patch_cs_bindings ommDescPatchBindings;
            uint32_t ommDescPatchIdx = -1;

            omm_index_write_cs_bindings ommIndexWriteBindings;
            uint32_t ommIndexWriteIdx = -1;

            omm_rasterize_bindings ommRasterizeDebugBindings;
            uint32_t ommRasterizeDebugIdx = -1;

            Pipelines(const StdAllocator<uint8_t>& stdAllocator)
            : ommClearBufferBindings(stdAllocator)
            , ommRenderTargetClearDebugBindings(stdAllocator)
            , ommInitBuffersCsBindings(stdAllocator)
            , ommInitBuffersGfxBindings(stdAllocator)
            , ommPostBuildInfoBindings(stdAllocator)
            , ommWorkSetupBakeOnlyCsBindings(stdAllocator)
            , ommWorkSetupCsBindings(stdAllocator)
            , ommWorkSetupGfxBindings(stdAllocator)
            , ommWorkSetupBakeOnlyGfxBindings(stdAllocator)
            , ommRasterizeBindings(stdAllocator)
            , ommRasterizeCsBindings(stdAllocator)
            , ommCompressBindings(stdAllocator)
            , ommDescPatchBindings(stdAllocator)
            , ommIndexWriteBindings(stdAllocator)
            , ommRasterizeDebugBindings(stdAllocator)
            {
            }

        } m_pipelines;

        struct PreDispatchInfo
        {
            BufferResource scratchBuffer;
            BufferResource scratchBuffer0;
            BufferResource indArgBuffer;
            BufferResource debugBuffer; // Contains asserts.

            BufferResource::SubRange hashTableBuffer;
            BufferResource::SubRange tempOmmIndexBuffer;
            BufferResource::SubRange tempOmmBakeScheduleTrackerBuffer;
            BufferResource::SubRange bakeResultBufferCounter;
            BufferResource::SubRange ommArrayAllocatorCounter;
            BufferResource::SubRange ommDescAllocatorCounter;
            BufferResource::SubRange dispatchIndirectThreadCounter;
            BufferResource::SubRange rasterItemsBuffer;
            BufferResource::SubRange specialIndicesStateBuffer;
            BufferResource::SubRange bakeResultBuffer;
            BufferResource::SubRange IEBakeBuffer;
            BufferResource::SubRange IEBakeCsBuffer;
            BufferResource::SubRange IECompressCsBuffer;
            BufferResource::SubRange IEBakeCsThreadCountBuffer;

            BufferResource::SubRange assertBuffer;

            uint32_t numTransientPoolBuffers = 0;
            uint32_t MaxBatchCount = 0;
            bool MayContain4StateFormats = false;
        };
        ommResult ConfigurePipeline(const ommGpuPipelineConfigDesc& config);
        ommResult GetPreDispatchInfo(const ommGpuDispatchConfigDesc& config, PreDispatchInfo& outInfo) const;
        ommResult InitGlobalConstants(const ommGpuDispatchConfigDesc& config, const PreDispatchInfo& info, GlobalConstants& cbuffer) const;
    };

    class BakerImpl
    {
    // Internal
    public:
        static inline constexpr HandleType kHandleType = HandleType::GpuBaker;

        inline BakerImpl(const StdAllocator<uint8_t>& stdAllocator) :
            m_stdAllocator(stdAllocator)
        {}

        ~BakerImpl();

        inline StdAllocator<uint8_t>& GetStdAllocator()
        { return m_stdAllocator; }
        inline const Logger& GetLog() const
        { return m_log; }

        ommResult Create(const ommBakerCreationDesc& bakeCreationDesc);

        ommResult CreatePipeline(const ommGpuPipelineConfigDesc& config, ommGpuPipeline* outPipeline);
        ommResult DestroyPipeline(ommGpuPipeline pipeline);

    private:
        StdAllocator<uint8_t> m_stdAllocator;
        Logger m_log;
        ommBakerCreationDesc m_bakeCreationDesc;
    };
} // namespace Gpu
} // namespace omm
