// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "call_stack.h"
#include "event_marker_tracker.h"


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
  constexpr bool setup(GlobalState &, ID3D12Device *, const Direct3D12Enviroment &) { return true; }
  constexpr void teardown() {}
  constexpr void preRecovery() {}
  constexpr void recover(ID3D12Device *, const Direct3D12Enviroment &) {}
  constexpr void sendGPUCrashDump(const char *, const void *, uintptr_t) {}
  constexpr void processDebugLog() {}
};
} // namespace null
} // namespace debug
} // namespace drv3d_dx12