//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

KRNLIMP void *os_dll_load(const char *filename);
KRNLIMP const char *os_dll_get_last_error_str();
KRNLIMP void *os_dll_get_symbol(void *handle, const char *function);
KRNLIMP bool os_dll_close(void *handle);

struct DagorDllCloser
{
  void operator()(void *handle) { handle ? (void)os_dll_close(handle) : (void)0; }
};

#include <supp/dag_undef_COREIMP.h>
