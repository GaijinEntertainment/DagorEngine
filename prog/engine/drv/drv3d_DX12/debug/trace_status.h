// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace drv3d_dx12::debug
{
/// Status of a trace entry
enum class TraceStatus
{
  /// GPU never reached the point to launch all previous commands
  NotLaunched,
  /// GPU launched all previous commands
  Launched,
  /// GPU completed all previous commands
  Completed
};

inline const char *prefix(TraceStatus status)
{
  switch (status)
  {
    default: return "[UNKNOWN  ]";
    case TraceStatus::NotLaunched: return "[RECORDED ]";
    case TraceStatus::Launched: return "[LAUNCHED ]";
    case TraceStatus::Completed: return "[COMPLETED]";
  }
}
} // namespace drv3d_dx12::debug