// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>

#include <render/daFrameGraph/detail/blob.h>
#include <dag/dag_vectorSet.h>
#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>


namespace dafg
{

class BarrierScheduler
{
public:
  struct Event
  {
    struct Activation
    {
      ResourceActivationAction action;
      eastl::variant<ResourceClearValue, intermediate::DynamicParameter> clearValue;
    };

    struct CpuActivation
    {
      intermediate::CtorFunc func;
    };

    struct CpuDeactivation
    {
      intermediate::DtorFunc func;
    };

    struct Barrier
    {
      ResourceBarrier barrier;
    };

    struct Deactivation
    {};

    intermediate::ResourceIndex resource;
    uint32_t frameResourceProducedOn;

    // NOTE: at each timepoint events are executed in order of their
    // type, from last one to first one. Meaning deactivations happen
    // before barriers, barriers before activations.
    using Payload = eastl::variant<CpuActivation, Activation, Barrier, Deactivation, CpuDeactivation>;
    Payload data;
  };

  static constexpr int SCHEDULE_FRAME_WINDOW = 2; // even and odd frames

  using NodeEvents = dag::Vector<Event>;
  using FrameEvents = IdIndexedMapping<intermediate::NodeIndex, NodeEvents>;
  using EventsCollection = eastl::array<FrameEvents, SCHEDULE_FRAME_WINDOW>;

  using ResourceLifetimesChanged = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  ResourceLifetimesChanged scheduleEvents(EventsCollection &node_events, const intermediate::Graph &graph,
    const PassColoring &pass_coloring, const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
    const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed);

private:
  using GracePoints = dag::VectorSet<uint32_t, eastl::less<uint32_t>, framemem_allocator>;
  using DirtyResources = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  GracePoints computeGracePoints(const intermediate::Graph &graph, const PassColoring &pass_coloring) const;

  DirtyResources computeDirtyResources(const intermediate::Graph &graph,
    const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
    const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed, const GracePoints &grace_points);

  void updateDirtyResourceEvents(const intermediate::Graph &graph, const DirtyResources &dirty_resources,
    const GracePoints &grace_points);

  struct PlacedEvent
  {
    Event event;
    uint32_t nodeTimepoint;
    uint32_t eventFrame;
  };

  // Persistent per-resource event cache (default allocator, survives across frames)
  IdIndexedMapping<intermediate::ResourceIndex, dag::Vector<PlacedEvent>> cachedResourceEvents;

  // Previous grace points for change detection
  dag::Vector<uint32_t> prevGracePoints;
};

} // namespace dafg
