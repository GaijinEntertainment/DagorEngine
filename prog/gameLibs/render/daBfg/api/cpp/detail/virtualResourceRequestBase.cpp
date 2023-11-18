#include <render/daBfg/detail/virtualResourceRequestBase.h>
#include <frontend/internalRegistry.h>
#include <render/daBfg/resourceCreation.h>


namespace dabfg::detail
{

static constexpr int FAKE_ID_FOR_VIEW_MATRIX_BINDINGS = -1;
static constexpr int FAKE_ID_FOR_PROJ_MATRIX_BINDINGS = -2;

void VirtualResourceRequestBase::texture(const Texture2dCreateInfo &info)
{
  auto &res = registry->resources.get(resUid.resId);

  TextureResourceDescription desc{};
  desc.clearValue = make_clear_value(0u, 0u, 0u, 0u);
  if (auto p = eastl::get_if<IPoint2>(&info.resolution))
  {
    desc.width = p->x;
    desc.height = p->y;
  }
  desc.cFlags = info.creationFlags;
  desc.mipLevels = info.mipLevels;
  res.type = ResourceType::Texture;
  res.creationInfo = ResourceDescription{desc};
  if (auto r = eastl::get_if<AutoResolutionRequest>(&info.resolution); r)
    res.resolution = AutoResolutionData{r->autoResTypeId, r->multiplier};
  else
    res.resolution = eastl::nullopt;
}

void VirtualResourceRequestBase::buffer(const BufferCreateInfo &info)
{
  auto &res = registry->resources.get(resUid.resId);

  BufferResourceDescription desc;
  desc.elementCount = info.elementCount;
  desc.elementSizeInBytes = info.elementSize;
  desc.viewFormat = info.format;
  desc.cFlags = info.flags;
  desc.activation = ResourceActivationAction::DISCARD_AS_UAV;

  res.creationInfo = ResourceDescription{desc};
  res.type = ResourceType::Buffer;
}

void VirtualResourceRequestBase::blob(const BlobDescription &desc)
{
  auto &res = registry->resources.get(resUid.resId);

  res.creationInfo = desc;
  res.type = ResourceType::Blob;

  markWithTag(desc.typeTag);
}

void VirtualResourceRequestBase::markWithTag(ResourceSubtypeTag tag) { thisRequest().subtypeTag = tag; }

void VirtualResourceRequestBase::optional() { thisRequest().optional = true; }

static void bind_to_shader_var_impl(const char *shader_var_name, ResUid res_uid, NodeNameId node_id, InternalRegistry *registry,
  const Binding &binding_info)
{
  if (shader_var_name == nullptr)
    shader_var_name = registry->knownNames.getShortName(res_uid.resId);

  // NOTE: The only point in using get_shader_variable_id is that it
  // logerrs if the variable was optional but not found. We cannot do
  // that check right here right now, so it will be deferred to a
  // latter time.
  const int svId = VariableMap::getVariableId(shader_var_name);

  auto &bindings = registry->nodes[node_id].bindings;

  if (EASTL_UNLIKELY(bindings.find(svId) != bindings.end()))
  {
    logerr("Encountered duplicate shader var '%s' binding requests within"
           " '%s' frame graph node! Ignoring one of them!",
      shader_var_name, registry->knownNames.getName(node_id));
    return;
  }
  bindings[svId] = binding_info;
}

static void bind_as_view_impl(NodeNameId node_id, InternalRegistry *registry, const Binding &binding_info)
{
  auto &bindings = registry->nodes[node_id].bindings;
  if (EASTL_UNLIKELY(bindings.find(FAKE_ID_FOR_VIEW_MATRIX_BINDINGS) != bindings.end()))
  {
    logerr("Encountered duplicate view matrix binding requests within"
           " '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(node_id));
    return;
  }
  bindings[FAKE_ID_FOR_VIEW_MATRIX_BINDINGS] = binding_info;
}

static void bind_as_proj_impl(NodeNameId node_id, InternalRegistry *registry, const Binding &binding_info)
{
  auto &bindings = registry->nodes[node_id].bindings;
  if (EASTL_UNLIKELY(bindings.find(FAKE_ID_FOR_PROJ_MATRIX_BINDINGS) != bindings.end()))
  {
    logerr("Encountered duplicate proj matrix binding requests within"
           " '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(node_id));
    return;
  }
  bindings[FAKE_ID_FOR_PROJ_MATRIX_BINDINGS] = binding_info;
}

void VirtualResourceRequestBase::bindToShaderVar(const char *shader_var_name, ResourceSubtypeTag projected_tag)
{
  const Binding bindingInfo = {BindingType::ShaderVar, resUid.resId, static_cast<bool>(resUid.history), projected_tag};
  bind_to_shader_var_impl(shader_var_name, resUid, nodeId, registry, bindingInfo);
  thisRequest().usage.type = Usage::SHADER_RESOURCE;
}

void VirtualResourceRequestBase::bindToShaderVar(const char *shader_var_name, ResourceSubtypeTag projected_tag,
  TypeErasedProjector projector)
{
  const Binding bindingInfo{BindingType::ShaderVar, resUid.resId, static_cast<bool>(resUid.history), projected_tag, projector};
  bind_to_shader_var_impl(shader_var_name, resUid, nodeId, registry, bindingInfo);
}

void VirtualResourceRequestBase::bindAsView(ResourceSubtypeTag projected_tag)
{
  const Binding bindingInfo{BindingType::ViewMatrix, resUid.resId, static_cast<bool>(resUid.history), projected_tag};
  bind_as_view_impl(nodeId, registry, bindingInfo);
}

void VirtualResourceRequestBase::bindAsView(ResourceSubtypeTag projected_tag, TypeErasedProjector projector)
{
  const Binding bindingInfo{BindingType::ViewMatrix, resUid.resId, static_cast<bool>(resUid.history), projected_tag, projector};
  bind_as_view_impl(nodeId, registry, bindingInfo);
}

void VirtualResourceRequestBase::bindAsProj(ResourceSubtypeTag projected_tag)
{
  const Binding bindingInfo{BindingType::ProjMatrix, resUid.resId, static_cast<bool>(resUid.history), projected_tag};
  bind_as_proj_impl(nodeId, registry, bindingInfo);
}

void VirtualResourceRequestBase::bindAsProj(ResourceSubtypeTag projected_tag, TypeErasedProjector projector)
{
  const Binding bindingInfo{BindingType::ProjMatrix, resUid.resId, static_cast<bool>(resUid.history), projected_tag, projector};
  bind_as_proj_impl(nodeId, registry, bindingInfo);
}

void VirtualResourceRequestBase::atStage(Stage stage) { thisRequest().usage.stage = stage; }

void VirtualResourceRequestBase::useAs(Usage type) { thisRequest().usage.type = type; }

const ResourceProvider *VirtualResourceRequestBase::provider() { return &registry->resourceProviderReference; }

ResourceRequest &VirtualResourceRequestBase::thisRequest()
{
  auto &nodeData = registry->nodes[nodeId];
  return resUid.history ? nodeData.historyResourceReadRequests[resUid.resId] : nodeData.resourceRequests[resUid.resId];
}

} // namespace dabfg::detail
