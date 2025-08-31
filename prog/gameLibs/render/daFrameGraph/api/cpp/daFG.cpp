// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <runtime/runtime.h>


namespace dafg
{

detail::NodeUid detail::register_node(NameSpaceNameId nsId, const char *name, const char *node_source,
  DeclarationCallback &&declaration_callback)
{
  if (!Runtime::isInitialized())
    return NodeTracker::pre_register_node(nsId, name, node_source, eastl::move(declaration_callback));

  auto &runtime = Runtime::get();
  auto &tracker = runtime.getNodeTracker();
  auto &registry = runtime.getInternalRegistry();

  const auto nodeId = registry.knownNames.addNameId<NodeNameId>(nsId, name);

  tracker.unregisterNode(nodeId, registry.nodes.get(nodeId).generation);
  tracker.registerNode(nullptr, nodeId);

  auto &nodeData = registry.nodes[nodeId];
  nodeData.declare = eastl::move(declaration_callback);
  nodeData.nodeSource = node_source;

  return {nodeId, nodeData.generation, 1};
}

void detail::unregister_node(detail::NodeUid uid)
{
  if (!Runtime::isInitialized())
    return;

  auto &tracker = Runtime::get().getNodeTracker();
  tracker.unregisterNode(uid.nodeId, uid.generation);
}

NameSpace root() { return {}; }

void update_external_state(ExternalState state) { Runtime::get().updateExternalState(state); }

void set_multiplexing_default_mode(multiplexing::Mode mode, multiplexing::Mode history_mode)
{
  auto &runtime = Runtime::get();
  auto &registry = runtime.getInternalRegistry();
  if (registry.defaultMultiplexingMode != mode || registry.defaultHistoryMultiplexingMode != history_mode)
  {
    registry.defaultMultiplexingMode = mode;
    registry.defaultHistoryMultiplexingMode = history_mode;
    runtime.markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
  }
}

void set_multiplexing_extents(multiplexing::Extents extents) { Runtime::get().setMultiplexingExtents(extents); }

bool run_nodes() { return Runtime::get().runNodes(); }

void startup() { Runtime::startup(); }

void shutdown() { Runtime::shutdown(); }

void invalidate_history() { Runtime::get().invalidateHistory(); }

} // namespace dafg
