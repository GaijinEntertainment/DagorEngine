// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "gpu_postmortem.h"
#include <device_context_cmd.h>
#include <driver.h>
#include <variant_vector.h>

#include <debug/dag_log.h>
#include <EASTL/array.h>


namespace drv3d_dx12::debug
{
struct GPUBreadcrumb
{
  TraceCheckpoint checkpoint;
};

namespace core
{
class FrameCommandLogger
{
  static constexpr size_t NumFrameCommandLogs = 5;

  using LoggedCommandPack = typename AppendToTypePack<AnyCommandPack, GPUBreadcrumb>::Type;

  struct FrameCommandLog
  {
    VariantStreamBuffer<LoggedCommandPack> log;
    uint32_t frameId = 0;
    // Keep track of the last checkpoint of the log, per log. During reporting we can do a quick check to
    // see if all commands where executed or not, by asking the recorder for the completed checkpoint for
    // this check points "timeline".
    TraceCheckpoint lastCheckpoint;
  };

  eastl::array<FrameCommandLog, NumFrameCommandLogs> frameLogs;
  uint32_t activeLogId = 0;
  uint64_t frameId = 0;

public:
  VariantStreamBuffer<LoggedCommandPack> &GetActiveFrameLog() { return frameLogs[activeLogId].log; }

  void initNextFrameLog()
  {
    frameId++;

    activeLogId = (activeLogId + 1) % NumFrameCommandLogs;
    frameLogs[activeLogId].frameId = frameId;
    frameLogs[activeLogId].lastCheckpoint = {};
    GetActiveFrameLog().clear();
  }

  enum class CheckpointValiationMode
  {
    Validate,
    Ignore,
  };

  void dumpCommandLog(debug::DeviceState &device_state)
  {
    for (int32_t i = 0; i < NumFrameCommandLogs; i++)
    {
      FrameCommandLog &frameCmdLog = frameLogs[(activeLogId + i + 1) % NumFrameCommandLogs];
      dumpFrameCommandLog(frameCmdLog, device_state, CheckpointValiationMode::Validate);
    }
  }

  // NOTE: this is only called when we detect an error during command list recording
  // trying to check GPU progress does not make any sense
  void dumpActiveFrameCommandLog(debug::DeviceState &device_state)
  {
    dumpFrameCommandLog(frameLogs[activeLogId % NumFrameCommandLogs], device_state, CheckpointValiationMode::Ignore);
  }

  void dumpFrameCommandLog(FrameCommandLog &frame_log, debug::DeviceState &device_state, CheckpointValiationMode mode);

  template <typename T, typename P0, typename P1>
  void logCommand(const ExtendedVariant2<T, P0, P1> &params)
  {
    GetActiveFrameLog().pushBack<T, P0, P1>(params.cmd, params.p0.data(), params.p0.size(), params.p1.data(), params.p1.size());
  }

  template <typename T, typename P0>
  void logCommand(const ExtendedVariant<T, P0> &params)
  {
    GetActiveFrameLog().pushBack<T, P0>(params.cmd, params.p0.data(), params.p0.size());
  }

  template <typename T>
  void logCommand(T cmd)
  {
    GetActiveFrameLog().pushBack(cmd);
  }

  template <typename T>
  void checkpoint(T &&checkpoint_provider)
  {
    auto checkpoint = checkpoint_provider();
    auto &lastCheckpoint = frameLogs[activeLogId].lastCheckpoint;
    if (lastCheckpoint.shouldRecord(checkpoint))
    {
      lastCheckpoint = checkpoint;
      logCommand(GPUBreadcrumb{checkpoint});
    }
    else if (!checkpoint.isValid() && lastCheckpoint.isInitial())
    {
      // have to update checkpoint when we are still at initial and we got an invalid checkpoint, otherwise later reporting can not
      // detect if checkpoints where available or not
      lastCheckpoint = checkpoint;
    }
  }
};
} // namespace core

namespace null
{
class FrameCommandLogger
{
public:
  constexpr void initNextFrameLog() {}
  constexpr void dumpCommandLog(debug::DeviceState &) {}
  constexpr void dumpActiveFrameCommandLog(debug::DeviceState &) {}

  template <typename T, typename P0, typename P1>
  constexpr void logCommand(const ExtendedVariant2<T, P0, P1> &)
  {}

  template <typename T, typename P0>
  constexpr void logCommand(const ExtendedVariant<T, P0> &)
  {}
  template <typename T>
  constexpr void logCommand(T)
  {}
  template <typename T>
  void checkpoint(T &&)
  {}
};
} // namespace null

#if DX12_ENABLE_FRAME_COMMAND_LOGGER
using FrameCommandLogger = core::FrameCommandLogger;
#else
using FrameCommandLogger = null::FrameCommandLogger;
#endif
} // namespace drv3d_dx12::debug
