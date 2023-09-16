#include <render/daBfg/virtualPassRequest.h>
#include <api/internalRegistry.h>
#include <render/daBfg/detail/virtualResourceRequestBase.h>


namespace dabfg
{

VirtualPassRequest::VirtualPassRequest(NodeNameId node, InternalRegistry *reg) : nodeId{node}, registry{reg}
{
  if (EASTL_UNLIKELY(registry->nodes[nodeId].renderingRequirements.has_value()))
    logerr("Encountered two renderpass requests within '%s' framegraph node!"
           " Ignoring one of them!",
      registry->knownNodeNames.getName(nodeId));
  registry->nodes[nodeId].renderingRequirements.emplace();
}

template <detail::ResourceRequestPolicy disallowPolicies, detail::ResourceRequestPolicy requirePolicies>
detail::ResUid VirtualPassRequest::processAttachment(detail::VirtualAttachmentRequest<disallowPolicies, requirePolicies> attachment,
  Access accessOverride)
{
  detail::ResUid result;

  auto &nodeData = registry->nodes[nodeId];

  if (eastl::holds_alternative<detail::ResUid>(attachment.image))
  {
    result = eastl::get<detail::ResUid>(attachment.image);
  }
  else
  {
    auto id = registry->knownResourceNames.addNameId(eastl::get<const char *>(attachment.image));
    if (accessOverride == Access::READ_ONLY)
      nodeData.readResources.insert(id);
    else
      nodeData.modifiedResources.insert(id);
    result.resId = id;
    result.history = false;
  }

  // For RO depth, we have to override the access.
  // For string name based requests, we need to set the access
  // in any case. For other cases, we simply set the access to the
  // same value it already had due to invariants.
  auto &req = nodeData.resourceRequests[result.resId];
  req.usage.access = accessOverride;
  req.usage.stage = Stage::POST_RASTER;
  req.usage.type = Usage::COLOR_ATTACHMENT;

  return result;
}

VirtualPassRequest VirtualPassRequest::color(std::initializer_list<ColorRwVirtualAttachmentRequest> attachments) &&
{
  auto &colors = registry->nodes[nodeId].renderingRequirements->colorAttachments;
  if (EASTL_UNLIKELY(!colors.empty()))
  {
    logerr("Encountered duplicate color attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNodeNames.getName(nodeId));
    colors.clear();
  }
  colors.reserve(attachments.size());
  for (const auto &attachment : attachments)
  {
    auto [resId, hist] = processAttachment(attachment, Access::READ_WRITE);
    G_ASSERT(!hist); // Sanity check, should be impossible by construction
    colors.emplace_back(VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer});
  }
  return *this;
}

VirtualPassRequest VirtualPassRequest::depthRw(RwVirtualAttachmentRequest attachment) &&
{
  auto &depth = registry->nodes[nodeId].renderingRequirements->depthAttachment;
  if (EASTL_UNLIKELY(depth.nameId != ResNameId::Invalid))
  {
    logerr("Encountered duplicate depth attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNodeNames.getName(nodeId));
  }
  auto [resId, hist] = processAttachment(attachment, Access::READ_WRITE);
  G_ASSERT(!hist); // Sanity check, should be impossible by construction
  depth = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  registry->nodes[nodeId].renderingRequirements->depthReadOnly = false;
  return *this;
}

VirtualPassRequest VirtualPassRequest::depthRo(DepthRoVirtualAttachmentRequest attachment) &&
{
  auto &depth = registry->nodes[nodeId].renderingRequirements->depthAttachment;
  if (EASTL_UNLIKELY(depth.nameId != ResNameId::Invalid))
  {
    logerr("Encountered duplicate depth attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNodeNames.getName(nodeId));
  }

  auto [resId, hist] = processAttachment(attachment, Access::READ_ONLY);
  G_ASSERTF(!hist, "Internal FG infrastructure does not support history"
                   " RO depth attachments yet!");

  depth = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  registry->nodes[nodeId].renderingRequirements->depthReadOnly = true;
  return *this;
}

VirtualPassRequest VirtualPassRequest::depthRoAndBindToShaderVars(DepthRoAndSvBindVirtualAttachmentRequest attachment,
  std::initializer_list<const char *> shader_var_names) &&
{
  // TODO: consider adding a virtualPassRequestBase to simplify the
  // code here and get rid of some copypasta.

  auto &depth = registry->nodes[nodeId].renderingRequirements->depthAttachment;
  if (EASTL_UNLIKELY(depth.nameId != ResNameId::Invalid))
  {
    logerr("Encountered duplicate depth attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNodeNames.getName(nodeId));
  }

  auto [resId, hist] = processAttachment(attachment, Access::READ_ONLY);
  G_ASSERTF(!hist, "Internal FG infrastructure does not support history"
                   " RO depth attachments yet!");

  depth = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  registry->nodes[nodeId].renderingRequirements->depthReadOnly = true;

  detail::VirtualResourceRequestBase fakeReq{{resId, hist}, nodeId, registry};
  for (auto name : shader_var_names)
    fakeReq.bindToShaderVar(name);

  auto &req = registry->nodes[nodeId].resourceRequests[resId];
  req.usage.stage |= Stage::POST_RASTER;
  req.usage.type = Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE;

  return *this;
}

VirtualPassRequest VirtualPassRequest::clear(RwVirtualAttachmentRequest attachment, ResourceClearValue color) &&
{
  G_UNUSED(attachment);
  G_UNUSED(color);
  G_ASSERTF(false, "NOT IMPLEMENTED!");
  return *this;
}

VirtualPassRequest VirtualPassRequest::area(IPoint2 from, IPoint2 to) &&
{
  G_UNUSED(from);
  G_UNUSED(to);
  G_ASSERTF(false, "NOT IMPLEMENTED!");
  return *this;
}

} // namespace dabfg
