// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nameResolver.h"

#include <common/genericPoint.h>


namespace dafg
{

NameResolver::Changes NameResolver::update(const NodesChanged &node_changed)
{
  NameResolver::Changes result;

  result.nameResolutionChanged.resize<ResNameId>(registry.knownNames.nameCount<ResNameId>(), false);
  result.nameResolutionChanged.resize<AutoResTypeNameId>(registry.knownNames.nameCount<AutoResTypeNameId>(), false);
  result.nodesWithChangedRequests.resize(registry.knownNames.nameCount<NodeNameId>(), false);

  FRAMEMEM_VALIDATE;

  // IMPORTANT: sizes of these MUST match the CURRENT registry sizes!
  // Otherwise we will miss new resources appearing!
  PrevResolvedNameForResource prevResolvedNameForResource(registry.knownNames.nameCount<ResNameId>(), ResNameId::Invalid);
  PrevResolvedNameForAutoResType prevResolvedNameForAutoResType(registry.knownNames.nameCount<AutoResTypeNameId>(),
    AutoResTypeNameId::Invalid);

  for (auto resId : IdRange<ResNameId>(resolver.size<ResNameId>()))
    prevResolvedNameForResource[resId] = getPrevResolvedNameForResource(resId);

  for (auto autoResTypeId : IdRange<AutoResTypeNameId>(resolver.size<AutoResTypeNameId>()))
    prevResolvedNameForAutoResType[autoResTypeId] = resolve(autoResTypeId);

  updateMapping();

  for (auto [resId, wasResolvedTo] : prevResolvedNameForResource.enumerate())
    result.nameResolutionChanged.set<ResNameId>(resId, wasResolvedTo != resolve(resId));

  for (auto [autoResTypeId, wasResolvedTo] : prevResolvedNameForAutoResType.enumerate())
    result.nameResolutionChanged.set<AutoResTypeNameId>(autoResTypeId, wasResolvedTo != resolve(autoResTypeId));

  updateInverseMapping(result.nodesWithChangedRequests, prevResolvedNameForResource, node_changed, result.nameResolutionChanged);

  return result;
}

ResNameId NameResolver::getPrevResolvedNameForResource(ResNameId res_id) const
{
  // Use this function before resolver.rebuild!

  // Missing optional resources will not get resolved to anything
  if (auto prevResolvedId = resolver.resolve<ResNameId>(res_id); prevResolvedId != ResNameId::Invalid)
    res_id = prevResolvedId;

  if (const auto &slotData = registry.resourceSlots[res_id]; slotData.has_value() && slotData->prevContents != ResNameId::Invalid)
    res_id = slotData->prevContents;

  return res_id;
}

void NameResolver::updateMapping()
{
  // This validity info simply tells us whether an entity was created
  // from a user's point of view and is only used for the purposes of
  // name lookup.
  IdIndexedFlags<ResNameId, framemem_allocator> resourceValid(registry.knownNames.nameCount<ResNameId>(), false);
  IdIndexedFlags<NodeNameId, framemem_allocator> nodeValid(registry.knownNames.nameCount<NodeNameId>(), false);
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    if (!nodeData.declare)
      continue;
    nodeValid[nodeId] = true;
    for (auto resId : nodeData.createdResources)
      resourceValid[resId] = true;
    for (auto [to, from] : nodeData.renamedResources)
      resourceValid[to] = true;
  }
  for (auto [resId, data] : registry.resourceSlots.enumerate())
    if (data.has_value())
    {
      auto &slot = data.value();
      slot.prevContents = slot.contents;
      resourceValid[resId] = true;
    }
  IdIndexedFlags<AutoResTypeNameId, framemem_allocator> autoResTypeValid(registry.knownNames.nameCount<AutoResTypeNameId>(), false);
  for (auto [autoResTypeId, data] : registry.autoResTypes.enumerate())
  {
    const bool positive = eastl::visit(
      [&](const auto &values) {
        if constexpr (eastl::is_same_v<eastl::decay_t<decltype(values)>, eastl::monostate>)
          return false;
        else
          return all_greater(values.staticResolution, 0) && all_greater(values.dynamicResolution, 0);
      },
      data.values);

    if (positive)
      autoResTypeValid[autoResTypeId] = true;
  }

  resolver.rebuild(registry.knownNames, resourceValid, nodeValid, autoResTypeValid);
}

