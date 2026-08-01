// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <initializer_list>
#include <stdint.h>

namespace dblk_test_binary
{
using Bytes = eastl::vector<uint8_t>;

inline void put_leb(Bytes &b, uint32_t v)
{
  for (;;)
  {
    uint8_t byte = v & 0x7F;
    v >>= 7;
    if (v)
      b.push_back(byte | 0x80);
    else
    {
      b.push_back(byte);
      break;
    }
  }
}

inline void put_u32(Bytes &b, uint32_t v)
{
  for (int i = 0; i < 4; ++i)
    b.push_back((v >> (i * 8)) & 0xFF);
}

inline void put_names(Bytes &b, std::initializer_list<const char *> names)
{
  put_leb(b, (uint32_t)names.size());
  if (names.size() == 0)
    return;

  Bytes raw;
  for (const char *n : names)
  {
    for (const char *p = n; *p; ++p)
      raw.push_back((uint8_t)*p);
    raw.push_back(0);
  }
  put_leb(b, (uint32_t)raw.size());
  b.insert(b.end(), raw.begin(), raw.end());
}

inline void put_param(Bytes &b, uint32_t name_id, uint32_t type, uint32_t v)
{
  put_u32(b, (name_id & 0xFFFFFF) | (type << 24));
  put_u32(b, v);
}

inline void put_node(Bytes &b, uint32_t name_id_inc, uint32_t p_count, uint32_t b_count, uint32_t f_block = 0)
{
  put_leb(b, name_id_inc);
  put_leb(b, p_count);
  put_leb(b, b_count);
  if (b_count)
    put_leb(b, f_block);
}
} // namespace dblk_test_binary
