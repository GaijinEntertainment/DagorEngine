// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "pipeline_resource_reporter.h"
#include <driver.h>
#include <pipeline.h>

#include <debug/dag_log.h>
#include <EASTL/string.h>


inline const char *to_string(D3D12_AUTO_BREADCRUMB_OP op)
{
  switch (op)
  {
    default: return "<invalid>";
    case D3D12_AUTO_BREADCRUMB_OP_SETMARKER: return "SETMARKER";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT: return "BEGINEVENT";
    case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT: return "ENDEVENT";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED: return "DRAWINSTANCED";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED: return "DRAWINDEXEDINSTANCED";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT: return "EXECUTEINDIRECT";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCH: return "DISPATCH";
    case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION: return "COPYBUFFERREGION";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION: return "COPYTEXTUREREGION";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE: return "COPYRESOURCE";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTILES: return "COPYTILES";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE: return "RESOLVESUBRESOURCE";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW: return "CLEARRENDERTARGETVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW: return "CLEARUNORDEREDACCESSVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW: return "CLEARDEPTHSTENCILVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER: return "RESOURCEBARRIER";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE: return "EXECUTEBUNDLE";
    case D3D12_AUTO_BREADCRUMB_OP_PRESENT: return "PRESENT";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA: return "RESOLVEQUERYDATA";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION: return "BEGINSUBMISSION";
    case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION: return "ENDSUBMISSION";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME: return "DECODEFRAME";
    case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES: return "PROCESSFRAMES";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT: return "ATOMICCOPYBUFFERUINT";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64: return "ATOMICCOPYBUFFERUINT64";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION: return "RESOLVESUBRESOURCEREGION";
    case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE: return "WRITEBUFFERIMMEDIATE";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1: return "DECODEFRAME1";
    case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION: return "SETPROTECTEDRESOURCESESSION";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2: return "DECODEFRAME2";
    case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1: return "PROCESSFRAMES1";
    case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE: return "BUILDRAYTRACINGACCELERATIONSTRUCTURE";
    case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
      return "EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE: return "COPYRAYTRACINGACCELERATIONSTRUCTURE";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS: return "DISPATCHRAYS";
    case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND: return "INITIALIZEMETACOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND: return "EXECUTEMETACOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION: return "ESTIMATEMOTION";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP: return "RESOLVEMOTIONVECTORHEAP";
    case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1: return "SETPIPELINESTATE1";
    case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND: return "INITIALIZEEXTENSIONCOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND: return "EXECUTEEXTENSIONCOMMAND";
  }
}

namespace drv3d_dx12::debug
{
class CommandListTraceBase
{
public:
  enum class TraceCompareResult
  {
    Matching,
    Mismatching,
    IgnoreForProgress
  };

  enum class CompletionStatus
  {
    Completed,
    LastCompleted,
    NotCompleted
  };

  static const char *prefix(CompletionStatus status)
  {
    switch (status)
    {
      default: return "[XX]";
      case CompletionStatus::Completed: return "[##]";
      case CompletionStatus::LastCompleted: return "[!!]";
      case CompletionStatus::NotCompleted: return "[??]";
    }
  }

  static void printLegend()
  {
    logdbg("DX12: Entry with %s where executed, %s last command executed, %s command that was not completed",
      prefix(CompletionStatus::Completed), prefix(CompletionStatus::LastCompleted), prefix(CompletionStatus::NotCompleted));
  }
};

template <typename T>
class CommandListTrace : public CommandListTraceBase
{
  using CallStackData = call_stack::CommandData;
  using EventTraceBase = typename T::EventTraceBase;
  using OperationTraceBase = typename T::OperationTraceBase;
  using EventTraceData = typename T::EventTraceData;
  using OperationTraceData = typename T::OperationTraceData;

  struct BeginEventTrace : EventTraceBase
  {
    eastl::string path;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT;

    void report(call_stack::Reporter &, CompletionStatus status) const { logdbg("DX12: %s BeginEvent <%s>", prefix(status), path); }
  };

  struct EndEventTrace : EventTraceBase
  {
    eastl::string path;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_ENDEVENT;

    void report(call_stack::Reporter &, CompletionStatus status) const { logdbg("DX12: %s EndEvent <%s>", prefix(status), path); }
  };

  struct MarkerTrace : EventTraceBase
  {
    eastl::string path;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_SETMARKER;

