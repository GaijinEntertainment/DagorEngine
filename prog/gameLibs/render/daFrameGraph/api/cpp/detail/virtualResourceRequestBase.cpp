// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/detail/virtualResourceRequestBase.h>
#include <frontend/internalRegistry.h>
#include <render/daFrameGraph/resourceCreation.h>
#include <runtime/runtime.h>
#include <EASTL/vector_set.h>

namespace dafg::detail
{

static eastl::vector_set<int> shadervars_to_reset;

static constexpr int FAKE_ID_FOR_VIEW_MATRIX_BINDINGS = -1;
static constexpr int FAKE_ID_FOR_PROJ_MATRIX_BINDINGS = -2;

void VirtualResourceRequestBase::texture(const Texture2dCreateInfo &info)
{
  if (auto r = eastl::get_if<IPoint2>(&info.resolution); r && DAGOR_UNLIKELY(r->x == 0 || r->y == 0))
    logerr("daFG: Encountered zero-sized texture creation request (%dx%d) within '%s' frame graph node!", r->x, r->y,
      registry->knownNames.getName(nodeId));

  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->type = ResourceType::Texture;
  createdRes->creationInfo = info;
  // TODO: get rid of this, simply use createdRes->creationInfo everywhere
  if (auto r = eastl::get_if<AutoResolutionRequest<2>>(&info.resolution); r)
    createdRes->resolution = AutoResolutionData{r->autoResTypeId, r->multiplier};
  else
    createdRes->resolution = eastl::nullopt;
}

void VirtualResourceRequestBase::texture(const Texture3dCreateInfo &info)
{
  if (auto r = eastl::get_if<IPoint3>(&info.resolution); r && DAGOR_UNLIKELY(r->x == 0 || r->y == 0 || r->z == 0))
    logerr("daFG: Encountered zero-sized texture creation request (%dx%dx%d) within '%s' frame graph node!", r->x, r->y, r->z,
      registry->knownNames.getName(nodeId));

  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->type = ResourceType::Texture;
  createdRes->creationInfo = info;
  // TODO: get rid of this, simply use createdRes->creationInfo everywhere
  if (auto r = eastl::get_if<AutoResolutionRequest<3>>(&info.resolution); r)
    createdRes->resolution = AutoResolutionData{r->autoResTypeId, r->multiplier};
  else
    createdRes->resolution = eastl::nullopt;
}

void VirtualResourceRequestBase::buffer(const BufferCreateInfo &info)
{
  if (DAGOR_UNLIKELY(info.elementCount == 0u || info.elementSize == 0u))
    logerr("daFG: Encountered zero-sized buffer creation request (elementSize=%u, elementCount=%u) within '%s' frame graph node!",
      info.elementSize, info.elementCount, registry->knownNames.getName(nodeId));
  if (DAGOR_UNLIKELY((info.flags & (SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_CPU_ACCESS_WRITE | SBCF_CPU_ACCESS_READ)) != 0u))
    logerr("daFG: Encountered CPU-accessed buffer creation request within '%s' frame graph node!",
      registry->knownNames.getName(nodeId));

  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->creationInfo = info;
  createdRes->type = ResourceType::Buffer;
}

void VirtualResourceRequestBase::blob(BlobDescription &&desc, detail::RTTI &&rtti)
{
  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  const auto typeTag = desc.typeTag;
  createdRes->creationInfo = eastl::move(desc);
  createdRes->type = ResourceType::Blob;

  dafg::Runtime::get().getTypeDb().registerNativeType(typeTag, eastl::move(rtti));

  markWithTag(typeTag);
}

void VirtualResourceRequestBase::backBuffer()
{
  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->creationInfo = DriverDeferredTexture{};
  createdRes->type = ResourceType::Texture;
}

void VirtualResourceRequestBase::externalBuffer(dafg::ExternalResourceProvider &&external_resource_provider)
{
  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->creationInfo = eastl::move(external_resource_provider);
  createdRes->type = ResourceType::Buffer;
}

void VirtualResourceRequestBase::externalTexture(dafg::ExternalResourceProvider &&external_resource_provider)
{
  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->creationInfo = eastl::move(external_resource_provider);
  createdRes->type = ResourceType::Texture;
}

void VirtualResourceRequestBase::markWithTag(ResourceSubtypeTag tag) { thisRequest().subtypeTag = tag; }

void VirtualResourceRequestBase::optional() { thisRequest().optional = true; }

void VirtualResourceRequestBase::bindToShaderVar(const char *shader_var_name, ResourceSubtypeTag projected_tag,
  TypeErasedProjector projector)
{
  if (shader_var_name == nullptr)
    shader_var_name = registry->knownNames.getShortName(resUid.resId);

  // NOTE: The only point in using get_shader_variable_id is that it
  // logerrs if the variable was optional but not found. We cannot do
  // that check right here right now, so it will be deferred to a
  // latter time.
  const int svId = VariableMap::getVariableId(shader_var_name);

  bool reset = shadervars_to_reset.count(svId) > 0;

  auto &bindings = registry->nodes[nodeId].bindings;

  if (DAGOR_UNLIKELY(bindings.find(svId) != bindings.end()))
  {
    logerr("daFG: Encountered duplicate shader var '%s' binding requests within"
           " '%s' frame graph node! Ignoring one of them!",
      shader_var_name, registry->knownNames.getName(nodeId));
    return;
  }
  bindings[svId] = Binding{
    BindingType::ShaderVar, resUid.resId, static_cast<bool>(resUid.history), reset, thisRequest().optional, projected_tag, projector};

  // NOTE: we don't need to set this for blobs, but it doesn't matter anyways
  thisRequest().usage.type = Usage::SHADER_RESOURCE;
}

void VirtualResourceRequestBase::bindAsView(ResourceSubtypeTag projected_tag, TypeErasedProjector projector)
{
  auto &bindings = registry->nodes[nodeId].bindings;
  if (DAGOR_UNLIKELY(bindings.find(FAKE_ID_FOR_VIEW_MATRIX_BINDINGS) != bindings.end()))
  {
    logerr("daFG: Encountered duplicate view matrix binding requests within"
           " '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
    return;
  }
  bindings[FAKE_ID_FOR_VIEW_MATRIX_BINDINGS] = Binding{
    BindingType::ViewMatrix, resUid.resId, static_cast<bool>(resUid.history), false, thisRequest().optional, projected_tag, projector};
}

void VirtualResourceRequestBase::bindAsProj(ResourceSubtypeTag projected_tag, TypeErasedProjector projector)
{
  auto &bindings = registry->nodes[nodeId].bindings;
  if (DAGOR_UNLIKELY(bindings.find(FAKE_ID_FOR_PROJ_MATRIX_BINDINGS) != bindings.end()))
  {
    logerr("daFG: Encountered duplicate proj matrix binding requests within"
           " '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
    return;
  }
  bindings[FAKE_ID_FOR_PROJ_MATRIX_BINDINGS] = Binding{
    BindingType::ProjMatrix, resUid.resId, static_cast<bool>(resUid.history), false, thisRequest().optional, projected_tag, projector};
}

void VirtualResourceRequestBase::bindAsVertexBuffer(uint32_t stream, uint32_t stride)
{
  thisRequest().usage.type |= Usage::VERTEX_BUFFER;
  auto &node = registry->nodes[nodeId];
  if (DAGOR_UNLIKELY(node.vertexSources[stream].has_value()))
    logerr("daFG: Encountered multiple vertex source requests within '%s' frame graph node!", registry->knownNames.getName(nodeId));

  node.vertexSources[stream].emplace(VertexSource{resUid.resId, stride});
}

void VirtualResourceRequestBase::bindAsIndexBuffer()
{
  thisRequest().usage.type |= Usage::INDEX_BUFFER;
  auto &node = registry->nodes[nodeId];
  if (DAGOR_UNLIKELY(node.indexSource.has_value()))
    logerr("daFG: Encountered multiple index source requests within '%s' frame graph node!", registry->knownNames.getName(nodeId));

  node.indexSource.emplace(IndexSource{resUid.resId});
}

void VirtualResourceRequestBase::atStage(Stage stage) { thisRequest().usage.stage = stage; }

void VirtualResourceRequestBase::useAs(Usage type) { thisRequest().usage.type = type; }

const ResourceProvider *VirtualResourceRequestBase::provider() { return &registry->resourceProviderReference; }

ResourceRequest &VirtualResourceRequestBase::thisRequest()
{
  auto &nodeData = registry->nodes[nodeId];
  return resUid.history ? nodeData.historyResourceReadRequests[resUid.resId] : nodeData.resourceRequests[resUid.resId];
}

void VirtualResourceRequestBase::withHistory(History history)
{
  auto resId = resUid.resId;
  for (const auto &[toResId, fromResId] : registry->nodes[nodeId].renamedResources)
  {
    if (fromResId == resUid.resId)
    {
      resId = toResId;
      break;
    }
  }
  auto &hist = registry->resources.get(resId).history;

  hist = history;
}

void VirtualResourceRequestBase::clear(const ResourceClearValue &clear_value, ResourceClearFlags flags)
{
  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  createdRes->clearValue = clear_value;
  createdRes->clearFlags = flags;
}

void VirtualResourceRequestBase::clear(ResourceSubtypeTag projectee, const char *blob_name, detail::TypeErasedProjector projector,
  ResourceClearFlags flags)
{
  auto &nodeData = registry->nodes[nodeId];
  auto &createdRes = registry->resources.get(resUid.resId).createdResData;

  const auto ourNs = registry->knownNames.getParent(nodeId);
  const auto blobId = registry->knownNames.addNameId<ResNameId>(ourNs, blob_name);

  // Implicitly read the blob
  nodeData.readResources.insert(blobId);
  nodeData.resourceRequests.emplace(blobId, ResourceRequest{{}, false, false, projectee});

  detail::VirtualResourceRequestBase fakeReq{{blobId, false}, nodeId, registry};
  fakeReq.markWithTag(projectee);

  createdRes->clearValue = DynamicParameter{blobId, tag_for<ResourceClearValue>(), projector};
  createdRes->clearFlags = flags;
}
} // namespace dafg::detail

namespace dafg
{
void reset_shadervar_after_use(const char *name)
{
  G_ASSERT(name);
  int id = VariableMap::getVariableId(name);
  if (VariableMap::isVariablePresent(id))
    detail::shadervars_to_reset.insert(id);
  else
    logerr("[daFG] can't register %s to be autoreset, the variable is not present", name);
}
} // namespace dafg
