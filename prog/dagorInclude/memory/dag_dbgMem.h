//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

namespace DagDbgMem
{
// pointer operations
KRNLIMP const char *get_ptr_allocation_call_stack(void *ptr);
KRNLIMP bool check_ptr(void *ptr, int *out_chk = NULL);

// whole memory operations
KRNLIMP void next_generation();

KRNLIMP bool check_memory(bool only_cur_gen);
KRNLIMP void dump_all_used_ptrs(bool only_cur_gen);
KRNLIMP void dump_leaks(bool only_cur_gen);
KRNLIMP void dump_raw_heap(const char *file_name);
KRNLIMP void dump_used_mem_and_gaps(bool summary_only);

// debugging state
// thorough checks: check all allocated pointers for overruns
// on any memory operation (default: disabled )
KRNLIMP void enable_thorough_checks(bool enable);

//! useful to temporary turn off / turn on stask fill for allocated pointers
//! (in case of slow stackhlp_fill_stack(), e.g. win64);
//! enable_stack_fill() is true by default;
//! returns previous state
KRNLIMP bool enable_stack_fill(bool enable);
}; // namespace DagDbgMem

#include <supp/dag_undef_COREIMP.h>
