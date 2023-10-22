#pragma once

#include "call_stack.h"
#include "event_marker_tracker.h"

namespace drv3d_dx12
{
namespace debug
{
namespace null
{
class DeviceState : public call_stack::Reporter, protected event_marker::Tracker
{
public:
#if DX12_DEBUG_GLOBAL_DEBUG_STATE
  bool setup(GlobalState &, ID3D12Device *) { return true; }
#endif
  void teardown() {}
  void recover(ID3D12Device *) {}
  void sendGPUCrashDump(const char *, const void *, uintptr_t) {}
  void processDebugLog() {}
};
} // namespace null
} // namespace debug
} // namespace drv3d_dx12