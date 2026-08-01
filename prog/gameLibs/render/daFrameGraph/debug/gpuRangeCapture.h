// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <frontend/internalRegistry.h>
#include <frontend/multiplexingInternal.h>
#include <backend/intermediateRepresentation.h>

namespace dafg
{

enum class GpuCaptureRangeStatus
{
  Idle,
  Requested,
  Capturing,
  Taken,
  Failed
};

struct GpuCaptureRangeUiState
{
  GpuCaptureRangeStatus status;
  const char *capturedLog;
};

void gpu_capture_range_request(NodeNameId start, NodeNameId end, intermediate::MultiplexingIndex extent_filter, int count,
  const InternalRegistry &registry);
GpuCaptureRangeUiState gpu_capture_range_get_state();

void gpu_capture_range_prepare(const intermediate::Graph &graph, multiplexing::Extents extents);
void gpu_capture_range_before_node(intermediate::NodeIndex idx);
void gpu_capture_range_after_node(intermediate::NodeIndex idx);
void gpu_capture_range_after_frame();

void append_multiplexing_index_label(eastl::string &out, intermediate::MultiplexingIndex idx, multiplexing::Extents extents,
  const char *separator);

} // namespace dafg
