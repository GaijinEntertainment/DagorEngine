// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_logSys.h>

#define D3D_ERROR(...) logmessage(LOGTAG_D3D_ERROR, __VA_ARGS__)

#define D3D_CONTRACT_ERROR(fmt, ...) logmessage(LOGTAG_D3D_ERROR, "Incorrect D3D API usage\n" fmt, ##__VA_ARGS__)

#define D3D_ERROR_ONCE(...)      \
  do                             \
  {                              \
    static bool logged_ = false; \
    if (!logged_)                \
    {                            \
      D3D_ERROR(__VA_ARGS__);    \
      logged_ = true;            \
    }                            \
  } while (0)
