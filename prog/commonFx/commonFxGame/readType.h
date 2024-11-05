// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

template <typename T>
inline T readType(const char *&ptr, int &len)
{
  G_ASSERT(len >= sizeof(T));

  T result = *(T *)(ptr);
  len -= sizeof(T);
  ptr += sizeof(T);
  return result;
}