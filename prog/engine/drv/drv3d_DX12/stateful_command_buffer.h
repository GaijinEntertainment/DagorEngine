// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "command_list.h"
#include "constants.h"
#include "d3d12_utils.h"
#include "driver.h"
#include "pipeline_cache.h"
#include "render_state.h"
#include "validation.h"
#include "versioned_com_ptr.h"


namespace drv3d_dx12
{
#if _TARGET_PC_WIN
typedef VersionedComPtr<D3DGraphicsCommandList, ID3D12GraphicsCommandList4, ID3D12GraphicsCommandList5, ID3D12GraphicsCommandList6>
  AnyCommandListComPtr;
typedef VersionedPtr<D3DGraphicsCommandList, ID3D12GraphicsCommandList4, ID3D12GraphicsCommandList5, ID3D12GraphicsCommandList6>
  AnyCommandListPtr;
#else
typedef VersionedComPtr<D3DGraphicsCommandList> AnyCommandListComPtr;
typedef VersionedPtr<D3DGraphicsCommandList> AnyCommandListPtr;
#endif

class StatefulCommandBuffer
{
  struct ShaderStageState
  {
    D3D12_GPU_VIRTUAL_ADDRESS cbvRange[dxil::MAX_B_REGISTERS] = {};
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    D3D12_GPU_DESCRIPTOR_HANDLE cbvExtraRange = {};
#endif
    D3D12_GPU_DESCRIPTOR_HANDLE samplerRange = {};
    D3D12_GPU_DESCRIPTOR_HANDLE srvRange = {};
    D3D12_GPU_DESCRIPTOR_HANDLE uavRange = {};
    uint32_t rootConstants[MAX_ROOT_CONSTANTS] = {};
  };
#if D3D_HAS_RAY_TRACING
  struct RaytraceState
  {
    struct DirtySet
    {
      enum Bits
      {
        SAMPLER_DESCRIPTORS,
        SRV_DESCRIPTORS,
        UAV_DESCRIPTORS,
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
        CBV_DESCRIPTORS,
#endif
        PIPELINE,
        // TODO if setting object will also set signature, then this is not needed
        SIGNATURE,
        COUNT,
        INVALID = COUNT
      };
      typedef Bitset<COUNT> type;
    };
    void markChange(uint32_t index, bool change)
    {
      DirtySet::type dif;
      dif.set(index, change);
      dirtyState |= dif;
    }
    void clearChange(uint32_t index) { dirtyState.set(index, false); }
    void dirtyAll() { dirtyState = ~RaytraceState::DirtySet::type(0); }
    DirtySet::type dirtyState = DirtySet::type(0);
    Bitset<dxil::MAX_B_REGISTERS> constBufferDirtyState = {};
    Bitset<MAX_ROOT_CONSTANTS> rootConstantDirtyState = {};
    ShaderStageState raygenShader;
    ID3D12StateObject *pipeline = nullptr;
    const RaytracePipelineSignature *rootSignature = nullptr;
  };
#endif
  struct ComputeState
  {
    struct DirtySet
    {
      enum Bits
      {
        SAMPLER_DESCRIPTORS,
        SRV_DESCRIPTORS,
        UAV_DESCRIPTORS,
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
        CBV_DESCRIPTORS,
#endif
        PIPELINE,
        SIGNATURE,
        COUNT,
        INVALID = COUNT
      };
      typedef Bitset<COUNT> type;
    };
    void markChange(uint32_t index, bool change)
    {
      DirtySet::type dif;
      dif.set(index, change);
      dirtyState |= dif;
    }
    void clearChange(uint32_t index) { dirtyState.set(index, false); }
    void dirtyAll() { dirtyState = ~ComputeState::DirtySet::type(0); }
    DirtySet::type dirtyState = DirtySet::type(0);
    Bitset<dxil::MAX_B_REGISTERS> constBufferDirtyState = {};
    Bitset<MAX_ROOT_CONSTANTS> rootConstantDirtyState = {};
    ShaderStageState computeShader;
    ID3D12PipelineState *pipeline = nullptr;
    ComputePipelineSignature *rootSignature = nullptr;
  };
  struct GraphicsState
  {
    struct DynamicState
    {
      float depthBoundsFrom = 0.f;
      float depthBoundsTo = 1.f;
      float blendConstantFactor[4] = {};
      D3D12_RECT scissorRects[Viewport::MAX_VIEWPORT_COUNT] = {};
      uint32_t scissorCount = 0;
      D3D12_VIEWPORT viewports[Viewport::MAX_VIEWPORT_COUNT] = {};
      uint32_t viewportCount = 0;
      uint32_t stencilReference = 0;
      D3D12_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    };
    struct RenderPassState
    {
      D3D12_CPU_DESCRIPTOR_HANDLE rtv[Driver3dRenderTarget::MAX_SIMRT] = {};
      D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
      D3D12_RECT area = {};
      uint8_t rtvCount = 0;
    };
    struct DirtySet
    {
      enum Bits
      {
        RENDER_TARGETS,
        DEPTH_BOUNDS,
        BLEND_CONSTANT_FACTOR,
        SCISSOR_RECT,
        VIEWPORT,
        STENCIL_REFERENCE,
        VERTEX_SAMPLER_DESCRIPTORS,
        VERTEX_SRV_DESCRIPTORS,
        VERTEX_UAV_DESCRIPTORS,
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
        VERTEX_CBV_DESCRIPTORS,
#endif
        PIXEL_SAMPLER_DESCRIPTORS,
        PIXEL_SRV_DESCRIPTORS,
        PIXEL_UAV_DESCRIPTORS,
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
        PIXEL_CBV_DESCRIPTORS,
#endif
        PIPELINE,
        SIGNATURE,
        PRIMITIVE_TOPOLOGY,
        SHADING_RATE,
        SHADING_RATE_TEXTURE,
        COUNT,
        INVALID = COUNT
      };
      typedef Bitset<COUNT> type;
    };
    void markChange(uint32_t index, bool change)
    {
      DirtySet::type dif;
      dif.set(index, change);
      dirtyState |= dif;
    }
    void clearChange(uint32_t index) { dirtyState.set(index, false); }
    void dirtyAll() { dirtyState.set(); }
    DirtySet::type dirtyState = DirtySet::type(0);
    Bitset<dxil::MAX_B_REGISTERS> vertexConstBufferDirtyState = {};
    Bitset<dxil::MAX_B_REGISTERS> pixelConstBufferDirtyState = {};
    Bitset<MAX_ROOT_CONSTANTS> vertexRootConstantDirtyState = {};
    Bitset<MAX_ROOT_CONSTANTS> pixelRootConstantDirtyState = {};
    ShaderStageState vertexShader;
    ShaderStageState pixelShader;
    ID3D12PipelineState *pipeline = nullptr;
    GraphicsPipelineSignature *rootSignature = nullptr;
    PipelineOptionalDynamicStateMask pipelineDynamicStateUse{};
    DynamicState dynamicState;
    RenderPassState renderPassState;
#if !_TARGET_XBOXONE
    D3D12_SHADING_RATE constantShadingRate = D3D12_SHADING_RATE_1X1;
    D3D12_SHADING_RATE_COMBINER shadingRateCombiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] = {};
    ID3D12Resource *shadingRateTexture = nullptr;
#endif
  };
  struct GlobalState
  {
    struct DirtySet
    {
      enum Bits
      {
        DESCRIPTOR_HEAPS,

