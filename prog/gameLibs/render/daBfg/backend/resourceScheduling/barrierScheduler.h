// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>

#include <render/daBfg/detail/blob.h>
#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>


namespace dabfg
{

class BarrierScheduler
{
public:
  struct Event
  {
    struct Activation
    {
      ResourceActivationAction action;
      ResourceClearValue clearValue;
    };

    struct CpuActivation
    {
      TypeErasedCall func;
    };

    struct CpuDeactivation
    {
      TypeErasedCall func;
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

  using NodeEventsRef = eastl::span<const Event>;
  using FrameEventsRef = eastl::span<const NodeEventsRef>;
  using EventsCollectionRef = eastl::array<FrameEventsRef, SCHEDULE_FRAME_WINDOW>;

  // Returned spans are valid until the method gets called again
  EventsCollectionRef scheduleEvents(const intermediate::Graph &graph, const PassColoring &pass_coloring);

private:
  dag::Vector<Event> eventStorage;
  eastl::array<dag::Vector<eastl::span<const Event>>, SCHEDULE_FRAME_WINDOW> eventRefStorage;
};

} // namespace dabfg
