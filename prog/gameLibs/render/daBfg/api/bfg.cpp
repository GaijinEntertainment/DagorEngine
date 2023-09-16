#include <render/daBfg/bfg.h>
#include <backend.h>

namespace dabfg
{

detail::NodeUid detail::register_node(const char *name, const char *node_source, DeclarationCallback &&declaration_callback)
{
  auto &tracker = NodeTracker::get();

  const auto nodeId = tracker.registry.knownNodeNames.addNameId(name);

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

void set_resolution(const char *typeName, IPoint2 value) { Backend::get().setResolution(typeName, value); }

void set_dynamic_resolution(const char *typeName, IPoint2 value) { Backend::get().setDynamicResolution(typeName, value); }

IPoint2 get_resolution(const char *typeName) { return Backend::get().getResolution(typeName); }

void fill_slot(NamedSlot slot, const char *res_name) { Backend::get().fillSlot(slot, res_name); }

void update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
{
  Backend::get().updateExternallyConsumedResourceSet(res_names);
}

void update_externally_consumed_resource_set(std::initializer_list<const char *> res_names)
{
  update_externally_consumed_resource_set(eastl::span<const char *const>(res_names.begin(), res_names.size()));
}

void mark_resource_externally_consumed(const char *res_name) { Backend::get().markResourceExternallyConsumed(res_name); }

void unmark_resource_externally_consumed(const char *res_name) { Backend::get().unmarkResourceExternallyConsumed(res_name); }

void update_external_state(ExternalState state) { Backend::get().updateExternalState(state); }

void set_multiplexing_extents(multiplexing::Extents extents) { Backend::get().setMultiplexingExtents(extents); }

void run_nodes() { Backend::get().runNodes(); }

void startup() { Backend::startup(); }

void shutdown() { Backend::shutdown(); }

} // namespace dabfg
