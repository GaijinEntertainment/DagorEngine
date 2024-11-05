//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if 1

#include <supp/dag_define_KRNLIMP.h>
extern KRNLIMP bool dgs_trace_inpdev_line;
#include <supp/dag_undef_KRNLIMP.h>

#include <debug/dag_log.h>
#define DEBUG_TRACE_INPDEV(...) \
  if (dgs_trace_inpdev_line)    \
  LOGMESSAGE_CTX('=PNI', __VA_ARGS__)

#else
#define DEBUG_TRACE_INPDEV(...)
#endif
