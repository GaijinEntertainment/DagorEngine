//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <b64/cencode.h>
#include <b64/cdecode.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>
#include <debug/dag_assert.h>

namespace base64
{

template <typename S = eastl::string>
inline S encode(const uint8_t *data, size_t sz)
{
  base64_encodestate b64st;
  base64_init_encodestate(&b64st);
  S s(typename S::CtorDoNotInitialize{}, (4 * sz / 3 + 4) & ~3);
  int cnt = base64_encode_block((const char *)data, sz, s.begin(), &b64st);
  cnt += base64_encode_blockend(&s.begin()[cnt], &b64st);
#ifdef _DEBUG_TAB_
  G_ASSERTF(cnt <= s.capacity(), "%d %d %d", cnt, s.capacity(), sz);
#endif
  s.begin()[cnt] = '\0';
  s.force_size(cnt);
  return s;
}

template <typename T = eastl::vector<char>>
inline T decode(eastl::string_view indata)
{
  base64_decodestate b64st;
  base64_init_decodestate(&b64st);
  T outdata;
  outdata.resize(indata.size());
  int len = base64_decode_block((const int8_t *)indata.data(), indata.length(), (int8_t *)&outdata[0], &b64st);
#ifdef _DEBUG_TAB_
  G_ASSERT(len >= 0);
#endif
  outdata.resize(len);
  return outdata;
}

} // namespace base64
