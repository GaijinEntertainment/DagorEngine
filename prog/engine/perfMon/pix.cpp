// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_pix.h>

#if _TARGET_XBOX && DAGOR_DBGLEVEL != 0

#ifndef USE_PIX
#define USE_PIX 1
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <pix.h>

void PIX_BEGIN_CPU_EVENT(const char *name) { PIXBeginEvent(0, name); }
void PIX_END_CPU_EVENT() { PIXEndEvent(); }

#endif