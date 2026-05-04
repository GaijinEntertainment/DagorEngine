// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "visualizationManager.h"

namespace dafg::visualization
{

eastl::unique_ptr<IVisualizationManager> make_real_manager(InternalRegistry &intRegistry, const NameResolver &nameResolver,
  const DependencyData &depData, const intermediate::Graph &irGraph, const PassColoring &coloring)
{
  return eastl::make_unique<VisManager>(intRegistry, nameResolver, depData, irGraph, coloring);
}

VisManager::VisManager(InternalRegistry &intRegistry, const NameResolver &nameResolver, const DependencyData &depData,
  const intermediate::Graph &irGraph, const PassColoring &coloring) :
  userGraphVisualizer{intRegistry, nameResolver, depData, irGraph, coloring},
  irGraphVisualizer{irGraph, coloring},
  resVisualizer{intRegistry, nameResolver, irGraph}
{}

VisManager::~VisManager() { close_visualization_texture(); }

void VisManager::updateUserGraphVisualization() { userGraphVisualizer.updateVisualization(); }

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