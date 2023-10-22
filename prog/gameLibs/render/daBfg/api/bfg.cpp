#include <render/daBfg/bfg.h>
#include <backend.h>


namespace dabfg
{

detail::NodeUid detail::register_node(NameSpaceNameId nsId, const char *name, const char *node_source,
  DeclarationCallback &&declaration_callback)
{
  auto &tracker = NodeTracker::get();

  const auto nodeId = tracker.registry.knownNames.addNameId<NodeNameId>(nsId, name);

  tracker.unregisterNode(nodeId, tracker.registry.nodes.get(nodeId).generation);
  tracker.registerNode(nodeId);

  auto &nodeData = tracker.registry.nodes[nodeId];
  nodeData.declare = eastl::move(declaration_callback);
  nodeData.nodeSource = node_source;

  return {nodeId, nodeData.generation, 1};
}

void detail::unregister_node(detail::NodeUid uid)
{
  auto &tracker = NodeTracker::get();
  tracker.unregisterNode(uid.nodeId, uid.generation);
}

NameSpace root() { return {}; }

void update_external_state(ExternalState state) { Backend::get().updateExternalState(state); }

void set_multiplexing_extents(multiplexing::Extents extents) { Backend::get().setMultiplexingExtents(extents); }

void run_nodes() { Backend::get().runNodes(); }

void startup() { Backend::startup(); }

void shutdown() { Backend::shutdown(); }

} // namespace dabfg
