// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <util/dag_convar.h>

#include <render/daFrameGraph/externalResources.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>

namespace dafg
{

struct InternalRegistry;
class NameResolver;
struct DependencyData;
class NodeTracker;
using DebugPassColoration = IdIndexedMapping<NodeNameId, PassColor>;

extern bool debug_graph_generation;
bool should_update_visualization();
void reset_texture_visualization();

void debug_clear_resource_placements();
void debug_clear_resource_barriers();
void debug_rec_resource_placement(ResNameId id, int frame, int heap, int offset, int size);
void debug_rec_resource_barrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier);

void validation_restart();
void validation_set_current_node(const InternalRegistry &registry, NodeNameId node);
void validation_add_resource(const D3dResource *res);
void validation_of_external_resources_duplication(
  const IdIndexedMapping<intermediate::ResourceIndex, eastl::optional<ExternalResource>> &resources,
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &resourceNames);
void validate_global_state(const InternalRegistry &registry, NodeNameId node);

} // namespace dafg
