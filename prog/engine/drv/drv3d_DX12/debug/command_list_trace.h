// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "pipeline_resource_reporter.h"
#include <driver.h>
#include <pipeline.h>

#include <debug/dag_log.h>
#include <EASTL/string.h>

#include "trace_status.h"
#include "trace_id.h"

namespace drv3d_dx12::debug
{
class CommandListTraceBase
{
public:
  static void printLegend()
  {
    logdbg("DX12: Commands prefixed with %s were executed completely, %s were launched but not completed, %s were recorded into the "
           "command buffer but not launched",
      prefix(TraceStatus::Completed), prefix(TraceStatus::Launched), prefix(TraceStatus::NotLaunched));
  }
};

class CommandListTrace : public CommandListTraceBase
{
  struct BeginEventTrace
  {
    eastl::string path;

    constexpr TraceID getTraceID() const { return null_trace_value; }
    void report(call_stack::Reporter &, TraceStatus status) const { logdbg("DX12: %s BeginEvent <%s>", prefix(status), path); }
  };

  struct EndEventTrace
  {
    eastl::string path;

    constexpr TraceID getTraceID() const { return null_trace_value; }
    void report(call_stack::Reporter &, TraceStatus status) const { logdbg("DX12: %s EndEvent <%s>", prefix(status), path); }
  };

  struct MarkerTrace
  {
    eastl::string path;

    constexpr TraceID getTraceID() const { return null_trace_value; }
    void report(call_stack::Reporter &, TraceStatus status) const { logdbg("DX12: %s <%s>", prefix(status), path); }
  };

  struct DrawTrace : call_stack::CommandData
  {
    TraceID traceID;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t primitiveCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DRAW", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DrawIndexedTrace : call_stack::CommandData
  {
    TraceID traceID;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t primitiveCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t firstInstance;
    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
    int32_t vertexOffset;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DRAW INDEXED", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DrawIndirectTrace : call_stack::CommandData
  {
    TraceID traceID;
    BufferResourceReferenceAndOffset indirectBuffer;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DRAW INDIRECT", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DrawIndexedIndirectTrace : call_stack::CommandData
  {
    TraceID traceID;
    BufferResourceReferenceAndOffset indirectBuffer;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DRAW INDEXED INDIRECT", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DispatchIndirectTrace : call_stack::CommandData
  {
    TraceID traceID;
    BufferResourceReferenceAndOffset indirectBuffer;
    PipelineStageStateBase computeStageState;
    ComputePipeline *pipeline;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DISPATCH INDIRECT", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipeline);
        report_resources(computeStageState, pipeline);
        reporter.report(*this);
      }
    }
  };

  struct DispatchTrace : call_stack::CommandData
  {
    TraceID traceID;
    PipelineStageStateBase computeStageState;
    ComputePipeline *pipeline;
    uint32_t x;
    uint32_t y;
    uint32_t z;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DISPATCH", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipeline);
        report_resources(computeStageState, pipeline);
        reporter.report(*this);
      }
    }
  };

  struct DispatchMeshTrace : call_stack::CommandData
  {
    TraceID traceID;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t x;
    uint32_t y;
    uint32_t z;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DISPATCH MESH", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DispatchMeshIndirectTrace : call_stack::CommandData
  {
    TraceID traceID;
    BufferResourceReferenceAndOffset argumentBuffer;
    BufferResourceReferenceAndOffset countBuffer;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t maxCount;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DISPATCH MESH INDIRECT", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct BlitTrace : call_stack::CommandData
  {
    TraceID traceID;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s BLIT", prefix(status));
      if (TraceStatus::Completed != status)
      {
        logdbg("DX12: Blit...");
        reporter.report(*this);
      }
    }
  };

#if D3D_HAS_RAY_TRACING
  struct DispatchRaysTrace : call_stack::CommandData
  {
    TraceID traceID;
    RayDispatchBasicParameters dispatchParameters;
    ResourceBindingTable resourceBindingTable;
    // UInt32ListRef::RangeType rootConstants;
    RayDispatchParameters rayDispatchParameters;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DISPATCH RAYS", prefix(status));
      if (TraceStatus::Completed != status)
      {
        reporter.report(*this);
      }
    }
  };

  struct DispatchRaysIndirect : call_stack::CommandData
  {
    TraceID traceID;
    RayDispatchBasicParameters dispatchParameters;
    ResourceBindingTable resourceBindingTable;
    RayDispatchIndirectParameters rayDispatchIndirectParameters;

    constexpr TraceID getTraceID() const { return traceID; }
    void report(call_stack::Reporter &reporter, TraceStatus status) const
    {
      logdbg("DX12: %s DISPATCH RAYS INDIRECT", prefix(status));
      if (TraceStatus::Completed != status)
      {
        reporter.report(*this);
      }
    }
  };
#endif

