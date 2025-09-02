// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace drv3d_dx12::debug
{
/// Status of a complete trace run
enum class TraceRunStatus
{
  /// GPU executed all commands
  CompletedExecution,
  /// GPU encountered an error during execution
  ErrorDuringExecution,
  /// The trace commands where not launched yet
  NotLaunched,
  /// No trace data is available
  NoTraceData
};
} // namespace drv3d_dx12::debug