// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <supp/dag_define_KRNLIMP.h>

KRNLIMP void win32_set_instance(void *hinst);
KRNLIMP void win32_set_main_wnd(void *hwnd);

extern void (*hook_set_thread_name)(const char *name);

#include <supp/dag_undef_KRNLIMP.h>
