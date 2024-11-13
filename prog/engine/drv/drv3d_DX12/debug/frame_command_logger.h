// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include <device_context_cmd.h>
#include <driver.h>
#include <variant_vector.h>

#include <debug/dag_log.h>
#include <EASTL/array.h>


namespace drv3d_dx12::debug
{
namespace core
{
class FrameCommandLogger
{
  static constexpr size_t NumFrameCommandLogs = 5;

  struct FrameCommandLog
  {
    VariantVectorRingBuffer<AnyCommandPack> log;
    uint32_t frameId = 0;
  };

  eastl::array<FrameCommandLog, NumFrameCommandLogs> frameLogs;
  uint32_t activeLogId = 0;
  uint64_t frameId = 0;

public:
  VariantVectorRingBuffer<AnyCommandPack> &GetActiveFrameLog() { return frameLogs[activeLogId].log; }

  void initNextFrameLog()
  {
    frameId++;

    activeLogId = (activeLogId + 1) % NumFrameCommandLogs;
    frameLogs[activeLogId].frameId = frameId;
    GetActiveFrameLog().clear();
  }

  void dumpCommandLog(call_stack::Reporter &call_stack_reporter)
  {
    for (int32_t i = 0; i < NumFrameCommandLogs; i++)
    {
      FrameCommandLog &frameCmdLog = frameLogs[(activeLogId + i + 1) % NumFrameCommandLogs];
      dumpFrameCommandLog(frameCmdLog, call_stack_reporter);
    }
  }

  void dumpActiveFrameCommandLog(call_stack::Reporter &call_stack_reporter)
  {
    dumpFrameCommandLog(frameLogs[activeLogId % NumFrameCommandLogs], call_stack_reporter);
  }

  void dumpFrameCommandLog(FrameCommandLog &frame_log, call_stack::Reporter &call_stack_reporter);

  template <typename T, typename P0, typename P1>
  void logCommand(const ExtendedVariant2<T, P0, P1> &params)
  {
    GetActiveFrameLog().pushBack<T, P0, P1>(params.cmd, params.p0.size(), [&params](auto index, auto first, auto second) {
      first(params.p0[index]);
      second(params.p1[index]);
    });
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
};
} // namespace core

namespace null
{
class FrameCommandLogger
{
public:
  constexpr void initNextFrameLog() {}
  constexpr void dumpCommandLog(call_stack::Reporter &) {}
  constexpr void dumpActiveFrameCommandLog(call_stack::Reporter &) {}

  template <typename T, typename P0, typename P1>
  constexpr void logCommand(const ExtendedVariant2<T, P0, P1> &)
  {}

  template <typename T, typename P0>
  constexpr void logCommand(const ExtendedVariant<T, P0> &)
  {}
  template <typename T>
  constexpr void logCommand(T)
  {}
};
} // namespace null

#if DX12_ENABLE_FRAME_COMMAND_LOGGER
using FrameCommandLogger = core::FrameCommandLogger;
#else
using FrameCommandLogger = null::FrameCommandLogger;
#endif
} // namespace drv3d_dx12::debug