    void report(call_stack::Reporter &, CompletionStatus status) const { logdbg("DX12: %s <%s>", prefix(status), path); }
  };

  struct DrawTrace : OperationTraceBase, CallStackData
  {
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t primitiveCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DrawIndexedTrace : OperationTraceBase, CallStackData
  {
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

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DrawIndirectTrace : OperationTraceBase, CallStackData
  {
    BufferResourceReferenceAndOffset indirectBuffer;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DrawIndexedIndirectTrace : OperationTraceBase, CallStackData
  {
    BufferResourceReferenceAndOffset indirectBuffer;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DispatchIndirectTrace : OperationTraceBase, CallStackData
  {
    BufferResourceReferenceAndOffset indirectBuffer;
    PipelineStageStateBase computeStageState;
    ComputePipeline *pipeline;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipeline);
        report_resources(computeStageState, pipeline);
        reporter.report(*this);
      }
    }
  };

  struct DispatchTrace : OperationTraceBase, CallStackData
  {
    PipelineStageStateBase computeStageState;
    ComputePipeline *pipeline;
    uint32_t x;
    uint32_t y;
    uint32_t z;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_DISPATCH;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipeline);
        report_resources(computeStageState, pipeline);
        reporter.report(*this);
      }
    }
  };

  struct DispatchMeshTrace : OperationTraceBase, CallStackData
  {
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t x;
    uint32_t y;
    uint32_t z;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct DispatchMeshIndirectTrace : OperationTraceBase, CallStackData
  {
    BufferResourceReferenceAndOffset argumentBuffer;
    BufferResourceReferenceAndOffset countBuffer;
    PipelineStageStateBase vertexStageState;
    PipelineStageStateBase pixelStageState;
    BasePipeline *pipelineBase;
    PipelineVariant *pipelineVariant;
    uint32_t maxCount;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: ...Pipeline %p...", pipelineVariant);
        report_resources(vertexStageState, pixelStageState, pipelineBase);
        reporter.report(*this);
      }
    }
  };

  struct BlitTrace : OperationTraceBase, CallStackData
  {
    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        logdbg("DX12: Blit...");
        reporter.report(*this);
      }
    }
  };

#if D3D_HAS_RAY_TRACING
  struct DispatchRaysTrace : OperationTraceBase, CallStackData
  {
    RayDispatchBasicParameters dispatchParameters;
    ResourceBindingTable resourceBindingTable;
    // UInt32ListRef::RangeType rootConstants;
    RayDispatchParameters rayDispatchParameters;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        reporter.report(*this);
      }
    }
  };

  struct DispatchRaysIndirect : OperationTraceBase, CallStackData
  {
    RayDispatchBasicParameters dispatchParameters;
    ResourceBindingTable resourceBindingTable;
    RayDispatchIndirectParameters rayDispatchIndirectParameters;

    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT;

    void report(call_stack::Reporter &reporter, CompletionStatus status) const
    {
      logdbg("DX12: %s %s", prefix(status), to_string(OpCode));
      if (CompletionStatus::NotCompleted == status)
      {
        reporter.report(*this);
      }
    }
  };
