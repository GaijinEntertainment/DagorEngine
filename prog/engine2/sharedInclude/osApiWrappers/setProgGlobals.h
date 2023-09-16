#ifndef _DAGOR_PROG_GLBL_H
#define _DAGOR_PROG_GLBL_H
#pragma once

#include <supp/dag_define_COREIMP.h>

KRNLIMP void win32_set_instance(void *hinst);
KRNLIMP void win32_set_main_wnd(void *hwnd);

extern void (*hook_set_thread_name)(const char *name);

#include <supp/dag_undef_COREIMP.h>

#endif
