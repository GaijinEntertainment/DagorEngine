// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "trace_id.h"

namespace drv3d_dx12::debug
{
struct TraceCheckpoint
{
  ID3D12GraphicsCommandList *commandList = nullptr;
  TraceID progress = null_trace_value;

  static constexpr TraceCheckpoint make_invalid() { return {.commandList = nullptr, .progress = invalid_trace_value}; }

  bool isValid() const { return invalid_trace_value != progress; }
  // initial state, this counts as progress before any other explicit progress and compares to any valid progress
  // other than other initial states to be earlier.
  bool isInitial() const { return isValid() && nullptr == commandList; }

  // returns true when 'next' is a checkpoint that should be recorded to track GPU forward progress
  bool shouldRecord(const TraceCheckpoint &next) const
  {
    if (!isValid() || !next.isValid())
    {
      return false;
    }
    if (commandList == next.commandList && progress == next.progress)
    {
      return false;
    }
    if (next.isInitial())
    {
      return false;
    }
    if (commandList != next.commandList)
    {
      return true;
    }
    if (isInitial())
    {
      return true;
    }
    return progress < next.progress;
  }
};
} // namespace drv3d_dx12::debug