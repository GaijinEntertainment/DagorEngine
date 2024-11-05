//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_simpleString.h>

namespace danet
{

bool get_internal_ip(char *buf, size_t buf_size);

inline void get_internal_ip_str(SimpleString &str)
{
  char buf[32];
  if (get_internal_ip(buf, sizeof(buf)))
    str = buf;
  else
    str = "0.0.0.0";
}

} // namespace danet
