#include "dependencyDataCalculator.h"

#include <frontend/dumpInternalRegistry.h>


namespace dabfg
{

void DependencyDataCalculator::recalculateResourceLifetimes()
{
  auto &lifetimes = depData.resourceLifetimes;

  // Don't free memory pre-allocated inside lifetime objects
  lifetimes.resize(registry.knownNames.nameCount<ResNameId>());
  for (auto &lifetime : lifetimes)
    lifetime = {};

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    auto markIntroducedByCurrent = [&, nodeId = nodeId](ResNameId resource) {
      auto &lifetime = lifetimes[resource];

      G_ASSERT_DO_AND_LOG(lifetime.introducedBy == NodeNameId::Invalid, dump_internal_registry(registry),
        "Virtual resource '%s' was introduced twice: by '%s' and by '%s'!", registry.knownNames.getName(resource),
        registry.knownNames.getName(nodeId), registry.knownNames.getName(lifetime.introducedBy));

      lifetime.introducedBy = nodeId;
      // See comment about nodes that both introduce and modify below.
      eastl::erase(lifetime.modificationChain, nodeId);
    };

    for (const auto &resId : nodeData.createdResources)
      markIntroducedByCurrent(resId);

    for (const auto &unresolvedResId : nodeData.readResources)
      lifetimes[nameResolver.resolve(unresolvedResId)].readers.push_back(nodeId);

    // Note that if the introductor/consumer node also modifies this
    // resource, we ignore the modification. This happens when a
    // node creates a rendertarget and then immediately renders to it,
    // or renames it after rendering.
    for (auto unresolvedResId : nodeData.modifiedResources)
    {
      const auto resId = nameResolver.resolve(unresolvedResId);
      if (lifetimes[resId].introducedBy != nodeId && lifetimes[resId].consumedBy != nodeId)
        lifetimes[resId].modificationChain.emplace_back(nodeId);
    }

    for (auto [produced, unresolvedConsumed] : nodeData.renamedResources)
    {
      // Sanity check (it's an invariant)
      G_ASSERT(produced != unresolvedConsumed);
      G_ASSERT(produced == nameResolver.resolve(produced));

      const auto consumed = nameResolver.resolve(unresolvedConsumed);
      auto &consumedLifetime = lifetimes[consumed];

      markIntroducedByCurrent(produced);

      G_ASSERT_DO_AND_LOG(consumedLifetime.consumedBy == NodeNameId::Invalid, dump_internal_registry(registry),
        "Virtual resource '%s' (resolved from '%s') was consumed twice: by '%s' and by '%s'!", registry.knownNames.getName(consumed),
        registry.knownNames.getName(unresolvedConsumed), registry.knownNames.getName(nodeId),
        registry.knownNames.getName(consumedLifetime.consumedBy));

      consumedLifetime.consumedBy = nodeId;
      // See comment about nodes that both introduce/consume and modify above.
      eastl::erase(consumedLifetime.modificationChain, nodeId);
    }
  }
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

void DependencyDataCalculator::updateRenamedResourceProperties()
{
  for (auto [resId, repId] : depData.renamingRepresentatives.enumerate())
    if (resId != repId)
    {
      // TODO: I kind of hate this now, gotta rework it one day
      // The internal registry state may contain out of date or
      // invalid states, the types don't really model how this system
      // actually works.
      // Idea to try: make registry.resources contain
      // variant<ResourceData, ResNameId> where the second one means
      // "redirection" to a different resource.
      auto &res = registry.resources[resId];
      const auto &rep = registry.resources[repId];
      res.type = rep.type;
      res.resolution = rep.resolution;
    }
}

void DependencyDataCalculator::recalculate()
{
  recalculateResourceLifetimes();
  resolveRenaming();
  updateRenamedResourceProperties();
}

} // namespace dabfg
