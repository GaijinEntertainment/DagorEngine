// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/detail/nodeNameId.h>
#include <render/daBfg/multiplexing.h>
#include <render/daBfg/externalState.h>

#include <osApiWrappers/dag_stackHlp.h>

#include <frontend/internalRegistry.h>
#include <frontend/nameResolver.h>
#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/resourceScheduler.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/nodeStateDeltas.h>


namespace dabfg
{

class NodeExecutor
{
public:
  NodeExecutor(ResourceScheduler &rs, intermediate::Graph &g, const intermediate::Mapping &m, InternalRegistry &reg,
    const NameResolver &res, ResourceProvider &rp) :
    resourceScheduler{rs}, graph{g}, mapping{m}, registry{reg}, nameResolver{res}, currentlyProvidedResources{rp}
  {}

  void execute(int prev_frame, int curr_frame, multiplexing::Extents multiplexing_extents,
    const BarrierScheduler::FrameEventsRef &events, const sd::NodeStateDeltas &state_deltas);

  ExternalState externalState;

private:
  static stackhelp::ext::CallStackResolverCallbackAndSizePair captureCallstackData(stackhelp::CallStackInfo stack, void *context);
  static unsigned resolveCallStackData(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack);

  void gatherExternalResources(multiplexing::Extents extents);

  void processEvents(BarrierScheduler::NodeEventsRef events) const;
  void applyState(const sd::NodeStateDelta &state, int frame, int prev_frame) const;
  void applyBindings(const intermediate::BindingsMap &bindings, int frame, int prev_frame) const;

  void bindShaderVar(int bind_idx, const intermediate::Binding &binding, int frame, int prev_frame) const;
  template <typename ProjectedType, auto bindSetter>
  void bindBlob(int bind_idx, const intermediate::Binding &binding, int frame) const;

  ManagedTexView getManagedTexView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const;
  ManagedTexView getManagedTexView(intermediate::ResourceIndex res_idx, int frame) const;
  ManagedBufView getManagedBufView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const;
  ManagedBufView getManagedBufView(intermediate::ResourceIndex res_idx, int frame) const;
  BlobView getBlobView(ResNameId res_name_id, int frame, intermediate::MultiplexingIndex multi_index) const;
  BlobView getBlobView(intermediate::ResourceIndex res_idx, int frame) const;

  template <class T>
  const T &getDynamicPassParameter(const intermediate::DynamicPassParameter &param, int frame) const;

private:
  ResourceScheduler &resourceScheduler;

  const intermediate::Graph &graph;
  const intermediate::Mapping &mapping;

  InternalRegistry &registry;
  const NameResolver &nameResolver;
  ResourceProvider &currentlyProvidedResources;

  using ExternalResourceStorage = IdIndexedMapping<intermediate::ResourceIndex, eastl::optional<ExternalResource>>;
  ExternalResourceStorage externalResources;
};

} // namespace dabfg