        COUNT,
        INVALID = COUNT
      };
      typedef Bitset<COUNT> type;
    };
    DirtySet::type dirtyState = DirtySet::type(0);
    // only two, uav/srv and sampler
    ID3D12DescriptorHeap *descriptorHeaps[2] = {};
    D3D12_GPU_DESCRIPTOR_HANDLE bindlessHeapAddress[2]{};
    D3D12_GPU_DESCRIPTOR_HANDLE activeBindlessHeapAddress[2]{};

    void markChange(uint32_t index, bool change)
    {
      DirtySet::type dif;
      dif.set(index, change);
      dirtyState |= dif;
    }
    void clearChange(uint32_t index) { dirtyState.set(index, false); }
    void dirtyAll() { dirtyState = ~DirtySet::type(0); }

    void resetActiveBindlessAddresses()
    {
      eastl::fill(eastl::begin(activeBindlessHeapAddress), eastl::end(activeBindlessHeapAddress), D3D12_GPU_DESCRIPTOR_HANDLE{0});
    }

    bool needsBindlessSamplerSet() const { return bindlessHeapAddress[0] != activeBindlessHeapAddress[0]; }

    void bindlessSamplerSetWasApplied() { activeBindlessHeapAddress[0] = bindlessHeapAddress[0]; }

    bool needsBindlessSRVSet() const { return bindlessHeapAddress[1] != activeBindlessHeapAddress[1]; }

    void bindlessSRVSetWasApplied() { activeBindlessHeapAddress[1] = bindlessHeapAddress[1]; }
  };
  GraphicsCommandList<AnyCommandListPtr> cmd;

  struct FeatureState
  {
    enum
    {
      ALLOW_DEPTH_BOUNDS,
#if !_TARGET_XBOXONE
      ALLOW_SHADING_RATE_T1,
      ALLOW_SHADING_RATE_T2,
#endif
      PIPELINE_HAS_GEOMETRY,
      PIPELINE_HAS_TESSELLATION,
      COUNT,
      INVALID = COUNT,
    };
    typedef Bitset<COUNT> Type;
  };
  GraphicsState graphicsState;
  ComputeState computeState;
#if D3D_HAS_RAY_TRACING
  RaytraceState raytraceState;
#endif
  GlobalState globalState;
  ID3D12PipelineState *activeEmbeddedPipeline = nullptr;
  FeatureState::Type activeFeatures = {};

  bool validateTopology()
  {
    // only validate if any debugging is allowed, otherwise just crash
#if DAGOR_DBGLEVEL > 0
    if (!validate_primitive_topology(graphicsState.dynamicState.top, activeFeatures.test(FeatureState::PIPELINE_HAS_GEOMETRY),
          activeFeatures.test(FeatureState::PIPELINE_HAS_TESSELLATION)))
    {
      D3D_ERROR("Invalid primitive topology %u for draw call encountered. Geometry stage active %s, "
                "Tessellation stage active %s.",
        (int)graphicsState.dynamicState.top, activeFeatures.test(FeatureState::PIPELINE_HAS_GEOMETRY) ? "yes" : "no",
        activeFeatures.test(FeatureState::PIPELINE_HAS_TESSELLATION) ? "yes" : "no");
      return false;
    }
#endif
    return true;
  }

  bool canDraw() { return nullptr != graphicsState.pipeline; }
  bool canDispatch() { return nullptr != computeState.pipeline; }

  enum class NeedSamplerHeap
  {
    No,
    Yes
  };
  void flushGlobalState(NeedSamplerHeap need_sampler_heap = NeedSamplerHeap::Yes)
  {
    if (globalState.dirtyState.test(GlobalState::DirtySet::DESCRIPTOR_HEAPS))
    {
      if (globalState.descriptorHeaps[0] && globalState.descriptorHeaps[1])
        cmd.setDescriptorHeaps(2, globalState.descriptorHeaps);
      else
      {
        if (need_sampler_heap == NeedSamplerHeap::Yes && globalState.descriptorHeaps[0] == nullptr)
          logwarn("The sampler heap is not set.");
        if (globalState.descriptorHeaps[1] == nullptr)
          logwarn("The resource heap is not set.");

        if (globalState.descriptorHeaps[0])
          cmd.setDescriptorHeaps(1, &globalState.descriptorHeaps[0]);
        if (globalState.descriptorHeaps[1])
          cmd.setDescriptorHeaps(1, &globalState.descriptorHeaps[1]);
      }
    }
    globalState.dirtyState.reset();
  }

public:
  void clearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE view, const FLOAT color[4], UINT rect_count, const D3D12_RECT *rects)
  {
    cmd.clearRenderTargetView(view, color, rect_count, rects);
  }
  void resolveSubresource(ID3D12Resource *dst_resource, UINT dst_subresource, ID3D12Resource *src_resource, UINT src_subresource,
    DXGI_FORMAT format)
  {
    cmd.resolveSubresource(dst_resource, dst_subresource, src_resource, src_subresource, format);
  }
  void clearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE view, D3D12_CLEAR_FLAGS flags, FLOAT d, UINT8 s, UINT rect_count,
    const D3D12_RECT *rects)
  {
    cmd.clearDepthStencilView(view, flags, d, s, rect_count, rects);
  }
  void beginQuery(ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, UINT index) { cmd.beginQuery(heap, type, index); }
  void endQuery(ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, UINT index) { cmd.endQuery(heap, type, index); }
  void resolveQueryData(ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, UINT index, UINT count, ID3D12Resource *target, UINT64 offset)
  {
    cmd.resolveQueryData(heap, type, index, count, target, offset);
  }
  void clearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc, ID3D12Resource *res,
    const FLOAT values[4], UINT num_rects, const D3D12_RECT *rects)
  {
    flushGlobalState(NeedSamplerHeap::No);
    cmd.clearUnorderedAccessViewFloat(gpu_desc, cpu_desc, res, values, num_rects, rects);
  }
  void clearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc, ID3D12Resource *res,
    const UINT values[4], UINT num_rects, const D3D12_RECT *rects)
  {
    flushGlobalState(NeedSamplerHeap::No);
    cmd.clearUnorderedAccessViewUint(gpu_desc, cpu_desc, res, values, num_rects, rects);
  }
  void copyResource(ID3D12Resource *dst, ID3D12Resource *src) { cmd.copyResource(dst, src); }

  void copyBuffer(ID3D12Resource *dst, UINT64 dst_offset, ID3D12Resource *src, UINT64 src_offset, UINT64 size)
  {
    cmd.copyBufferRegion(dst, dst_offset, src, src_offset, size);
  }
  void copyTexture(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT dx, UINT dy, UINT dz, const D3D12_TEXTURE_COPY_LOCATION *src,
    const D3D12_BOX *sb = nullptr)
  {
    cmd.copyTextureRegion(dst, dx, dy, dz, src, sb);
  }
  void barriers(uint32_t count, const D3D12_RESOURCE_BARRIER *barriers) { cmd.resourceBarrier(count, barriers); }
#if D3D_HAS_RAY_TRACING
  void buildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC *desc,
    uint32_t num_post_build_info_descs, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *post_build_info_descs)
  {
    G_ASSERTF(cmd.is<ID3D12GraphicsCommandList4>(), "Trying to execute raytrace commands on unsupported command list version");
    cmd.buildRaytracingAccelerationStructure(desc, num_post_build_info_descs, post_build_info_descs);
  }
