//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>

#include <supp/dag_define_COREIMP.h>

class IMemAlloc;

namespace dagor_memory_tracker
{
//! registers new heap
KRNLIMP void register_heap(IMemAlloc *heap, const char *name);
//! track memory block
KRNLIMP void add_block(IMemAlloc *heap, void *addr, size_t size, bool user_heap = false);
//! untrack memory block
KRNLIMP bool remove_block(IMemAlloc *heap, void *addr, size_t user_size = 0, bool anywhere = false);
//! check if block allocated
KRNLIMP bool is_addr_valid(void *addr, size_t size);
//! assert if block allocated
KRNLIMP void assert_addr(void *addr, size_t size);
//! save current tracking state
KRNLIMP void save_full_dump(const char *filename);
//! print stats to console
KRNLIMP void print_full_stats();
} // namespace dagor_memory_tracker

#include <supp/dag_undef_COREIMP.h>
