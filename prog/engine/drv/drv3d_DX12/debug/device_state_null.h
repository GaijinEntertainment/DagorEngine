// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "event_marker_tracker.h"
#include "trace_run_status.h"
#include "trace_status.h"
#include "trace_checkpoint.h"


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug
{
class GlobalState;
namespace null
{
class DeviceState : public call_stack::Reporter, protected event_marker::Tracker
{
public:
  constexpr bool setup(GlobalState &, D3DDevice *, const Direct3D12Enviroment &) { return true; }
  constexpr void teardown() {}
  constexpr void preRecovery() {}
  constexpr void recover(D3DDevice *, const Direct3D12Enviroment &) {}
  constexpr void sendGPUCrashDump(const char *, const void *, uintptr_t) {}
  constexpr void processDebugLog() {}
  constexpr bool setRtValidationCallback(const eastl::function<void()> &) { return false; }
  constexpr void nameResource(ID3D12Resource *, eastl::string_view) {}
  constexpr void nameResource(ID3D12Resource *, eastl::wstring_view) {}
  constexpr void nameObject(ID3D12Object *, eastl::string_view) {}
  constexpr void nameObject(ID3D12Object *, eastl::wstring_view) {}
  constexpr TraceCheckpoint getTraceCheckpoint() const { return TraceCheckpoint::make_invalid(); }
  constexpr TraceRunStatus getTraceRunStatusFor(const TraceCheckpoint &) const { return TraceRunStatus::NoTraceData; }
  constexpr TraceStatus getTraceStatusFor(const TraceCheckpoint &) const { return TraceStatus::NotLaunched; }
  constexpr void reportTraceDataForRange(const TraceCheckpoint &, const TraceCheckpoint &) const {}
};
} // namespace null
} // namespace debug
} // namespace drv3d_dx12