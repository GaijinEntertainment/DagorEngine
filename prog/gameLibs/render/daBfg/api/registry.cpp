#include <render/daBfg/registry.h>
#include <api/internalRegistry.h>


namespace dabfg
{

Registry::Registry(NodeNameId node, InternalRegistry *reg) : nodeId{node}, registry{reg} {}

Registry Registry::orderMeBefore(const char *name)
{
  // This is not a typo. Preced means "I want to get run before this guy",
  // which in turn means that this guy must follow us.
  const auto nameId = registry->knownNodeNames.getNameId(name);
  if (DAGOR_LIKELY(nameId != NodeNameId::Invalid))
    registry->nodes[nodeId].followingNodeIds.insert(nameId);
  else
    logerr("FG: node %s tries to order it before not exist node %s, skipping this order.", registry->knownNodeNames.getName(nodeId),
      name);

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
  const auto nameId = registry->knownNodeNames.getNameId(name);

  if (DAGOR_LIKELY(nameId != NodeNameId::Invalid))
    registry->nodes[nodeId].precedingNodeIds.insert(nameId);
  else
    logerr("FG: node %s tries to order it after not exist node %s, skipping this order.", registry->knownNodeNames.getName(nodeId),
      name);

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

// === Resource requesting ===

VirtualResourceCreationSemiRequest Registry::create(const char *name, History history)
{
  auto resId = registry->knownResourceNames.addNameId(name);

  auto &res = registry->resources.get(resId);
  res.history = history;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<Registry::NewRoRequestPolicy> Registry::read(const char *name)
{
  auto resId = registry->knownResourceNames.addNameId(name);
  registry->nodes[nodeId].readResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_ONLY}});

  return {{resId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<Registry::NewRoRequestPolicy> Registry::read(NamedSlot slot_name)
{
  auto slotResId = registry->knownResourceNames.addNameId(slot_name.name);
  registry->nodes[nodeId].readResources.insert(slotResId);
  registry->nodes[nodeId].resourceRequests.emplace(slotResId, ResourceRequest{ResourceUsage{Access::READ_ONLY}, true});

  return {{slotResId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<Registry::NewHistRequestPolicy> Registry::historyFor(const char *name)
{
  auto resId = registry->knownResourceNames.addNameId(name);
  registry->nodes[nodeId].historyResourceReadRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_ONLY}});
  return {{resId, true}, nodeId, registry};
}

VirtualResourceSemiRequest<Registry::NewRwRequestPolicy> Registry::modify(const char *name)
{
  auto resId = registry->knownResourceNames.addNameId(name);
  registry->nodes[nodeId].modifiedResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<Registry::NewRwRequestPolicy> Registry::modify(NamedSlot slot_name)
{
  auto slotResId = registry->knownResourceNames.addNameId(slot_name.name);
  registry->nodes[nodeId].modifiedResources.insert(slotResId);
  registry->nodes[nodeId].resourceRequests.emplace(slotResId, ResourceRequest{ResourceUsage{Access::READ_WRITE}, true});

  return {{slotResId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<Registry::NewRwRequestPolicy> Registry::rename(const char *from, const char *to, History history)
{
  const auto fromResId = registry->knownResourceNames.addNameId(from);
  const auto toResId = registry->knownResourceNames.addNameId(to);

  registry->nodes[nodeId].renamedResources.emplace(toResId, fromResId);
  registry->nodes[nodeId].resourceRequests.emplace(fromResId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});
  registry->resources.get(toResId).history = history;

  return {{fromResId, false}, nodeId, registry};
}

// === Resource creation ===

detail::ResUid Registry::registerTexture2dImpl(const char *name, dabfg::ExternalResourceProvider &&p)
{
  auto resId = registry->knownResourceNames.addNameId(name);
  auto &res = registry->resources.get(resId);

  res.creationInfo = eastl::move(p);
  res.type = ResourceType::Texture;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});

  return {resId, false};
}

} // namespace dabfg
