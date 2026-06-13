// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "visualizationManager.h"

namespace dafg::visualization
{

eastl::unique_ptr<IVisualizationManager> make_real_manager(InternalRegistry &int_registry, const NameResolver &name_resolver,
  const DependencyData &dep_data, const intermediate::Graph &ir_graph, const PassColoring &coloring,
  const sd::NodeStateDeltas &state_deltas)
{
  return eastl::make_unique<VisManager>(int_registry, name_resolver, dep_data, ir_graph, coloring, state_deltas);
}

VisManager::VisManager(InternalRegistry &int_registry, const NameResolver &name_resolver, const DependencyData &dep_data,
  const intermediate::Graph &ir_graph, const PassColoring &coloring, const sd::NodeStateDeltas &state_deltas) :
  userGraphVisualizer{int_registry, dep_data, ir_graph, coloring},
  irGraphVisualizer{int_registry, ir_graph, coloring, state_deltas},
  resVisualizer{int_registry, name_resolver, ir_graph}
{}

VisManager::~VisManager() { close_visualization_texture(); }

void VisManager::updateUserGraphVisualization(const IdIndexedFlags<NodeNameId, framemem_allocator> &nodes_changed)
{
  userGraphVisualizer.updateVisualization(nodes_changed);
}

void VisManager::updateIRGraphVisualization() { irGraphVisualizer.updateVisualization(); }

void VisManager::updateResourceVisualization() { resVisualizer.updateVisualization(); }

void VisManager::updateTextureVisualization() {}


void VisManager::clearResourcePlacements() { resVisualizer.clearResourcePlacements(); }

void VisManager::clearResourceBarriers() { resVisualizer.clearResourceBarriers(); }

void VisManager::recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size, bool is_cpu)
{
  resVisualizer.recResourcePlacement(id, frame, heap, offset, size, is_cpu);
}

void VisManager::recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier)
{
  resVisualizer.recResourceBarrier(res_id, res_frame, exec_time, exec_frame, barrier);
}

} // namespace dafg::visualization