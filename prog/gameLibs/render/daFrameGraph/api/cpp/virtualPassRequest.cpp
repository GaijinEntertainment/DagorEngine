// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/virtualPassRequest.h>
#include <frontend/internalRegistry.h>
#include <render/daFrameGraph/detail/virtualResourceRequestBase.h>
#include <drv/3d/dag_info.h>


namespace dafg
{

VirtualPassRequest::VirtualPassRequest(NodeNameId node, InternalRegistry *reg) : nodeId{node}, registry{reg}
{
  if (DAGOR_UNLIKELY(registry->nodes[nodeId].renderingRequirements.has_value()))
    logerr("daFG: Encountered two renderpass requests within '%s' framegraph node!"
           " Ignoring one of them!",
      registry->knownNames.getName(nodeId));
  registry->nodes[nodeId].renderingRequirements.emplace();
}

template <detail::ResourceRequestPolicy disallowPolicies, detail::ResourceRequestPolicy requirePolicies>
detail::ResUid VirtualPassRequest::getResUidForAttachment(
  detail::VirtualAttachmentRequest<disallowPolicies, requirePolicies> attachment)
{
  if (auto uid = eastl::get_if<detail::ResUid>(&attachment.image))
    return *uid;
  else
  {
    const auto nodeNsId = registry->knownNames.getParent(nodeId);
    const auto id = registry->knownNames.addNameId<ResNameId>(nodeNsId, eastl::get<const char *>(attachment.image));
    return {id, false};
  }
}

VirtualPassRequest VirtualPassRequest::color(std::initializer_list<ColorRwVirtualAttachmentRequest> attachments) &&
{
  auto &nodeData = registry->nodes[nodeId];
  auto &colors = nodeData.renderingRequirements->colorAttachments;
  if (DAGOR_UNLIKELY(!colors.empty()))
  {
    logerr("daFG: Encountered duplicate color attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
    colors.clear();
  }
  colors.reserve(attachments.size());
  for (const auto &attachment : attachments)
  {
    auto [resId, hist] = getResUidForAttachment(attachment);
    G_ASSERT(!hist); // Sanity check, should be impossible by construction
    nodeData.modifiedResources.insert(resId);
    nodeData.resourceRequests[resId].usage = ResourceUsage{Usage::COLOR_ATTACHMENT, Access::READ_WRITE, Stage::POST_RASTER};
    colors.emplace_back(VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer});
  }
  return *this;
}

VirtualPassRequest VirtualPassRequest::depthRw(RwVirtualAttachmentRequest attachment) &&
{
  auto &nodeData = registry->nodes[nodeId];
  auto &depth = nodeData.renderingRequirements->depthAttachment;
  if (DAGOR_UNLIKELY(depth.nameId != ResNameId::Invalid))
  {
    logerr("daFG: Encountered duplicate depth attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
  }
  auto [resId, hist] = getResUidForAttachment(attachment);
  G_ASSERT(!hist); // Sanity check, should be impossible by construction
  nodeData.modifiedResources.insert(resId);
  nodeData.resourceRequests[resId].usage = ResourceUsage{Usage::DEPTH_ATTACHMENT, Access::READ_WRITE, Stage::POST_RASTER};
  depth = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  nodeData.renderingRequirements->depthReadOnly = false;
  return *this;
}

VirtualPassRequest VirtualPassRequest::depthRo(DepthRoVirtualAttachmentRequest attachment) &&
{
  auto &nodeData = registry->nodes[nodeId];
  auto &depth = nodeData.renderingRequirements->depthAttachment;
  if (DAGOR_UNLIKELY(depth.nameId != ResNameId::Invalid))
  {
    logerr("daFG: Encountered duplicate depth attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
  }

  auto [resId, hist] = getResUidForAttachment(attachment);
  G_ASSERTF(!hist, "Internal FG infrastructure does not support history"
                   " RO depth attachments yet!");
  // Dirty hack to allow for depthRO to masquarade as a modification,
  // TODO: purge with fire and remove
  if (eastl::holds_alternative<const char *>(attachment.image))
    nodeData.readResources.insert(resId);
  nodeData.resourceRequests[resId].usage =
    ResourceUsage{Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE, Access::READ_ONLY, Stage::POST_RASTER};

  depth = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  nodeData.renderingRequirements->depthReadOnly = true;
  return *this;
}

VirtualPassRequest VirtualPassRequest::depthRoAndBindToShaderVars(DepthRoAndSvBindVirtualAttachmentRequest attachment,
  std::initializer_list<const char *> shader_var_names) &&
{
  auto &nodeData = registry->nodes[nodeId];

  auto &depth = nodeData.renderingRequirements->depthAttachment;
  if (DAGOR_UNLIKELY(depth.nameId != ResNameId::Invalid))
  {
    logerr("daFG: Encountered duplicate depth attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
  }

  auto [resId, hist] = getResUidForAttachment(attachment);
  G_ASSERTF(!hist, "Internal FG infrastructure does not support history"
                   " RO depth attachments yet!");
  // Dirty hack to allow for depthRO to masquarade as a modification,
  // TODO: purge with fire and remove
  if (eastl::holds_alternative<const char *>(attachment.image))
    nodeData.readResources.insert(resId);

  depth = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  nodeData.renderingRequirements->depthReadOnly = true;

  detail::VirtualResourceRequestBase fakeReq{{resId, hist}, nodeId, registry};
  for (auto name : shader_var_names)
    fakeReq.bindToShaderVar(name, ResourceSubtypeTag::Unknown, detail::identity_projector);

  // This function can be called with a string name, in which case we
  // default the stage to post-raster, but it can also be called with an
  // explicit request object with a stage specified, in which case we
  // want to preserve that stage (for the very rare use case of RO
  // depth being sampled in the vertex shader)
  const auto originalStage = nodeData.resourceRequests[resId].usage.stage;

  nodeData.resourceRequests[resId].usage =
    ResourceUsage{Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE, Access::READ_ONLY, originalStage | Stage::POST_RASTER};

  return *this;
}

VirtualPassRequest VirtualPassRequest::resolve(RwVirtualAttachmentRequest from, RwVirtualAttachmentRequest to) &&
{
  auto [fromId, fromHist] = getResUidForAttachment(from);
  G_ASSERT(!fromHist);
  auto [toId, toHist] = getResUidForAttachment(to);
  G_ASSERT(!toHist);

  auto &nodeData = registry->nodes[nodeId];

  if (nodeData.renderingRequirements->resolves.contains(fromId))
    logerr("daFG: Encountered duplicate resolve request for attachment '%s' "
           "in node '%s'! Ignoring one of them!",
      registry->knownNames.getName(fromId), registry->knownNames.getName(nodeId));

  nodeData.modifiedResources.insert(toId);
  nodeData.resourceRequests[toId].usage = ResourceUsage{Usage::RESOLVE_ATTACHMENT, Access::READ_WRITE, Stage::TRANSFER};

  nodeData.renderingRequirements->resolves.emplace(fromId, toId);

  return *this;
}

VirtualPassRequest VirtualPassRequest::vrsRate(RoVirtualAttachmentRequest attachment) &&
{
  auto &StateReq = registry->nodes[nodeId].stateRequirements;
  if (DAGOR_UNLIKELY(StateReq.has_value() && StateReq->vrsState.pixelCombiner != VariableRateShadingCombiner::VRS_PASSTHROUGH))
  {
    logerr("daFG: Node %s requested a VRS rate texture to be used after requesting maxVrs!"
           " Using max VRS and using a rate texture are incompatible, ignoring the rate texture.",
      registry->knownNames.getName(nodeId));

    return *this;
  }

  auto &nodeData = registry->nodes[nodeId];
  auto &rate = nodeData.renderingRequirements->vrsRateAttachment;
  if (DAGOR_UNLIKELY(rate.nameId != ResNameId::Invalid))
  {
    logerr("daFG: Encountered duplicate VRS rate attachment calls on the same pass request"
           " in '%s' frame graph node! Ignoring one of them!",
      registry->knownNames.getName(nodeId));
  }
  auto [resId, hist] = getResUidForAttachment(attachment);
  G_ASSERT(!hist); // Sanity check, should be impossible by construction
  nodeData.readResources.insert(resId);
  nodeData.resourceRequests[resId].usage = ResourceUsage{Usage::VRS_RATE_TEXTURE, Access::READ_ONLY, Stage::POST_RASTER};
  if (!d3d::get_driver_desc().caps.hasVariableRateShadingTexture)
    nodeData.resourceRequests[resId].optional = true;
  rate = VirtualSubresourceRef{resId, attachment.mipLevel, attachment.layer};
  if (attachment.mipLevel != 0 || attachment.layer != 0)
    logerr("daFG: Warning: attempted to specify a mip level or a layer for a VRS rate texture in node '%s'."
           " This is currently supported by NONE of our drivers and will be ignored.",
      registry->knownNames.getName(nodeId));

  return *this;
}

VirtualPassRequest VirtualPassRequest::area(IPoint2 from, IPoint2 to) &&
{
  G_UNUSED(from);
  G_UNUSED(to);
  G_ASSERTF(false, "NOT IMPLEMENTED!");
  return *this;
}

} // namespace dafg
