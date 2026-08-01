// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpuRangeCapture.h"

#include <frontend/multiplexingInternal.h>
namespace dafg
{
struct GpuRangeCaptureState
{
  NodeNameId start{NodeNameId::Invalid};
  NodeNameId end{NodeNameId::Invalid};

  intermediate::MultiplexingIndex extentFilter{intermediate::MultiplexingIndex::Invalid};

  GpuCaptureRangeStatus status{GpuCaptureRangeStatus::Idle};

  eastl::string capturedLog;
  eastl::string rangeNameFlat;
  eastl::string fileBaseName;

  eastl::optional<intermediate::NodeIndex> beginIdx;
  eastl::optional<intermediate::NodeIndex> endIdx;

  int totalCaptures = 1;
  int remainingCaptures = 1;
};

GpuRangeCaptureState capture;

static eastl::string flatten_name(NodeNameId id, const InternalRegistry &registry)
{
  if (id == NodeNameId::Invalid)
    return "frame";
  const char *name = registry.knownNames.getName(id);
  eastl::string out(name[0] == '/' ? name + 1 : name);
  for (char &c : out)
    if (c == '/')
      c = '_';
  return out;
}

static bool node_matches(const intermediate::Node &ir_node, NodeNameId id, intermediate::MultiplexingIndex extent)
{
  return ir_node.frontendNode && *ir_node.frontendNode == id &&
         (extent == intermediate::MultiplexingIndex::Invalid || ir_node.multiplexingIndex == extent);
}

static void append_captured_line(const eastl::string &line)
{
  if (!capture.capturedLog.empty())
    capture.capturedLog.push_back('\n');
  capture.capturedLog += line;
}

static void build_file_base_name(multiplexing::Extents extents, intermediate::MultiplexingIndex multiplexing)
{
  capture.fileBaseName.sprintf("dafg_%s", capture.rangeNameFlat.c_str());

  if (multiplexing != intermediate::MultiplexingIndex::Invalid)
    append_multiplexing_index_label(capture.fileBaseName, multiplexing, extents, "_");

  if (capture.totalCaptures > 1)
    capture.fileBaseName.append_sprintf("_%d", capture.totalCaptures - capture.remainingCaptures + 1);
}

static void begin_capture()
{
  constexpr size_t MAX_PATH = 256;
  eastl::fixed_string<wchar_t, MAX_PATH> fileName;
  for (char c : capture.fileBaseName)
    fileName.push_back(static_cast<wchar_t>(c));

  d3d::driver_command(Drv3dCommand::PIX_GPU_BEGIN_CAPTURE, nullptr, fileName.data());
  capture.status = GpuCaptureRangeStatus::Capturing;
  debug("daFG: GPU range capture begin: %s", capture.fileBaseName.c_str());
}

static void end_capture()
{
  G_ASSERT(capture.status == GpuCaptureRangeStatus::Capturing);
  d3d::driver_command(Drv3dCommand::PIX_GPU_END_CAPTURE);
  capture.status = GpuCaptureRangeStatus::Taken;
  append_captured_line("GpuCaptures/" + capture.fileBaseName);
  debug("daFG: GPU range capture end: %s", capture.fileBaseName.c_str());
}

static void fail_capture(const char *reason)
{
  logerr("GPU range capture failed: %s", reason);
  capture.status = GpuCaptureRangeStatus::Failed;
  capture.capturedLog = reason;
  capture.beginIdx.reset();
  capture.endIdx.reset();
  capture.remainingCaptures = 1;
}

void gpu_capture_range_request(NodeNameId start, NodeNameId end, intermediate::MultiplexingIndex extent_filter, int count,
  const InternalRegistry &registry)
{
  if (capture.status == GpuCaptureRangeStatus::Requested || capture.status == GpuCaptureRangeStatus::Capturing)
  {
    logwarn("daFG: superseding in-progress GPU range capture: %s", capture.rangeNameFlat.c_str());
    if (capture.status == GpuCaptureRangeStatus::Capturing)
      end_capture();
  }

  capture.start = start;
  capture.end = end;
  capture.extentFilter = extent_filter;
  capture.totalCaptures = eastl::max(count, 1);
  capture.remainingCaptures = capture.totalCaptures;
  capture.capturedLog.clear();
  capture.rangeNameFlat.sprintf("%s_to_%s", flatten_name(start, registry).c_str(), flatten_name(end, registry).c_str());
  capture.status = GpuCaptureRangeStatus::Requested;

  debug("daFG: GPU range capture in progress: %s", capture.rangeNameFlat.c_str());
}

GpuCaptureRangeUiState gpu_capture_range_get_state() { return {capture.status, capture.capturedLog.c_str()}; }

void gpu_capture_range_prepare(const intermediate::Graph &graph, multiplexing::Extents extents)
{
  if (capture.status != GpuCaptureRangeStatus::Requested)
    return;

  capture.beginIdx.reset();
  capture.endIdx.reset();
  capture.fileBaseName.clear();

  for (auto i : graph.nodes.keys())
    if (capture.start == NodeNameId::Invalid || node_matches(graph.nodes[i], capture.start, capture.extentFilter))
    {
      capture.beginIdx = i;
      break;
    }

  if (!capture.beginIdx.has_value())
  {
    fail_capture("start node was not scheduled");
    return;
  }

  const auto multiplexing = graph.nodes[*capture.beginIdx].multiplexingIndex;

  if (capture.end != NodeNameId::Invalid)
    for (auto i : graph.nodes.keys())
      if (node_matches(graph.nodes[i], capture.end, multiplexing))
      {
        capture.endIdx = i;
        break;
      }

  if (capture.end != NodeNameId::Invalid && !capture.endIdx.has_value())
  {
    fail_capture("end node was not scheduled");
    return;
  }
  if (capture.endIdx.has_value() && capture.beginIdx.has_value() && *capture.endIdx < *capture.beginIdx)
  {
    fail_capture("end node is scheduled before start node");
    return;
  }

  build_file_base_name(extents, multiplexing);
}

void gpu_capture_range_before_node(intermediate::NodeIndex idx)
{
  if (capture.status != GpuCaptureRangeStatus::Requested)
    return;
  if (capture.beginIdx == idx)
    begin_capture();
}

void gpu_capture_range_after_node(intermediate::NodeIndex idx)
{
  if (capture.status != GpuCaptureRangeStatus::Capturing)
    return;
  if (capture.endIdx == idx)
    end_capture();
}

void gpu_capture_range_after_frame()
{
  if (capture.status == GpuCaptureRangeStatus::Capturing)
  {
    G_ASSERTF(!capture.endIdx.has_value(), "Unclosed GPU range capture at frame end");
    end_capture();
  }

  if (capture.status == GpuCaptureRangeStatus::Taken && capture.remainingCaptures > 1)
  {
    --capture.remainingCaptures;
    capture.status = GpuCaptureRangeStatus::Requested;
    logdbg("daFG: remaining GPU range captures: %d", capture.remainingCaptures);
  }
}

void append_multiplexing_index_label(eastl::string &out, intermediate::MultiplexingIndex idx, multiplexing::Extents extents,
  const char *separator)
{
  const multiplexing::Index mi = multiplexing_index_from_ir(idx, extents);
  const auto addTag = [&](const char *tag, uint32_t value) {
    out.append_sprintf("%s%s%u", out.empty() ? "" : separator, tag, static_cast<unsigned>(value));
  };
  if (extents.viewports > 1)
    addTag("vp", mi.viewport);
  if (extents.subSamples > 1)
    addTag("sub", mi.subSample);
  if (extents.superSamples > 1)
    addTag("ss", mi.superSample);
  if (extents.subCameras > 1)
    addTag("cam", mi.subCamera);
}

} // namespace dafg
