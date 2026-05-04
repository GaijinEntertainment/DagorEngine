// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/detail/baseRegistry.h>
#include <render/daFrameGraph/detail/shaderNameId.h>
#include <frontend/internalRegistry.h>


namespace dafg
{

BaseRegistry::BaseRegistry(NodeNameId node, InternalRegistry *reg) : NameSpaceRequest(reg->knownNames.getParent(node), node, reg) {}

BaseRegistry BaseRegistry::allowAsyncPipelines()
{
  registry->nodes[nodeId].allowAsyncPipelines = true;
  return *this;
}

BaseRegistry BaseRegistry::orderMeBefore(const char *name)
{
  const auto beforeId = registry->knownNames.addNameId<NodeNameId>(nameSpaceId, name);

  if (DAGOR_LIKELY(beforeId != NodeNameId::Invalid))
    registry->nodes[nodeId].followingNodeIds.insert(beforeId);
  else
    logerr("daFG: FG: node %s tries to order it before a non-existent node %s, skipping this ordering.",
      registry->knownNames.getName(nodeId), name);

  return *this;
}

BaseRegistry BaseRegistry::orderMeBefore(std::initializer_list<const char *> names)
{
  for (auto name : names)
    orderMeBefore(name);
  return *this;
}

BaseRegistry BaseRegistry::orderMeAfter(const char *name)
{
  const auto afterId = registry->knownNames.addNameId<NodeNameId>(nameSpaceId, name);

  if (DAGOR_LIKELY(afterId != NodeNameId::Invalid))
    registry->nodes[nodeId].precedingNodeIds.insert(afterId);
  else
    logerr("daFG: FG: node %s tries to order it after a non-existent node %s, skipping this ordering.",
      registry->knownNames.getName(nodeId), name);

  return *this;
}

BaseRegistry BaseRegistry::orderMeAfter(std::initializer_list<const char *> names)
{
  for (auto name : names)
    orderMeAfter(name);
  return *this;
}

BaseRegistry BaseRegistry::setPriority(priority_t prio)
{
  registry->nodes[nodeId].priority = prio;
  return *this;
}

BaseRegistry BaseRegistry::multiplex(multiplexing::Mode mode)
{
  registry->nodes[nodeId].multiplexingMode = mode;
  return *this;
}

BaseRegistry BaseRegistry::executionHas(SideEffects sideEffect)
{
  registry->nodes[nodeId].sideEffect = sideEffect;
  return *this;
}

StateRequest BaseRegistry::requestState() { return {registry, nodeId}; }

VirtualPassRequest BaseRegistry::requestRenderPass() { return {nodeId, registry}; }

DrawRequest<detail::DrawRequestPolicy::Default, false> BaseRegistry::draw(const char *shader_name, dafg::DrawPrimitive primitive)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return {registry, nodeId, shaderId, primitive};
}

DrawRequest<detail::DrawRequestPolicy::Default, true> BaseRegistry::drawIndexed(const char *shader_name, dafg::DrawPrimitive primitive)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return {registry, nodeId, shaderId, primitive};
}

DispatchComputeThreadsRequest BaseRegistry::dispatchThreads(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).threads();
}

DispatchComputeGroupsRequest BaseRegistry::dispatchGroups(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).groups();
}

DispatchComputeIndirectRequest BaseRegistry::dispatchIndirect(const char *shader_name, const char *buffer)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).indirect(buffer);
}

DispatchMeshThreadsRequest BaseRegistry::dispatchMeshThreads(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).mesh().threads();
}

DispatchMeshGroupsRequest BaseRegistry::dispatchMeshGroups(const char *shader_name)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).mesh().groups();
}

DispatchMeshIndirectRequest BaseRegistry::dispatchMeshIndirect(const char *shader_name, const char *buffer)
{
  const ShaderNameId shaderId = registry->knownShaders.addNameId(shader_name);
  return DispatchRequest<detail::DispatchRequestPolicy::Default>(registry, nodeId, shaderId).mesh().indirect(buffer);
}

NameSpaceRequest BaseRegistry::root() const { return {registry->knownNames.root(), nodeId, registry}; }

VirtualResourceCreationSemiRequest BaseRegistry::create(const char *name)
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);

  // History option can be provided later
  registry->resources.get(resId) = ResourceData{.createdResData = CreatedResourceData{}};

  auto &node = registry->nodes[nodeId];
  // node treates both created and registered resources as "created"
  if (DAGOR_UNLIKELY(node.createdResources.find(resId) != node.createdResources.end()))
    logerr("daFG: FG: node %s recreates an already created/registered resource %s.", registry->knownNames.getName(nodeId), name);

  node.createdResources.insert(resId);
  node.resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Usage::UNKNOWN, Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

VirtualResourceRegistrationSemiRequest BaseRegistry::registerExternal(const char *name)
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);

  // History is always No for external resources
  registry->resources.get(resId) = ResourceData{.createdResData = CreatedResourceData{}};

  auto &node = registry->nodes[nodeId];
  // node treates both created and registered resources as "created"
  if (DAGOR_UNLIKELY(node.createdResources.find(resId) != node.createdResources.end()))
    logerr("daFG: FG: node %s re-registers an already created/registered resource %s.", registry->knownNames.getName(nodeId), name);

  node.createdResources.insert(resId);
  node.resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Usage::UNKNOWN, Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

} // namespace dafg
