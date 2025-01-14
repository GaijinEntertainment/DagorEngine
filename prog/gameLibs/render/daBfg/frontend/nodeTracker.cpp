// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeTracker.h"

#include <debug/backendDebug.h>
#include <math/random/dag_random.h>

#include <frontend/dumpInternalRegistry.h>
#include <frontend/dependencyDataCalculator.h>
#include <frontend/internalRegistry.h>

#include <perfMon/dag_statDrv.h>


// Useful for ensuring that not dependencies are missing.
// Will shuffle nodes while preserving all ordering constraints.
// If anything depends on a "lucky" node order, this should break it.
CONSOLE_BOOL_VAL("dabfg", randomize_order, false);

namespace dabfg
{

void NodeTracker::registerNode(void *context, NodeNameId nodeId)
{
  checkChangesLock();

  nodesChanged = true;
  invalidate_graph_visualization();

  deferredDeclarationQueue.emplace(nodeId);

  // Make sure that the id is mapped, as the nodes map is the
  // single point of truth here.
  registry.nodes.get(nodeId);

  nodeToContext.set(nodeId, context);
  trackedContexts.insert(context);
}

void NodeTracker::unregisterNode(NodeNameId nodeId, uint16_t gen)
{
  checkChangesLock();

  // If there was no such node in the first place, no need to
  // invalidate caches and try to clean up
  if (!registry.nodes.get(nodeId).declare)
    return;

  // If the node was already re-registered and generation incremented,
  // we shouldn't wipe the "new" node's data
  if (gen < registry.nodes[nodeId].generation)
    return;

  if (auto ctx = nodeToContext.get(nodeId))
    collectCreatedBlobs(nodeId, deferredResourceWipeSets[ctx]);

  nodesChanged = true;
  invalidate_graph_visualization();

  // In case the node didn't have a chance to declare resources yet,
  // clear from it the cache
  deferredDeclarationQueue.erase(nodeId);

  auto evictResId = [this](ResNameId res_id) {
    // Invalidate all renamed versions of this resource
    for (;;)
    {
      if (registry.resources.isMapped(res_id))
      {
        // History needs to be preserved, as it is determined by
        // the virtual resource itself and not inhereted from the
        // thing that was renamed into it. Everything else should be
        // reset to the defaults.
        auto &res = registry.resources.get(res_id);
        auto oldHist = res.history;
        res = {};
        res.history = oldHist;
      }

      if (!depData.renamingChains.isMapped(res_id) || depData.renamingChains[res_id] == res_id)
        break;

      res_id = depData.renamingChains[res_id];
    }
  };

  auto &nodeData = registry.nodes[nodeId];

  // Evict all resNameIds *produced* by this node
  for (const auto &[to, _] : nodeData.renamedResources)
    evictResId(to);

  for (const auto &resId : nodeData.createdResources)
    evictResId(resId);

  // Clear node data
  nodeData = {};
  nodeData.generation = gen + 1;
  nodeToContext.set(nodeId, {});
}

void NodeTracker::collectCreatedBlobs(NodeNameId node_id, ResourceWipeSet &into) const
{
  for (const auto resId : registry.nodes[node_id].createdResources)
  {
    if (registry.resources[resId].type != ResourceType::Blob)
      continue;
    into.insert(resId);
  }
}

eastl::optional<NodeTracker::ResourceWipeSet> NodeTracker::wipeContextNodes(void *context)
{
  const auto it = trackedContexts.find(context);
  if (it == trackedContexts.end())
    return eastl::nullopt;

  debug("daBfg: Wiping nodes and resources managed by context %p...", context);

  NodeTracker::ResourceWipeSet result = eastl::move(deferredResourceWipeSets[context]);
  deferredResourceWipeSets[context].clear(); // moved-from state is valid but unspecified

  for (auto [nodeId, ctx] : nodeToContext.enumerate())
  {
    if (ctx != context)
      continue;

    collectCreatedBlobs(nodeId, result);

    unregisterNode(nodeId, registry.nodes[nodeId].generation);
    debug("daBfg: Wiped node %s with context %p", registry.knownNames.getName(nodeId), ctx);
  }

  trackedContexts.erase(it);

  for (auto resId : result)
    debug("daBfg: Resource %s needs to be wiped", registry.knownNames.getName(resId));

  return eastl::optional{eastl::move(result)};
}

void NodeTracker::updateNodeDeclarations()
{
  if (randomize_order.get())
    eastl::random_shuffle(deferredDeclarationQueue.begin(), deferredDeclarationQueue.end(),
      [](uint32_t n) { return static_cast<uint32_t>(grnd() % n); });

  for (auto nodeId : deferredDeclarationQueue)
  {
    auto &node = registry.nodes.get(nodeId);
#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
    if (!node.dapToken)
      node.dapToken = getProfileToken(nodeId);
#endif
    if (auto &declare = node.declare)
    {
      // Reset the value first to avoid funny side-effects later on
      registry.nodes[nodeId].execute = {};
      registry.nodes[nodeId].execute = declare(nodeId, &registry);
    }
  }

  deferredDeclarationQueue.clear();

  // Makes sure that further code doesn't go out of bounds on any of these
  registry.resources.resize(registry.knownNames.nameCount<ResNameId>());
  registry.nodes.resize(registry.knownNames.nameCount<NodeNameId>());
  registry.autoResTypes.resize(registry.knownNames.nameCount<AutoResTypeNameId>());
  registry.resourceSlots.resize(registry.knownNames.nameCount<ResNameId>());

  // If we are recompiling the entire graph before contexts are destroyed,
  // the resources will be wiped automatically if need be.
  deferredResourceWipeSets.clear();
}

void NodeTracker::dumpRawUserGraph() const { dump_internal_registry(registry); }

void NodeTracker::checkChangesLock() const
{
  if (nodeChangesLocked)
  {
    logerr("Attempted to modify framegraph structure while it was being executed!"
           "This is not supported, see callstack and remove the modification!");
  }
}

#if DAGOR_DBGLEVEL > 0
uint32_t NodeTracker::getProfileToken(NodeNameId nodeId) const
{
#if TIME_PROFILER_ENABLED
  const char *namePtr = registry.knownNames.getName(nodeId);
  if (namePtr[0] == '/')
    namePtr++;
  return ::da_profiler::add_description(__FILE__, __LINE__, /*flags*/ 0, namePtr);
#else
  G_UNUSED(nodeId);
  return 0;
#endif
}
#endif

} // namespace dabfg
