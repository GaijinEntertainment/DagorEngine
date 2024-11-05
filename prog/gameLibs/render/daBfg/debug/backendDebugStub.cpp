// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "backendDebug.h"

#include <runtime/runtime.h>


namespace dabfg
{

void update_graph_visualization(InternalRegistry &, const NameResolver &, const DependencyData &, const DebugPassColoration &,
  eastl::span<const NodeNameId>)
{}
void invalidate_graph_visualization() {}
void reset_texture_visualization() {}

void Runtime::dumpGraph(const eastl::string &) const {}

void update_resource_visualization(const InternalRegistry &, eastl::span<const NodeNameId>) {}
void debug_rec_resource_placement(ResNameId, int, int, int, int) {}
void debug_rec_resource_barrier(ResNameId, int, int, int, ResourceBarrier) {}

void validation_restart() {}
void validation_set_current_node(const InternalRegistry &, NodeNameId) {}
void validation_add_resource(const D3dResource *) {}
void validation_of_external_resources_duplication(
  const IdIndexedMapping<intermediate::ResourceIndex, eastl::optional<ExternalResource>> &,
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &)
{}
void validate_global_state(const InternalRegistry &, NodeNameId) {}
} // namespace dabfg

namespace dabfg
{
void mark_external_resource_for_validation(const D3dResource *) {}
} // namespace dabfg