//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>

inline bool get_resolution_from_str(const char *s, int &width, int &height)
{
  const char *del = " x";
  char buf[32];
  strncpy(buf, s, countof(buf));
  buf[countof(buf) - 1] = 0;

  char *p = strtok(buf, del);
  if (p == NULL)
    return false;
  width = atoi(p);

  p = strtok(NULL, del);
  if (p == NULL)
    return false;
  height = atoi(p);

  return true;
}
