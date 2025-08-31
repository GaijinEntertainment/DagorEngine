// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerInterface.h>

#include <debug/userGraphVisualizer.h>
#include <debug/irGraphVisualizer.h>
#include <debug/resourceVisualizer.h>
#include <debug/textureVisualizer.h>


namespace dafg::visualization
{

// TODO:
// make separate stage for updating visualization
// add mark that particular visualization is dirty

class FGVisualizer final : public IVisualizer
{
public:
  FGVisualizer(InternalRegistry &intRegistry, const NameResolver &nameResolver, const DependencyData &depData,
    const intermediate::Graph &irGraph, const PassColoring &coloring);
  ~FGVisualizer() override = default;

  void updateUserGraphVisualisation() override;
  void updateIRGraphVisualisation() override;
  void updateResourceVisualisation() override;
  void updateTextureVisualisation() override;

  void clearResourcePlacements() override;
  void clearResourceBarriers() override;
  void recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size) override;
  void recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier) override;

private:
  UserGraphVisualizer userGraphVisualizer;
  IRGraphVisualizer irGraphVisualizer;
  ResourseVisualizer resVisualizer;
  TextureVisualizer texVisualizer;
};

} // namespace dafg::visualization