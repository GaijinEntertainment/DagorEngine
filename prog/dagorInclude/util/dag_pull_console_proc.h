//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#define DAG_CONSOLE_CC0(a, b)           a##b
#define DAG_CONSOLE_CC1(a, b)           DAG_CONSOLE_CC0(a, b)
#define DAG_CONSOLE_PULL_VAR_NAME(func) DAG_CONSOLE_CC1(console_pull_, func)

#define PULL_CONSOLE_PROC(func)                             \
  namespace console                                         \
  {                                                         \
  extern int DAG_CONSOLE_PULL_VAR_NAME(func);               \
  }                                                         \
  namespace console##func                                   \
  {                                                         \
    int pulled = console ::DAG_CONSOLE_PULL_VAR_NAME(func); \
  }
