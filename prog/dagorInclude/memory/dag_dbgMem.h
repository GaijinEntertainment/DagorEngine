//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_daProfileMemory.h>

#include <supp/dag_define_KRNLIMP.h>

namespace DagDbgMem
{
// pointer operations
KRNLIMP const char *get_ptr_allocation_call_stack(void *ptr);

//! useful to temporary turn off / turn on stask fill for allocated pointers
//! (in case of slow stackhlp_fill_stack(), e.g. win64);
//! enable_stack_fill() is true by default;
//! returns previous state
KRNLIMP bool enable_stack_fill(bool enable);

//! all allocations in this thread will use this current callstack now. Without corresponding unset that will cause memory leak!
//! returns handle to previously set forced callstack, to be passed to unset_forced_profile_callstack() (for recursion case);
KRNLIMP da_profiler::profile_mem_data_t set_forced_profile_callstack();

//! all allocations continue to gather callstacks
KRNLIMP void unset_forced_profile_callstack(da_profiler::profile_mem_data_t = da_profiler::invalid_memory_profile);

//! thread unsafe debug function, allowing gathering callstacks for profiling / memleaks
KRNLIMP void set_allow_fill_stack(bool);

struct ForcedProfileCallStackRAII
{
  da_profiler::profile_mem_data_t handle = da_profiler::invalid_memory_profile;
  ForcedProfileCallStackRAII() { handle = set_forced_profile_callstack(); }
  ~ForcedProfileCallStackRAII() { unset_forced_profile_callstack(handle); }
};
}; // namespace DagDbgMem

class IMemAllocExtAPI
{
public:
  virtual da_profiler::profile_mem_data_t setForcedProfileCallstack() = 0;
  virtual void unsetForcedProfileCallstack(da_profiler::profile_mem_data_t) = 0;
  virtual const char *getPtrAllocationCallstack(void *ptr) = 0;
};

KRNLIMP extern IMemAllocExtAPI *stdmem_extapi; // can be nullptr when extended API is not available

#include <supp/dag_undef_KRNLIMP.h>