#if _TARGET_XBOX
#include "stateful_command_buffer_xbox.inc.h"
#endif
  void copyRaytracingAccelerationStructure(D3D12_GPU_VIRTUAL_ADDRESS dst, D3D12_GPU_VIRTUAL_ADDRESS src,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode)
  {
    G_ASSERTF(cmd.is<ID3D12GraphicsCommandList4>(), "Trying to execute raytrace commands on unsupported command list version");
    cmd.copyRaytracingAccelerationStructure(dst, src, mode);
  }
#endif
  void setSamplerHeap(ID3D12DescriptorHeap *heap, D3D12_GPU_DESCRIPTOR_HANDLE binldess_address)
  {
    globalState.markChange(GlobalState::DirtySet::DESCRIPTOR_HEAPS, globalState.descriptorHeaps[0] != heap);
    globalState.descriptorHeaps[0] = heap;
    globalState.bindlessHeapAddress[0] = binldess_address;
  }
  void setResourceHeap(ID3D12DescriptorHeap *heap, D3D12_GPU_DESCRIPTOR_HANDLE binldess_address)
  {
    globalState.markChange(GlobalState::DirtySet::DESCRIPTOR_HEAPS, globalState.descriptorHeaps[1] != heap);
    globalState.descriptorHeaps[1] = heap;
    globalState.bindlessHeapAddress[1] = binldess_address;
  }
  bool areDescriptorHeapsDirty() { return globalState.dirtyState.test(GlobalState::DirtySet::DESCRIPTOR_HEAPS); }
  void resetForFrameStart() { globalState.descriptorHeaps[0] = globalState.descriptorHeaps[1] = nullptr; }
  D3DGraphicsCommandList *getHandle() { return cmd.get(); }

  void dirtyAll()
  {
    globalState.dirtyAll();
    computeState.dirtyAll();
    graphicsState.dirtyAll();
#if D3D_HAS_RAY_TRACING
    raytraceState.dirtyAll();
#endif
  }

  void prepareBufferForSubmit()
  {
    dirtyAll();
    activeEmbeddedPipeline = nullptr;
  }

  const GraphicsCommandList<AnyCommandListPtr> &getBufferWrapper() const { return cmd; }

  void resetBuffer() { cmd.reset(); }

  bool isReadyForRecording() const { return static_cast<bool>(cmd); }

  void makeReadyForRecording(AnyCommandListPtr c, bool allow_depth_bounds, unsigned shading_rate_tier)
  {
    cmd = c;
    activeFeatures.set(FeatureState::ALLOW_DEPTH_BOUNDS, allow_depth_bounds);
#if _TARGET_XBOXONE
    G_UNUSED(shading_rate_tier);
#else
    activeFeatures.set(FeatureState::ALLOW_SHADING_RATE_T1, shading_rate_tier > 0);
    activeFeatures.set(FeatureState::ALLOW_SHADING_RATE_T2, shading_rate_tier > 1);
#endif
  }

  void writeMarker(D3D12_GPU_VIRTUAL_ADDRESS target, uint32_t value, D3D12_WRITEBUFFERIMMEDIATE_MODE mode)
  {
    G_UNUSED(target);
    G_UNUSED(value);
    G_UNUSED(mode);
#if _TARGET_PC_WIN
    D3D12_WRITEBUFFERIMMEDIATE_PARAMETER writeInfo{target, value};
    cmd.writeBufferImmediate(1, &writeInfo, &mode);
#endif
  }

#if D3D_HAS_RAY_TRACING
  void bindRaytracePipeline(const RaytracePipelineSignature *signature, ID3D12StateObject *pipeline)
  {
    raytraceState.markChange(RaytraceState::DirtySet::PIPELINE, raytraceState.pipeline != pipeline);
    raytraceState.markChange(RaytraceState::DirtySet::SIGNATURE, raytraceState.rootSignature != signature);
    raytraceState.pipeline = pipeline;
    raytraceState.rootSignature = signature;
  }

  void setRaytraceConstantBuffer(uint32_t slot, D3D12_GPU_VIRTUAL_ADDRESS cbv)
  {
    raytraceState.constBufferDirtyState.set(slot, raytraceState.raygenShader.cbvRange[slot] != cbv);
    raytraceState.raygenShader.cbvRange[slot] = cbv;
  }

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  void setRaytraceConstantBufferDescriptors(D3D12_GPU_DESCRIPTOR_HANDLE ptr)
  {
    raytraceState.markChange(ComputeState::DirtySet::CBV_DESCRIPTORS, raytraceState.raygenShader.cbvExtraRange != ptr);
    raytraceState.raygenShader.cbvExtraRange = ptr;
  }
