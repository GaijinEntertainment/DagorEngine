//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_preprocessor.h>

#define DAG_CONSOLE_PULL_VAR_NAME(func) DAG_CONCAT(console_pull_, func)

#define PULL_CONSOLE_PROC(func)               \
  namespace console                           \
  {                                           \
  extern int DAG_CONSOLE_PULL_VAR_NAME(func); \
  }                                           \
  namespace console##func { int pulled = console ::DAG_CONSOLE_PULL_VAR_NAME(func); }
