// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device.h"

#if D3D_HAS_RAY_TRACING
namespace
{
struct ShaderResourceUsageTableWithResourceTypes
{
  dxil::ShaderResourceUsageTable resourceUsageTable;
  drv3d_dx12::RayTracePipelineResourceTypeTable resourceTypeTable;
};

struct ShaderCollectionImportInfo
{
  D3D12_EXISTING_COLLECTION_DESC desc;
  dag::Vector<D3D12_EXPORT_DESC> exports;
};

bool equals(const ShaderResourceUsageTableWithResourceTypes &set, const dxil::ShaderResourceUsageTable &other_use,
  const drv3d_dx12::RayTracePipelineResourceTypeTable &other_type)
{
  auto &set_use = set.resourceUsageTable;
  auto &set_type = set.resourceTypeTable;
  if ((set_use.tRegisterUseMask != other_use.tRegisterUseMask) || (set_use.sRegisterUseMask != other_use.sRegisterUseMask) ||
      (set_use.bindlessUsageMask != other_use.bindlessUsageMask) || (set_use.bRegisterUseMask != other_use.bRegisterUseMask) ||
      (set_use.uRegisterUseMask != other_use.uRegisterUseMask) || (set_use.rootConstantDwords != other_use.rootConstantDwords) ||
      (set_use.specialConstantsMask != other_use.specialConstantsMask))
  {
    return false;
  }
  if (0 != (set_use.sRegisterUseMask & (set_type.sRegisterCompareUseMask ^ other_type.sRegisterCompareUseMask)))
  {
    return false;
  }
  for (auto i : LsbVisitor{set_use.tRegisterUseMask})
  {
    if (set_type.tRegisterTypes[i] != other_type.tRegisterTypes[i])
    {
      return false;
    }
  }
  for (auto i : LsbVisitor{set_use.uRegisterUseMask})
  {
    if (set_type.uRegisterTypes[i] != other_type.uRegisterTypes[i])
    {
      return false;
    }
  }
  return true;
}

ShaderResourceUsageTableWithResourceTypes merge(const ShaderResourceUsageTableWithResourceTypes &base,
  const dxil::ShaderResourceUsageTable &expand_shader, const dxil::LibraryResourceInformation &expand_lib, bool &has_conflict)
{
  has_conflict = false;
  ShaderResourceUsageTableWithResourceTypes result;
  result.resourceUsageTable.tRegisterUseMask = base.resourceUsageTable.tRegisterUseMask | expand_shader.tRegisterUseMask;
  result.resourceUsageTable.sRegisterUseMask = base.resourceUsageTable.sRegisterUseMask | expand_shader.sRegisterUseMask;
  result.resourceUsageTable.bindlessUsageMask = base.resourceUsageTable.bindlessUsageMask | expand_shader.bindlessUsageMask;
  result.resourceUsageTable.bRegisterUseMask = base.resourceUsageTable.bRegisterUseMask | expand_shader.bRegisterUseMask;
  result.resourceUsageTable.uRegisterUseMask = base.resourceUsageTable.uRegisterUseMask | expand_shader.uRegisterUseMask;
  result.resourceUsageTable.rootConstantDwords = max(base.resourceUsageTable.rootConstantDwords, expand_shader.rootConstantDwords);
  result.resourceUsageTable.specialConstantsMask = base.resourceUsageTable.specialConstantsMask | expand_shader.specialConstantsMask;
  auto expandSRegisterCompareUseMask = expand_shader.sRegisterUseMask & expand_lib.sRegisterCompareUseMask;
  result.resourceTypeTable.sRegisterCompareUseMask = base.resourceTypeTable.sRegisterCompareUseMask | expandSRegisterCompareUseMask;
  // on the s register slots that are used by both base and expand_shader, they have to be matching compare or non-compare samplers
  // types
  if ((base.resourceUsageTable.sRegisterUseMask & expand_shader.sRegisterUseMask) &
      (base.resourceTypeTable.sRegisterCompareUseMask ^ expandSRegisterCompareUseMask))
  {
    has_conflict = true;
  }
  for (auto i : LsbVisitor{result.resourceUsageTable.tRegisterUseMask})
  {
    uint32_t bit = 1u << i;
    result.resourceTypeTable.tRegisterTypes[i] = base.resourceTypeTable.tRegisterTypes[i] | expand_lib.tRegisterTypes[i];
    has_conflict = has_conflict || (((base.resourceUsageTable.tRegisterUseMask & bit) == (expand_shader.tRegisterUseMask & bit)) &&
                                     (base.resourceTypeTable.tRegisterTypes[i] != expand_lib.tRegisterTypes[i]));
  }
  for (auto i : LsbVisitor{result.resourceUsageTable.uRegisterUseMask})
  {
    uint32_t bit = 1u << i;
    result.resourceTypeTable.uRegisterTypes[i] = base.resourceTypeTable.uRegisterTypes[i] | expand_lib.uRegisterTypes[i];
    has_conflict = has_conflict || (((base.resourceUsageTable.uRegisterUseMask & bit) == (expand_shader.uRegisterUseMask & bit)) &&
                                     (base.resourceTypeTable.uRegisterTypes[i] != expand_lib.uRegisterTypes[i]));
  }
  return result;
}

const drv3d_dx12::ShaderLibrary *to_dx12_lib(const ::ShaderLibrary lib)
{
  return reinterpret_cast<const drv3d_dx12::ShaderLibrary *>(lib);
}

void append_name(eastl::wstring &target, const eastl::wstring_view prefix, ShaderInLibraryReference ref)
{
  if (InvalidShaderLibrary == ref.library)
  {
    return;
  }

  target.append(begin(prefix), end(prefix));
  to_dx12_lib(ref.library)->appendNameOfTo(ref.index, target);
}

const wchar_t *make_hit_group_name(ShaderInLibraryReference any_hit, ShaderInLibraryReference closest_hit,
  ShaderInLibraryReference intersection, DynamicArray<wchar_t> &target)
{
  eastl::wstring buffer;
  append_name(buffer, L"aHit_", any_hit);
  append_name(buffer, L"cHit_", closest_hit);
  append_name(buffer, L"isec_", intersection);
  target.resize(buffer.length() + 1);
  eastl::copy(begin(buffer), end(buffer), target.data());
  target[buffer.length()] = L'\0';
  return target.data();
}

const wchar_t *copy_string_to(DynamicArray<wchar_t> &&source, DynamicArray<wchar_t> &target)
{
  target = eastl::move(source);
  return target.data();
}

DynamicArray<wchar_t> name_of_shader_ref(const ShaderInLibraryReference &ref)
{
  return InvalidShaderLibrary != ref.library ? to_dx12_lib(ref.library)->makeNameOfAsDynArray(ref.index) : DynamicArray<wchar_t>{};
}

template <unsigned C>
struct AutoLifetimeTimer
{
  eastl::string timingFormatString;
  AutoFuncProfT<C> timingData;
  template <typename T>
  AutoLifetimeTimer(T maker) : timingFormatString{maker()}, timingData{timingFormatString.c_str()}
  {}

