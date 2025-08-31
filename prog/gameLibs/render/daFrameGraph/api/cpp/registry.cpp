// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/registry.h>
#include <render/daFrameGraph/detail/shaderNameId.h>
#include <frontend/internalRegistry.h>


namespace dafg
{

Registry::Registry(NodeNameId node, InternalRegistry *reg) : NameSpaceRequest(reg->knownNames.getParent(node), node, reg) {}

Registry Registry::allowAsyncPipelines()
{
  registry->nodes[nodeId].allowAsyncPipelines = true;
  return *this;
}

Registry Registry::orderMeBefore(const char *name)
{
  const auto beforeId = registry->knownNames.addNameId<NodeNameId>(nameSpaceId, name);

  if (DAGOR_LIKELY(beforeId != NodeNameId::Invalid))
    registry->nodes[nodeId].followingNodeIds.insert(beforeId);
  else
    logerr("daFG: FG: node %s tries to order it before a non-existent node %s, skipping this ordering.",
      registry->knownNames.getName(nodeId), name);

  return *this;
}

Registry Registry::orderMeBefore(std::initializer_list<const char *> names)
{
  for (auto name : names)
    orderMeBefore(name);
  return *this;
}

Registry Registry::orderMeAfter(const char *name)
{
  const auto afterId = registry->knownNames.addNameId<NodeNameId>(nameSpaceId, name);

  if (DAGOR_LIKELY(afterId != NodeNameId::Invalid))
    registry->nodes[nodeId].precedingNodeIds.insert(afterId);
  else
    logerr("daFG: FG: node %s tries to order it after a non-existent node %s, skipping this ordering.",
      registry->knownNames.getName(nodeId), name);

  return *this;
}

Registry Registry::orderMeAfter(std::initializer_list<const char *> names)
{
  for (auto name : names)
    orderMeAfter(name);
  return *this;
}

Registry Registry::setPriority(priority_t prio)
{
  registry->nodes[nodeId].priority = prio;
  return *this;
}

Registry Registry::multiplex(multiplexing::Mode mode)
{
  registry->nodes[nodeId].multiplexingMode = mode;
  return *this;
}

Registry Registry::executionHas(SideEffects sideEffect)
{
  registry->nodes[nodeId].sideEffect = sideEffect;
  return *this;
}

StateRequest Registry::requestState() { return {registry, nodeId}; }

VirtualPassRequest Registry::requestRenderPass() { return {nodeId, registry}; }

DrawRequest<detail::DrawRequestPolicy::Default, false> Registry::draw(const char *shader_name, dafg::DrawPrimitive primitive)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return {registry, nodeId, shaderId, primitive};
}

DrawRequest<detail::DrawRequestPolicy::Default, true> Registry::drawIndexed(const char *shader_name, dafg::DrawPrimitive primitive)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return {registry, nodeId, shaderId, primitive};
}

DispatchComputeThreadsRequest Registry::dispatchThreads(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).threads();
}

DispatchComputeGroupsRequest Registry::dispatchGroups(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).groups();
}

DispatchComputeIndirectRequest Registry::dispatchIndirect(const char *shader_name, const char *buffer)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).indirect(buffer);
}

DispatchMeshThreadsRequest Registry::dispatchMeshThreads(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).mesh().threads();
}

DispatchMeshGroupsRequest Registry::dispatchMeshGroups(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).mesh().groups();
}

DispatchMeshIndirectRequest Registry::dispatchMeshIndirect(const char *shader_name, const char *buffer)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).mesh().indirect(buffer);
}

NameSpaceRequest Registry::root() const { return {registry->knownNames.root(), nodeId, registry}; }

VirtualResourceCreationSemiRequest Registry::create(const char *name, History history)
{
  auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);

  auto &res = registry->resources.get(resId);
  res.history = history;

  auto &node = registry->nodes[nodeId];
  if (DAGOR_UNLIKELY(node.createdResources.find(resId) != node.createdResources.end()))
    logerr("daFG: FG: node %s recreates an already created resource %s.", registry->knownNames.getName(nodeId), name);

  node.createdResources.insert(resId);
  node.resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Usage::UNKNOWN, Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

Registry::VirtualTextureRequest<Registry::NewRwRequestPolicy> Registry::registerBackBuffer(const char *name)
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);
  auto &res = registry->resources.get(resId);

  res.creationInfo = DriverDeferredTexture{};
  res.type = ResourceType::Texture;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Usage::UNKNOWN, Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

detail::ResUid Registry::registerBufferImpl(const char *name, dafg::ExternalResourceProvider &&p)
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);
  auto &res = registry->resources.get(resId);

  res.creationInfo = eastl::move(p);
  res.type = ResourceType::Buffer;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Usage::UNKNOWN, Access::READ_WRITE}});

  return {resId, false};
}

detail::ResUid Registry::registerTextureImpl(const char *name, dafg::ExternalResourceProvider &&p)
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);
  auto &res = registry->resources.get(resId);

  res.creationInfo = eastl::move(p);
  res.type = ResourceType::Texture;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Usage::UNKNOWN, Access::READ_WRITE}});

  return {resId, false};
}

} // namespace dafg
