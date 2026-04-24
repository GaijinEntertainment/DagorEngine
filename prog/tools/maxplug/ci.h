// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <algorithm>

struct CaseInsensitiveHash
{
  size_t operator()(const std::string &keyval) const
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
  bool operator()(const std::string &left, const std::string &right) const
  {
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(), [](char a, char b) { return tolower(a) == tolower(b); });
  }
};
