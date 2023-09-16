//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>

static inline const char *get_multi_line_str_end(const char *str, int max_ln, int max_byte_sz)
{
  const char *str_end = str;
  for (int i = 0; str_end && str_end < str + max_byte_sz && i < max_ln; i++)
    str_end = strchr(str_end + 1, '\n');
  return str_end ? str_end : (str ? str + strlen(str) : (const char *)NULL);
}
