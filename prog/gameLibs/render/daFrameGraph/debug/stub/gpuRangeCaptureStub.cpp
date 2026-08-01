// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/gpuRangeCapture.h>

namespace dafg
{

GpuCaptureRangeUiState gpu_capture_range_get_state() { return {GpuCaptureRangeStatus::Idle, ""}; }
void gpu_capture_range_request(NodeNameId, NodeNameId, intermediate::MultiplexingIndex, int, const InternalRegistry &) {}
void gpu_capture_range_prepare(const intermediate::Graph &, multiplexing::Extents) {}
void gpu_capture_range_before_node(intermediate::NodeIndex) {}
void gpu_capture_range_after_node(intermediate::NodeIndex) {}
void gpu_capture_range_after_frame() {}

} // namespace dafg
