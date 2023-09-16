// Copyright 2006-2015 Tobias Sargeant (tobias.sargeant@gmail.com).
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "intersect_common.hpp"

template <typename T>
static int is_same(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    return false;
  }

  const size_t S = a.size();
  size_t i, j, p;

  for (p = 0; p < S; ++p) {
    if (a[0] == b[p]) {
      break;
    }
  }
  if (p == S) {
    return 0;
  }

  for (i = 1, j = p + 1; j < S; ++i, ++j) {
    if (a[i] != b[j]) {
      goto not_fwd;
    }
  }
  for (j = 0; i < S; ++i, ++j) {
    if (a[i] != b[j]) {
      goto not_fwd;
    }
  }
  return +1;

not_fwd:
  for (i = 1, j = p - 1; j != (size_t)-1; ++i, --j) {
    if (a[i] != b[j]) {
      goto not_rev;
    }
  }
  for (j = S - 1; i < S; ++i, --j) {
    if (a[i] != b[j]) {
      goto not_rev;
    }
  }
  return -1;

not_rev:
  return 0;
}
