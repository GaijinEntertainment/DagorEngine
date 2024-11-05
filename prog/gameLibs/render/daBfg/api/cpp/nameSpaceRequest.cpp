// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/nameSpaceRequest.h>
#include <frontend/internalRegistry.h>


namespace dabfg
{

NameSpaceRequest NameSpaceRequest::operator/(const char *child_name) const
{
  const auto nsId = registry->knownNames.addNameId<NameSpaceNameId>(nameSpaceId, child_name);
  return {nsId, nodeId, registry};
}

template <int D>
AutoResolutionRequest<D> NameSpaceRequest::getResolution(const char *type_name, float multiplier) const
{
  const auto autoResTypeId = registry->knownNames.addNameId<AutoResTypeNameId>(nameSpaceId, type_name);
  return {autoResTypeId, multiplier, &registry->resourceProviderReference};
}

template AutoResolutionRequest<2> NameSpaceRequest::getResolution<2>(const char *, float) const;
template AutoResolutionRequest<3> NameSpaceRequest::getResolution<3>(const char *, float) const;

// === Resource requesting ===

VirtualResourceSemiRequest<NameSpaceRequest::NewRoRequestPolicy> NameSpaceRequest::read(const char *name) const
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);

  registry->nodes[nodeId].readResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_ONLY}});

  return {{resId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<NameSpaceRequest::NewRoRequestPolicy> NameSpaceRequest::read(NamedSlot slot_name) const
{
  const auto slotResId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, slot_name.name);

  registry->nodes[nodeId].readResources.insert(slotResId);
  registry->nodes[nodeId].resourceRequests.emplace(slotResId, ResourceRequest{ResourceUsage{Access::READ_ONLY}, true});

  return {{slotResId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<NameSpaceRequest::NewHistRequestPolicy> NameSpaceRequest::historyFor(const char *name) const
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);
  registry->nodes[nodeId].historyResourceReadRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_ONLY}});
  return {{resId, true}, nodeId, registry};
}

VirtualResourceSemiRequest<NameSpaceRequest::NewRwRequestPolicy> NameSpaceRequest::modify(const char *name) const
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);

  registry->nodes[nodeId].modifiedResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<NameSpaceRequest::NewRwRequestPolicy> NameSpaceRequest::modify(NamedSlot slot_name) const
{
  const auto slotResId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, slot_name.name);

  registry->nodes[nodeId].modifiedResources.insert(slotResId);
  registry->nodes[nodeId].resourceRequests.emplace(slotResId, ResourceRequest{ResourceUsage{Access::READ_WRITE}, true});

  return {{slotResId, false}, nodeId, registry};
}

VirtualResourceSemiRequest<NameSpaceRequest::NewRwRequestPolicy> NameSpaceRequest::rename(const char *from, const char *to,
  History history) const
{
  const auto fromResId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, from);
  const auto nodeNsId = registry->knownNames.getParent(nodeId);
  const auto toResId = registry->knownNames.addNameId<ResNameId>(nodeNsId, to);

  registry->nodes[nodeId].renamedResources.emplace(toResId, fromResId);
  registry->nodes[nodeId].resourceRequests.emplace(fromResId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});
  registry->resources.get(toResId).history = history;

  return {{fromResId, false}, nodeId, registry};
}

} // namespace dabfg