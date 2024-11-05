//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_KRNLIMP.h>

// console CPU/GPU phys memory
namespace dagor_phys_memory
{
enum
{
  PM_PROT_CPU_READ = 1,
  PM_PROT_CPU_WRITE = 2,
  PM_PROT_CPU_ALL = (PM_PROT_CPU_READ | PM_PROT_CPU_WRITE),
  PM_PROT_GPU_READ = 4,
  PM_PROT_GPU_WRITE = 8,
  PM_PROT_GPU_ALL = (PM_PROT_GPU_READ | PM_PROT_GPU_WRITE),
  PM_PROT_ALL = (PM_PROT_CPU_ALL | PM_PROT_GPU_ALL),

  PM_ALIGN_PAGE = 0,       // 4k+
  PM_ALIGN_LARGE_PAGE = 1, // 64K+
  PM_ALIGN_GPU_PAGE = 2,   // 1MB+
  PM_ALIGN_HUGE_PAGE = 3   // 1MB+
};

//! returns total number of calls to malloc since program start
KRNLIMP void *alloc_phys_mem(size_t size, size_t alignment = PM_ALIGN_PAGE, uint32_t prot_flags = PM_PROT_CPU_ALL,
  bool cpu_cached = true, bool log_failure = true);

//! returns false if pointer wasn't found in previously allocated blocks
KRNLIMP bool free_phys_mem(void *ptr, bool ignore_unknown = false);

//! returns current memory usage
KRNLIMP size_t get_allocated_phys_mem();
//! returns max memory usage
KRNLIMP size_t get_max_allocated_phys_mem();
//! add value to internal counters to track directly allocated memory
KRNLIMP void add_ext_allocated_phys_mem(int64_t size);

struct Deleter
{
  void operator()(void *ptr) { free_phys_mem(ptr); }
};

} // namespace dagor_phys_memory

#include <supp/dag_undef_KRNLIMP.h>
