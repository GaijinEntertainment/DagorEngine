// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualizationManagerInterface.h>

#include <debug/visualization/userGraphVisualizer.h>
#include <debug/visualization/irGraphVisualizer.h>
#include <debug/visualization/resourceVisualizer.h>
#include <debug/visualization/textureVisualizer.h>


namespace dafg::visualization
{

class VisManager final : public IVisualizationManager
{
public:
  VisManager(InternalRegistry &intRegistry, const NameResolver &nameResolver, const DependencyData &depData,
    const intermediate::Graph &irGraph, const PassColoring &coloring);
  ~VisManager() override;

  void updateUserGraphVisualization() override;
  void updateIRGraphVisualization() override;
  void updateResourceVisualization() override;
  void updateTextureVisualization() override;

  void clearResourcePlacements() override;
  void clearResourceBarriers() override;
  void recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size, bool is_cpu) override;
  void recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier) override;

private:
  usergraph::Visualizer userGraphVisualizer;

  IRGraphVisualizer irGraphVisualizer;
  ResourseVisualizer resVisualizer;
  TextureVisualizer texVisualizer;
};

} // namespace dafg::visualization