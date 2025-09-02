// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "backendDebug.h"

namespace dafg
{

bool debug_graph_generation = false;
bool should_update_visualization() { return false; }
void reset_texture_visualization() {}

void debug_clear_resource_placements() {}
void debug_clear_resource_barriers() {}
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

} // namespace dafg

namespace dafg
{
void mark_external_resource_for_validation(const D3dResource *) {}
} // namespace dafg