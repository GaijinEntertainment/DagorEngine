// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include "device_caps_and_shader_model.h"
#include "format_store.h"
#include "frame_buffer.h"
#include "render_state.h"
#include "shader.h"

#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <generic/dag_bitset.h>
#include <math/dag_lsbVisitor.h>
#include <supp/dag_comPtr.h>


inline const char CACHE_FILE_NAME[] = "cache/dx12.cache";

namespace drv3d_dx12
{
struct BasePipelineIdentifierHashSet
{
  char vsHash[1 + 2 * sizeof(dxil::HashValue)]{};
  char psHash[1 + 2 * sizeof(dxil::HashValue)]{};
};

struct BasePipelineIdentifier
{
  dxil::HashValue vs;
  dxil::HashValue ps;

  operator BasePipelineIdentifierHashSet() const
  {
    BasePipelineIdentifierHashSet set;
    vs.convertToString(set.vsHash, sizeof(set.vsHash));
    ps.convertToString(set.psHash, sizeof(set.psHash));
    return set;
  }
};

inline bool operator==(const BasePipelineIdentifier &l, const BasePipelineIdentifier &r) { return l.vs == r.vs && l.ps == r.ps; }
inline bool operator!=(const BasePipelineIdentifier &l, const BasePipelineIdentifier &r) { return !(l == r); }

struct MeshPipelineVariantState
{
  uint32_t framebufferLayoutIndex;
  uint32_t staticRenderStateIndex;
  // using u32 for predictable size and the option to turn this into a flag field
  uint32_t isWireFrame;
};

struct GraphicsPipelineVariantState : MeshPipelineVariantState
{
  D3D12_PRIMITIVE_TOPOLOGY_TYPE topology;
  uint32_t inputLayoutIndex;
};

struct ComputePipelineIdentifierHashSet
{
  char hash[1 + 2 * sizeof(dxil::HashValue)]{};
};

struct ComputePipelineIdentifier
{
  dxil::HashValue hash;

  operator ComputePipelineIdentifierHashSet() const
  {
    ComputePipelineIdentifierHashSet set;
    hash.convertToString(set.hash, sizeof(set.hash));
    return set;
  }
};

struct RootSignatureStageLayout
{
  uint8_t rootConstantsParamIndex = 0;
  uint8_t constBufferViewsParamOffset = 0;
  uint8_t samplersParamIndex = 0;
  uint8_t shaderResourceViewParamIndex = 0;
  uint8_t unorderedAccessViewParamIndex = 0;

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  void setConstBufferDescriptorIndex(uint8_t index, bool is_root_descriptor)
  {
    constBufferViewsParamOffset = (is_root_descriptor ? 0u : 1u) | (index << 1);
  }

  uint8_t getConstBufferDescriptorIndex() const { return constBufferViewsParamOffset >> 1; }

  bool usesConstBufferRootDescriptors() const { return 0 == (constBufferViewsParamOffset & 1); }

  bool usesConstBufferRootDescriptors(const RootSignatureStageLayout &vs, const RootSignatureStageLayout &gs,
    const RootSignatureStageLayout &hs, const RootSignatureStageLayout &ds) const
  {
    return 0 == (1 & (constBufferViewsParamOffset | vs.constBufferViewsParamOffset | gs.constBufferViewsParamOffset |
                       hs.constBufferViewsParamOffset | ds.constBufferViewsParamOffset));
  }
#else
  void setConstBufferDescriptorIndex(uint8_t index, bool is_root_descriptor)
  {
    G_ASSERT(is_root_descriptor);
    G_UNUSED(is_root_descriptor);
    constBufferViewsParamOffset = index;
  }

  uint8_t getConstBufferDescriptorIndex() const { return constBufferViewsParamOffset; }

  constexpr bool usesConstBufferRootDescriptors() const { return true; }

