// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDumpCommon.h"

namespace dafg::jsondump
{

extern Writer w;


static const char *activation_action_name(ResourceActivationAction action)
{
  switch (action)
  {
    case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION: return "REWRITE_AS_COPY_DESTINATION";
    case ResourceActivationAction::REWRITE_AS_UAV: return "REWRITE_AS_UAV";
    case ResourceActivationAction::REWRITE_AS_RTV_DSV: return "REWRITE_AS_RTV_DSV";
    case ResourceActivationAction::CLEAR_F_AS_UAV: return "CLEAR_F_AS_UAV";
    case ResourceActivationAction::CLEAR_I_AS_UAV: return "CLEAR_I_AS_UAV";
    case ResourceActivationAction::CLEAR_AS_RTV_DSV: return "CLEAR_AS_RTV_DSV";
    case ResourceActivationAction::DISCARD_AS_UAV: return "DISCARD_AS_UAV";
    case ResourceActivationAction::DISCARD_AS_RTV_DSV: return "DISCARD_AS_RTV_DSV";
  }
  return "<invalid>";
}

static const char *barrier_kind_name(BarrierScheduler::Event::BarrierKind kind)
{
  switch (kind)
  {
    case BarrierScheduler::Event::BarrierKind::None: return "None";
    case BarrierScheduler::Event::BarrierKind::Release: return "Release";
    case BarrierScheduler::Event::BarrierKind::Transition: return "Transition";
    case BarrierScheduler::Event::BarrierKind::FirstUse: return "FirstUse";
  }
  return "<invalid>";
}

static void print_pipeline_stages(const char *key, const d3d::PipelineSyncOperation &op)
{
  w.Key(key);
  OBJECT_SCOPED

  w.Key("src");
  write_string(w, detail::decode_flag_names(d3d::PipelineStageFlag(op.src.asInteger())));

  w.Key("dst");
  write_string(w, detail::decode_flag_names(d3d::PipelineStageFlag(op.dst.asInteger())));
}

static void print_memory_access(const char *key, const d3d::MemorySyncOperation &op)
{
  w.Key(key);
  OBJECT_SCOPED

  w.Key("src");
  write_string(w, detail::decode_flag_names(d3d::AccessFlag(op.src.asInteger())));

  w.Key("dst");
  write_string(w, detail::decode_flag_names(d3d::AccessFlag(op.dst.asInteger())));
}

static void print_range(const char *key, const d3d::Range &range)
{
  w.Key(key);
  OBJECT_SCOPED

  w.Key("first");
  w.Uint(range.first);

  w.Key("count");
  w.Uint(range.count);
}

static void print_clear_value(const eastl::variant<ResourceClearValue, intermediate::DynamicParameter> &value,
  const intermediate::Graph &graph)
{
  w.Key("clearValue");
  OBJECT_SCOPED

  w.Key("variant");
  eastl::visit(
    [&](const auto &clear_value) {
      using T = eastl::decay_t<decltype(clear_value)>;

      if constexpr (std::is_same_v<T, ResourceClearValue>)
      {
        w.String("ResourceClearValue");

        w.Key("asUint");
        ARRAY_SCOPED
        for (const auto v : clear_value.asUint)
          w.Uint(v);
      }
      else
      {
        w.String("DynamicParameter");

        w.Key("resource");
        w.String(get_name(graph, clear_value.resource));
      }
    },
    value);
}

static void print_event(const BarrierScheduler::Event &evt, const intermediate::Graph &graph)
{
  using Event = BarrierScheduler::Event;

  OBJECT_SCOPED

  w.Key("resource");
  w.String(get_name(graph, evt.resource));

  w.Key("frameResourceProducedOn");
  w.Uint(evt.frameResourceProducedOn);

  if (graph.resources.isMapped(evt.resource))
  {
    const intermediate::Resource &res = graph.resources[evt.resource];

    w.Key("resType");
    write_string(w, detail::get_enum_name(res.getResType()));

    // The executor does not run events on the back buffer
    if (res.isDriverDeferredTexture())
    {
      w.Key("skippedDriverDeferred");
      w.Bool(true);
    }
  }

  w.Key("type");
  eastl::visit(
    [&](const auto &data) {
      using T = eastl::decay_t<decltype(data)>;

      if constexpr (std::is_same_v<T, Event::CpuActivation>)
      {
        w.String("CpuActivation");
      }
      else if constexpr (std::is_same_v<T, Event::CpuDeactivation>)
      {
        w.String("CpuDeactivation");
      }
      else if constexpr (std::is_same_v<T, Event::Deactivation>)
      {
        w.String("Deactivation");
      }
      else if constexpr (std::is_same_v<T, Event::Activation>)
      {
        w.String("Activation");

        w.Key("action");
        w.String(activation_action_name(data.action));

        print_clear_value(data.clearValue, graph);
      }
      else if constexpr (std::is_same_v<T, Event::Barrier>)
      {
        w.String("Barrier");

        w.Key("barrier");
        write_string(w, detail::decode_flag_names(data.barrier));

        // NodeExecutor drops split barrier begin on Vulkan
        w.Key("splitBegin");
        w.Bool((data.barrier & RB_FLAG_SPLIT_BARRIER_BEGIN) != 0);
      }
      else if constexpr (std::is_same_v<T, Event::EnhancedBufferBarrier>)
      {
        w.String("EnhancedBufferBarrier");

        w.Key("kind");
        w.String(barrier_kind_name(data.kind));

        print_pipeline_stages("pipelineSync", data.barrier.pipelineSync);
        print_memory_access("memorySync", data.barrier.memorySync);
      }
      else if constexpr (std::is_same_v<T, Event::EnhancedTextureBarrier>)
      {
        w.String("EnhancedTextureBarrier");

        w.Key("kind");
        w.String(barrier_kind_name(data.kind));

        print_pipeline_stages("pipelineSync", data.barrier.pipelineSync);
        print_memory_access("memorySync", data.barrier.memorySync);

        w.Key("layoutTransition");
        {
          OBJECT_SCOPED

          w.Key("src");
          write_string(w, detail::get_enum_name(data.barrier.layoutTransition.src));

          w.Key("dst");
          write_string(w, detail::get_enum_name(data.barrier.layoutTransition.dst));
        }

        w.Key("subresources");
        {
          OBJECT_SCOPED
          print_range("mips", data.barrier.subresources.mips);
          print_range("layers", data.barrier.subresources.layers);
          print_range("planes", data.barrier.subresources.planes);
        }
      }
      else
      {
        w.String("<unknown>");
      }
    },
    evt.data);
}


void dump_events_to_json(const intermediate::Graph &graph, const BarrierScheduler::EventsCollection &events)
{
  w.Key("resourceEvents");
  OBJECT_SCOPED

  for (uint32_t frame = 0; frame < events.size(); ++frame)
  {
    w.Key(String(0, "frame%u", frame));
    OBJECT_SCOPED

    const auto &frameEvents = events[frame];
    for (const auto &[nodeIdx, nodeEvents] : frameEvents.enumerate())
    {
      if (nodeEvents.empty())
        continue;

      // The last bucket has no node: it holds the end-of-frame events
      const bool endOfFrame = !graph.nodes.isMapped(nodeIdx);
      w.Key(endOfFrame ? "<endOfFrame>" : get_name(graph, nodeIdx));
      ARRAY_SCOPED
      for (const auto &evt : nodeEvents)
        print_event(evt, graph);
    }
  }
}

} // namespace dafg::jsondump