#endif

  void setRaytraceSamplers(D3D12_GPU_DESCRIPTOR_HANDLE samplers)
  {
    raytraceState.markChange(ComputeState::DirtySet::SAMPLER_DESCRIPTORS, raytraceState.raygenShader.samplerRange != samplers);
    raytraceState.raygenShader.samplerRange = samplers;
  }

  void setRaytraceUAVs(D3D12_GPU_DESCRIPTOR_HANDLE uavs)
  {
    raytraceState.markChange(ComputeState::DirtySet::UAV_DESCRIPTORS, raytraceState.raygenShader.uavRange != uavs);
    raytraceState.raygenShader.uavRange = uavs;
  }

  void setRaytraceSRVs(D3D12_GPU_DESCRIPTOR_HANDLE srvs)
  {
    raytraceState.markChange(ComputeState::DirtySet::SRV_DESCRIPTORS, raytraceState.raygenShader.srvRange != srvs);

    raytraceState.raygenShader.srvRange = srvs;
  }

  void updateRaytraceRootConstant(uint32_t offset, uint32_t value)
  {
    or_bit(raytraceState.rootConstantDirtyState, offset, raytraceState.raygenShader.rootConstants[offset] != value);
    raytraceState.raygenShader.rootConstants[offset] = value;
  }

  void flushRaytrace()
  {
    G_ASSERTF(cmd.is<ID3D12GraphicsCommandList4>(), "Trying to execute raytrace commands on unsupported command list version");
    flushGlobalState();

    // only one pipeline can be active at a time
    graphicsState.dirtyState.set(GraphicsState::DirtySet::PIPELINE);
    computeState.dirtyState.set(ComputeState::DirtySet::PIPELINE);
    // global raytrace signature slot is the same as the compute one
    computeState.dirtyState.set(ComputeState::DirtySet::SIGNATURE);

    if (raytraceState.dirtyState.test(RaytraceState::DirtySet::PIPELINE))
    {
      cmd.setPipelineState1(raytraceState.pipeline);
      activeEmbeddedPipeline = nullptr;
    }

    if (raytraceState.dirtyState.test(RaytraceState::DirtySet::SIGNATURE))
    {
      cmd.setComputeRootSignature(raytraceState.rootSignature->signature.Get());
      // force update of descriptors
      raytraceState.constBufferDirtyState.set();
      raytraceState.rootConstantDirtyState.set();
      raytraceState.dirtyState.set(RaytraceState::DirtySet::SAMPLER_DESCRIPTORS);
      raytraceState.dirtyState.set(RaytraceState::DirtySet::SRV_DESCRIPTORS);
      raytraceState.dirtyState.set(RaytraceState::DirtySet::UAV_DESCRIPTORS);
      globalState.resetActiveBindlessAddresses();
    }
    if (raytraceState.rootConstantDirtyState.any())
    {
      raytraceState.rootSignature->updateRootConstants(cmd, raytraceState.raygenShader.rootConstants,
        raytraceState.rootConstantDirtyState);
      raytraceState.rootConstantDirtyState.reset();
    }
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    bool cbvDescriptorsDirty = raytraceState.dirtyState.test(RaytraceState::DirtySet::CBV_DESCRIPTORS);
    D3D12_GPU_DESCRIPTOR_HANDLE cbvDescriptors = raytraceState.raygenShader.cbvExtraRange;
#else
    constexpr bool cbvDescriptorsDirty = false;
    constexpr D3D12_GPU_DESCRIPTOR_HANDLE cbvDescriptors{};
#endif
    if (raytraceState.constBufferDirtyState.any() || cbvDescriptorsDirty)
    {
      // const buffer array is compacted
      raytraceState.rootSignature->updateCBVRange(cmd, raytraceState.raygenShader.cbvRange, cbvDescriptors,
        raytraceState.constBufferDirtyState, cbvDescriptorsDirty, true);
      raytraceState.constBufferDirtyState.reset();
    }

    if (raytraceState.dirtyState.test(RaytraceState::DirtySet::SRV_DESCRIPTORS))
    {
      raytraceState.rootSignature->updateSRVRange(cmd, raytraceState.raygenShader.srvRange);
    }
    if (raytraceState.dirtyState.test(RaytraceState::DirtySet::UAV_DESCRIPTORS))
    {
      raytraceState.rootSignature->updateUAVRange(cmd, raytraceState.raygenShader.uavRange);
    }
    if (raytraceState.dirtyState.test(RaytraceState::DirtySet::SAMPLER_DESCRIPTORS))
    {
      raytraceState.rootSignature->updateSamplerRange(cmd, raytraceState.raygenShader.samplerRange);
    }
    if (globalState.needsBindlessSamplerSet())
    {
      raytraceState.rootSignature->updateBindlessSamplerRange(cmd, globalState.bindlessHeapAddress[0]);
      globalState.bindlessSamplerSetWasApplied();
    }
    if (globalState.needsBindlessSRVSet())
    {
      raytraceState.rootSignature->updateUnboundedSRVRanges(cmd, globalState.bindlessHeapAddress[1]);
      globalState.bindlessSRVSetWasApplied();
    }

    raytraceState.dirtyState.reset();
  }

  void traceRays(D3D12_GPU_VIRTUAL_ADDRESS ray_gen, uint32_t ray_gen_size, D3D12_GPU_VIRTUAL_ADDRESS miss, uint32_t miss_size,
    uint32_t miss_stride, D3D12_GPU_VIRTUAL_ADDRESS hit, uint32_t hit_size, uint32_t hit_stride, D3D12_GPU_VIRTUAL_ADDRESS call,
    uint32_t call_size, uint32_t call_stride, UINT width, UINT hegiht, UINT depth)
  {
    D3D12_DISPATCH_RAYS_DESC def = //
      {{ray_gen, ray_gen_size}, {miss, miss_size, miss_stride}, {hit, hit_size, hit_stride}, {call, call_size, call_stride}, width,
        hegiht, depth};
    G_ASSERTF(cmd.is<ID3D12GraphicsCommandList4>(), "Trying to execute raytrace commands on unsupported command list version");
    flushRaytrace();
    cmd.dispatchRays(&def);
  }

  void dispatchaysIndirect(ID3D12CommandSignature *signature, ID3D12Resource *args_buffer, uint64_t args_offset,
    ID3D12Resource *count_buffer, uint64_t count_offset, uint32_t max_count)
  {
    G_ASSERTF(cmd.is<ID3D12GraphicsCommandList4>(), "Trying to execute raytrace commands on unsupported command list version");
    flushRaytrace();
    cmd.executeIndirect(signature, max_count, args_buffer, args_offset, count_buffer, count_offset);
  }
#endif

  void bindComputePipeline(ComputePipelineSignature *signature, ID3D12PipelineState *pipeline)
  {
    computeState.markChange(ComputeState::DirtySet::PIPELINE, computeState.pipeline != pipeline);
    computeState.markChange(ComputeState::DirtySet::SIGNATURE, computeState.rootSignature != signature);
    computeState.pipeline = pipeline;
    computeState.rootSignature = signature;
  }

  void setComputeConstantBuffer(uint32_t slot, D3D12_GPU_VIRTUAL_ADDRESS cbv)
  {
    computeState.constBufferDirtyState.set(slot, computeState.computeShader.cbvRange[slot] != cbv);
    computeState.computeShader.cbvRange[slot] = cbv;
  }

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  void setComputeConstantBufferDescriptors(D3D12_GPU_DESCRIPTOR_HANDLE ptr)
  {
    computeState.markChange(ComputeState::DirtySet::CBV_DESCRIPTORS, computeState.computeShader.cbvExtraRange != ptr);
    computeState.computeShader.cbvExtraRange = ptr;
  }