  void abbortTiming() { timingData.fmt = nullptr; }
};

// TODO copy pasta from compute signature builder, needs to be refactored
struct RootSignatureGeneratorCallback
{
  drv3d_dx12::RaytracePipelineSignature *signature = nullptr;
  D3D12_DESCRIPTOR_RANGE ranges[dxil::MAX_T_REGISTERS + dxil::MAX_S_REGISTERS + dxil::MAX_U_REGISTERS + dxil::MAX_B_REGISTERS] = {};
  // cbuffer each one param, then srv, uav and sampler each one and one for each bindless sampler and srv globally
  D3D12_ROOT_PARAMETER params[dxil::MAX_B_REGISTERS + 1 + 1 + 1 + 2] = {};
  D3D12_ROOT_SIGNATURE_DESC desc = {};
  D3D12_DESCRIPTOR_RANGE *rangePosition = &ranges[0];
  D3D12_ROOT_PARAMETER *unboundedSamplersRootParam = nullptr;
  D3D12_ROOT_PARAMETER *bindlessSRVRootParam = nullptr;
  uint32_t rangeSize = 0;
  uint32_t signatureCost = 0;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  bool useConstantBufferRootDescriptors = false;
  bool shouldUseConstantBufferRootDescriptors() const { return useConstantBufferRootDescriptors; }
#else
  constexpr bool shouldUseConstantBufferRootDescriptors() const { return true; }
#endif
  void begin()
  {
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    desc.pParameters = params;
  }
  void end() {}
  void beginFlags() {}
  void endFlags() {}
  void hasAccelerationStructure()
  {
#if _TARGET_SCARLETT
    desc.Flags |= ROOT_SIGNATURE_FLAG_INTERNAL_XDXR;
#endif
  }
  void rootConstantBuffer(uint32_t space, uint32_t index, uint32_t dwords)
  {
    G_ASSERT(desc.NumParameters < countof(params));

    signature->def.csLayout.rootConstantsParamIndex = desc.NumParameters;

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    target.Constants.ShaderRegister = index;
    target.Constants.RegisterSpace = space;
    target.Constants.Num32BitValues = dwords;

    signatureCost += dwords;
  }
  void beginConstantBuffers()
  {
    signature->def.csLayout.setConstBufferDescriptorIndex(desc.NumParameters, shouldUseConstantBufferRootDescriptors());
  }
  void endConstantBuffers()
  {
    if (!shouldUseConstantBufferRootDescriptors())
    {
      G_ASSERT(desc.NumParameters < countof(params));
      G_ASSERT(rangeSize > 0);

      auto &target = params[desc.NumParameters++];
      target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
      target.DescriptorTable.NumDescriptorRanges = rangeSize;
      target.DescriptorTable.pDescriptorRanges = rangePosition;

      rangePosition += rangeSize;
      rangeSize = 0;
      G_ASSERT(rangePosition <= eastl::end(ranges));

      signatureCost += 1; // offset into active descriptor heap
    }
  }
  void constantBuffer(uint32_t space, uint32_t slot, uint32_t linear_index)
  {
    if (shouldUseConstantBufferRootDescriptors())
    {
      G_UNUSED(linear_index);
      G_ASSERT(desc.NumParameters < countof(params));

      auto &target = params[desc.NumParameters++];
      target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
      target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
      target.Descriptor.ShaderRegister = slot;
      target.Descriptor.RegisterSpace = space;

      signatureCost += 2; // cbuffer is a 64bit gpu address
    }
    else
    {
      auto &target = rangePosition[rangeSize++];
      G_ASSERT(&target < eastl::end(ranges));
      target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      target.NumDescriptors = 1;
      target.BaseShaderRegister = slot;
      target.RegisterSpace = space;
      target.OffsetInDescriptorsFromTableStart = linear_index;
    }
  }
  void beginSamplers() { signature->def.csLayout.samplersParamIndex = desc.NumParameters; }
  void endSamplers()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    G_ASSERT(rangeSize > 0);

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    target.DescriptorTable.NumDescriptorRanges = rangeSize;
    target.DescriptorTable.pDescriptorRanges = rangePosition;

