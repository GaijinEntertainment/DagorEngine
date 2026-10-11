// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "pipeline_cache.h"

#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <supp/dag_comPtr.h>

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>


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

struct RayTracePipelineRebuildData
{
  struct Import
  {
    ComPtr<ID3D12StateObject> collection;
    eastl::vector<eastl::wstring> exportNames;
  };
  struct HitGroup
  {
    eastl::wstring exportName;
    eastl::wstring anyHitImport;
    eastl::wstring closestHitImport;
    eastl::wstring intersectionImport;
    D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
  };
  eastl::vector<Import> imports;
  eastl::vector<HitGroup> hitGroups;
  // export name of each shader group in shader handle order, for identifier lookup after a rebuild
  eastl::vector<eastl::wstring> groupExportNames;
  // Config the rebuild emits and the next expansion inherits. appendRebuildData overwrites all of
  // these with the addition's values, so after an expansion they describe the expanded pipeline.
  uint32_t minRecursion = 1;
  uint32_t minPayload = 4;
  uint32_t minAttributes = sizeof(float) * 2;
  bool allowOpacityMicroMaps = false;
};

class RayTracePipeline
{
  ComPtr<ID3D12StateObject> object;
  RaytracePipelineSignature rootSignature;
  RayTracePipelineResourceTypeTable resourceTypeTable;
  DynamicArray<uint8_t> shaderReferenceTableStorage;
  eastl::string debugName;
  // Data for a later expansion, expand refuses a pipeline that has none or that is not expandable.
  eastl::optional<RayTracePipelineRebuildData> rebuildData;

public:
  RayTracePipeline() = default;
  RayTracePipeline(const RayTracePipeline &) = delete;
  RayTracePipeline(RayTracePipeline &&) = delete;
  RayTracePipeline &operator=(const RayTracePipeline &) = delete;
  RayTracePipeline &operator=(RayTracePipeline &&) = delete;
  ~RayTracePipeline() = default;

  struct BuildConfig
  {
    // Collection-style libraries (raygen-only / hit-group imports) carry their shader and pipeline
    // config subobjects in the DXIL, so the driver must not emit duplicates. When set, the driver
    // trusts the embedded configs. The builder still forces a CPU-side pipeline config when a library
    // carries a DSHL raytrace_pipeline block, which it detects from the per-shader flags itself.
    bool useEmbeddedObjects = false;
  };

  bool build(AnyDevicePtr device_ptr, bool allow_native_expand, bool device_allows_omm, PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializer,
    RayTracePipeline *base, const ::raytrace::PipelineCreateInfo &ci);
  // build function for simpler pipeline builds with supplied root signature, currently used for CS on RayGen
  bool build(D3DDevice *device, const RaytracePipelineSignature &root_signature,
    const RayTracePipelineResourceTypeTable &res_type_table, const ::raytrace::PipelineCreateInfo &ci, const BuildConfig &build_config,
    bool device_allows_omm);
  RayTracePipeline *expand(ID3D12Device7 *device, const ::raytrace::PipelineExpandInfo &ei, bool allow_native_expand) const;
  ID3D12StateObject *get() { return object.Get(); }
  const RaytracePipelineSignature &getSignature() { return rootSignature; }
  const uint8_t *getShaderReferenceTable() const { return shaderReferenceTableStorage.data(); }
  uint32_t getShaderCount() const { return shaderReferenceTableStorage.size() / D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; }
  dag::ConstSpan<uint8_t> getShaderReferenceTableSpan() const
  {
    return make_span(shaderReferenceTableStorage.data(), shaderReferenceTableStorage.size());
  }
  bool isExpandable() const { return rebuildData.has_value(); }
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