  bool usesConstBufferRootDescriptors(const RootSignatureStageLayout &, const RootSignatureStageLayout &,
    const RootSignatureStageLayout &, const RootSignatureStageLayout &) const
  {
    return true;
  }
#endif
};

struct RootSignatureGlobalLayout
{
  uint8_t bindlessSamplersParamIndex = 0;
  uint8_t bindlessShaderResourceViewParamIndex = 0;
};

struct ComputePipelineSignature
{
  ComPtr<ID3D12RootSignature> signature;
  struct Definition
  {
    dxil::ShaderResourceUsageTable registers = {};
    RootSignatureGlobalLayout layout;
    RootSignatureStageLayout csLayout;
#if _TARGET_SCARLETT
    bool hasAccelerationStructure = false;
#endif
  } def;

  // const buffer is different, there each cbv is one entry directly at the signature and not a
  // range
  template <typename C>
  void updateCBVRange(C cmd, D3D12_GPU_VIRTUAL_ADDRESS *cbvs, D3D12_GPU_DESCRIPTOR_HANDLE offset,
    Bitset<dxil::MAX_B_REGISTERS> dirty_mask, bool descriptor_range_dirty, bool compacted_buffer_array) const
  {
    uint32_t index = def.csLayout.getConstBufferDescriptorIndex();
    if (def.csLayout.usesConstBufferRootDescriptors())
    {
      if (!compacted_buffer_array)
      {
        for (auto i : LsbVisitor{def.registers.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX)})
        {
          if (dirty_mask.test(i))
          {
            cmd.setComputeRootConstantBufferView(index, cbvs[i]);
          }
          ++index;
        }
      }
      else
      {
        const uint32_t constCount = __popcount(def.registers.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX));
        for (uint32_t i = 0; i < constCount; ++i)
        {
          cmd.setComputeRootConstantBufferView(index + i, cbvs[i]);
        }
      }
    }
    else if (descriptor_range_dirty)
    {
      cmd.setComputeRootDescriptorTable(index, offset);
    }
  }
  template <typename C>
  void updateSRVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.tRegisterUseMask)
    {
      cmd.setComputeRootDescriptorTable(def.csLayout.shaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateUAVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.uRegisterUseMask)
    {
      cmd.setComputeRootDescriptorTable(def.csLayout.unorderedAccessViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateSamplerRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.sRegisterUseMask)
    {
      cmd.setComputeRootDescriptorTable(def.csLayout.samplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateUnboundedSRVRanges(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.bindlessUsageMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
    {
      cmd.setComputeRootDescriptorTable(def.layout.bindlessShaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateBindlessSamplerRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.bindlessUsageMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
    {
      cmd.setComputeRootDescriptorTable(def.layout.bindlessSamplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateRootConstants(C cmd, uint32_t values[MAX_ROOT_CONSTANTS], Bitset<MAX_ROOT_CONSTANTS> dirty_bits) const
  {
    if (def.registers.rootConstantDwords)
    {
      // only update if the region we cover was dirty
      if (dirty_bits.find_first() < def.registers.rootConstantDwords)
      {
        // TODO this could be optimized by only updating dirty region
        cmd.setComputeRoot32BitConstants(def.csLayout.rootConstantsParamIndex, def.registers.rootConstantDwords, values, 0);
      }
    }
  }
};

// TODO implement different modes
//      there is at least one alternative where we
//      put cbv in b0 in root argument and the
//      rest into a range if needed
struct GraphicsPipelineSignature
{
  ComPtr<ID3D12RootSignature> signature;
  struct Definition
  {
    dxil::ShaderResourceUsageTable vertexShaderRegisters = {};
    dxil::ShaderResourceUsageTable pixelShaderRegisters = {};
    dxil::ShaderResourceUsageTable geometryShaderRegisters = {};
    dxil::ShaderResourceUsageTable hullShaderRegisters = {};
    dxil::ShaderResourceUsageTable domainShaderRegisters = {};
    RootSignatureGlobalLayout layout;
    RootSignatureStageLayout vsLayout;
    RootSignatureStageLayout psLayout;
    RootSignatureStageLayout gsLayout;
    RootSignatureStageLayout hsLayout;
    RootSignatureStageLayout dsLayout;
    bool hasVertexInputs = false;
#if _TARGET_SCARLETT
    bool hasAccelerationStructure = false;
#endif
  } def;
  uint32_t vsCombinedTRegisterMask = 0;
  uint32_t vsCombinedSRegisterMask = 0;
  uint32_t allCombinedBindlessMask = 0;
  uint16_t vsCombinedBRegisterMask = 0;
  uint16_t vsCombinedURegisterMask = 0;

  void deriveComboMasks()
  {
    vsCombinedBRegisterMask = def.vertexShaderRegisters.bRegisterUseMask | def.geometryShaderRegisters.bRegisterUseMask |
                              def.hullShaderRegisters.bRegisterUseMask | def.domainShaderRegisters.bRegisterUseMask;

    vsCombinedTRegisterMask = def.vertexShaderRegisters.tRegisterUseMask | def.geometryShaderRegisters.tRegisterUseMask |
                              def.hullShaderRegisters.tRegisterUseMask | def.domainShaderRegisters.tRegisterUseMask;

    vsCombinedSRegisterMask = def.vertexShaderRegisters.sRegisterUseMask | def.geometryShaderRegisters.sRegisterUseMask |
                              def.hullShaderRegisters.sRegisterUseMask | def.domainShaderRegisters.sRegisterUseMask;

    vsCombinedURegisterMask = def.vertexShaderRegisters.uRegisterUseMask | def.geometryShaderRegisters.uRegisterUseMask |
                              def.hullShaderRegisters.uRegisterUseMask | def.domainShaderRegisters.uRegisterUseMask;

    allCombinedBindlessMask = def.vertexShaderRegisters.bindlessUsageMask | def.geometryShaderRegisters.bindlessUsageMask |
                              def.hullShaderRegisters.bindlessUsageMask | def.domainShaderRegisters.bindlessUsageMask |
                              def.pixelShaderRegisters.bindlessUsageMask;
  }

  bool hasUAVAccess() const { return 0 != (vsCombinedURegisterMask | def.pixelShaderRegisters.uRegisterUseMask); }

  // const buffer is different, there each cbv is one entry directly at the signature and not a range
  template <typename C>
  void updateVertexCBVRange(C cmd, D3D12_GPU_VIRTUAL_ADDRESS *bcvs, D3D12_GPU_DESCRIPTOR_HANDLE offset,
    Bitset<dxil::MAX_B_REGISTERS> dirty_mask, bool descriptor_range_dirty)
  {
    if (def.psLayout.usesConstBufferRootDescriptors(def.vsLayout, def.gsLayout, def.hsLayout, def.dsLayout))
    {
      uint32_t index = def.vsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.vertexShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX)})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
      index = def.gsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.geometryShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX)})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
      index = def.hsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.hullShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX)})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
      index = def.dsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.domainShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX)})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
    }
    else if (descriptor_range_dirty)
    {
      if (def.vertexShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX))
      {
        cmd.setGraphicsRootDescriptorTable(def.vsLayout.getConstBufferDescriptorIndex(), offset);
      }

      if (def.geometryShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX))
      {
        cmd.setGraphicsRootDescriptorTable(def.gsLayout.getConstBufferDescriptorIndex(), offset);
      }

      if (def.hullShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX))
      {
        cmd.setGraphicsRootDescriptorTable(def.hsLayout.getConstBufferDescriptorIndex(), offset);
      }

      if (def.domainShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX))
      {
        cmd.setGraphicsRootDescriptorTable(def.dsLayout.getConstBufferDescriptorIndex(), offset);
      }
    }
  }
  template <typename C>
  void updateVertexSRVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.vertexShaderRegisters.tRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.vsLayout.shaderResourceViewParamIndex, offset);
    }
    if (def.geometryShaderRegisters.tRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.gsLayout.shaderResourceViewParamIndex, offset);
    }
    if (def.hullShaderRegisters.tRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.hsLayout.shaderResourceViewParamIndex, offset);
    }
    if (def.domainShaderRegisters.tRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.dsLayout.shaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateVertexUAVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.vertexShaderRegisters.uRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.vsLayout.unorderedAccessViewParamIndex, offset);
    }
    if (def.geometryShaderRegisters.uRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.gsLayout.unorderedAccessViewParamIndex, offset);
    }
    if (def.hullShaderRegisters.uRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.hsLayout.unorderedAccessViewParamIndex, offset);
    }
    if (def.domainShaderRegisters.uRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.dsLayout.unorderedAccessViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateVertexSamplerRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.vertexShaderRegisters.sRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.vsLayout.samplersParamIndex, offset);
    }
    if (def.geometryShaderRegisters.sRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.gsLayout.samplersParamIndex, offset);
    }
    if (def.hullShaderRegisters.sRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.hsLayout.samplersParamIndex, offset);
    }
    if (def.domainShaderRegisters.sRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.dsLayout.samplersParamIndex, offset);
    }
  }

  // const buffer is different, there each cbv is one entry directly at the signature and not a range
  template <typename C>
  void updatePixelCBVRange(C cmd, D3D12_GPU_VIRTUAL_ADDRESS *cbvs, D3D12_GPU_DESCRIPTOR_HANDLE offset,
    Bitset<dxil::MAX_B_REGISTERS> dirty_mask, bool descriptor_range_dirty)
  {
    uint32_t index = def.psLayout.getConstBufferDescriptorIndex();
    if (def.psLayout.usesConstBufferRootDescriptors())
    {
      for (auto i : LsbVisitor{def.pixelShaderRegisters.bRegisterUseMask & ~(1u << ROOT_CONSTANT_BUFFER_INDEX)})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, cbvs[i]);
        }
        ++index;
      }
    }
    else if (descriptor_range_dirty)
    {
      cmd.setGraphicsRootDescriptorTable(index, offset);
    }
  }
  template <typename C>
  void updatePixelSRVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.pixelShaderRegisters.tRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.psLayout.shaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updatePixelUAVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.pixelShaderRegisters.uRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.psLayout.unorderedAccessViewParamIndex, offset);
    }
  }
  template <typename C>
  void updatePixelSamplerRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.pixelShaderRegisters.sRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.psLayout.samplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateVertexRootConstants(C cmd, uint32_t values[MAX_ROOT_CONSTANTS], Bitset<MAX_ROOT_CONSTANTS> dirty_bits)
  {
    auto first_bit = dirty_bits.find_first();
    if (def.vertexShaderRegisters.rootConstantDwords)
    {
      // only update if the region we cover was dirty
      if (first_bit < def.vertexShaderRegisters.rootConstantDwords)
      {
        // TODO this could be optimized by only updating dirty region
        cmd.setGraphicsRoot32BitConstants(def.vsLayout.rootConstantsParamIndex, def.vertexShaderRegisters.rootConstantDwords, values,
          0);
      }
    }
    if (def.geometryShaderRegisters.rootConstantDwords)
    {
      // only update if the region we cover was dirty
      if (first_bit < def.geometryShaderRegisters.rootConstantDwords)
      {
        // TODO this could be optimized by only updating dirty region
        cmd.setGraphicsRoot32BitConstants(def.gsLayout.rootConstantsParamIndex, def.geometryShaderRegisters.rootConstantDwords, values,
          0);
      }
    }
    if (def.hullShaderRegisters.rootConstantDwords)
    {
      // only update if the region we cover was dirty
      if (first_bit < def.hullShaderRegisters.rootConstantDwords)
      {
        // TODO this could be optimized by only updating dirty region
        cmd.setGraphicsRoot32BitConstants(def.hsLayout.rootConstantsParamIndex, def.hullShaderRegisters.rootConstantDwords, values, 0);
      }
    }
    if (def.domainShaderRegisters.rootConstantDwords)
    {
      // only update if the region we cover was dirty
      if (first_bit < def.domainShaderRegisters.rootConstantDwords)
      {
        // TODO this could be optimized by only updating dirty region
        cmd.setGraphicsRoot32BitConstants(def.dsLayout.rootConstantsParamIndex, def.domainShaderRegisters.rootConstantDwords, values,
          0);
      }
    }
  }
  template <typename C>
  void updatePixelRootConstants(C cmd, uint32_t values[MAX_ROOT_CONSTANTS], Bitset<MAX_ROOT_CONSTANTS> dirty_bits)
  {
    if (def.pixelShaderRegisters.rootConstantDwords)
    {
      // only update if the region we cover was dirty
      if (dirty_bits.find_first() < def.pixelShaderRegisters.rootConstantDwords)
      {
        // TODO this could be optimized by only updating dirty region
        cmd.setGraphicsRoot32BitConstants(def.psLayout.rootConstantsParamIndex, def.pixelShaderRegisters.rootConstantDwords, values,
          0);
      }
    }
  }
  template <typename C>
  void updateBindlessSamplerRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (allCombinedBindlessMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
    {
      cmd.setGraphicsRootDescriptorTable(def.layout.bindlessSamplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateBindlessSRVRange(C cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (allCombinedBindlessMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
    {
      cmd.setGraphicsRootDescriptorTable(def.layout.bindlessShaderResourceViewParamIndex, offset);
    }
  }
};

#if D3D_HAS_RAY_TRACING
struct RaytracePipelineSignature : ComputePipelineSignature
{};
#endif

struct GraphicsPipelineBaseCacheIdTag
{};
typedef TaggedHandle<ptrdiff_t, -1, GraphicsPipelineBaseCacheIdTag> GraphicsPipelineBaseCacheId;
/*
 * This pipeline cache can run in multiple modes:
 * D3D12_SHADER_CACHE_SUPPORT_NONE - no device support mode, stores pipeline variant table only.
 * D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO - device per pso cache, stores additionally the pso cache
 * per variant.
 * D3D12_SHADER_CACHE_SUPPORT_LIBRARY - only stores library object.
 * D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE - ignored, automatic thing the os does for us,
 * still try to use other cache modes.
 * D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE - same as D3D12_SHADER_CACHE_SUPPORT_NONE so we
 * know which variants exist.
 *
 * TODO: Fold this into pipeline manager, removes duplication of static states and input layouts tables
 */
class PipelineCache
{
public:
  using BasePipelineIdentifier = ::drv3d_dx12::BasePipelineIdentifier;

  struct ShutdownParameters
  {
    const char *fileName;
    UINT deviceVendorID;
    UINT deviceID;
    UINT subSystemID;
    UINT revisionID;
    UINT64 rawDriverVersion;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    bool rootSignaturesUsesCBVDescriptorRanges;
#else
    static constexpr bool rootSignaturesUsesCBVDescriptorRanges = false;
#endif
    bool generateBlks;
    bool alwaysGenerateBlks;
    DeviceCapsAndShaderModel features;
  };

  struct SetupParameters : ShutdownParameters
  {
    ID3D12Device1 *device;
    D3D12_SHADER_CACHE_SUPPORT_FLAGS allowedModes;
  };

  void init(const SetupParameters &params);
  void shutdown(const ShutdownParameters &params);

  GraphicsPipelineBaseCacheId getGraphicsPipeline(const BasePipelineIdentifier &ident);
  size_t getGraphicsPipelineVariantCount(GraphicsPipelineBaseCacheId base_id);
  size_t addGraphicsPipelineVariant(GraphicsPipelineBaseCacheId base_id, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
    const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, ID3D12PipelineState *pipeline);
  size_t addGraphicsMeshPipelineVariant(GraphicsPipelineBaseCacheId base_id, bool is_wire_frame,
    const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, ID3D12PipelineState *pipeline);
  // Returns new count (eg getGraphicsPipelineVariantCount)
  size_t removeGraphicsPipelineVariant(GraphicsPipelineBaseCacheId base_id, size_t index);
  ComPtr<ID3D12PipelineState> loadGraphicsPipelineVariant(GraphicsPipelineBaseCacheId base_id, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
    const InputLayout &input_layout, bool is_wire_frame, const RenderStateSystem::StaticState &static_state,
    const FramebufferLayout &fb_layout, D3D12_PIPELINE_STATE_STREAM_DESC desc, D3D12_CACHED_PIPELINE_STATE &blob_target);
  ComPtr<ID3D12PipelineState> loadGraphicsMeshPipelineVariant(GraphicsPipelineBaseCacheId base_id, bool is_wire_frame,
    const RenderStateSystem::StaticState &static_state, const FramebufferLayout &fb_layout, D3D12_PIPELINE_STATE_STREAM_DESC desc,
    D3D12_CACHED_PIPELINE_STATE &blob_target);
  D3D12_PRIMITIVE_TOPOLOGY_TYPE
  getGraphicsPipelineVariantDesc(GraphicsPipelineBaseCacheId base_id, size_t index, InputLayout &input_layout, bool &is_wire_frame,
    RenderStateSystem::StaticState &static_state, FramebufferLayout &fb_layout) const;
  ComPtr<ID3D12PipelineState> loadGraphicsPipelineVariantFromIndex(GraphicsPipelineBaseCacheId base_id, size_t index,
    D3D12_PIPELINE_STATE_STREAM_DESC desc, D3D12_CACHED_PIPELINE_STATE &blob_target);
  bool containsGraphicsPipeline(const BasePipelineIdentifier &ident);

  void addCompute(const dxil::HashValue &shader, ID3D12PipelineState *pipeline);
  ComPtr<ID3D12PipelineState> loadCompute(const dxil::HashValue &shader, D3D12_PIPELINE_STATE_STREAM_DESC desc,
    D3D12_CACHED_PIPELINE_STATE &blob_target);

  // NOTE: no checks for duplicates are performed
  void addComputeSignature(const ComputePipelineSignature::Definition &def, ID3DBlob *blob)
  {
    ComputeSignatureBlob csb;
    csb.def = def;
    auto from = reinterpret_cast<const uint8_t *>(blob->GetBufferPointer());
    csb.blob.assign(from, from + blob->GetBufferSize());
    computeSignatures.push_back(eastl::move(csb));
    hasChanged = true;
  }
  template <typename T>
  void enumrateComputeSignatures(T clb)
  {
    for (auto &&sig : computeSignatures)
      clb(sig.def, sig.blob);
  }

  // NOTE: no checks for duplicates are performed
  void addGraphicsSignature(const GraphicsPipelineSignature::Definition &def, ID3DBlob *blob)
  {
    GraphicsSignatureBlob gsb;
    gsb.def = def;
    auto from = reinterpret_cast<const uint8_t *>(blob->GetBufferPointer());
    gsb.blob.assign(from, from + blob->GetBufferSize());
    graphicsSignatures.push_back(eastl::move(gsb));
    hasChanged = true;
  }
  template <typename T>
  void enumerateGraphicsSignatures(T clb)
  {
    for (auto &&sig : graphicsSignatures)
      clb(sig.def, sig.blob);
  }

  void addGraphicsMeshSignature(const GraphicsPipelineSignature::Definition &def, ID3DBlob *blob)
  {
    GraphicsSignatureBlob gsb;
    gsb.def = def;
    auto from = reinterpret_cast<const uint8_t *>(blob->GetBufferPointer());
    gsb.blob.assign(from, from + blob->GetBufferSize());
    graphicsMeshSignatures.push_back(eastl::move(gsb));
    hasChanged = true;
  }
  template <typename T>
  void enumerateGraphicsMeshSignatures(T clb)
  {
    for (auto &&sig : graphicsMeshSignatures)
      clb(sig.def, sig.blob);
  }

  void preRecovery();
  void recover(ID3D12Device1 *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS allowed_modes);

  template <typename T>
  void enumerateStaticRenderStates(T clb)
  {
    for (auto &&srs : staticRenderStates)
      clb(srs);
  }

  template <typename T>
  void enumerateInputLayouts(T clb)
  {
    for (auto &&il : inputLayouts)
      clb(il);
  }

  /// \returns True when the cache matches the hashes, otherwise false and the cache data is reset.
  bool onBindumpLoad(ID3D12Device1 *device, eastl::span<const dxil::HashValue> all_shader_hashes);

private:
  bool loadFromFile(const SetupParameters &params);

  uint32_t getStaticRenderStateIndex(const RenderStateSystem::StaticState &state)
  {
    auto ref = eastl::find(begin(staticRenderStates), end(staticRenderStates), state);
    if (ref == end(staticRenderStates))
    {
      ref = staticRenderStates.insert(end(staticRenderStates), state);
      hasChanged = true;
    }
    return static_cast<uint32_t>(ref - begin(staticRenderStates));
  }

  uint32_t getInputLayoutIndex(const InputLayout &layout)
  {
    auto ref = eastl::find(begin(inputLayouts), end(inputLayouts), layout);
    if (ref == end(inputLayouts))
    {
      ref = inputLayouts.insert(end(inputLayouts), layout);
      hasChanged = true;
    }
    return static_cast<uint32_t>(ref - begin(inputLayouts));
  }

  uint32_t getFramebufferLayoutIndex(const FramebufferLayout &layout)
  {
    auto ref = eastl::find(begin(framebufferLayouts), end(framebufferLayouts), layout);
    if (ref == end(framebufferLayouts))
    {
      ref = framebufferLayouts.insert(end(framebufferLayouts), layout);
      hasChanged = true;
    }
    return static_cast<uint32_t>(ref - begin(framebufferLayouts));
  }

  // only use library if lib is supported but no automatic disk cache, then we only keep track
  // of variants.
  bool useLibrary() const
  {
    return (deviceFeatures & (D3D12_SHADER_CACHE_SUPPORT_LIBRARY | D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE)) ==
           D3D12_SHADER_CACHE_SUPPORT_LIBRARY;
  }

  // always track variants.
  bool trackVariants() const { return true; }

  // only use individual pso blobs if the driver supports it, library and automatic disk
  // cache is not supported.
  bool usePSOBlobs() const
  {
    return (deviceFeatures & (D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO | D3D12_SHADER_CACHE_SUPPORT_LIBRARY |
                               D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE)) == D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO;
  }

  struct GraphicsPipeline
  {
    BasePipelineIdentifier ident;
    struct VariantCacheEntry : GraphicsPipelineVariantState
    {
      dag::Vector<uint8_t> blob;
    };
    dag::Vector<VariantCacheEntry> variantCache;
  };

  struct GraphicsPipelineVariantName
  {
    static constexpr size_t stage_length = sizeof(dxil::HashValue) * 2 + 1;
    static constexpr size_t base_length = 1 + (stage_length - 1) * 2 + 1;
    static constexpr size_t varaint_ident_length = (sizeof(uint32_t) * 2 + sizeof(uint32_t) * 2 + 1 + sizeof(uint32_t)) * 2 + 1;
    static constexpr size_t topology_length = 3; // one hex byte
    static constexpr size_t length = base_length + varaint_ident_length - 1 + topology_length - 1;

    wchar_t str[length];

    void generate(const BasePipelineIdentifier &base, D3D12_PRIMITIVE_TOPOLOGY_TYPE top, uint32_t input_layout_index,
      bool is_wire_frame, uint32_t static_render_state_index, uint32_t frame_buffer_layout_index)
    {
      str[0] = L'G';
      auto ot = eastl::begin(str) + 1;
      // auto ed = eastl::end(str);

      char lbuf[stage_length];
      base.ps.convertToString(lbuf, sizeof(lbuf));
      ot = eastl::copy(eastl::begin(lbuf), eastl::end(lbuf) - 1, ot);

      base.vs.convertToString(lbuf, sizeof(lbuf));
      ot = eastl::copy(eastl::begin(lbuf), eastl::end(lbuf) - 1, ot);

      // as one hex byte
      auto topByte = static_cast<uint8_t>(top);
      data_to_str_hex_buf(lbuf, sizeof(lbuf), &topByte, sizeof(topByte));
      ot = eastl::copy(eastl::begin(lbuf), eastl::begin(lbuf) + topology_length - 1, ot);

      char sbuf[sizeof(uint32_t) * 2 + 1];
      data_to_str_hex_buf(sbuf, sizeof(sbuf), &input_layout_index, sizeof(input_layout_index));
      ot = eastl::copy(eastl::begin(sbuf), eastl::end(sbuf) - 1, ot);

      *ot++ = is_wire_frame ? L'1' : L'0';

      char srsibuf[sizeof(uint32_t) * 2 + 1];
      data_to_str_hex_buf(srsibuf, sizeof(srsibuf), &static_render_state_index, sizeof(uint32_t));
      ot = eastl::copy(eastl::begin(srsibuf), eastl::end(srsibuf) - 1, ot);

      char fbuf[sizeof(uint32_t) * 2 + 1];
      data_to_str_hex_buf(fbuf, sizeof(fbuf), &frame_buffer_layout_index, sizeof(uint32_t));
      eastl::copy(eastl::begin(fbuf), eastl::end(fbuf), ot);
    }
  };

  struct ComputePipelineName
  {
    constexpr static size_t length = 1 + sizeof(dxil::HashValue) * 2 + 1;
    wchar_t str[length];

    ComputePipelineName(const dxil::HashValue &hash) { assign(hash); }
    ComputePipelineName &operator=(const dxil::HashValue &hash) { return assign(hash); }
    ComputePipelineName &assign(const dxil::HashValue &hash)
    {
      str[0] = L'C';
      char lbuf[length - 1];
      hash.convertToString(lbuf, sizeof(lbuf));
      eastl::copy(eastl::begin(lbuf), eastl::end(lbuf), eastl::begin(str) + 1);
      return *this;
    }
  };

  struct ComputeBlob : ComputePipelineIdentifier
  {
    dag::Vector<uint8_t> blob;
  };

  struct ComputeSignatureBlob
  {
    ComputePipelineSignature::Definition def;
    dag::Vector<uint8_t> blob;
  };

  struct GraphicsSignatureBlob
  {
    GraphicsPipelineSignature::Definition def;
    dag::Vector<uint8_t> blob;
  };

#if !_TARGET_SCARLETT
  ComPtr<ID3D12PipelineLibrary1> library;
#endif
  dag::Vector<uint8_t> initialPipelineBlob;
  dag::Vector<GraphicsPipeline> graphicsCache;
  dag::Vector<ComputeBlob> computeBlobs;
  dag::Vector<ComputeSignatureBlob> computeSignatures;
  dag::Vector<GraphicsSignatureBlob> graphicsSignatures;
  dag::Vector<GraphicsSignatureBlob> graphicsMeshSignatures;
  dag::Vector<RenderStateSystem::StaticState> staticRenderStates;
  dag::Vector<InputLayout> inputLayouts;
  dag::Vector<FramebufferLayout> framebufferLayouts;
  uint32_t shaderInDumpCount = 0;
  dxil::HashValue shaderInDumpHash{};
  D3D12_SHADER_CACHE_SUPPORT_FLAGS deviceFeatures = D3D12_SHADER_CACHE_SUPPORT_NONE;
  bool hasChanged = false;
};
} // namespace drv3d_dx12