    rangePosition += rangeSize;
    rangeSize = 0;
    G_ASSERT(rangePosition <= eastl::end(ranges));

    signatureCost += 1; // offset into active descriptor heap
  }
  void sampler(uint32_t space, uint32_t slot, uint32_t linear_index)
  {
    G_UNUSED(linear_index); // rangeSize will be the same as linear_index
    auto &target = rangePosition[rangeSize];
    G_ASSERT(&target < eastl::end(ranges));
    target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    target.NumDescriptors = 1;
    target.BaseShaderRegister = slot;
    target.RegisterSpace = space;
    target.OffsetInDescriptorsFromTableStart = rangeSize++;
  }
  void beginBindlessSamplers()
  {
    // compute has only one stage, all unbounded samplers should be added within a single begin-end block
    G_ASSERT(unboundedSamplersRootParam == nullptr);
    G_ASSERT(desc.NumParameters < countof(params));
    //        signature->def.layout.bindlessSamplersParamIndex = desc.NumParameters++;
    unboundedSamplersRootParam = &params[0 /*signature->def.layout.bindlessSamplersParamIndex*/];
    unboundedSamplersRootParam->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    unboundedSamplersRootParam->DescriptorTable.pDescriptorRanges = rangePosition;
    unboundedSamplersRootParam->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    signatureCost += 1; // 1 root param for all unbounded sampler array ranges
  }
  void endBindlessSamplers() { G_ASSERT(unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges != 0); }
  void bindlessSamplers(uint32_t space, uint32_t slot)
  {
    G_ASSERT(space < dxil::MAX_UNBOUNDED_REGISTER_SPACES);

    auto &smpRange = rangePosition[0];
    G_ASSERT(&smpRange < eastl::end(ranges));
    smpRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    smpRange.NumDescriptors = UINT_MAX; // UINT_MAX means unbounded
    smpRange.BaseShaderRegister = slot;
    smpRange.RegisterSpace = space;
    smpRange.OffsetInDescriptorsFromTableStart = 0;

    unboundedSamplersRootParam->DescriptorTable.NumDescriptorRanges++;
    rangePosition++;
    G_ASSERT(rangePosition <= eastl::end(ranges));
  }
  void beginShaderResourceViews() { signature->def.csLayout.shaderResourceViewParamIndex = desc.NumParameters; }
  void endShaderResourceViews()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    G_ASSERT(rangeSize > 0);

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    target.DescriptorTable.NumDescriptorRanges = rangeSize;
    target.DescriptorTable.pDescriptorRanges = rangePosition;

    rangePosition += rangeSize;
    rangeSize = 0;
    G_ASSERT(rangePosition <= eastl::end(ranges));

    signatureCost += 1; // offset into active descriptor heap
  }
  void shaderResourceView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
  {
    auto &target = rangePosition[rangeSize++];
    G_ASSERT(&target < eastl::end(ranges));
    target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    target.NumDescriptors = descriptor_count;
    target.BaseShaderRegister = slot;
    target.RegisterSpace = space;
    target.OffsetInDescriptorsFromTableStart = linear_index;
  }
  void beginBindlessShaderResourceViews()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    //        signature->def.layout.bindlessShaderResourceViewParamIndex = desc.NumParameters++;
    bindlessSRVRootParam = &params[0 /*signature->def.layout.bindlessShaderResourceViewParamIndex*/];
    bindlessSRVRootParam->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    bindlessSRVRootParam->DescriptorTable.pDescriptorRanges = rangePosition;
    bindlessSRVRootParam->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    signatureCost += 1; // 1 root param for all unbounded sampler array ranges
  }
  void endBindlessShaderResourceViews() {}
  void bindlessShaderResourceViews(uint32_t space, uint32_t slot)
  {
    G_ASSERT(space < dxil::MAX_UNBOUNDED_REGISTER_SPACES);

    auto &registerRange = rangePosition[0];
    G_ASSERT(&registerRange < eastl::end(ranges));
    registerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    registerRange.NumDescriptors = UINT_MAX; // UINT_MAX means unbounded
    registerRange.BaseShaderRegister = slot;
    registerRange.RegisterSpace = space;
    registerRange.OffsetInDescriptorsFromTableStart = 0;

    bindlessSRVRootParam->DescriptorTable.NumDescriptorRanges++;
    rangePosition++;
    G_ASSERT(rangePosition <= eastl::end(ranges));
  }
  void beginUnorderedAccessViews() { signature->def.csLayout.unorderedAccessViewParamIndex = desc.NumParameters; }
  void endUnorderedAccessViews()
  {
    G_ASSERT(desc.NumParameters < countof(params));
    G_ASSERT(rangeSize > 0);

    auto &target = params[desc.NumParameters++];
    target.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    target.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    target.DescriptorTable.NumDescriptorRanges = rangeSize;
    target.DescriptorTable.pDescriptorRanges = rangePosition;

    rangePosition += rangeSize;
    rangeSize = 0;
    G_ASSERT(rangePosition <= eastl::end(ranges));

    signatureCost += 1; // offset into active descriptor heap
  }
  void unorderedAccessView(uint32_t space, uint32_t slot, uint32_t descriptor_count, uint32_t linear_index)
  {
    auto &target = rangePosition[rangeSize++];
    G_ASSERT(&target < eastl::end(ranges));
    target.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    target.NumDescriptors = descriptor_count;
    target.BaseShaderRegister = slot;
    target.RegisterSpace = space;
    target.OffsetInDescriptorsFromTableStart = linear_index;
  }
};

