// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include "frame_buffer.h"
#include "render_state.h"
#include "shader.h"

#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_shaderModelVersion.h>

#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <generic/dag_bitset.h>
#include <math/dag_lsbVisitor.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <supp/dag_comPtr.h>


inline const char CACHE_FILE_NAME[] = "cache/dx12.cache";
inline const char BUILD_IN_CACHE_FILE_NAME[] = "cache/dx12_build_in.cache";
inline const char DUMP_CACHE_FILE_PREFIX[] = "cache/dx12_";
inline const char DUMP_CACHE_FILE_POSTFIX[] = ".cache";

namespace drv3d_dx12
{
inline d3d::shadermodel::Version shader_model_from_dx(D3D_SHADER_MODEL model)
{
  unsigned int ma = (model >> 4) & 0xF;
  unsigned int mi = (model >> 0) & 0xF;
  return {ma, mi};
}

inline D3D_SHADER_MODEL shader_model_to_dx(d3d::shadermodel::Version model)
{
  return static_cast<D3D_SHADER_MODEL>(model.major << 4 | model.minor);
}

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
    union
    {
      uint32_t rawBits = 0;
      struct
      {
#if _TARGET_SCARLETT
        bool hasAccelerationStructure : 1;
#endif
        bool useResourceDescriptorHeapIndexing : 1;
        bool useSamplerDescriptorHeapIndexing : 1;
      };
    };
  } def;

  // const buffer is different, there each cbv is one entry directly at the signature and not a
  // range
  template <typename C>
  void updateCBVRange(C &cmd, D3D12_GPU_VIRTUAL_ADDRESS *cbvs, D3D12_GPU_DESCRIPTOR_HANDLE offset,
    Bitset<dxil::MAX_B_REGISTERS> dirty_mask, bool descriptor_range_dirty, bool compacted_buffer_array) const
  {
    uint32_t index = def.csLayout.getConstBufferDescriptorIndex();
    if (def.csLayout.usesConstBufferRootDescriptors())
    {
      const uint32_t bRegisters = def.registers.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK;
      if (!compacted_buffer_array)
      {
        for (auto i : LsbVisitor{bRegisters})
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
        const uint32_t constCount = __popcount(bRegisters);
        for (auto cbv : eastl::span{cbvs, constCount})
        {
          cmd.setComputeRootConstantBufferView(index, cbv);
          ++index;
        }
      }
    }
    else if (descriptor_range_dirty)
    {
      cmd.setComputeRootDescriptorTable(index, offset);
    }
  }
  template <typename C>
  void updateSRVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.tRegisterUseMask)
    {
      cmd.setComputeRootDescriptorTable(def.csLayout.shaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateUAVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.uRegisterUseMask)
    {
      cmd.setComputeRootDescriptorTable(def.csLayout.unorderedAccessViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateSamplerRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.sRegisterUseMask)
    {
      cmd.setComputeRootDescriptorTable(def.csLayout.samplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateUnboundedSRVRanges(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.bindlessUsageMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
    {
      cmd.setComputeRootDescriptorTable(def.layout.bindlessShaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updateBindlessSamplerRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset) const
  {
    if (def.registers.bindlessUsageMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
    {
      cmd.setComputeRootDescriptorTable(def.layout.bindlessSamplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateRootConstants(C &cmd, uint32_t values[MAX_ROOT_CONSTANTS], Bitset<MAX_ROOT_CONSTANTS> dirty_bits) const
  {
    const auto firstBit = dirty_bits.find_first();
    // only update if the region we cover was dirty
    if (firstBit < def.registers.rootConstantDwords)
    {
      // TODO this could be optimized by only updating dirty region
      cmd.setComputeRoot32BitConstants(def.csLayout.rootConstantsParamIndex, def.registers.rootConstantDwords, values, 0);
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
    union
    {
      uint32_t rawBits = 0;
      struct
      {
        bool hasVertexInputs : 1;
        bool hasStreamOutput : 1;
#if _TARGET_SCARLETT
        bool hasAccelerationStructure : 1;
        bool hasAmplificationShader : 1;
#endif
        bool useResourceDescriptorHeapIndexing : 1;
        bool useSamplerDescriptorHeapIndexing : 1;
        bool isMesh : 1;
      };
    };
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
  void updateVertexCBVRange(C &cmd, D3D12_GPU_VIRTUAL_ADDRESS *bcvs, D3D12_GPU_DESCRIPTOR_HANDLE offset,
    Bitset<dxil::MAX_B_REGISTERS> dirty_mask, bool descriptor_range_dirty)
  {
    if (def.psLayout.usesConstBufferRootDescriptors(def.vsLayout, def.gsLayout, def.hsLayout, def.dsLayout))
    {
      uint32_t index = def.vsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.vertexShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
      index = def.gsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.geometryShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
      index = def.hsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.hullShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK})
      {
        if (dirty_mask.test(i))
        {
          cmd.setGraphicsRootConstantBufferView(index, bcvs[i]);
        }
        ++index;
      }
      index = def.dsLayout.getConstBufferDescriptorIndex();
      for (auto i : LsbVisitor{def.domainShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK})
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
      if (def.vertexShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK)
      {
        cmd.setGraphicsRootDescriptorTable(def.vsLayout.getConstBufferDescriptorIndex(), offset);
      }

      if (def.geometryShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK)
      {
        cmd.setGraphicsRootDescriptorTable(def.gsLayout.getConstBufferDescriptorIndex(), offset);
      }

      if (def.hullShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK)
      {
        cmd.setGraphicsRootDescriptorTable(def.hsLayout.getConstBufferDescriptorIndex(), offset);
      }

      if (def.domainShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK)
      {
        cmd.setGraphicsRootDescriptorTable(def.dsLayout.getConstBufferDescriptorIndex(), offset);
      }
    }
  }
  template <typename C>
  void updateVertexSRVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
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
  void updateVertexUAVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
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
  void updateVertexSamplerRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
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
  void updatePixelCBVRange(C &cmd, D3D12_GPU_VIRTUAL_ADDRESS *cbvs, D3D12_GPU_DESCRIPTOR_HANDLE offset,
    Bitset<dxil::MAX_B_REGISTERS> dirty_mask, bool descriptor_range_dirty)
  {
    uint32_t index = def.psLayout.getConstBufferDescriptorIndex();
    if (def.psLayout.usesConstBufferRootDescriptors())
    {
      for (auto i : LsbVisitor{def.pixelShaderRegisters.bRegisterUseMask & ROOT_CONSTANT_BUFFER_MASK})
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
  void updatePixelSRVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.pixelShaderRegisters.tRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.psLayout.shaderResourceViewParamIndex, offset);
    }
  }
  template <typename C>
  void updatePixelUAVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.pixelShaderRegisters.uRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.psLayout.unorderedAccessViewParamIndex, offset);
    }
  }
  template <typename C>
  void updatePixelSamplerRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (def.pixelShaderRegisters.sRegisterUseMask)
    {
      cmd.setGraphicsRootDescriptorTable(def.psLayout.samplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateVertexRootConstants(C &cmd, uint32_t values[MAX_ROOT_CONSTANTS], Bitset<MAX_ROOT_CONSTANTS> dirty_bits)
  {
    const auto firstBit = dirty_bits.find_first();
    // only update if the region we cover was dirty
    if (firstBit < def.vertexShaderRegisters.rootConstantDwords)
    {
      // TODO this could be optimized by only updating dirty region
      cmd.setGraphicsRoot32BitConstants(def.vsLayout.rootConstantsParamIndex, def.vertexShaderRegisters.rootConstantDwords, values, 0);
    }

    // only update if the region we cover was dirty
    if (firstBit < def.geometryShaderRegisters.rootConstantDwords)
    {
      // TODO this could be optimized by only updating dirty region
      cmd.setGraphicsRoot32BitConstants(def.gsLayout.rootConstantsParamIndex, def.geometryShaderRegisters.rootConstantDwords, values,
        0);
    }

    // only update if the region we cover was dirty
    if (firstBit < def.hullShaderRegisters.rootConstantDwords)
    {
      // TODO this could be optimized by only updating dirty region
      cmd.setGraphicsRoot32BitConstants(def.hsLayout.rootConstantsParamIndex, def.hullShaderRegisters.rootConstantDwords, values, 0);
    }

    // only update if the region we cover was dirty
    if (firstBit < def.domainShaderRegisters.rootConstantDwords)
    {
      // TODO this could be optimized by only updating dirty region
      cmd.setGraphicsRoot32BitConstants(def.dsLayout.rootConstantsParamIndex, def.domainShaderRegisters.rootConstantDwords, values, 0);
    }
  }
  template <typename C>
  void updatePixelRootConstants(C &cmd, uint32_t values[MAX_ROOT_CONSTANTS], Bitset<MAX_ROOT_CONSTANTS> dirty_bits)
  {
    const auto firstBit = dirty_bits.find_first();
    // only update if the region we cover was dirty
    if (firstBit < def.pixelShaderRegisters.rootConstantDwords)
    {
      // TODO this could be optimized by only updating dirty region
      cmd.setGraphicsRoot32BitConstants(def.psLayout.rootConstantsParamIndex, def.pixelShaderRegisters.rootConstantDwords, values, 0);
    }
  }
  template <typename C>
  void updateBindlessSamplerRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (allCombinedBindlessMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
    {
      cmd.setGraphicsRootDescriptorTable(def.layout.bindlessSamplersParamIndex, offset);
    }
  }
  template <typename C>
  void updateBindlessSRVRange(C &cmd, D3D12_GPU_DESCRIPTOR_HANDLE offset)
  {
    if (allCombinedBindlessMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
    {
      cmd.setGraphicsRootDescriptorTable(def.layout.bindlessShaderResourceViewParamIndex, offset);
    }
  }
};

#if D3D_HAS_RAY_TRACING
using RaytracePipelineSignature = ComputePipelineSignature;
#endif

class PipelineCache
{
public:
  using BasePipelineIdentifier = ::drv3d_dx12::BasePipelineIdentifier;

  struct ShutdownParameters
  {
#if _TARGET_PC_WIN
    const char *fileName;
    const char *buildInCacheFileName;
    ShaderHashValue blitByteCodeHash;
    ShaderHashValue clearByteCodeHash;
#endif
  };

  struct SetupParameters : ShutdownParameters
  {
#if _TARGET_PC_WIN
    bool rootSignaturesUsesCBVDescriptorRanges;
    D3DDevice *device;
    D3D12_SHADER_CACHE_SUPPORT_FLAGS allowedModes;
    bool recordBuildInCache;
#endif
  };

  void init(const SetupParameters &params);
  void shutdown(const ShutdownParameters &params);

  void preRecovery();
  void recover(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS allowed_modes);

  // Format based build in pipelines are pipelines with "build in" (eg byte code is part of the executable) shaders that only
  // differ in their output format of a single render target.
  enum class FormatBasedBuildInPipelineType
  {
    Blit,
    Clear,
  };
#if _TARGET_PC_WIN
  // Attempts to load the specified format based pipeline for the given output format, on success a valid pipeline object
  // is returned and on failure a return code is returned.
  // When the requested pipeline is not in the library, dx12 validation layer may complain.
  // This is not thread safe, call from single thread only!
  dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> loadPipeline(FormatBasedBuildInPipelineType type,
    const D3D12_PIPELINE_STATE_STREAM_DESC &desc, DXGI_FORMAT out_format);
  // Adds a new format based pipeline to the cache set. May return an error code on failure to add a new entry.
  // This is not thread safe, call from single thread only!
  dag::Expected<void, HRESULT> storePipeline(FormatBasedBuildInPipelineType type, ID3D12PipelineState *pipeline,
    DXGI_FORMAT out_format);
  // Returns a list of all formats where we have seen for format based pipelines in the past. This can include values that may
  // return an error when passed to loadPipeline. May return nullptr when type is a invalid value.
  // This is not thread safe, call from single thread only!
  const dag::Vector<DXGI_FORMAT> *getKnownPipelineFormats(FormatBasedBuildInPipelineType type);
  // Returns true when the cache system has a pipeline library available to store pipeline cache blobs, returns false
  // when not and may only has format lists of previous uses.
  bool hasFormatBasedBuildInLibrary();
  // Returns true when the cache system has restored the current pipeline library from disk cache from a previous run,
  // returns false when the pipeline library, should it be available, is a new and empty one.
  // After a device reset and a library was restored from that reset device, this may also return true.
  bool hasExistingFormatBasedBuildInLibrary();
  // Loads a cache file for the given dump_name into the cache slot for group_index. Group index can not be 0, loads
  // for this group will be rejected. all_shader_hashes is used to identify if a cache file is current or has outdated
  // entries. The manager may either remove outdated entries on load or runs in a rebuild mode where a new pipeline
  // library is created as a replacement.
  // This may block on all pending cache group writes of storeAndUnloadCacheForShaderGroup.
  bool loadCacheForShaderGroup(D3DDevice *device, uint32_t group_index, eastl::string_view dump_name,
    eastl::span<const dxil::HashValue> all_shader_hashes);
  // Stores the current cache state to the a file for the given dump_name.
  // The store to disk spins up a new thread to execute the write to disk, to hide IO latency.
  void storeAndUnloadCacheForShaderGroup(uint32_t group_index, eastl::string_view dump_name);
  // Attempts to add a graphics pipeline variant to the cache of the group with group_index, should it fail it will return it as an
  // hresult error code.
  dag::Expected<void, HRESULT> addGraphicsPipelineVariant(uint32_t group_index, ID3D12PipelineState *pipeline,
    const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const BasePipelineIdentifier &ident);
  // Attempts to load a graphics pipeline variant from the cache of the group with group_index, should it fail it will return it as an
  // hresult error code. Error code E_INVALIDARG indicates the cache does not have an entry for that pipeline.
  dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> loadGraphicsPipelineVariant(uint32_t group_index,
    const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const BasePipelineIdentifier &ident);
  dag::Expected<void, HRESULT> addComputePipeline(uint32_t group_index, ID3D12PipelineState *pipeline,
    const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const dxil::HashValue &shader_identifier);
  dag::Expected<ComPtr<ID3D12PipelineState>, HRESULT> loadComputePipeline(uint32_t group_index,
    const D3D12_PIPELINE_STATE_STREAM_DESC &desc, const dxil::HashValue &shader_identifier);
  // Looks up if there are entries of a base pipeline with the given identifier in the cache of the given group.
  // Returns true if there is any or false when there is none. This may return false positives, but never false negatives.
  bool hasBasePipeline(uint32_t group_index, const BasePipelineIdentifier &ident);
  // Looks up if there is a entry of a compute pipeline with the given shader identifier (hash) in the cache of the given group.
  // Returns true if there is one or false when there is none.
  bool hasComputePipeline(uint32_t group_index, const dxil::HashValue &shader_identifier);
#endif

private:
  D3D12_SHADER_CACHE_SUPPORT_FLAGS deviceFeatures = D3D12_SHADER_CACHE_SUPPORT_NONE;

#if _TARGET_PC_WIN
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  bool rootSignaturesUsesCBVDescriptorRanges = false;
#else
  static constexpr bool rootSignaturesUsesCBVDescriptorRanges = false;
#endif

  enum class ShaderLookupResult
  {
    Missing,
    InConstantStore,
    InDynamicStore,
  };

  enum class GroupPreRecoveryResult
  {
    NoLibs,
    NoChanges,
    NoRescue,
    Rescue,
  };

  template <typename ShaderInfoStorageType>
  struct GroupBaseInfo
  {
    struct DynamicData
    {
      // May points to the same object a paired ConstantData::library does.
      ComPtr<ID3D12PipelineLibrary1> library;
      ShaderInfoStorageType vertexShaderSet;
      ShaderInfoStorageType pixelShaderSet;
      ShaderInfoStorageType computeShaderSet;
    };
    struct ConstantData : DynamicData
    {
      DynamicArray<uint8_t> libraryBlob;
    };
    ConstantData constantState;
    DynamicData dynamicState;

    bool hasLibrary() const { return constantState.library && dynamicState.library; }

    bool isInReplacementMode() const { return constantState.library.Get() != dynamicState.library.Get(); }

    ShaderLookupResult hasVertexShader(auto &&key) const
    {
      return constantState.vertexShaderSet.count(key) > 0  ? ShaderLookupResult::InConstantStore
             : dynamicState.vertexShaderSet.count(key) > 0 ? ShaderLookupResult::InDynamicStore
                                                           : ShaderLookupResult::Missing;
    }

    ShaderLookupResult hasPixelShader(auto &&key) const
    {
      return constantState.pixelShaderSet.count(key) > 0  ? ShaderLookupResult::InConstantStore
             : dynamicState.pixelShaderSet.count(key) > 0 ? ShaderLookupResult::InDynamicStore
                                                          : ShaderLookupResult::Missing;
    }

    ShaderLookupResult hasComputeShader(auto &&key) const
    {
      return constantState.computeShaderSet.count(key) > 0  ? ShaderLookupResult::InConstantStore
             : dynamicState.computeShaderSet.count(key) > 0 ? ShaderLookupResult::InDynamicStore
                                                            : ShaderLookupResult::Missing;
    }

    bool hasAnyData() const
    {
      return 0 < constantState.vertexShaderSet.size() || 0 < constantState.pixelShaderSet.size() ||
             0 < constantState.computeShaderSet.size() || 0 < dynamicState.vertexShaderSet.size() ||
             0 < dynamicState.pixelShaderSet.size() || 0 < dynamicState.computeShaderSet.size();
    }

    void reset()
    {
      constantState.vertexShaderSet.clear();
      constantState.pixelShaderSet.clear();
      constantState.computeShaderSet.clear();
      constantState.library.Reset();
      constantState.libraryBlob.resize(0);

      dynamicState.vertexShaderSet.clear();
      dynamicState.pixelShaderSet.clear();
      dynamicState.computeShaderSet.clear();
      dynamicState.library.Reset();
    }

    bool hasChanged() const
    {
      if (dynamicState.library.Get() != constantState.library.Get())
      {
        // when the dynamic one is different, then we are are in replacement mode and this means the lib has always changed
        return true;
      }
      if (!dynamicState.vertexShaderSet.empty() || !dynamicState.pixelShaderSet.empty() || !dynamicState.computeShaderSet.empty())
      {
        // if in dynamic is anything new, then something has changed
        return true;
      }
      // have to check if the size has changed, if no then nothing was added
      return constantState.libraryBlob.size() != dynamicState.library->GetSerializedSize();
    }

    static auto serializeShaderInfos(const auto &constant_store, const auto &dynamic_store, bool is_in_replacement_mode)
    {
      size_t count = dynamic_store.size();
      if (!is_in_replacement_mode)
      {
        count += constant_store.size();
      }
      DynamicArray<eastl::decay_t<decltype(*dynamic_store.begin())>> serialized;
      serialized.resize(count);
      eastl::copy(dynamic_store.begin(), dynamic_store.end(), serialized.data());
      if (!is_in_replacement_mode)
      {
        eastl::copy(constant_store.begin(), constant_store.end(), serialized.data() + dynamic_store.size());
      }
      return serialized;
    }

    auto serializeVertexShaderInfos() const
    {
      return serializeShaderInfos(constantState.vertexShaderSet, dynamicState.vertexShaderSet, isInReplacementMode());
    }
    auto serializePixelShaderInfo() const
    {
      return serializeShaderInfos(constantState.pixelShaderSet, dynamicState.pixelShaderSet, isInReplacementMode());
    }
    auto serializeComputeShaderInfo() const
    {
      return serializeShaderInfos(constantState.computeShaderSet, dynamicState.computeShaderSet, isInReplacementMode());
    }

    dag::Expected<DynamicArray<uint8_t>, HRESULT> serializeLibrary() const
    {
      if (!hasLibrary())
      {
        return dag::Unexpected<HRESULT>{DXGI_ERROR_UNSUPPORTED};
      }
      DynamicArray<uint8_t> store;
      store.resize(dynamicState.library->GetSerializedSize());
      if (0 == store.size())
      {
        return dag::Unexpected<HRESULT>{DXGI_ERROR_UNSUPPORTED};
      }
      auto errorCode = dynamicState.library->Serialize(store.data(), store.size());
      if (FAILED(errorCode))
      {
        return dag::Unexpected{errorCode};
      }
      return store;
    }

    GroupPreRecoveryResult preRecovery()
    {
      if (!hasLibrary())
      {
        return GroupPreRecoveryResult::NoLibs;
      }

      GroupPreRecoveryResult result = GroupPreRecoveryResult::NoChanges;
      if (hasChanged())
      {
        auto serializeLibraryResult = serializeLibrary();

        // compares both lib pointers, so need to store the result before reset
        const bool wasInReplacementModeOn = isInReplacementMode();

        // have to reset libs here, otherwise the replacement of libraryBlob, replaces still in use memory and may result in a crash
        constantState.library.Reset();
        dynamicState.library.Reset();

        if (serializeLibraryResult)
        {
          constantState.libraryBlob = eastl::move(serializeLibraryResult.value());
          if (!wasInReplacementModeOn)
          {
            // When not in replacement mode dynamic needs to be combined with constant state. Dynamic state may has
            // more up to date data as constant state, so we add constant to dynamic and then later swap them.
            dynamicState.vertexShaderSet.insert(constantState.vertexShaderSet.begin(), constantState.vertexShaderSet.end());
            dynamicState.pixelShaderSet.insert(constantState.pixelShaderSet.begin(), constantState.pixelShaderSet.end());
            dynamicState.computeShaderSet.insert(constantState.computeShaderSet.begin(), constantState.computeShaderSet.end());
          }
          // always swap, either it replaces the const state or const state was added to it
          eastl::swap(constantState.vertexShaderSet, dynamicState.vertexShaderSet);
          eastl::swap(constantState.pixelShaderSet, dynamicState.pixelShaderSet);
          eastl::swap(constantState.computeShaderSet, dynamicState.computeShaderSet);
          result = GroupPreRecoveryResult::Rescue;
        }
        else
        {
          result = GroupPreRecoveryResult::NoRescue;
        }

        // in all cases dynamic state is lost, on rescue the constant state was swapped with the dynamic state and so we resetting the
        // former constant state
        dynamicState.vertexShaderSet.clear();
        dynamicState.pixelShaderSet.clear();
        dynamicState.computeShaderSet.clear();

        // here we copy one single entry for each set over to the dynamic state, so that 'hasChanged' will stay true
        if (!constantState.vertexShaderSet.empty())
        {
          dynamicState.vertexShaderSet.insert(*constantState.vertexShaderSet.begin());
        }
        if (!constantState.pixelShaderSet.empty())
        {
          dynamicState.pixelShaderSet.insert(*constantState.pixelShaderSet.begin());
        }
        if (!constantState.computeShaderSet.empty())
        {
          dynamicState.computeShaderSet.insert(*constantState.computeShaderSet.begin());
        }
      }
      else
      {
        constantState.library.Reset();
        dynamicState.library.Reset();
      }

      return result;
    }

    // Returned success value tells the result from blob restore. S_FALSE indicates no blob was available and empty was created.
    dag::Expected<HRESULT, HRESULT> recover(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS features)
    {
      if (0 == (features & D3D12_SHADER_CACHE_SUPPORT_LIBRARY))
      {
        reset();
        return dag::Unexpected<HRESULT>{E_NOINTERFACE};
      }

      HRESULT fromBlobResult = S_FALSE;
      if (0 != constantState.libraryBlob.size())
      {
        fromBlobResult = device->CreatePipelineLibrary(constantState.libraryBlob.data(), constantState.libraryBlob.size(),
          COM_ARGS(&constantState.library));
        if (FAILED(fromBlobResult))
        {
          constantState.libraryBlob.resize(0);
        }
      }

      if (!constantState.library)
      {
        // when nothing was restored from blob then the lib is empty
        constantState.vertexShaderSet.clear();
        constantState.pixelShaderSet.clear();
        constantState.computeShaderSet.clear();
        dynamicState.vertexShaderSet.clear();
        dynamicState.pixelShaderSet.clear();
        dynamicState.computeShaderSet.clear();

        auto errorCode = device->CreatePipelineLibrary(nullptr, 0, COM_ARGS(&constantState.library));
        if (FAILED(errorCode))
        {
          return dag::Unexpected{errorCode};
        }
      }

      dynamicState.library = constantState.library;

      return {fromBlobResult};
    }
  };

  struct BuildInPipelineLibraryFileHeader
  {
    static constexpr uint32_t magic_value = 'BIPL';
    // Versions:
    // 1: Initial version
    //    - Support for blit pipelines and format table
    //    - Support for clear pipelines and format table
    //    - Support for pipeline library to store blit and clear pipeline caches
    //    - integrity of each stored part is validated with a ShaderHashValue
    //    - layout is header, header hash as ShaderHashValue, blit format array, clear format array, shader library blob
    static constexpr uint32_t version_value = 1;
    uint32_t magic;
    uint32_t version;
    ShaderHashValue blitHash;
    ShaderHashValue clearHash;
    ShaderHashValue blitFormatsHash;
    ShaderHashValue clearFormatsHash;
    uint32_t blitCount;
    uint32_t clearCount;
    uint32_t librarySize;
  };

  dag::Vector<DXGI_FORMAT> blitOutputFormats;
  dag::Vector<DXGI_FORMAT> clearOutputFormats;
  DynamicArray<uint8_t> loadedBuildInPipelineLibraryBlob;
  ComPtr<ID3D12PipelineLibrary1> buildInPipelineLibrary;
  bool buildInUpdated : 1 = false;
  bool buildInShouldRecord : 1 = false;

  void setupBuildInPipelineLibrary(const SetupParameters &params, const D3D12_SHADER_CACHE_SUPPORT_FLAGS cache_support);
  void loadBuildInPipelineLibraryFile(const SetupParameters &params);
  void storeBuildInPipelineLibraryFile(const ShutdownParameters &params);
  void preRecoveryBuildInPipelineLibrary();
  void recoverBuildInPipelineLibrary(D3DDevice *device, D3D12_SHADER_CACHE_SUPPORT_FLAGS allowed_modes);
  void shutdownBuildInPipelineLibrary();

  struct UngroupedShaderInfo
  {
    uint64_t firstRun;
    uint64_t lastRun;
  };

  struct UngroupedPipelinesInfo : GroupBaseInfo<dag::VectorMap<dxil::HashValue, UngroupedShaderInfo>>
  {
    bool needsLoadRewrite() const { return true; }

    void addVertexShaderInfo(const dxil::HashValue &key, uint64_t run_value)
    {
      dynamicState.vertexShaderSet.insert(eastl::make_pair(key, UngroupedShaderInfo{.firstRun = run_value, .lastRun = run_value}));
    }
    void addPixelShaderInfo(const dxil::HashValue &key, uint64_t run_value)
    {
      dynamicState.pixelShaderSet.insert(eastl::make_pair(key, UngroupedShaderInfo{.firstRun = run_value, .lastRun = run_value}));
    }
    void addComputeShaderInfo(const dxil::HashValue &key, uint64_t run_value)
    {
      dynamicState.computeShaderSet.insert(eastl::make_pair(key, UngroupedShaderInfo{.firstRun = run_value, .lastRun = run_value}));
    }

    void rewriteVertexShaderInfo(const dxil::HashValue &key, uint64_t run_value)
    {
      if (constantState.vertexShaderSet.count(key) > 0)
      {
        constantState.vertexShaderSet[key].lastRun = run_value;
      }
    }

    void rewritePixelShaderInfo(const dxil::HashValue &key, uint64_t run_value)
    {
      if (constantState.pixelShaderSet.count(key) > 0)
      {
        constantState.pixelShaderSet[key].lastRun = run_value;
      }
    }

    void rewriteComputeShaderInfo(const dxil::HashValue &key, uint64_t run_value)
    {
      if (constantState.computeShaderSet.count(key) > 0)
      {
        constantState.computeShaderSet[key].lastRun = run_value;
      }
    }

    void replacementVertexShaderUse(const dxil::HashValue &key)
    {
      dynamicState.vertexShaderSet[key] = constantState.vertexShaderSet[key];
    }

    void replacementPixelShaderUse(const dxil::HashValue &key)
    {
      dynamicState.pixelShaderSet[key] = constantState.pixelShaderSet[key];
    }

    void replacementComputeShaderUse(const dxil::HashValue &key)
    {
      dynamicState.computeShaderSet[key] = constantState.computeShaderSet[key];
    }
  };

  ContainerMutexWrapper<UngroupedPipelinesInfo, OSSpinlock> ungroupedPipelinesInfo;
  // After load this is constant, so no need to be in the wrapper
  uint64_t ungroupedPipelinesRunIndex = 0;

  bool setupUngroupedPipelineLibrary(const SetupParameters &params);
  void storeUngroupedPipelineLibraryFile(const ShutdownParameters &params);
  void preRecoveryUngroupedPipelineLibrary();
  void shutdownUngroupedPipelineLibrary();

  struct ShaderDumpInfo : GroupBaseInfo<dag::VectorSet<dxil::HashValue>>
  {
    bool needsLoadRewrite() const { return isInReplacementMode(); }

    void addVertexShaderInfo(const dxil::HashValue &key) { dynamicState.vertexShaderSet.insert(key); }
    void addPixelShaderInfo(const dxil::HashValue &key) { dynamicState.pixelShaderSet.insert(key); }
    void addComputeShaderInfo(const dxil::HashValue &key) { dynamicState.computeShaderSet.insert(key); }

    void rewriteVertexShaderInfo(const dxil::HashValue &) {}
    void rewritePixelShaderInfo(const dxil::HashValue &) {}
    void rewriteComputeShaderInfo(const dxil::HashValue &) {}

    void replacementVertexShaderUse(const dxil::HashValue &key) { dynamicState.vertexShaderSet.insert(key); }
    void replacementPixelShaderUse(const dxil::HashValue &key) { dynamicState.pixelShaderSet.insert(key); }
    void replacementComputeShaderUse(const dxil::HashValue &key) { dynamicState.computeShaderSet.insert(key); }
  };

  uint32_t preRecoverActiveGroup = 0;
  // minus one because group 0 is special and not handled by this
  ContainerMutexWrapper<ShaderDumpInfo, OSSpinlock> shaderDumpsInfos[max_scripted_shaders_bin_groups - 1];
  ContainerMutexWrapper<ShaderDumpInfo, OSSpinlock> *getGroup(uint32_t index)
  {
    return ((index > 0) && (index < max_scripted_shaders_bin_groups)) ? &shaderDumpsInfos[index - 1] : nullptr;
  }

  void waitForWriterThreads();

  // counts how many async writes were kicked off, but have not started their thread
  dag::AtomicInteger<uint32_t> issuedCacheWrites{0};
  // counts how many async write threads are working on the write operation
  dag::AtomicInteger<uint32_t> pendingCacheWrites{0};
#endif
};
} // namespace drv3d_dx12
