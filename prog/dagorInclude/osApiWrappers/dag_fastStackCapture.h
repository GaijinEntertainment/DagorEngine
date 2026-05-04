//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

// Fast stack capture alternatives for Win64 (on other platform fallback to slow versions).
//
// 1) fast_stackhlp_fill_stack_cached:
//    Replaces RtlLookupFunctionEntry (internal spinlock per frame) with a cached
//    binary search over PE RUNTIME_FUNCTION tables. Uses RtlVirtualUnwind for
//    correct unwinding. Works with any compilation settings.
//
// 2) fast_stackhlp_fill_stack_rbp:
//    Lightweight unwinder. Avoids RtlCaptureContext, RtlVirtualUnwind, and
//    RtlLookupFunctionEntry entirely. Parses UNWIND_INFO directly.
//
// 3) fast_get_thread_handle_callstack:
//    Cross-thread stack capture. SuspendThread + GetThreadContext + cached unwind.
//    Avoids RtlLookupFunctionEntry spinlock that causes contention when sampling
//    multiple threads.

// Strategy for handling cache misses during stack unwinding.
// Flags-style enum: UpdateAndResolve = UpdateCache | ResolveWithOS.
enum class CacheMissStrategy : uint8_t
{
  None = 0,            // Return nullptr on cache miss (safe for cross-thread)
  UpdateCache = 1,     // Try to add the missing module incrementally
  ResolveWithOS = 2,   // Fall back to stackhlp_fill_stack on SEH
  UpdateAndResolve = 3 // Try incremental update first, then OS fallback
};

// Cached unwind tables approach - correct, no spinlock.
unsigned fast_stackhlp_fill_stack_cached(void **stack, unsigned max_size, int skip_frames = 0,
  CacheMissStrategy strategy = CacheMissStrategy::UpdateCache);

// Lightweight unwinder - fastest, no OS calls in hot loop.
unsigned fast_stackhlp_fill_stack_rbp(void **stack, unsigned max_size, int skip_frames = 0,
  CacheMissStrategy strategy = CacheMissStrategy::UpdateCache);

// Cross-thread stack capture with cached unwind tables.
// Returns frame count, or -1 on failure (same contract as get_thread_handle_callstack).
int fast_get_thread_handle_callstack(intptr_t thread_handle, void **stack, uint32_t max_stack_size);

// Cross-thread stack capture with lightweight unwinder - no RtlVirtualUnwind.
// Returns frame count, or -1 on failure (same contract as get_thread_handle_callstack).
int fast_get_thread_handle_callstack_lightweight(intptr_t thread_handle, void **stack, uint32_t max_stack_size);

// Call once before timing (and when modules are loaded) to exclude one-time cache init cost.
// call with reinit = true, to force reinit (when new module became (un)loaded)
void fast_stackhlp_init_cache(bool reinit = false);