struct PipelineBuilder : AutoLifetimeTimer<AFP_MSEC>
{
  PipelineBuilder(const ::raytrace::PipelineExpandInfo &ei, uint32_t min_rec, uint32_t min_pl, uint32_t min_attr,
    const drv3d_dx12::RaytracePipelineSignature &rps, const drv3d_dx12::RayTracePipelineResourceTypeTable &rtprtt) :
    AutoLifetimeTimer<AFP_MSEC>{[&ei]() {
      eastl::string s;
      s = "DX12: PipelineBuilder: Expanding pipeline <";
      s += ei.name ? ei.name : "UNKNOWN";
      s += "> took %ums";
      return s;
    }},
    pipelineInfo{ei},
    minRecursion{min_rec},
    minPayload{min_pl},
    minAttributes{min_attr}
  {
    resourceUses.resourceUsageTable = rps.def.registers;
    resourceUses.resourceTypeTable = rtprtt;
    nameTable.reserve(ei.groupMemebers.size());
    for (auto m : ei.groupMemebers)
    {
      m.visit([this](const auto &member) { onMember(member); });
    }
  }
  PipelineBuilder(const ::raytrace::PipelineCreateInfo &ci) :
    AutoLifetimeTimer<AFP_MSEC>{[&ci]() {
      eastl::string s;
      s = "DX12: PipelineBuilder: building pipeline <";
      s += ci.name ? ci.name : "UNKNOWN";
      s += "> took %ums";
      return s;
    }},
    pipelineInfo{ci},
    minRecursion{max<uint32_t>(1, ci.maxRecursionDepth)},
    minPayload{max<uint32_t>(4u, ci.maxPayloadSize)},
    minAttributes{max<uint32_t>(sizeof(float) * 2, ci.maxAttributeSize)}
  {
    nameTable.reserve(ci.groupMemebers.size());
    for (auto m : ci.groupMemebers)
    {
      m.visit([this](const auto &member) { onMember(member); });
    }
  }
  const ::raytrace::PipelineExpandInfo &pipelineInfo;
  const char *name() const { return pipelineInfo.name ? pipelineInfo.name : "UNKNOWN"; }
  bool isOk = true;
  ShaderResourceUsageTableWithResourceTypes resourceUses;
  eastl::vector<ShaderCollectionImportInfo> importInfos;
  eastl::vector<D3D12_HIT_GROUP_DESC> hitGroups;
  dag::Vector<DynamicArray<wchar_t>> nameTable;
  dag::Vector<DynamicArray<wchar_t>> tempNameTable;
  uint32_t minRecursion = 1;
  uint32_t minPayload = 4;
  uint32_t minAttributes = sizeof(float) * 2;
  void onError()
  {
    isOk = false;
    abbortTiming();
  }
  void addImport(const drv3d_dx12::ShaderLibrary *lib, uint32_t index)
  {
    auto shaderName = lib->makeNameOfAsDynArray(index);
    if (0 == shaderName.size())
    {
      return;
    }
    auto importRef = eastl::find_if(begin(importInfos), end(importInfos),
      [lib](const auto &info) { return info.desc.pExistingCollection == lib->get(); });
    if (importRef == end(importInfos))
    {
      importRef = importInfos.emplace(importRef);
      importRef->desc.pExistingCollection = lib->get();
    }
    auto &newExport = importRef->exports.emplace_back();
    newExport.Name = copy_string_to(eastl::move(shaderName), tempNameTable.emplace_back());
    newExport.ExportToRename = nullptr;
    newExport.Flags = D3D12_EXPORT_FLAG_NONE;
    importRef->desc.NumExports = importRef->exports.size();
    importRef->desc.pExports = importRef->exports.data();
  }
  bool addResourceUsageTable(const ShaderInLibraryReference &shader_ref, bool is_optional)
  {
    if (InvalidShaderLibrary == shader_ref.library)
    {
      if (is_optional)
      {
        return false;
      }
      logerr("DX12: RayTracePipeline::PipelineBuilder: required shader was not defined");
      // report invalid input
      onError();
    }
    if (!isOk)
    {
      return false;
    }

    auto lib = to_dx12_lib(shader_ref.library);
    if (pipelineInfo.expandable)
    {
      if (!lib->canBeUsedByExpandablePipelines())
      {
        // lib needs to support expandable if we try to create a expandable pipeline
        logerr("DX12: RayTracePipeline::PipelineBuilder: Attempting to create a pipeline with expandable support, but shader library "
               "<%s> was created without support for expandable pipelines",
          lib->name());
        onError();
        return false;
      }
    }

    logdbg("DX12: RayTracePipeline::PipelineBuilder: Trying to merge resource usages");
    bool hasConflict = false;
    auto newUses =
      merge(resourceUses, lib->getShaderPropertiesOf(shader_ref.index).resourceUsageTable, lib->getGlobalResourceInfo(), hasConflict);
    if (hasConflict)
    {
      logerr("DX12: RayTracePipeline::PipelineBuilder: Unable to merge resource usages of shaders");
      onError();
      return false;
    }
    logdbg("DX12: RayTracePipeline::PipelineBuilder: Merged resource usages successfully");

    resourceUses = newUses;

    auto &properties = lib->getShaderPropertiesOf(shader_ref.index);
    minRecursion = max<uint32_t>(minRecursion, properties.maxRecusionDepth);
    minPayload = max<uint32_t>(minPayload, properties.maxPayloadSizeInBytes);
    minAttributes = max<uint32_t>(minAttributes, properties.maxAttributeSizeInBytes);

    addImport(lib, shader_ref.index);

    return true;
  }

