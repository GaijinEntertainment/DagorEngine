// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>

#include <render/daFrameGraph/detail/blob.h>
#include <dag/dag_vectorSet.h>
#include <EASTL/optional.h>
#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>
#include <backend/resourceScheduling/resourceLifetimes.h>


namespace dafg
{

class BarrierScheduler
{
public:
  struct Event
  {
    enum class BarrierKind : uint8_t
    {
      None,
      Release,
      Transition,
      FirstUse
    };

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

    struct EnhancedBufferBarrier
    {
      d3d::BufferBarrier barrier;
      BarrierKind kind;
    };

    struct EnhancedTextureBarrier
    {
      d3d::TextureBarrier barrier;
      BarrierKind kind;
    };

    struct Deactivation
    {};

    intermediate::ResourceIndex resource;
    uint32_t frameResourceProducedOn;

    using Payload =
      eastl::variant<CpuActivation, Activation, Barrier, EnhancedBufferBarrier, EnhancedTextureBarrier, Deactivation, CpuDeactivation>;
    Payload data;
  };

  static uint32_t execution_rank(const Event &event);

  static Event::BarrierKind barrier_kind(const Event &event);

  using NodeEvents = dag::Vector<Event>;
  using FrameEvents = IdIndexedMapping<intermediate::NodeIndex, NodeEvents>;
  using EventsCollection = eastl::array<FrameEvents, SCHEDULE_FRAME_WINDOW>;

  struct UsageSyncStages
  {
    d3d::PipelineStageFlags firstUse;
    d3d::PipelineStageFlags lastUse;
  };
  using UsageSyncStagesMapping = IdIndexedMapping<intermediate::ResourceIndex, UsageSyncStages>;

  void scheduleEvents(EventsCollection &node_events, const intermediate::Graph &graph, const ResourceLifetimes &lifetimes,
    const PassColoring &pass_coloring, const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
    const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed,
    const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &lifetimes_changed);

  const UsageSyncStagesMapping &usageSyncStages() const { return usageSyncStagesPerResource; }

  void setAliasSyncStages(EventsCollection &node_events, intermediate::ResourceIndex res_idx, d3d::PipelineStageFlags sync_before,
    d3d::PipelineStageFlags sync_after);

  void resetIncrementalState() { *this = BarrierScheduler(); }

private:
  using DirtyResources = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  void computeUsageSyncStages(const intermediate::Graph &graph);

  DirtyResources computeDirtyResources(const intermediate::Graph &graph,
    const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
    const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed,
    const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &lifetimes_changed, const GracePoints &grace_points);

  void updateDirtyResourceEvents(const intermediate::Graph &graph, const ResourceLifetimes &lifetimes,
    const DirtyResources &dirty_resources, const GracePoints &grace_points);

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

  // First and last use stages of every untracked resource.
  UsageSyncStagesMapping usageSyncStagesPerResource;
};

} // namespace dafg
