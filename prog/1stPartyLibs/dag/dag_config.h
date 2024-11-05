//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/internal/config.h>

//dagor settings
#include <debug/dag_assert.h>
#define DAG_ASSERTF G_ASSERTF
#define DAG_REALLOC memrealloc_default
#define DAG_RESIZE_IN_PLACE memresizeinplace_default
#define DAG_STD_EXPAND_MIN_SIZE 4096
//- dagor settings

#ifndef DAG_ASSERTF
  #define DAG_ASSERTF(expression, fmt, ...) EASTL_ASSERT_MSG(expression, fmt)
#endif
#ifndef DAG_REALLOC
  #define DAG_REALLOC realloc
#endif

#ifndef DAG_RESIZE_IN_PLACE
  #define DAG_RESIZE_IN_PLACE(a,b) false
#endif

#ifndef DAG_STD_EXPAND_MIN_SIZE
  #define DAG_STD_EXPAND_MIN_SIZE 0
#endif