  void onMember(const raytrace::RayGenShaderGroupMember &member)
  {
    if (addResourceUsageTable(member.rayGen, false))
    {
      copy_string_to(name_of_shader_ref(member.rayGen), nameTable.emplace_back());
    }
  }
  void onMember(const raytrace::MissShaderGroupMember &member)
  {
    if (addResourceUsageTable(member.miss, false))
    {
      copy_string_to(name_of_shader_ref(member.miss), nameTable.emplace_back());
    }
  }
  void onMember(const raytrace::CallableShaderGroupMember &member)
  {
    if (addResourceUsageTable(member.callable, false))
    {
      copy_string_to(name_of_shader_ref(member.callable), nameTable.emplace_back());
    }
  }
  void onMember(const raytrace::TriangleShaderGroupMember &member)
  {
    bool hasHit = addResourceUsageTable(member.anyHit, true);
    bool hasCHit = addResourceUsageTable(member.closestHit, !hasHit);

    if (hasHit || hasCHit)
    {
      D3D12_HIT_GROUP_DESC &hitGroup = hitGroups.emplace_back();
      hitGroup.HitGroupExport =
        make_hit_group_name(member.anyHit, member.closestHit, {InvalidShaderLibrary, 0}, nameTable.emplace_back());
      hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
      hitGroup.AnyHitShaderImport = copy_string_to(name_of_shader_ref(member.anyHit), tempNameTable.emplace_back());
      hitGroup.ClosestHitShaderImport = copy_string_to(name_of_shader_ref(member.closestHit), tempNameTable.emplace_back());
      hitGroup.IntersectionShaderImport = nullptr;
    }
  }
  void onMember(const raytrace::ProceduralShaderGroupMember &member)
  {
    bool hasHit = addResourceUsageTable(member.anyHit, true);
    bool hasCHit = addResourceUsageTable(member.closestHit, !hasHit);
    bool hasIsec = addResourceUsageTable(member.intersection, false);

    if ((hasHit || hasCHit) && hasIsec)
    {
      D3D12_HIT_GROUP_DESC &hitGroup = hitGroups.emplace_back();
      hitGroup.HitGroupExport = make_hit_group_name(member.anyHit, member.closestHit, member.intersection, nameTable.emplace_back());
      hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
      hitGroup.AnyHitShaderImport = copy_string_to(name_of_shader_ref(member.anyHit), tempNameTable.emplace_back());
      hitGroup.ClosestHitShaderImport = copy_string_to(name_of_shader_ref(member.closestHit), tempNameTable.emplace_back());
      hitGroup.IntersectionShaderImport = copy_string_to(name_of_shader_ref(member.intersection), tempNameTable.emplace_back());
    }
  }

  bool createSignature(ID3D12Device *device, PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializer,
    drv3d_dx12::RaytracePipelineSignature &target, drv3d_dx12::RayTracePipelineResourceTypeTable &type_table)
  {
    logdbg("DX12: Generating root signature...");
    RootSignatureGeneratorCallback generator;
    generator.signature = &target;
    target.def.registers = resourceUses.resourceUsageTable;
    type_table = resourceUses.resourceTypeTable;
    decode_compute_root_signature(true, resourceUses.resourceUsageTable, generator);
    logdbg("DX12: Completed with %u parameters, creating object", generator.desc.NumParameters);
    ComPtr<ID3DBlob> rootSignBlob;
    ComPtr<ID3DBlob> errorBlob;

    if (DX12_CHECK_FAIL(serializer(&generator.desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob)))
    {
      logerr("DX12: D3D12SerializeRootSignature failed with %s", reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
      return false;
    }

    auto errorCode = DX12_CHECK_RESULT(
      device->CreateRootSignature(0, rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), COM_ARGS(&target.signature)));
    return DX12_CHECK_OK(errorCode);
  }

