// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <algorithm>
#include <string_view>

struct CaseInsensitiveHash
{
  using is_transparent = void;

  size_t operator()(std::string_view keyval) const
  {
    size_t hash = 525201411107845655ULL;
    std::for_each(keyval.begin(), keyval.end(), [&hash](char c) {
      hash ^= tolower(c);
      hash *= 0x5bd1e9955bd1e995ULL;
      hash ^= hash >> 47;
    });
    return hash;
  }
};

struct CaseInsensitiveEqual
{
  using is_transparent = void;

  bool operator()(std::string_view left, std::string_view right) const
  {
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(), [](char a, char b) { return tolower(a) == tolower(b); });
  }
};
