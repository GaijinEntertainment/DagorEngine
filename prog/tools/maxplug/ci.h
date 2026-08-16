// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <algorithm>
#include <string_view>

constexpr unsigned char ci_tolower(unsigned char c) { return (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c - 'A' + 'a') : c; }

struct CaseInsensitiveHash
{
  using is_transparent = void;

  size_t operator()(std::string_view keyval) const
  {
    size_t hash = 525201411107845655ULL;
    for (char c : keyval)
    {
      hash ^= ci_tolower(c);
      hash *= 0x5bd1e9955bd1e995ULL;
      hash ^= hash >> 47;
    }
    return hash;
  }
};

struct CaseInsensitiveEqual
{
  using is_transparent = void;

  bool operator()(std::string_view left, std::string_view right) const
  {
    return std::equal(left.begin(), left.end(), right.begin(), right.end(),
      [](char a, char b) { return ci_tolower(a) == ci_tolower(b); });
  }
};
