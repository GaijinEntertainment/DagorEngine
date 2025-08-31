// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <frontend/nodeTracker.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>


namespace dafg::visualization
{

#if DAGOR_DBGLEVEL == 0
class IVisualizer final //-V1052
#else
class IVisualizer
#endif
{
public:
  virtual ~IVisualizer() {}
  virtual void updateUserGraphVisualisation() {}
  virtual void updateIRGraphVisualisation() {}
  virtual void updateResourceVisualisation() {}
  virtual void updateTextureVisualisation() {}

  virtual void clearResourcePlacements() {}
  virtual void clearResourceBarriers() {}
  virtual void recResourcePlacement(ResNameId, int, int, int, int) {}
  virtual void recResourceBarrier(ResNameId, int, int, int, ResourceBarrier) {}
};

inline eastl::unique_ptr<IVisualizer> make_dummy_visualizer() { return eastl::make_unique<IVisualizer>(); }

eastl::unique_ptr<IVisualizer> make_real_visualizer(InternalRegistry &intRegistry, const NameResolver &nameResolver,
  const DependencyData &depData, const intermediate::Graph &irGraph, const PassColoring &coloring);

} // namespace dafg::visualization