#endif

  struct NullTrace
  {
    static constexpr D3D12_AUTO_BREADCRUMB_OP OpCode = static_cast<D3D12_AUTO_BREADCRUMB_OP>(~0ul); // -V1016
    void report(call_stack::Reporter &, CompletionStatus) const {}
    template <typename U>
    TraceCompareResult compare(const U &) const
    {
      return TraceCompareResult::IgnoreForProgress;
    }
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

  static bool isTracedOp(D3D12_AUTO_BREADCRUMB_OP op)
  {
    switch (op)
    {
      default: return false;
      case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
      case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
      case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
      case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
      case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
      case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
      case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
      case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
      case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS: return true;
    }
  }

  static bool isEventOp(D3D12_AUTO_BREADCRUMB_OP op)
  {
    switch (op)
    {
      default: return false;
      case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
      case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
      case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT: return true;
    }
  }

  uint32_t reportEvents(uint32_t index, call_stack::Reporter &reporter, CompletionStatus status) const
  {
    for (; index < traces.size(); ++index)
    {
      bool wasEvent = eastl::visit(
        [&reporter, status](auto &trace) -> bool {
          if (!isEventOp(trace.OpCode))
          {
            return false;
          }
          trace.report(reporter, status);
          return true;
        },
        traces[index]);
      if (!wasEvent)
      {
        break;
      }
    }
    return index;
  }

  uint32_t reportOps(uint32_t index, D3D12_AUTO_BREADCRUMB_OP until_op, call_stack::Reporter &reporter, CompletionStatus status) const
  {
    for (; index < traces.size(); ++index)
    {
      bool keepGoing = eastl::visit(
        [&reporter, until_op, status](auto &trace) -> bool {
          if (isEventOp(trace.OpCode))
          {
            logwarn("DX12: Unexpected event trace encountered!");
            return true;
          }
          else if (trace.OpCode != until_op)
          {
            logwarn("DX12: Expected next trace op to be %s but report for %s was requested", to_string(trace.OpCode),
              to_string(until_op));
            return true;
          }
          trace.report(reporter, status);
          return false;
        },
        traces[index]);
      if (!keepGoing)
      {
        break;
      }
    }
    return index;
  }

  // This tries to find the last entry that is not a event entry that matches returns true for continueUntil
  template <typename U>
  uint32_t findUntilPosition(uint32_t index, const U &data) const
  {
    uint32_t lastEndPoint = index;
    for (; index < traces.size(); ++index)
    {
      auto result = eastl::visit([&data](auto &trace) -> TraceCompareResult { return trace.compare(data); }, traces[index]);

      if (TraceCompareResult::Matching == result)
      {
        break;
      }
      if (TraceCompareResult::Mismatching == result)
      {
        lastEndPoint = index;
      }
    }
    return lastEndPoint;
  }

  template <typename U>
  uint32_t findAfterPosition(uint32_t index, const U &data) const
  {
    uint32_t lastEndPoint = index;
    for (; index < traces.size(); ++index)
    {
      auto result = eastl::visit([&data](auto &trace) -> TraceCompareResult { return trace.compare(data); }, traces[index]);

      if (TraceCompareResult::Mismatching == result)
      {
        break;
      }
      if (TraceCompareResult::Matching == result)
      {
        lastEndPoint = index;
      }
    }
    return lastEndPoint;
  }

public:
  void beginTrace() { traces.clear(); }
  void endTrace() {}
  void beginEvent(const EventTraceData &etd, eastl::span<const char>, eastl::span<const char> full_path)
  {
    traces.emplace_back(BeginEventTrace{etd, {begin(full_path), end(full_path)}});
  }
  void endEvent(const EventTraceData &etd, eastl::span<const char> full_path)
  {
    traces.push_back(EndEventTrace{etd, {begin(full_path), end(full_path)}});
  }
  void marker(const EventTraceData &etd, eastl::span<const char> text)
  {
    traces.emplace_back(MarkerTrace{etd, {begin(text), end(text)}});
  }
  void draw(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    traces.emplace_back(
      DrawTrace{otd, debug_info, vs, ps, &pipeline_base, &pipeline, count, instance_count, start, first_instance, topology});
  }
  void drawIndexed(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
    uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
  {
    traces.emplace_back(DrawIndexedTrace{
      otd, debug_info, vs, ps, &pipeline_base, &pipeline, count, instance_count, index_start, first_instance, topology, vertex_base});
  }
  void drawIndirect(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    traces.emplace_back(DrawIndirectTrace{otd, debug_info, buffer, vs, ps, &pipeline_base, &pipeline});
  }
  void drawIndexedIndirect(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &buffer)
  {
    traces.emplace_back(DrawIndexedIndirectTrace{otd, debug_info, buffer, vs, ps, &pipeline_base, &pipeline});
  }
  void dispatchIndirect(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
  {
    traces.emplace_back(DispatchIndirectTrace{otd, debug_info, buffer, state, &pipeline});
  }
  void dispatch(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &state,
    ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    traces.emplace_back(DispatchTrace{otd, debug_info, state, &pipeline, x, y, z});
  }
  void dispatchMesh(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t x, uint32_t y, uint32_t z)
  {
    traces.emplace_back(DispatchMeshTrace{otd, debug_info, vs, ps, &pipeline_base, &pipeline, x, y, z});
  }
  void dispatchMeshIndirect(const OperationTraceData &otd, const call_stack::CommandData &debug_info, const PipelineStageStateBase &vs,
    const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
    const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
  {
    traces.emplace_back(DispatchMeshIndirectTrace{otd, debug_info, args, count, vs, ps, &pipeline_base, &pipeline, max_count});
  }
  void blit(const OperationTraceData &otd, const call_stack::CommandData &debug_info)
  {
    traces.emplace_back(BlitTrace{otd, debug_info});
  }
#if D3D_HAS_RAY_TRACING
  void dispatchRays(const OperationTraceData &otd, const call_stack::CommandData &debug_info,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
  {
    traces.emplace_back(DispatchRaysTrace{otd, debug_info, dispatch_parameters, rbt, rdp});
  }
  void dispatchRaysIndirect(const OperationTraceData &otd, const call_stack::CommandData &debug_info,
    const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
  {
    traces.emplace_back(DispatchRaysIndirect{otd, debug_info, dispatch_parameters, rbt, rdip});
  }
#endif
  void reset() { traces.clear(); }

  struct VisitorContext
  {
    uint32_t index = 0;
  };

  VisitorContext beginVisitation() const { return {}; }

  void reportAsCompleted(VisitorContext &ctx, D3D12_AUTO_BREADCRUMB_OP until_op_type, call_stack::Reporter &reporter) const
  {
    if (!isTracedOp(until_op_type))
    {
      return;
    }
    if (isEventOp(until_op_type))
    {
      return;
    }
    uint32_t index = reportEvents(ctx.index, reporter, CompletionStatus::Completed);
    ctx.index = reportOps(index, until_op_type, reporter, CompletionStatus::Completed);
  }
  void reportAsLastCompleted(VisitorContext &ctx, D3D12_AUTO_BREADCRUMB_OP until_op_type, call_stack::Reporter &reporter) const
  {
    if (!isTracedOp(until_op_type))
    {
      return;
    }
    if (isEventOp(until_op_type))
    {
      return;
    }
    uint32_t index = reportEvents(ctx.index, reporter, CompletionStatus::LastCompleted);
    ctx.index = reportOps(index, until_op_type, reporter, CompletionStatus::LastCompleted);
  }
  void reportAsNotCompleted(VisitorContext &ctx, D3D12_AUTO_BREADCRUMB_OP until_op_type, call_stack::Reporter &reporter) const
  {
    if (!isTracedOp(until_op_type))
    {
      return;
    }
    if (isEventOp(until_op_type))
    {
      return;
    }
    uint32_t index = reportEvents(ctx.index, reporter, CompletionStatus::NotCompleted);
    ctx.index = reportOps(index, until_op_type, reporter, CompletionStatus::NotCompleted);
  }
  template <typename U>
  void reportEverythingUntilAsCompleted(VisitorContext &ctx, const U &until_point, call_stack::Reporter &reporter) const
  {
    uint32_t endPos = findUntilPosition(ctx.index, until_point);
    uint32_t index = ctx.index;
    for (; index < endPos; ++index)
    {
      eastl::visit([&reporter](auto &trace) { trace.report(reporter, CompletionStatus::Completed); }, traces[index]);
    }
    ctx.index = endPos;
  }
  template <typename U>
  void reportEverythingMatchingAsLastCompleted(VisitorContext &ctx, const U &until_point, call_stack::Reporter &reporter) const
  {
    uint32_t endPos = findAfterPosition(ctx.index, until_point);
    uint32_t index = ctx.index;
    for (; index < endPos; ++index)
    {
      eastl::visit([&reporter](auto &trace) { trace.report(reporter, CompletionStatus::LastCompleted); }, traces[index]);
    }
    ctx.index = endPos;
  }
  template <typename U>
  void reportEverythingAsNotCompleted(VisitorContext &ctx, const U &until_point, call_stack::Reporter &reporter) const
  {
    uint32_t untilPos = findUntilPosition(ctx.index, until_point);
    uint32_t endPos = findAfterPosition(untilPos, until_point);
    uint32_t index = ctx.index;
    for (; index < endPos; ++index)
    {
      eastl::visit([&reporter](auto &trace) { trace.report(reporter, CompletionStatus::NotCompleted); }, traces[index]);
    }
    ctx.index = endPos;
  }
  void reportEverythingAsNoCompleted(VisitorContext &ctx, call_stack::Reporter &reporter) const
  {
    auto index = ctx.index;
    for (; index < traces.size(); ++index)
    {
      eastl::visit([&reporter](auto &trace) { trace.report(reporter, CompletionStatus::NotCompleted); }, traces[index]);
    }
    ctx.index = index;
  }
};
} // namespace drv3d_dx12::debug
