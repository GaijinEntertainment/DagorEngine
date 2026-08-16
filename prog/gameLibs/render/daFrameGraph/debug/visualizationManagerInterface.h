// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <frontend/nodeTracker.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>
#include <backend/nodeStateDeltas.h>

#include <drv/3d/dag_enhanced_barrier.h>


namespace dafg::visualization
{

#if DAGOR_DBGLEVEL == 0
class IVisualizationManager final //-V1052
#else
class IVisualizationManager
#endif
{
public:
  virtual ~IVisualizationManager() {}
  virtual void updateUserGraphVisualization(const IdIndexedFlags<NodeNameId, framemem_allocator> &) {}
  virtual void updateIRGraphVisualization() {}
  virtual void updateResourceVisualization() {}
  virtual void updateTextureVisualization() {}

  virtual void sendBlobData(NodeNameId, ResNameId, const BlobView &) {}

  virtual void clearResourcePlacements() {}
  virtual void clearResourceBarriers() {}
  virtual void recResourcePlacement(ResNameId, int, int, int, int, bool) {}
  virtual void recResourceBarrier(ResNameId, int, int, int, ResourceBarrier) {}
  virtual void recEnhancedBufferBarrier(ResNameId, int, int, int, const d3d::BufferBarrier &) {}
  virtual void recEnhancedTextureBarrier(ResNameId, int, int, int, const d3d::TextureBarrier &) {}
};

inline eastl::unique_ptr<IVisualizationManager> make_dummy_manager() { return eastl::make_unique<IVisualizationManager>(); }

eastl::unique_ptr<IVisualizationManager> make_real_manager(InternalRegistry &int_registry, const NameResolver &name_resolver,
  const DependencyData &dep_data, const intermediate::Graph &ir_graph, const PassColoring &coloring,
  const sd::NodeStateDeltas &state_deltas);

} // namespace dafg::visualization