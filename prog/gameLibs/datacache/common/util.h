// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ctype.h> // tolower

class DataBlock;

namespace datacache
{

template <int N>
static const char *get_real_key(const char *key, char (&buf)[N])
{
  const char *c = key;
  for (; *c && *c != '?' && c < (key + N - 1); ++c)
    buf[c - key] = *c;
  buf[c - key] = '\0';
  return &buf[0];
}

// FNV-1a hash
inline size_t get_entry_hash_key(const char *key, int *key_len = NULL)
{
  const unsigned char *p = (const unsigned char *)key;
  unsigned int c, result = 2166136261U;
  while ((c = *p++) != 0)
  {
    unsigned lc = tolower(c); // to make path case insensitive
    result = (result ^ lc) * 16777619;
    if (key_len)
      (*key_len)++;
  }
  return (size_t)result;
}

}; // namespace datacache
