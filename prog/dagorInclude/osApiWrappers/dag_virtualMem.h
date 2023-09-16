//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

//! size of ordinary page (typically 4K)
static inline size_t os_virtual_mem_get_page_size() { return 4 << 10; }

//! allocate (reserve) memory in linear address space
KRNLIMP void *os_virtual_mem_alloc(size_t sz);
//! free (release) memory in linear address space (sz is important for posix targets)
KRNLIMP bool os_virtual_mem_free(void *ptr, size_t sz);


//! memory access flags for os_virtual_mem_protect
enum
{
  VM_PROT_EXEC = (1 << 0),
  VM_PROT_READ = (1 << 1),
  VM_PROT_WRITE = (1 << 2)
};

//! protect (set access flags) memory subregion (part of linear space allocated with os_virtual_mem_alloc)
KRNLIMP bool os_virtual_mem_protect(void *ptr_region_start, size_t region_sz, unsigned prot_flags);

//! commit memory subregion (assign phys pages)
KRNLIMP bool os_virtual_mem_commit(void *ptr_region_start, size_t region_sz);
//! decommit memory subregion (release phys pages)
KRNLIMP bool os_virtual_mem_decommit(void *ptr_region_start, size_t region_sz);
//! reset memory subregion (content of phys pages is no more needed)
KRNLIMP bool os_virtual_mem_reset(void *ptr_region_start, size_t region_sz);

#include <supp/dag_undef_COREIMP.h>
