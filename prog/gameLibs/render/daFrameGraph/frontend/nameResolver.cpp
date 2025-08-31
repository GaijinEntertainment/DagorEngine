// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nameResolver.h"
#include "recompilationData.h"

#include <common/genericPoint.h>


namespace dafg
{

void NameResolver::update()
{
  FRAMEMEM_VALIDATE;

  PrevResolvedNameForResource prevResolvedNameForResource(resolver.size<ResNameId>(), ResNameId::Invalid);

  for (auto [resId, wasResolvedTo] : prevResolvedNameForResource.enumerate())
    wasResolvedTo = getPrevResolvedNameForResource(resId);

  updateMapping();
  updateFrontendRecompilationData(prevResolvedNameForResource);
  updateInverseMapping(prevResolvedNameForResource);
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

void NameResolver::updateFrontendRecompilationData(const PrevResolvedNameForResource &prev_resolved_name_for_resource)
{
  auto &resourceResolvedNameUnchanged = frontendRecompilationData.resourceResolvedNameUnchanged;

  resourceResolvedNameUnchanged.clear();
  resourceResolvedNameUnchanged.resize(registry.knownNames.nameCount<ResNameId>(), false);

  for (auto [resId, wasResolvedTo] : prev_resolved_name_for_resource.enumerate())
    resourceResolvedNameUnchanged.set(resId, wasResolvedTo == resolve(resId));
}

void NameResolver::updateInverseMapping(const PrevResolvedNameForResource &prev_resolved_name_for_resource)
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

  const auto &resourceResolvedNameUnchanged = frontendRecompilationData.resourceResolvedNameUnchanged;

  auto &resourceRequestsForNodeUnchagned = frontendRecompilationData.resourceRequestsForNodeUnchanged;
  auto &nodeUnchanged = frontendRecompilationData.nodeUnchanged;
  resourceRequestsForNodeUnchagned = nodeUnchanged;

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    auto &nodeInverseMapping = inverseMapping[nodeId];
    auto &nodeInverseHistoyMapping = inverseHistoryMapping[nodeId];

    if (!frontendRecompilationData.nodeUnchanged[nodeId])
    {
      nodeInverseMapping.clear();
      nodeInverseHistoyMapping.clear();

      for (const auto &[resId, _] : nodeData.resourceRequests)
        nodeInverseMapping[resolve(resId)].push_back(resId);
      for (const auto &[resId, _] : nodeData.historyResourceReadRequests)
        nodeInverseHistoyMapping[resolve(resId)].push_back(resId);
    }
    else
    {
      bool resolvedRequestsUnchanged = true;
      for (const auto &[resId, _] : nodeData.resourceRequests)
        if (!resourceResolvedNameUnchanged[resId])
        {
          updateInverseMappingForResWithChangedResolvedName(resId, nodeInverseMapping, nodeId);
          resolvedRequestsUnchanged = false;
        }

      bool resolvedHistoryRequestsUnchanged = true;
      for (const auto &[resId, _] : nodeData.historyResourceReadRequests)
        if (!resourceResolvedNameUnchanged[resId])
        {
          updateInverseMappingForResWithChangedResolvedName(resId, nodeInverseHistoyMapping, nodeId);
          resolvedHistoryRequestsUnchanged = false;
        }

      // nodeUnchanged --- true if node declaration, resolved requests and resolved history requests weren't changed
      // resourceRequestsForNodeUnchagned --- true if node declaration and resolved requests weren't changed
      // We need both flags, because recalculation of lifetimes in dependencyDataCalculator uses only resourceRequests,
      // but irNode calculation can only be skipped if resourceRequests and historyResourceReadRequests weren't changed
      frontendRecompilationData.nodeUnchanged.set(nodeId, resolvedRequestsUnchanged && resolvedHistoryRequestsUnchanged);
      frontendRecompilationData.resourceRequestsForNodeUnchanged.set(nodeId, resolvedRequestsUnchanged);
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
template NodeNameId NameResolver::resolve(NodeNameId res_name_id) const;
template AutoResTypeNameId NameResolver::resolve(AutoResTypeNameId res_name_id) const;

eastl::span<ResNameId const> NameResolver::unresolve(NodeNameId nodeId, ResNameId resId) const
{
  const auto it = inverseMapping[nodeId].find(resId);
  return it != inverseMapping[nodeId].end() ? it->second : eastl::span<ResNameId const>{};
}

} // namespace dafg