  ComPtr<ID3D12StateObject> createPipeline(ID3D12Device5 *device, ID3D12RootSignature *signature)
  {
    eastl::vector<D3D12_STATE_SUBOBJECT> subObjects;
    subObjects.reserve(4 + importInfos.size() + hitGroups.size());

    D3D12_STATE_OBJECT_CONFIG config;
    config.Flags = D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS;
    D3D12_STATE_SUBOBJECT configSubObject{D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, &config};
    subObjects.push_back(configSubObject);

    D3D12_GLOBAL_ROOT_SIGNATURE signatureInfo{signature};
    D3D12_STATE_SUBOBJECT signatureSubObject{D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &signatureInfo};
    subObjects.push_back(signatureSubObject);

    eastl::transform(begin(importInfos), end(importInfos), eastl::back_inserter(subObjects), [](const auto &e) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, &e.desc};
    });

    D3D12_RAYTRACING_SHADER_CONFIG rtCFG{minPayload, minAttributes};
    D3D12_STATE_SUBOBJECT rtCFGSubObject{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &rtCFG};
    subObjects.push_back(rtCFGSubObject);
    D3D12_RAYTRACING_PIPELINE_CONFIG pCFG{minRecursion};
    D3D12_STATE_SUBOBJECT pCFGSubObject{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pCFG};
    subObjects.push_back(pCFGSubObject);

    eastl::transform(begin(hitGroups), end(hitGroups), eastl::back_inserter(subObjects), [](const auto &desc) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &desc};
    });

    ComPtr<ID3D12StateObject> o;
    D3D12_STATE_OBJECT_DESC desc{};
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    desc.NumSubobjects = subObjects.size();
    desc.pSubobjects = subObjects.data();
    device->CreateStateObject(&desc, COM_ARGS(&o));
    return o;
  }

  ComPtr<ID3D12StateObject> expandPipeline(ID3D12Device7 *device, ID3D12RootSignature *signature, ID3D12StateObject *base)
  {
    eastl::vector<D3D12_STATE_SUBOBJECT> subObjects;
    subObjects.reserve(4 + importInfos.size() + hitGroups.size());

    D3D12_STATE_OBJECT_CONFIG config;
    config.Flags = D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS;
    D3D12_STATE_SUBOBJECT configSubObject{D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, &config};
    subObjects.push_back(configSubObject);

    D3D12_GLOBAL_ROOT_SIGNATURE signatureInfo{signature};
    D3D12_STATE_SUBOBJECT signatureSubObject{D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &signatureInfo};
    subObjects.push_back(signatureSubObject);

    eastl::transform(begin(importInfos), end(importInfos), eastl::back_inserter(subObjects), [](const auto &e) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, &e.desc};
    });

    D3D12_RAYTRACING_SHADER_CONFIG rtCFG{minPayload, minAttributes};
    D3D12_STATE_SUBOBJECT rtCFGSubObject{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &rtCFG};
    subObjects.push_back(rtCFGSubObject);
    D3D12_RAYTRACING_PIPELINE_CONFIG pCFG{minRecursion};
    D3D12_STATE_SUBOBJECT pCFGSubObject{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pCFG};
    subObjects.push_back(pCFGSubObject);

    eastl::transform(begin(hitGroups), end(hitGroups), eastl::back_inserter(subObjects), [](const auto &desc) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &desc};
    });

    ComPtr<ID3D12StateObject> o;
    D3D12_STATE_OBJECT_DESC desc{};
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    desc.NumSubobjects = subObjects.size();
    desc.pSubobjects = subObjects.data();
    device->AddToStateObject(&desc, base, COM_ARGS(&o));
    return o;
  }

  template <typename T, typename U>
  ComPtr<ID3D12StateObject> rebuildPipeline(ID3D12Device5 *device, ID3D12RootSignature *signature, const T &base_imports,
    const U &base_hit_groups)
  {
    eastl::vector<D3D12_STATE_SUBOBJECT> subObjects;
    subObjects.reserve(4 + importInfos.size() + hitGroups.size() + base_imports.size() + base_hit_groups.size());

    D3D12_STATE_OBJECT_CONFIG config;
    config.Flags = D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS;
    D3D12_STATE_SUBOBJECT configSubObject{D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, &config};
    subObjects.push_back(configSubObject);

    D3D12_GLOBAL_ROOT_SIGNATURE signatureInfo{signature};
    D3D12_STATE_SUBOBJECT signatureSubObject{D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &signatureInfo};
    subObjects.push_back(signatureSubObject);

    eastl::transform(begin(importInfos), end(importInfos), eastl::back_inserter(subObjects), [](const auto &e) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, &e.desc};
    });
    eastl::transform(begin(base_imports), end(base_imports), eastl::back_inserter(subObjects), [](const auto &e) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, &e};
    });

    D3D12_RAYTRACING_SHADER_CONFIG rtCFG{minPayload, minAttributes};
    D3D12_STATE_SUBOBJECT rtCFGSubObject{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &rtCFG};
    subObjects.push_back(rtCFGSubObject);
    D3D12_RAYTRACING_PIPELINE_CONFIG pCFG{minRecursion};
    D3D12_STATE_SUBOBJECT pCFGSubObject{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pCFG};
    subObjects.push_back(pCFGSubObject);

    eastl::transform(begin(hitGroups), end(hitGroups), eastl::back_inserter(subObjects), [](const auto &desc) {
      return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &desc};
    }); /*
     eastl::transform(begin(base_hit_groups), end(base_hit_groups), eastl::back_inserter(subObjects),
       [](const D3D12_HIT_GROUP_DESC &desc) {
         return D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &desc};
       });*/

    ComPtr<ID3D12StateObject> o;
    D3D12_STATE_OBJECT_DESC desc{};
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    desc.NumSubobjects = subObjects.size();
    desc.pSubobjects = subObjects.data();
    device->CreateStateObject(&desc, COM_ARGS(&o));
    return o;
  }

  // stores all names next to each other in name_store and outputs the table of name string pointers into shader_names
  void exportShaderRefData(DynamicArray<const wchar_t *> &shader_names, DynamicArray<wchar_t> &name_store)
  {
    auto totalSize = eastl::accumulate(begin(nameTable), end(nameTable), 0, [](size_t v, const auto &e) { return v + e.size(); });

    name_store.resize(totalSize);
    shader_names.resize(nameTable.size());

    size_t offset = 0;
    eastl::transform(begin(nameTable), end(nameTable), shader_names.data(), [&name_store, &offset](const auto &e) {
      auto target = name_store.data() + offset;
      eastl::copy(e.data(), e.data() + e.size(), target);
      offset += e.size();
      return target;
    });
  }

  void exportShaderRefData(DynamicArray<const wchar_t *> &shader_names, DynamicArray<wchar_t> &name_store,
    const DynamicArray<const wchar_t *> &base_shader_names, const DynamicArray<wchar_t> &base_name_store)
  {
    auto totalSize = eastl::accumulate(begin(nameTable), end(nameTable), 0, [](size_t v, const auto &e) { return v + e.size(); });

    name_store.resize(totalSize + base_name_store.size());
    eastl::copy(base_name_store.data(), base_name_store.data() + base_name_store.size(), name_store.data());

    shader_names.resize(nameTable.size() + base_shader_names.size());
    eastl::transform(base_shader_names.data(), base_shader_names.data() + base_shader_names.size(), shader_names.data(),
      [&](const wchar_t *str) { return name_store.data() + (str - base_name_store.data()); });

    size_t offset = base_name_store.size();
    eastl::transform(begin(nameTable), end(nameTable), shader_names.data() + base_shader_names.size(),
      [&name_store, &offset](const auto &e) {
        auto target = name_store.data() + offset;
        eastl::copy(e.data(), e.data() + e.size(), target);
        offset += e.size();
        return target;
      });
  }

  // visitor should return false to exit early
  // returns true when every entry was visited and visitor did not return false for any of them
  // returns false when visitor did return false for one of the names
  template <typename T>
  bool visitShaderNames(T visitor)
  {
    for (uint32_t i = 0; i < nameTable.size(); ++i)
    {
      if (!visitor(i, nameTable[i].data()))
      {
        return false;
      }
    }
    return true;
  }

  bool hasEqualRootSignature(const drv3d_dx12::RaytracePipelineSignature &rps,
    const drv3d_dx12::RayTracePipelineResourceTypeTable &rtprtt) const
  {
    return equals(resourceUses, rps.def.registers, rtprtt);
  }
};
} // namespace

