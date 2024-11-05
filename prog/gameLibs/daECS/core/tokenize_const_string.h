// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/string_view.h>

// calls Cb(eastl::string_view found_view)
//  if Cb returns false, tokenize_const_string will return false

template <typename Cb>
inline bool tokenize_const_string(eastl::string_view str, const char *delim, Cb cb)
{
  size_t start, end = 0;
  while (end < str.length() && str[end] != 0)
  {
    start = end;
    while (start < str.length() && str[start] && strchr(delim, str[start]) != nullptr)
      start++; // skip initial delimeters
    end = start;
    while (end < str.length() && str[end] && strchr(delim, str[end]) == nullptr)
      end++; // skip to end of word

    if (end - start != 0) // just ignore zero-length strings.
      if (!cb(eastl::string_view(str.data() + start, end - start)))
        return false;
    ++end;
  }
  return true;
}
