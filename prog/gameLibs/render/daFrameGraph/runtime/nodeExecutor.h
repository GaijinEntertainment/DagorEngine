// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/externalState.h>

#include <osApiWrappers/dag_stackHlp.h>

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/resourceAllocator.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/nodeStateDeltas.h>
#include <runtime/graphHierarchicalMarksParser.h>


namespace dafg
{

class NodeExecutor
{
public:
  NodeExecutor(ResourceAllocator &rs, intermediate::Graph &g, const intermediate::Mapping &m, InternalRegistry &reg,
    const NameResolver &res, ResourceProvider &rp) :
    resourceAllocator{rs}, graph{g}, mapping{m}, registry{reg}, nameResolver{res}, currentlyProvidedResources{rp}
  {}

  void execute(int prev_frame, int curr_frame, multiplexing::Extents multiplexing_extents, const BarrierScheduler::FrameEvents &events,
    const sd::NodeStateDeltas &state_deltas);

#if TIME_PROFILER_ENABLED
  void parseGraphMarks() { graphMarksParser.parseGraphMarks(registry, graph); }
#endif

  ExternalState externalState;

private:
  struct CallstackData;
  void gatherExternalResources(multiplexing::Extents extents);

  void processEvents(const BarrierScheduler::NodeEvents &events, int frame) const;
  void applyStateBeforeEvents(const sd::NodeStateDelta &state, int frame, int prev_frame) const;
  void applyState(const sd::NodeStateDelta &state, int frame, int prev_frame) const;
  void applyBindings(const sd::BindingsMap &bindings, int frame, int prev_frame) const;

  void bindShaderVar(int bind_idx, const intermediate::Binding &binding, int frame, int prev_frame) const;
  template <typename ProjectedType, auto bindSetter>
  void bindBlob(int bind_idx, const intermediate::Binding &binding, int frame) const;

  BaseTexture *getTexture(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const;
  BaseTexture *getTexture(intermediate::ResourceIndex res_idx, int frame) const;
  Sbuffer *getBuffer(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const;
  Sbuffer *getBuffer(intermediate::ResourceIndex res_idx, int frame) const;
  BlobView getBlobView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const;
  BlobView getBlobView(intermediate::ResourceIndex res_idx, int frame) const;

  template <class T>
  const T &getDynamicParameter(const intermediate::DynamicParameter &param, int frame) const;

  bool shouldSwitchVrsTex() const;

private:
  ResourceAllocator &resourceAllocator;

  const intermediate::Graph &graph;
  const intermediate::Mapping &mapping;

  InternalRegistry &registry;
  const NameResolver &nameResolver;
  ResourceProvider &currentlyProvidedResources;

#if TIME_PROFILER_ENABLED
  GraphHierarchicalMarksParser graphMarksParser;
#endif

  using ExternalResourceStorage = IdSparseIndexedMapping<intermediate::ResourceIndex, ExternalResource>;
  ExternalResourceStorage externalResources;
};

} // namespace dafg