bool drv3d_dx12::RayTracePipeline::build(AnyDevicePtr device_ptr, bool has_native_expand,
  PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializer, RayTracePipeline *base, const ::raytrace::PipelineCreateInfo &ci)
{
  if (ci.name)
  {
    debugName = ci.name;
  }

  if (base && !base->isExpandable() && has_native_expand)
  {
    // warns if base was provided, but base has no expandable flag.
    // this warning is not generated when the OS/HW/Device driver do not support it.
    logwarn("DX12: Using pipeline <%s> as a base for <%s>, but <%s> had no expandable flag set, can only reuse root signature",
      base->debugName, debugName, base->debugName);
  }

  auto d5 = device_ptr.as<ID3D12Device5>();
  if (!d5)
  {
    return false;
  }
  PipelineBuilder pipelineBuilder{ci};

  if (!pipelineBuilder.isOk)
  {
    return false;
  }

  if (base && pipelineBuilder.hasEqualRootSignature(base->rootSignature, base->resourceTypeTable))
  {
    logdbg("DX12: RT pipeline <%s> reusing root signature of <%s>", debugName, base->debugName);
    // should the base and our computed root signature be matching, we can reuse it
    rootSignature = base->rootSignature;
    resourceTypeTable = base->resourceTypeTable;
  }
  else
  {
    if (!pipelineBuilder.createSignature(d5, serializer, rootSignature, resourceTypeTable))
    {
      pipelineBuilder.onError();
      return false;
    }
  }

  if (base && base->isExpandable())
  {
    auto d7 = device_ptr.as<ID3D12Device7>();
    if (d7)
    {
      logdbg("DX12: Attempting to expand <%s> from base <%s>", debugName, base->debugName);
      object = pipelineBuilder.expandPipeline(d7, rootSignature.signature.Get(), base->object.Get());
      if (!object)
      {
        logdbg("DX12: Failed, continuing with regular build");
      }
    }
    else
    {
      logdbg("DX12: Skipping expand attempt as device 7 interface is unavailable");
    }
  }
  if (!object)
  {
    object = pipelineBuilder.createPipeline(d5, rootSignature.signature.Get());
  }
  if (!object)
  {
    pipelineBuilder.onError();
    return false;
  }

  ComPtr<ID3D12StateObjectProperties> objectProperties;
  object.As(&objectProperties);
  if (!objectProperties)
  {
    logerr("DX12: Unable to obtain pipeline properties interface");
    pipelineBuilder.onError();
    return false;
  }

  shaderReferenceTableStorage.resize(ci.groupMemebers.size() * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  bool hadError = !pipelineBuilder.visitShaderNames(
    [target = shaderReferenceTableStorage.data(), props = objectProperties.Get()](uint32_t index, const wchar_t *name) {
      auto ptr = props->GetShaderIdentifier(name);
      if (!ptr)
      {
        return false;
      }
      memcpy(&target[index * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES], ptr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
      return true;
    });

  if (hadError)
  {
    logerr("DX12: Error while obtaining shader identifiers");
    pipelineBuilder.onError();
    return false;
  }

  if (ci.expandable)
  {
    expansionData.minRecursion = pipelineBuilder.minRecursion;
    expansionData.minPayload = pipelineBuilder.minPayload;
    expansionData.minAttributes = pipelineBuilder.minAttributes;
    expansionData.isSupported = true;
  }
  return true;
}

drv3d_dx12::RayTracePipeline *drv3d_dx12::RayTracePipeline::expand(ID3D12Device7 *device,
  const ::raytrace::PipelineExpandInfo &ei) const
{
  if (!isExpandable())
  {
    logwarn("DX12: Attempted to expand ray trace pipeline that does not support expansion");
    return nullptr;
  }
  PipelineBuilder pipelineBuilder{
    ei, expansionData.minRecursion, expansionData.minPayload, expansionData.minAttributes, rootSignature, resourceTypeTable};

  if (!pipelineBuilder.isOk)
  {
    return nullptr;
  }

  if (!pipelineBuilder.hasEqualRootSignature(rootSignature, resourceTypeTable))
  {
    logdbg("DX12: Root signature mismatch, unable to expand pipeline");
    // disables timing reporting
    pipelineBuilder.onError();
    return nullptr;
  }

  auto newObject = eastl::make_unique<drv3d_dx12::RayTracePipeline>();
  if (!newObject)
  {
    logerr("DX12: Unable to create new RayTracePipeline object");
    return nullptr;
  }

  newObject->debugName = ei.name;
  newObject->rootSignature = rootSignature;
  newObject->resourceTypeTable = resourceTypeTable;

  if (device)
  {
    logdbg("DX12: Attempting to expand pipeline");
    newObject->object = pipelineBuilder.expandPipeline(device, newObject->rootSignature.signature.Get(), object.Get());
    if (!newObject->object)
    {
      logdbg("DX12: Expand pipeline failed, unable to expand pipeline");
      return nullptr;
    }
  }

  ComPtr<ID3D12StateObjectProperties> objectProperties;
  newObject->object.As(&objectProperties);
  if (!objectProperties)
  {
    logerr("DX12: Unable to obtain pipeline properties interface");
    return nullptr;
  }

  // In DX12 shaders in pipeline state objects are globally shared between all state objects they are used by
  // eg, if we expand a pipeline, the shaders that where inherited from the base pipeline have the same
  // shader header as the shaders at the base pipeline. With this, we can simply copy the existing table
  // of the base pipeline and just append the new shaders added by the expansion.
  newObject->shaderReferenceTableStorage.resize(
    shaderReferenceTableStorage.size() + ei.groupMemebers.size() * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  memcpy(newObject->shaderReferenceTableStorage.data(), shaderReferenceTableStorage.data(), shaderReferenceTableStorage.size());
  bool hadError =
    !pipelineBuilder.visitShaderNames([target = newObject->shaderReferenceTableStorage.data() + shaderReferenceTableStorage.size(),
                                        props = objectProperties.Get()](uint32_t index, const wchar_t *name) {
      auto ptr = props->GetShaderIdentifier(name);
      if (!ptr)
      {
        return false;
      }
      memcpy(&target[index * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES], ptr, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
      return true;
    });

  if (hadError)
  {
    logerr("DX12: Error while obtaining shader identifiers");
    pipelineBuilder.onError();
    return nullptr;
  }

  if (ei.expandable)
  {
    newObject->expansionData.minRecursion = pipelineBuilder.minRecursion;
    newObject->expansionData.minPayload = pipelineBuilder.minPayload;
    newObject->expansionData.minAttributes = pipelineBuilder.minAttributes;
    newObject->expansionData.isSupported = true;
  }
  return newObject.release();
}
#endif