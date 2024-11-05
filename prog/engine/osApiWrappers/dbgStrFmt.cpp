// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_dbgStr.h>
#include <stdio.h>
#include <stdarg.h>

#include <util/dag_globDef.h>

#if _TARGET_PC_LINUX // econom mode
static thread_local char out_buf[1 << 10];
#else
static thread_local char out_buf[8 << 10];
#endif

void out_debug_str_fmt(const char *fmt, ...)
{
  char *buf = out_buf;
  va_list list;

  va_start(list, fmt);
  _vsnprintf(buf, sizeof(out_buf), fmt, list);
  va_end(list);

  buf[sizeof(out_buf) - 1] = 0;

  out_debug_str(buf);
}

#define EXPORT_PULL dll_pull_osapiwrappers_dbgStrFmt
#include <supp/exportPull.h>
