//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <generic/dag_fixedMoveOnlyFunction.h>
#include <supp/dag_define_KRNLIMP.h>

// Might return even if addr is not changed (a.k.a spurious wake up)
// Note: `wait_ms != -1` (finite wait) is non portable (i.e. only some backends implement it)
KRNLIMP void os_wait_on_address(volatile uint32_t *addr, const uint32_t *cmpaddr, int wait_ms = -1);

KRNLIMP void os_wait_on_address_64(volatile uint64_t *addr, const uint64_t *cmpaddr, int wait_ms = -1)
#if _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
  ;
#else
  = delete; // Non portable
#endif

#if _TARGET_PC_WIN
KRNLIMP void init_win_wait_on_address();
#endif

#if _TARGET_PC_WIN && !defined(_TARGET_ARCH_ARM64)
using os_wait_on_address_cb_t = dag::FixedMoveOnlyFunction<sizeof(void *), decltype(os_wait_on_address)>;
KRNLIMP os_wait_on_address_cb_t os_get_native_wait_on_address_impl();
#else
inline constexpr auto os_get_native_wait_on_address_impl() { return &os_wait_on_address; }
#endif

KRNLIMP void os_wake_on_address_all(volatile uint32_t *addr);
KRNLIMP void os_wake_on_address_one(volatile uint32_t *addr);

#include <supp/dag_undef_KRNLIMP.h>
