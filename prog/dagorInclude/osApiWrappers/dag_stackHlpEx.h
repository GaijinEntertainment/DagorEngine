//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_COREIMP.h>

#include <EASTL/functional.h>
typedef eastl::function<bool(const char *, size_t base, size_t size)> enum_stack_modules_cb;

//! enumerate current modules, their names and position. SLOW and only WINDOWS

KRNLIMP bool stackhlp_get_symbol(void *addr, uint32_t &line, char *filename, size_t max_file_name, char *symbolname,
  size_t max_symbol_name);
KRNLIMP bool stackhlp_enum_modules(const enum_stack_modules_cb &cb);
KRNLIMP int get_thread_handle_callstack(intptr_t thread_handle, void **stack, uint32_t max_stack_size);
KRNLIMP intptr_t sampling_open_thread(intptr_t thread_id);
KRNLIMP void sampling_close_thread(intptr_t thread_handle);
KRNLIMP bool can_unwind_callstack_for_other_thread();

#include <supp/dag_undef_COREIMP.h>
