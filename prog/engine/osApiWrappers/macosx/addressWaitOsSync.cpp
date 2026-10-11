// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Native macOS/iOS/tvOS impl for os_wait_on_address using os_sync_wait_on_address (Darwin's futex).
// It is available since macOS 14.4/iOS 17.4/tvOS 17.4 only (weak linked), older OS fall back to cond var buckets.

#include <osApiWrappers/dag_addressWait.h>
#include <os/os_sync_wait_on_address.h>
#include "../addressWaitStdCondVar.h"

// Constant per process, so wait and wake never mix native and cond var backends
static bool has_os_sync()
{
  static const bool value = [] {
    if (__builtin_available(macOS 14.4, iOS 17.4, tvOS 17.4, *))
      return true;
    return false;
  }();
  return value;
}

// All os_sync_* calls below are guarded by has_os_sync()
#pragma clang diagnostic ignored "-Wunguarded-availability"

void os_wait_on_address(volatile uint32_t *addr, const uint32_t *cmpaddr, int wait_ms)
{
  if (!has_os_sync()) [[unlikely]]
    return addr_wait_cv::wait(addr, cmpaddr, wait_ms);
  void *a = const_cast<uint32_t *>(addr);
  if (wait_ms < 0)
    os_sync_wait_on_address(a, *cmpaddr, sizeof(*addr), OS_SYNC_WAIT_ON_ADDRESS_NONE);
  else if (wait_ms > 0) // zero timeout is EINVAL
    os_sync_wait_on_address_with_timeout(a, *cmpaddr, sizeof(*addr), OS_SYNC_WAIT_ON_ADDRESS_NONE, OS_CLOCK_MACH_ABSOLUTE_TIME,
      uint64_t(wait_ms) * 1000000);
}

void os_wake_on_address_all(volatile uint32_t *addr)
{
  if (has_os_sync()) [[likely]]
    os_sync_wake_by_address_all(const_cast<uint32_t *>(addr), sizeof(*addr), OS_SYNC_WAKE_BY_ADDRESS_NONE);
  else
    addr_wait_cv::wake(addr);
}

void os_wake_on_address_one(volatile uint32_t *addr)
{
  if (has_os_sync()) [[likely]]
    os_sync_wake_by_address_any(const_cast<uint32_t *>(addr), sizeof(*addr), OS_SYNC_WAKE_BY_ADDRESS_NONE);
  else
    addr_wait_cv::wake(addr);
}
