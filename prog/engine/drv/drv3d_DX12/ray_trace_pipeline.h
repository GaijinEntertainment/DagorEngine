// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include "shader_library.h"
#include <EASTL/intrusive_ptr.h>

#if D3D_HAS_RAY_TRACING

namespace drv3d_dx12
{
struct RayTracePipelineResourceTypeTable
{
  uint32_t sRegisterCompareUseMask;
  // Needed for null fallback, need to know which kind to use
  uint8_t tRegisterTypes[dxil::MAX_T_REGISTERS];
  uint8_t uRegisterTypes[dxil::MAX_U_REGISTERS];
};

class RayTracePipeline
{
  // Holds the data we need to properly support expansion
  struct ExpansionSupportData
  {
    uint32_t minRecursion = 0;
    uint32_t minPayload = 0;
    uint32_t minAttributes = 0;
    bool isSupported = false;
  };
  ComPtr<ID3D12StateObject> object;
  RaytracePipelineSignature rootSignature;
  RayTracePipelineResourceTypeTable resourceTypeTable;
  DynamicArray<uint8_t> shaderReferenceTableStorage;
  eastl::string debugName;
  ExpansionSupportData expansionData;

public:
  RayTracePipeline() = default;
  RayTracePipeline(const RayTracePipeline &) = delete;
  RayTracePipeline(RayTracePipeline &&) = delete;
  RayTracePipeline &operator=(const RayTracePipeline &) = delete;
  RayTracePipeline &operator=(RayTracePipeline &&) = delete;
  ~RayTracePipeline() = default;

  bool build(AnyDevicePtr device_ptr, bool has_native_expand, PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializer, RayTracePipeline *base,
    const ::raytrace::PipelineCreateInfo &ci);
  RayTracePipeline *expand(ID3D12Device7 *device, const ::raytrace::PipelineExpandInfo &ei) const;
  ID3D12StateObject *get() { return object.Get(); }
  const RaytracePipelineSignature &getSignature() { return rootSignature; }
  const uint8_t *getShaderReferenceTable() const { return shaderReferenceTableStorage.data(); }
  uint32_t getShaderCount() const { return shaderReferenceTableStorage.size() / D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; }
  bool isExpandable() const { return expansionData.isSupported; }
  const eastl::string &name() const { return debugName; }
  // TODO
  uint32_t getRayGenGroupShaderRecordDataSize() const { return 0; }
  uint32_t getMissGroupShaderRecordDataSize() const { return 0; }
  uint32_t getHitGroupShaderRecordDataSize() const { return 0; }
  uint32_t getCallableGroupShaderRecordDataSize() const { return 0; }
};

// small wrapper to make it possible to create pipeline values and access their members without hacky casts.
class RayTracePipelineWrapper final : public ::raytrace::Pipeline
{
public:
  RayTracePipelineWrapper(const ::raytrace::Pipeline &base) : ::raytrace::Pipeline{base} {}
  RayTracePipelineWrapper(RayTracePipeline *ptr) :
    ::raytrace::Pipeline{ptr, ptr->getShaderReferenceTable(), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, ptr->getShaderCount()}
  {}

  RayTracePipeline *get() const { return static_cast<RayTracePipeline *>(driverObject); }
};
} // namespace drv3d_dx12

#endif
