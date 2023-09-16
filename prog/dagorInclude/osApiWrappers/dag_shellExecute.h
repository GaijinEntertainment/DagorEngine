//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

KRNLIMP void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool force_sync = false);

KRNLIMP void os_shell_execute_w(const wchar_t *op, const wchar_t *file, const wchar_t *params, const wchar_t *dir,
  bool force_sync = false);

#include <supp/dag_undef_COREIMP.h>
