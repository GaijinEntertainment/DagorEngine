//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdarg.h>
#include <string.h>

inline bool df_is_fname_url_like(const char *fn)
{
  return fn && strncmp(fn, "http", 4) == 0 && (fn[4] == ':' || (fn[4] == 's' && fn[5] == ':'));
}