void NameResolver::updateInverseMapping(NodesChanged &out_nodes_with_changed_requests,
  const PrevResolvedNameForResource &prev_resolved_name_for_resource, const NodesChanged &node_changed,
  const NameResolutionChanged &name_resolution_changed)
{
  // Don't free memory pre-allocated inside the vector maps
  inverseMapping.resize(registry.knownNames.nameCount<NodeNameId>());
  inverseHistoryMapping.resize(registry.knownNames.nameCount<NodeNameId>());

  auto updateInverseMappingForResWithChangedResolvedName = [&](ResNameId unresolved_res_id, auto &inv_mapping, NodeNameId node_id) {
    const auto prevResolvedName = prev_resolved_name_for_resource[unresolved_res_id];
    G_UNUSED(node_id);
    G_FAST_ASSERT(prevResolvedName != ResNameId::Invalid);

    // delete old resolved -> unresolved
    auto &prevUnresolvedResources = inv_mapping[prevResolvedName];
    auto it = eastl::find(prevUnresolvedResources.begin(), prevUnresolvedResources.end(), unresolved_res_id);
    G_ASSERTF(it != prevUnresolvedResources.end(), "can't find prev request %s -> %s for node %s",
      registry.knownNames.getName(unresolved_res_id), registry.knownNames.getName(prevResolvedName),
      registry.knownNames.getName(node_id));
    if (prevUnresolvedResources.size() == 1)
    {
      inv_mapping.erase(prevResolvedName);
    }
    else
    {
      eastl::swap(*it, prevUnresolvedResources.back());
      prevUnresolvedResources.pop_back();
    }

    inv_mapping[resolve(unresolved_res_id)].push_back(unresolved_res_id);
  };

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    auto &nodeInverseMapping = inverseMapping[nodeId];
    auto &nodeInverseHistoryMapping = inverseHistoryMapping[nodeId];

    if (node_changed[nodeId])
    {
      nodeInverseMapping.clear();
      nodeInverseHistoryMapping.clear();

      for (const auto &[resId, _] : nodeData.resourceRequests)
        nodeInverseMapping[resolve(resId)].push_back(resId);
      for (const auto &[resId, _] : nodeData.historyResourceReadRequests)
        nodeInverseHistoryMapping[resolve(resId)].push_back(resId);
    }
    else
    {
      bool resolvedRequestsChanged = false;
      for (const auto &[resId, _] : nodeData.resourceRequests)
        if (name_resolution_changed[resId])
        {
          updateInverseMappingForResWithChangedResolvedName(resId, nodeInverseMapping, nodeId);
          resolvedRequestsChanged = true;
        }

      bool resolvedHistoryRequestsChanged = false;
      for (const auto &[resId, _] : nodeData.historyResourceReadRequests)
        if (name_resolution_changed[resId])
        {
          updateInverseMappingForResWithChangedResolvedName(resId, nodeInverseHistoryMapping, nodeId);
          resolvedHistoryRequestsChanged = true;
        }

      out_nodes_with_changed_requests.set(nodeId, resolvedRequestsChanged || resolvedHistoryRequestsChanged);
    }
  }
}

template <class T>
T NameResolver::resolve(T name_id) const
{
  // Missing optional resources will not get resolved to anything
  if (auto resolvedId = resolver.resolve<T>(name_id); resolvedId != T::Invalid)
    name_id = resolvedId;

  // We may want to have slots stuff other than resources in the future
  if constexpr (eastl::is_same_v<T, ResNameId>)
    if (const auto &slotData = registry.resourceSlots[name_id]; slotData.has_value())
      name_id = slotData->contents;

  return name_id;
}

template ResNameId NameResolver::resolve(ResNameId res_name_id) const;
template AutoResTypeNameId NameResolver::resolve(AutoResTypeNameId res_name_id) const;

eastl::span<ResNameId const> NameResolver::unresolve(NodeNameId nodeId, ResNameId resId) const
{
  const auto it = inverseMapping[nodeId].find(resId);
  return it != inverseMapping[nodeId].end() ? it->second : eastl::span<ResNameId const>{};
}

eastl::span<ResNameId const> NameResolver::historyUnresolve(NodeNameId nodeId, ResNameId resId) const
{
  const auto it = inverseHistoryMapping[nodeId].find(resId);
  return it != inverseHistoryMapping[nodeId].end() ? it->second : eastl::span<ResNameId const>{};
}

} // namespace dafg