#endif

  void setComputeSamplers(D3D12_GPU_DESCRIPTOR_HANDLE samplers)
  {
    computeState.markChange(ComputeState::DirtySet::SAMPLER_DESCRIPTORS, computeState.computeShader.samplerRange != samplers);
    computeState.computeShader.samplerRange = samplers;
  }

  void setComputeUAVs(D3D12_GPU_DESCRIPTOR_HANDLE uavs)
  {
    computeState.markChange(ComputeState::DirtySet::UAV_DESCRIPTORS, computeState.computeShader.uavRange != uavs);
    computeState.computeShader.uavRange = uavs;
  }

  void setComputeSRVs(D3D12_GPU_DESCRIPTOR_HANDLE srvs)
  {
    computeState.markChange(ComputeState::DirtySet::SRV_DESCRIPTORS, computeState.computeShader.srvRange != srvs);

    computeState.computeShader.srvRange = srvs;
  }

  void updateComputeRootConstant(uint32_t offset, uint32_t value)
  {
    or_bit(computeState.rootConstantDirtyState, offset, computeState.computeShader.rootConstants[offset] != value);
    computeState.computeShader.rootConstants[offset] = value;
  }

  void flushCompute()
  {
    flushGlobalState();

    // only one pipeline can be active at a time
    graphicsState.dirtyState.set(GraphicsState::DirtySet::PIPELINE);
#if D3D_HAS_RAY_TRACING
    raytraceState.dirtyState.set(RaytraceState::DirtySet::PIPELINE);
    // global raytrace signature slot is the same as the compute one
    raytraceState.dirtyState.set(RaytraceState::DirtySet::SIGNATURE);
#endif

    if (computeState.dirtyState.test(ComputeState::DirtySet::PIPELINE))
    {
      cmd.setPipelineState(computeState.pipeline);
      activeEmbeddedPipeline = nullptr;
    }
    if (computeState.dirtyState.test(ComputeState::DirtySet::SIGNATURE))
    {
      cmd.setComputeRootSignature(computeState.rootSignature->signature.Get());
      // force update of descriptors
      computeState.constBufferDirtyState.set();
      computeState.rootConstantDirtyState.set();
      computeState.dirtyState.set(ComputeState::DirtySet::SAMPLER_DESCRIPTORS);
      computeState.dirtyState.set(ComputeState::DirtySet::SRV_DESCRIPTORS);
      computeState.dirtyState.set(ComputeState::DirtySet::UAV_DESCRIPTORS);
      globalState.resetActiveBindlessAddresses();
    }
    if (computeState.rootConstantDirtyState.any())
    {
      computeState.rootSignature->updateRootConstants(cmd, computeState.computeShader.rootConstants,
        computeState.rootConstantDirtyState);
      computeState.rootConstantDirtyState.reset();
    }
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    bool cbvDescriptorsDirty = computeState.dirtyState.test(ComputeState::DirtySet::CBV_DESCRIPTORS);
    D3D12_GPU_DESCRIPTOR_HANDLE cbvDescriptors = computeState.computeShader.cbvExtraRange;
#else
    constexpr bool cbvDescriptorsDirty = false;
    constexpr D3D12_GPU_DESCRIPTOR_HANDLE cbvDescriptors{};
#endif
    if (computeState.constBufferDirtyState.any() || cbvDescriptorsDirty)
    {
      // const buffer array is not compacted (eg slots match the usage mask)
      computeState.rootSignature->updateCBVRange(cmd, computeState.computeShader.cbvRange, cbvDescriptors,
        computeState.constBufferDirtyState, cbvDescriptorsDirty, false);
      computeState.constBufferDirtyState.reset();
    }
    if (computeState.dirtyState.test(ComputeState::DirtySet::SRV_DESCRIPTORS))
    {
      computeState.rootSignature->updateSRVRange(cmd, computeState.computeShader.srvRange);
    }
    if (computeState.dirtyState.test(ComputeState::DirtySet::UAV_DESCRIPTORS))
    {
      computeState.rootSignature->updateUAVRange(cmd, computeState.computeShader.uavRange);
    }
    if (computeState.dirtyState.test(ComputeState::DirtySet::SAMPLER_DESCRIPTORS))
    {
      computeState.rootSignature->updateSamplerRange(cmd, computeState.computeShader.samplerRange);
    }
    if (globalState.needsBindlessSamplerSet())
    {
      computeState.rootSignature->updateBindlessSamplerRange(cmd, globalState.bindlessHeapAddress[0]);
      globalState.bindlessSamplerSetWasApplied();
    }
    if (globalState.needsBindlessSRVSet())
    {
      computeState.rootSignature->updateUnboundedSRVRanges(cmd, globalState.bindlessHeapAddress[1]);
      globalState.bindlessSRVSetWasApplied();
    }
    computeState.dirtyState.reset();
  }

  void dispatch(uint32_t x, uint32_t y, uint32_t z)
  {
    if (!canDispatch())
      return;

    flushCompute();
    cmd.dispatch(x, y, z);
  }

  void dispatchIndirect(ID3D12CommandSignature *signature, ID3D12Resource *buffer, uint64_t offset)
  {
    if (!canDispatch())
      return;

    flushCompute();
    cmd.executeIndirect(signature, 1, buffer, offset, nullptr, 0);
  }

  void bindGraphicsPipeline(GraphicsPipelineSignature *signature, ID3D12PipelineState *pipeline, bool has_gs, bool has_ts,
    PipelineOptionalDynamicStateMask dyn_use)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PIPELINE, graphicsState.pipeline != pipeline);
    graphicsState.markChange(GraphicsState::DirtySet::SIGNATURE, graphicsState.rootSignature != signature);
    graphicsState.pipeline = pipeline;
    graphicsState.rootSignature = signature;
    graphicsState.pipelineDynamicStateUse = dyn_use;
    activeFeatures.set(FeatureState::PIPELINE_HAS_GEOMETRY, has_gs);
    activeFeatures.set(FeatureState::PIPELINE_HAS_TESSELLATION, has_ts);
  }

  void bindGraphicsMeshPipeline(GraphicsPipelineSignature *signature, ID3D12PipelineState *pipeline,
    PipelineOptionalDynamicStateMask dyn_use)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PIPELINE, graphicsState.pipeline != pipeline);
    graphicsState.markChange(GraphicsState::DirtySet::SIGNATURE, graphicsState.rootSignature != signature);
    graphicsState.pipeline = pipeline;
    graphicsState.rootSignature = signature;
    graphicsState.pipelineDynamicStateUse = dyn_use;
    activeFeatures.set(FeatureState::PIPELINE_HAS_GEOMETRY, false);
    activeFeatures.set(FeatureState::PIPELINE_HAS_TESSELLATION, false);
  }

  void setConstantBuffer(uint32_t stage, uint32_t slot, D3D12_GPU_VIRTUAL_ADDRESS cbv)
  {
    if (STAGE_VS == stage)
    {
      setVertexConstantBuffer(slot, cbv);
    }
    else if (STAGE_PS == stage)
    {
      setPixelConstantBuffer(slot, cbv);
    }
    else if (STAGE_CS == stage)
    {
      setComputeConstantBuffer(slot, cbv);
    }
#if D3D_HAS_RAY_TRACING
    // TODO: NYI
    // else if (STAGE_RAYTRACE == stage)
    // {
    // }
#endif
    else
    {
      G_ASSERT(!"DX12: Invalid shader stage");
    }
  }

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  void setVertexConstantBufferDescriptors(D3D12_GPU_DESCRIPTOR_HANDLE ptr)
  {
    graphicsState.markChange(GraphicsState::DirtySet::VERTEX_CBV_DESCRIPTORS, graphicsState.vertexShader.cbvExtraRange != ptr);
    graphicsState.vertexShader.cbvExtraRange = ptr;
  }

  void setPixelConstantBufferDescriptors(D3D12_GPU_DESCRIPTOR_HANDLE ptr)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PIXEL_CBV_DESCRIPTORS, graphicsState.pixelShader.cbvExtraRange != ptr);
    graphicsState.pixelShader.cbvExtraRange = ptr;
  }
