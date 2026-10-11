// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_addressWait.h>
#include "addressWaitStdCondVar.h"

// Note: dead code, but could be useful for debugging this module on windows
#if _TARGET_PC_WIN
void init_win_wait_on_address() {}
os_wait_on_address_cb_t os_get_native_wait_on_address_impl()
{
  return [=](volatile uint32_t *a, const uint32_t *ca, int w) { os_wait_on_address(a, ca, w); };
}
#endif

void os_wait_on_address(volatile uint32_t *addr, const uint32_t *cmpaddr, int wait_ms) { addr_wait_cv::wait(addr, cmpaddr, wait_ms); }

void os_wake_on_address_all(volatile uint32_t *addr) { addr_wait_cv::wake(addr); }

void os_wake_on_address_one(volatile uint32_t *addr) { addr_wait_cv::wake(addr); }
