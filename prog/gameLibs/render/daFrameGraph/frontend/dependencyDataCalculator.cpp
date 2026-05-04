// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dependencyDataCalculator.h"

#include <frontend/dumpInternalRegistry.h>


namespace dafg
{

DependencyDataCalculator::ResourcesChanged DependencyDataCalculator::recalculateResourceLifetimes(const NodesChanged &node_changed)
{
  DependencyDataCalculator::ResourcesChanged result;
  result.resize(registry.knownNames.nameCount<ResNameId>(), false);

  FRAMEMEM_VALIDATE;

  auto &lifetimes = depData.resourceLifetimes;

  for (auto [resId, lifetime] : lifetimes.enumerate())
  {
    if (node_changed.test(lifetime.introducedBy, false))
    {
      lifetime.introducedBy = NodeNameId::Invalid;
      result.set(resId, true);
    }

    if (node_changed.test(lifetime.consumedBy, false))
    {
      lifetime.consumedBy = NodeNameId::Invalid;
      result.set(resId, true);
    }

    const auto nodeChanged = [&node_changed](NodeNameId nodeId) { return node_changed[nodeId]; };

    if (eastl::erase_if(lifetime.modificationChain, nodeChanged) > 0)
      result.set(resId, true);
    if (eastl::erase_if(lifetime.readers, nodeChanged) > 0)
      result.set(resId, true);
  }

  lifetimes.resize(registry.knownNames.nameCount<ResNameId>());
  depDataClone.resourceLifetimes.resize(registry.knownNames.nameCount<ResNameId>());

  for (auto nodeId : node_changed.trueKeys())
  {
    auto &nodeData = registry.nodes[nodeId];

    auto markIntroducedByCurrent = [&, nodeId = nodeId](ResNameId resource) {
      auto &lifetime = lifetimes[resource];

      if (lifetime.introducedBy == NodeNameId::Invalid)
        lifetime.introducedBy = nodeId;
      else
        lifetime.erroneousIntroducers.insert(nodeId);

      // See comment about nodes that both introduce and modify below.
      eastl::erase(lifetime.modificationChain, nodeId);

      result.set(resource, true);
    };

    for (const auto &resId : nodeData.createdResources)
      markIntroducedByCurrent(resId);

    for (const auto &unresolvedResId : nodeData.readResources)
    {
      const auto resId = nameResolver.resolve(unresolvedResId);
      lifetimes[resId].readers.push_back(nodeId);
      result.set(resId, true);
    }

    for (const auto &resRequest : nodeData.historyResourceReadRequests)
    {
      const auto resId = nameResolver.resolve(resRequest.first);
      lifetimes[resId].historyReaders.push_back(nodeId);
      result.set(resId, true);
    }

    // Note that if the introductor/consumer node also modifies this
    // resource, we ignore the modification. This happens when a
    // node creates a rendertarget and then immediately renders to it,
    // or renames it after rendering.
    for (auto unresolvedResId : nodeData.modifiedResources)
    {
      const auto resId = nameResolver.resolve(unresolvedResId);
      if (lifetimes[resId].introducedBy != nodeId && lifetimes[resId].consumedBy != nodeId)
        lifetimes[resId].modificationChain.emplace_back(nodeId);
      result.set(resId, true);
    }

    for (auto [produced, unresolvedConsumed] : nodeData.renamedResources)
    {
      // Sanity check (it's an invariant)
      G_ASSERT(produced != unresolvedConsumed);
      G_ASSERT(produced == nameResolver.resolve(produced));

      const auto consumed = nameResolver.resolve(unresolvedConsumed);
      auto &consumedLifetime = lifetimes[consumed];

      markIntroducedByCurrent(produced);

      if (consumedLifetime.consumedBy == NodeNameId::Invalid)
        consumedLifetime.consumedBy = nodeId;
      else
        consumedLifetime.erroneousConsumers.insert(nodeId);

      // See comment about nodes that both introduce/consume and modify above.
      eastl::erase(consumedLifetime.modificationChain, nodeId);

      result.set(consumed, true);
    }
  }

  for (auto [resId, lifetime] : lifetimes.enumerate())
  {
    if (!result[resId])
      continue;

    auto &prevLifetime = depDataClone.resourceLifetimes[resId];
    if (lifetime == prevLifetime)
      result.set(resId, false);
    else // Manual loop fusion to do less memory reads/writes.
      prevLifetime = lifetime;
  }

  return result;
}

void DependencyDataCalculator::resolveRenaming()
{
  auto &representatives = depData.renamingRepresentatives;
  auto &chains = depData.renamingChains;

  auto iotaRange = IdRange<ResNameId>(registry.knownNames.nameCount<ResNameId>());
  representatives.assign(iotaRange.begin(), iotaRange.end());
  chains.assign(iotaRange.begin(), iotaRange.end());

  for (const auto &nodeData : registry.nodes)
    for (const auto [to, unresolvedFrom] : nodeData.renamedResources)
    {
      const auto from = nameResolver.resolve(unresolvedFrom);
      representatives[to] = from;
      chains[from] = to;
    }

  for (auto [resId, repRef] : representatives.enumerate())
  {
    while (representatives[repRef] != repRef)
    {
      // This code will be executed at most once for every modification,
      // but modifications are bounded by resource count, so O(n)
      auto tmp = representatives[repRef];
      representatives[repRef] = representatives[representatives[repRef]];
      repRef = tmp;
    }
  }
}

DependencyDataCalculator::ResourcesChanged DependencyDataCalculator::recalculate(const NodesChanged &node_changed)
{
  auto result = recalculateResourceLifetimes(node_changed);
  resolveRenaming();
  return result;
}

} // namespace dafg