#endif

  void setVertexConstantBuffer(uint32_t slot, D3D12_GPU_VIRTUAL_ADDRESS cbv)
  {
    graphicsState.vertexConstBufferDirtyState.set(slot, graphicsState.vertexShader.cbvRange[slot] != cbv);
    graphicsState.vertexShader.cbvRange[slot] = cbv;
  }

  void setVertexSamplers(D3D12_GPU_DESCRIPTOR_HANDLE samplers)
  {
    graphicsState.markChange(GraphicsState::DirtySet::VERTEX_SAMPLER_DESCRIPTORS, graphicsState.vertexShader.samplerRange != samplers);
    graphicsState.vertexShader.samplerRange = samplers;
  }

  void setVertexUAVs(D3D12_GPU_DESCRIPTOR_HANDLE uavs)
  {
    graphicsState.markChange(GraphicsState::DirtySet::VERTEX_UAV_DESCRIPTORS, graphicsState.vertexShader.uavRange != uavs);
    graphicsState.vertexShader.uavRange = uavs;
  }

  void setVertexSRVs(D3D12_GPU_DESCRIPTOR_HANDLE srvs)
  {
    graphicsState.markChange(GraphicsState::DirtySet::VERTEX_SRV_DESCRIPTORS, graphicsState.vertexShader.srvRange != srvs);
    graphicsState.vertexShader.srvRange = srvs;
  }

  void setPixelConstantBuffer(uint32_t slot, D3D12_GPU_VIRTUAL_ADDRESS cbv)
  {
    graphicsState.pixelConstBufferDirtyState.set(slot, graphicsState.pixelShader.cbvRange[slot] != cbv);
    graphicsState.pixelShader.cbvRange[slot] = cbv;
  }

  void setPixelSamplers(D3D12_GPU_DESCRIPTOR_HANDLE samplers)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PIXEL_SAMPLER_DESCRIPTORS, graphicsState.pixelShader.samplerRange != samplers);

    graphicsState.pixelShader.samplerRange = samplers;
  }

  void setPixelUAVs(D3D12_GPU_DESCRIPTOR_HANDLE uavs)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PIXEL_UAV_DESCRIPTORS, graphicsState.pixelShader.uavRange != uavs);
    graphicsState.pixelShader.uavRange = uavs;
  }

  void setPixelSRVs(D3D12_GPU_DESCRIPTOR_HANDLE srvs)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PIXEL_SRV_DESCRIPTORS, graphicsState.pixelShader.srvRange != srvs);
    graphicsState.pixelShader.srvRange = srvs;
  }

  void updateVertexRootConstant(uint32_t offset, uint32_t value)
  {
    or_bit(graphicsState.vertexRootConstantDirtyState, offset, graphicsState.vertexShader.rootConstants[offset] != value);
    graphicsState.vertexShader.rootConstants[offset] = value;
  }

  void updatePixelRootConstant(uint32_t offset, uint32_t value)
  {
    or_bit(graphicsState.pixelRootConstantDirtyState, offset, graphicsState.pixelShader.rootConstants[offset] != value);
    graphicsState.pixelShader.rootConstants[offset] = value;
  }

  void setStencilReference(uint32_t value)
  {
    graphicsState.markChange(GraphicsState::DirtySet::STENCIL_REFERENCE, graphicsState.dynamicState.stencilReference != value);
    graphicsState.dynamicState.stencilReference = value;
  }
  void setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY top)
  {
    graphicsState.markChange(GraphicsState::DirtySet::PRIMITIVE_TOPOLOGY, graphicsState.dynamicState.top != top);
    graphicsState.dynamicState.top = top;
  }

  D3D12_PRIMITIVE_TOPOLOGY getPrimitiveTopology() const { return graphicsState.dynamicState.top; }

  void setDepthBoundsRange(float from, float to)
  {
    bool change = graphicsState.dynamicState.depthBoundsFrom != from;
    change = change || graphicsState.dynamicState.depthBoundsTo != to;
    graphicsState.markChange(GraphicsState::DirtySet::DEPTH_BOUNDS, change);
    graphicsState.dynamicState.depthBoundsFrom = from;
    graphicsState.dynamicState.depthBoundsTo = to;
  }

  void setBlendConstantFactor(const float *values)
  {
    bool change = false;
    for (uint32_t i = 0; i < 4; ++i)
    {
      change = change || graphicsState.dynamicState.blendConstantFactor[i] != values[i];
      graphicsState.dynamicState.blendConstantFactor[i] = values[i];
    }
    graphicsState.markChange(GraphicsState::DirtySet::BLEND_CONSTANT_FACTOR, change);
  }

  void setScissorRects(dag::ConstSpan<D3D12_RECT> rects)
  {
    if (rects.size() != graphicsState.dynamicState.scissorCount ||
        !eastl::equal(rects.begin(), rects.end(), graphicsState.dynamicState.scissorRects))
    {
      graphicsState.markChange(GraphicsState::DirtySet::SCISSOR_RECT, true);
      eastl::copy_n(rects.begin(), rects.size(), graphicsState.dynamicState.scissorRects);
      graphicsState.dynamicState.scissorCount = rects.size();
    }
  }

  void setViewports(dag::ConstSpan<D3D12_VIEWPORT> viewports)
  {
    if (viewports.size() != graphicsState.dynamicState.viewportCount ||
        !eastl::equal(viewports.begin(), viewports.end(), graphicsState.dynamicState.viewports))
    {
      graphicsState.markChange(GraphicsState::DirtySet::VIEWPORT, true);
      eastl::copy_n(viewports.begin(), viewports.size(), graphicsState.dynamicState.viewports);
      graphicsState.dynamicState.viewportCount = viewports.size();
    }
  }

  void setRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[Driver3dRenderTarget::MAX_SIMRT], uint8_t rtv_count,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv)
  {
    bool change = graphicsState.renderPassState.rtvCount != rtv_count;
    change = change || graphicsState.renderPassState.dsv != dsv;
    if (!change)
    {
      change = !eastl::equal(eastl::begin(graphicsState.renderPassState.rtv), eastl::end(graphicsState.renderPassState.rtv), rtvs);
    }
    if (change)
    {
      graphicsState.markChange(GraphicsState::DirtySet::RENDER_TARGETS, true);

      eastl::copy(rtvs, rtvs + Driver3dRenderTarget::MAX_SIMRT, graphicsState.renderPassState.rtv);
      graphicsState.renderPassState.dsv = dsv;
      graphicsState.renderPassState.rtvCount = rtv_count;
    }
  }

  void flushRenderTargets()
  {
    if (!graphicsState.dirtyState.test(GraphicsState::DirtySet::RENDER_TARGETS))
      return;
    cmd.omSetRenderTargets(graphicsState.renderPassState.rtvCount, graphicsState.renderPassState.rtv, FALSE,
      graphicsState.renderPassState.dsv.ptr ? &graphicsState.renderPassState.dsv : nullptr);
    graphicsState.dirtyState.reset(GraphicsState::DirtySet::RENDER_TARGETS);
#if _TARGET_XBOX
    graphicsState.markChange(GraphicsState::DirtySet::DEPTH_BOUNDS,
      graphicsState.renderPassState.dsv.ptr &&
        (graphicsState.dynamicState.depthBoundsFrom != 0.0f || graphicsState.dynamicState.depthBoundsTo != 1.0f));
#endif
  }

  enum class PrimitivePipeline
  {
    VERTEX,
    MESH,
  };

  void flushGraphics(PrimitivePipeline prim_pipe)
  {
    flushGlobalState();

    flushRenderTargets();

    // this mask is used to mask off dirty bits of features the device does not support
#if _TARGET_PC_WIN
    GraphicsState::DirtySet::type unsupportedMask;
    unsupportedMask.set(GraphicsState::DirtySet::DEPTH_BOUNDS, !activeFeatures.test(FeatureState::ALLOW_DEPTH_BOUNDS));

    unsupportedMask.set(GraphicsState::DirtySet::SHADING_RATE, !activeFeatures.test(FeatureState::ALLOW_SHADING_RATE_T1));
    unsupportedMask.set(GraphicsState::DirtySet::SHADING_RATE_TEXTURE, !activeFeatures.test(FeatureState::ALLOW_SHADING_RATE_T2));

    graphicsState.dirtyState &= ~unsupportedMask;
#endif
    GraphicsState::DirtySet::type igonreMask;
    igonreMask.set(GraphicsState::DirtySet::DEPTH_BOUNDS, 0 == graphicsState.pipelineDynamicStateUse.hasDepthBoundsTest);
    igonreMask.set(GraphicsState::DirtySet::STENCIL_REFERENCE, 0 == graphicsState.pipelineDynamicStateUse.hasStencilTest);
    igonreMask.set(GraphicsState::DirtySet::BLEND_CONSTANT_FACTOR, 0 == graphicsState.pipelineDynamicStateUse.hasBlendConstants);
    if (PrimitivePipeline::MESH == prim_pipe)
    {
      igonreMask.set(GraphicsState::DirtySet::PRIMITIVE_TOPOLOGY);
    }

    auto graphicsDirtyState = graphicsState.dirtyState & ~igonreMask;

    if (!graphicsDirtyState.any() && !graphicsState.vertexConstBufferDirtyState.any() &&
        !graphicsState.pixelConstBufferDirtyState.any() && !graphicsState.vertexRootConstantDirtyState.any() &&
        !graphicsState.pixelRootConstantDirtyState.any() && !globalState.needsBindlessSamplerSet() &&
        !globalState.needsBindlessSRVSet())
      return;

    // only one pipeline can be active at a time
    computeState.dirtyState.set(ComputeState::DirtySet::PIPELINE);
#if D3D_HAS_RAY_TRACING
    raytraceState.dirtyState.set(RaytraceState::DirtySet::PIPELINE);
#endif

    if (graphicsDirtyState.test(GraphicsState::DirtySet::PIPELINE))
    {
      cmd.setPipelineState(graphicsState.pipeline);
      activeEmbeddedPipeline = nullptr;
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::SIGNATURE))
    {
      cmd.setGraphicsRootSignature(graphicsState.rootSignature->signature.Get());

      graphicsState.vertexConstBufferDirtyState.set();
      graphicsState.pixelConstBufferDirtyState.set();
      graphicsState.vertexRootConstantDirtyState.set();
      graphicsState.pixelRootConstantDirtyState.set();
      graphicsDirtyState.set(GraphicsState::DirtySet::VERTEX_SAMPLER_DESCRIPTORS);
      graphicsDirtyState.set(GraphicsState::DirtySet::VERTEX_SRV_DESCRIPTORS);
      graphicsDirtyState.set(GraphicsState::DirtySet::VERTEX_UAV_DESCRIPTORS);
      graphicsDirtyState.set(GraphicsState::DirtySet::PIXEL_SAMPLER_DESCRIPTORS);
      graphicsDirtyState.set(GraphicsState::DirtySet::PIXEL_SRV_DESCRIPTORS);
      graphicsDirtyState.set(GraphicsState::DirtySet::PIXEL_UAV_DESCRIPTORS);
      globalState.resetActiveBindlessAddresses();
    }
    if (graphicsState.dirtyState.test(GraphicsState::DirtySet::PRIMITIVE_TOPOLOGY))
    {
      cmd.iaSetPrimitiveTopology(graphicsState.dynamicState.top);
    }
    if (graphicsState.vertexRootConstantDirtyState.any())
    {
      graphicsState.rootSignature->updateVertexRootConstants(cmd, graphicsState.vertexShader.rootConstants,
        graphicsState.vertexRootConstantDirtyState);
      graphicsState.vertexRootConstantDirtyState.reset();
    }
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    bool vertexCBVDescriptorsDirty = graphicsState.dirtyState.test(GraphicsState::DirtySet::VERTEX_CBV_DESCRIPTORS);
    D3D12_GPU_DESCRIPTOR_HANDLE vertexCBVDescriptors = graphicsState.vertexShader.cbvExtraRange;
