//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

enum class OpenConsoleMode
{
  Show,
  Hide
};

KRNLIMP void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool force_sync = false,
  OpenConsoleMode open_mode = OpenConsoleMode::Show);

KRNLIMP void os_shell_execute_w(const wchar_t *op, const wchar_t *file, const wchar_t *params, const wchar_t *dir,
  bool force_sync = false, OpenConsoleMode open_mode = OpenConsoleMode::Show);

#include <supp/dag_undef_KRNLIMP.h>
