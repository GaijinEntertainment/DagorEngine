// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_pix.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <pix3.h>

void PIX_BEGIN_CPU_EVENT(const char *name) { PIXBeginEvent(0, name); }
void PIX_END_CPU_EVENT() { PIXEndEvent(); }