#else
    constexpr bool vertexCBVDescriptorsDirty = false;
    constexpr D3D12_GPU_DESCRIPTOR_HANDLE vertexCBVDescriptors{};
#endif
    if (graphicsState.vertexConstBufferDirtyState.any() || vertexCBVDescriptorsDirty)
    {
      graphicsState.rootSignature->updateVertexCBVRange(cmd, graphicsState.vertexShader.cbvRange, vertexCBVDescriptors,
        graphicsState.vertexConstBufferDirtyState, vertexCBVDescriptorsDirty);
      graphicsState.vertexConstBufferDirtyState.reset();
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::VERTEX_SRV_DESCRIPTORS))
    {
      graphicsState.rootSignature->updateVertexSRVRange(cmd, graphicsState.vertexShader.srvRange);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::VERTEX_UAV_DESCRIPTORS))
    {
      graphicsState.rootSignature->updateVertexUAVRange(cmd, graphicsState.vertexShader.uavRange);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::VERTEX_SAMPLER_DESCRIPTORS))
    {
      graphicsState.rootSignature->updateVertexSamplerRange(cmd, graphicsState.vertexShader.samplerRange);
    }
    if (graphicsState.pixelRootConstantDirtyState.any())
    {
      graphicsState.rootSignature->updatePixelRootConstants(cmd, graphicsState.pixelShader.rootConstants,
        graphicsState.pixelRootConstantDirtyState);
      graphicsState.pixelRootConstantDirtyState.reset();
    }
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    bool pixelCBVDescriptorsDirty = graphicsState.dirtyState.test(GraphicsState::DirtySet::PIXEL_CBV_DESCRIPTORS);
    D3D12_GPU_DESCRIPTOR_HANDLE pixelCBVDescriptors = graphicsState.pixelShader.cbvExtraRange;
#else
    constexpr bool pixelCBVDescriptorsDirty = false;
    constexpr D3D12_GPU_DESCRIPTOR_HANDLE pixelCBVDescriptors{};
#endif
    if (graphicsState.pixelConstBufferDirtyState.any() || pixelCBVDescriptorsDirty)
    {
      graphicsState.rootSignature->updatePixelCBVRange(cmd, graphicsState.pixelShader.cbvRange, pixelCBVDescriptors,
        graphicsState.pixelConstBufferDirtyState, pixelCBVDescriptorsDirty);
      graphicsState.pixelConstBufferDirtyState.reset();
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::PIXEL_SRV_DESCRIPTORS))
    {
      graphicsState.rootSignature->updatePixelSRVRange(cmd, graphicsState.pixelShader.srvRange);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::PIXEL_UAV_DESCRIPTORS))
    {
      graphicsState.rootSignature->updatePixelUAVRange(cmd, graphicsState.pixelShader.uavRange);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::PIXEL_SAMPLER_DESCRIPTORS))
    {
      graphicsState.rootSignature->updatePixelSamplerRange(cmd, graphicsState.pixelShader.samplerRange);
    }

    if (globalState.needsBindlessSamplerSet())
    {
      graphicsState.rootSignature->updateBindlessSamplerRange(cmd, globalState.bindlessHeapAddress[0]);
      globalState.bindlessSamplerSetWasApplied();
    }
    if (globalState.needsBindlessSRVSet())
    {
      graphicsState.rootSignature->updateBindlessSRVRange(cmd, globalState.bindlessHeapAddress[1]);
      globalState.bindlessSRVSetWasApplied();
    }

    if (graphicsDirtyState.test(GraphicsState::DirtySet::DEPTH_BOUNDS))
    {
      cmd.omSetDepthBounds(graphicsState.dynamicState.depthBoundsFrom, graphicsState.dynamicState.depthBoundsTo);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::BLEND_CONSTANT_FACTOR))
    {
      cmd.omSetBlendFactor(graphicsState.dynamicState.blendConstantFactor);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::SCISSOR_RECT))
    {
      cmd.rsSetScissorRects(graphicsState.dynamicState.scissorCount, graphicsState.dynamicState.scissorRects);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::VIEWPORT))
    {
      cmd.rsSetViewports(graphicsState.dynamicState.viewportCount, graphicsState.dynamicState.viewports);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::STENCIL_REFERENCE))
    {
      cmd.omSetStencilRef(graphicsState.dynamicState.stencilReference);
    }
#if !_TARGET_XBOXONE
    if (graphicsDirtyState.test(GraphicsState::DirtySet::SHADING_RATE))
    {
      cmd.rsSetShadingRate(graphicsState.constantShadingRate,
        activeFeatures.test(FeatureState::ALLOW_SHADING_RATE_T2) ? graphicsState.shadingRateCombiners : nullptr);
    }
    if (graphicsDirtyState.test(GraphicsState::DirtySet::SHADING_RATE_TEXTURE))
    {
      cmd.rsSetShadingRateImage(graphicsState.shadingRateTexture);
    }