  struct NullTrace
  {
    constexpr TraceID getTraceID() const { return null_trace_value; }
    void report(call_stack::Reporter &, TraceStatus) const {}
  };

  using AnyTraceEvent =
    eastl::variant<NullTrace, BeginEventTrace, EndEventTrace, MarkerTrace, DrawTrace, DrawIndexedTrace, DrawIndirectTrace,
      DrawIndexedIndirectTrace, DispatchIndirectTrace, DispatchTrace, DispatchMeshTrace, DispatchMeshIndirectTrace, BlitTrace
#if D3D_HAS_RAY_TRACING
      ,
      DispatchRaysTrace, DispatchRaysIndirect
#endif
      >;

  dag::Vector<AnyTraceEvent> traces;

public:
  void beginTrace() { traces.clear(); }
  void endTrace() {}
  void beginEvent(eastl::span<const char>, eastl::span<const char> full_path)
  {
    traces.emplace_back(BeginEventTrace{.path = {begin(full_path), end(full_path)}});
  }
  void endEvent(eastl::span<const char> full_path) { traces.push_back(EndEventTrace{.path = {begin(full_path), end(full_path)}}); }
  void marker(eastl::span<const char> text) { traces.emplace_back(MarkerTrace{.path = {begin(text), end(text)}}); }
  void draw(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    traces.emplace_back(
      DrawTrace{debug_info, otd, vs, ps, &pipeline_base, &pipeline, count, instance_count, start, first_instance, topology});
  }
  void drawIndexed(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    traces.emplace_back(DrawIndexedTrace{
      debug_info, otd, vs, ps, &pipeline_base, &pipeline, count, instance_count, index_start, first_instance, topology, vertex_base});
  }
  void drawIndirect(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    traces.emplace_back(DrawIndirectTrace{debug_info, otd, buffer, vs, ps, &pipeline_base, &pipeline});
  }
  void drawIndexedIndirect(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    traces.emplace_back(DrawIndexedIndirectTrace{debug_info, otd, buffer, vs, ps, &pipeline_base, &pipeline});
  }
  void dispatchIndirect(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
  {
    traces.emplace_back(DispatchIndirectTrace{debug_info, otd, buffer, state, &pipeline});
  }
  void dispatch(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    traces.emplace_back(DispatchTrace{debug_info, otd, state, &pipeline, x, y, z});
  }
  void dispatchMesh(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    traces.emplace_back(DispatchMeshTrace{debug_info, otd, vs, ps, &pipeline_base, &pipeline, x, y, z});
  }
  void dispatchMeshIndirect(const TraceID &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
  {
    traces.emplace_back(DispatchMeshIndirectTrace{debug_info, otd, args, count, vs, ps, &pipeline_base, &pipeline, max_count});
  }
  void blit(const TraceID &otd, const call_stack::CommandData &debug_info) { traces.emplace_back(BlitTrace{debug_info, otd}); }
#if D3D_HAS_RAY_TRACING
  void dispatchRays(const TraceID &otd, const call_stack::CommandData &debug_info,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
  {
    traces.emplace_back(DispatchRaysTrace{debug_info, otd, dispatch_parameters, rbt, rdp});
  }
  void dispatchRaysIndirect(const TraceID &otd, const call_stack::CommandData &debug_info,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
  {
    traces.emplace_back(DispatchRaysIndirect{debug_info, otd, dispatch_parameters, rbt, rdip});
  }
#endif
  void reset() { traces.clear(); }

  /// this is a bit different, it will report entries one after 'from_point' and including 'until_point', which
  /// is not like the usual iterator / pointer like behavior
  template <typename T>
  void reportRange(TraceID from_point, TraceID until_point, call_stack::Reporter &reporter, T &&status_query) const
  {
    // find starting point
    auto it = eastl::find_if(traces.begin(), traces.end(),
      [from_point](auto &e) { return eastl::visit([from_point](auto &trace) { return trace.getTraceID() > from_point; }, e); });

    for (bool stillInRange = true; stillInRange && it != traces.end(); ++it)
    {
      stillInRange = eastl::visit(
        [until_point, &status_query, &reporter](auto &trace) {
          const auto id = trace.getTraceID();
          if (id <= until_point)
          {
            trace.report(reporter, status_query(id));
            return true;
          }
          return false;
        },
        *it);
    };
  }
  template <typename T>
  void report(call_stack::Reporter &reporter, T &&status_query)
  {
    TraceStatus status = TraceStatus::Completed;
    for (auto &&trace : traces)
    {
      eastl::visit(
        [&reporter, &status_query, &status](auto &trace) {
          const auto id = trace.getTraceID();
          if ((null_trace_value != id) && (invalid_trace_value != id))
          {
            status = status_query(id);
          }
          trace.report(reporter, status);
        },
        trace);
    }
  }
};
} // namespace drv3d_dx12::debug
