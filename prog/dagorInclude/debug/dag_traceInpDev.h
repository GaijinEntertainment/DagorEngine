//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if 1

#include <supp/dag_define_COREIMP.h>
extern KRNLIMP bool dgs_trace_inpdev_line;
#include <supp/dag_undef_COREIMP.h>

#include <debug/dag_log.h>
#define DEBUG_TRACE_INPDEV(...) dgs_trace_inpdev_line ? logmessage_ctx('=PNI', __VA_ARGS__) : (void)0

#else
#define DEBUG_TRACE_INPDEV(...) \
  do                            \
  {                             \
  } while (0)

#endif