#endif
    graphicsState.dirtyState &= igonreMask;
  }

  void drawIndirect(ID3D12CommandSignature *signature, ID3D12Resource *buffer, uint64_t offset, uint32_t draw_count)
  {
    if (!validateTopology())
      return;
    if (!canDraw())
      return;
    flushGraphics(PrimitivePipeline::VERTEX);
    cmd.executeIndirect(signature, draw_count, buffer, offset, nullptr, 0);
  }

  void drawIndexedIndirect(ID3D12CommandSignature *signature, ID3D12Resource *buffer, uint64_t offset, uint32_t draw_count)
  {
    if (!validateTopology())
      return;
    if (!canDraw())
      return;
    flushGraphics(PrimitivePipeline::VERTEX);
    cmd.executeIndirect(signature, draw_count, buffer, offset, nullptr, 0);
  }

  void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
  {
    if (!validateTopology())
      return;
    if (!canDraw())
      return;
    flushGraphics(PrimitivePipeline::VERTEX);
    cmd.drawInstanced(vertex_count, instance_count, first_vertex, first_instance);
  }

  void drawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
  {
    if (!validateTopology())
      return;
    if (!canDraw())
      return;

    flushGraphics(PrimitivePipeline::VERTEX);
    cmd.drawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance);
  }

  void embeddedExecute(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline, DWORD const_values[4],
    eastl::optional<D3D12_GPU_DESCRIPTOR_HANDLE> src, D3D12_CPU_DESCRIPTOR_HANDLE dst, D3D12_RECT dst_rect)
  {
    G_ASSERT(signature);
    G_ASSERT(pipeline);

    D3D12_VIEWPORT dstViewport;
    dstViewport.TopLeftX = dst_rect.left;
    dstViewport.TopLeftY = dst_rect.top;
    dstViewport.Width = dst_rect.right - dst_rect.left;
    dstViewport.Height = dst_rect.bottom - dst_rect.top;
    dstViewport.MinDepth = 0.01f;
    dstViewport.MaxDepth = 1.f;

    if (eastl::exchange(activeEmbeddedPipeline, pipeline) != pipeline)
    {
      cmd.setGraphicsRootSignature(signature);
      cmd.setPipelineState(pipeline);
    }

    // heap pointers might have changed
    flushGlobalState(NeedSamplerHeap::No);
    // blit target rect
    cmd.rsSetViewports(1, &dstViewport);
    cmd.rsSetScissorRects(1, &dst_rect);
    // contains source coord offset and scaling to produce source rect
    cmd.setGraphicsRoot32BitConstants(0, 4, const_values, 0);
    if (src)
      cmd.setGraphicsRootDescriptorTable(1, src.value());
    cmd.omSetRenderTargets(1, &dst, FALSE, nullptr);
    cmd.iaSetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#if !_TARGET_XBOXONE
    // Have to manage VRS here or we may blit with VRS on and it will end up generating garbage.
    // State can only diverge from default when VRS is supported.
    if ((D3D12_SHADING_RATE_1X1 != graphicsState.constantShadingRate ||
          D3D12_SHADING_RATE_COMBINER_PASSTHROUGH != graphicsState.shadingRateCombiners[0] ||
          D3D12_SHADING_RATE_COMBINER_PASSTHROUGH != graphicsState.shadingRateCombiners[1]))
    {
      // the assumption here is that passing null for the combiners resets them to default
      cmd.rsSetShadingRate(D3D12_SHADING_RATE_1X1, nullptr);

      graphicsState.dirtyState.set(GraphicsState::DirtySet::SHADING_RATE);
    }
#endif
    cmd.drawInstanced(3, 1, 0, 0);

    graphicsState.dirtyState.set(GraphicsState::DirtySet::RENDER_TARGETS);
    graphicsState.dirtyState.set(GraphicsState::DirtySet::SCISSOR_RECT);
    graphicsState.dirtyState.set(GraphicsState::DirtySet::VIEWPORT);
    graphicsState.dirtyState.set(GraphicsState::DirtySet::PRIMITIVE_TOPOLOGY);

    // will also invalidate descriptors, cbv table and root constant table
    graphicsState.dirtyState.set(GraphicsState::DirtySet::SIGNATURE);

    graphicsState.dirtyState.set(GraphicsState::DirtySet::PIPELINE);
    computeState.dirtyState.set(ComputeState::DirtySet::PIPELINE);
#if D3D_HAS_RAY_TRACING
    raytraceState.dirtyState.set(RaytraceState::DirtySet::PIPELINE);
#endif
  }

  void blitExecute(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline, DWORD const_values[4],
    D3D12_GPU_DESCRIPTOR_HANDLE src, D3D12_CPU_DESCRIPTOR_HANDLE dst, D3D12_RECT dst_rect)
  {
    embeddedExecute(signature, pipeline, const_values, src, dst, dst_rect);
  }

  void clearExecute(ID3D12RootSignature *signature, ID3D12PipelineState *pipeline, DWORD const_values[4],
    D3D12_CPU_DESCRIPTOR_HANDLE dst, D3D12_RECT dst_rect)
  {
    embeddedExecute(signature, pipeline, const_values, eastl::nullopt, dst, dst_rect);
  }

#if !_TARGET_XBOXONE
  void setShadingRate(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner, D3D12_SHADING_RATE_COMBINER ps_combiner)
  {
    G_STATIC_ASSERT(D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT == 2);
    graphicsState.markChange(GraphicsState::DirtySet::SHADING_RATE, graphicsState.constantShadingRate != rate ||
                                                                      graphicsState.shadingRateCombiners[0] != vs_combiner ||
                                                                      graphicsState.shadingRateCombiners[1] != ps_combiner);
    graphicsState.constantShadingRate = rate;
    graphicsState.shadingRateCombiners[0] = vs_combiner;
    graphicsState.shadingRateCombiners[1] = ps_combiner;
  }

  void setShadingRateTexture(ID3D12Resource *texture)
  {
    graphicsState.markChange(GraphicsState::DirtySet::SHADING_RATE_TEXTURE, graphicsState.shadingRateTexture != texture);
    graphicsState.shadingRateTexture = texture;
  }
#endif

#if _TARGET_XBOX
  void resummarizeHtile(ID3D12Resource *depth) { xbox_resummarize_htile(cmd, depth); }
#endif

  void iaSetVertexBuffers(UINT start_slot, UINT num_views, const D3D12_VERTEX_BUFFER_VIEW *views)
  {
    cmd.iaSetVertexBuffers(start_slot, num_views, views);
  }
  void iaSetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW *view) { cmd.iaSetIndexBuffer(view); }
  void discardResource(ID3D12Resource *resource, const D3D12_DISCARD_REGION *region) { cmd.discardResource(resource, region); }
  void setPredication(ID3D12Resource *buffer, UINT64 aligned_buffer_offset, D3D12_PREDICATION_OP operation)
  {
    cmd.setPredication(buffer, aligned_buffer_offset, operation);
  }
  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers) { cmd.resourceBarrier(num_barriers, barriers); }

#if !_TARGET_XBOXONE
  void dispatchMesh(uint32_t x, uint32_t y, uint32_t z)
  {
    if (!canDraw())
      return;
    flushGraphics(PrimitivePipeline::MESH);
    cmd.dispatchMesh(x, y, z);
  }
  void dispatchMeshIndirect(ID3D12CommandSignature *signature, ID3D12Resource *args_buffer, uint64_t args_offset,
    ID3D12Resource *count_buffer, uint64_t count_offset, uint32_t max_count)
  {
    if (!canDraw())
      return;
    flushGraphics(PrimitivePipeline::MESH);
    cmd.executeIndirect(signature, max_count, args_buffer, args_offset, count_buffer, count_offset);
  }
#endif
};
} // namespace drv3d_dx12
