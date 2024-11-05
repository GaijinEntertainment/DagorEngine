//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

#include <supp/dag_define_KRNLIMP.h>

namespace dagor_memory_stat
{
//! returns total number of calls to malloc since program start
KRNLIMP int64_t get_malloc_call_count();

//! returns total number of allocated memory block
KRNLIMP size_t get_memchunk_count(bool with_crt = false);

//! returns current memory usage (bytesAlloc - bytesFree)
KRNLIMP size_t get_memory_allocated(bool with_crt = false);

//! returns current memory usage in KB
KRNLIMP int get_memory_allocated_kb(bool with_crt = false);
} // namespace dagor_memory_stat

#include <supp/dag_undef_KRNLIMP.h>
