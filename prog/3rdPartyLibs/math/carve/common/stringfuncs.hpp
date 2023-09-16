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

#include <string>

namespace str {

static inline bool startswith(const std::string& a, const std::string& b) {
  if (b.size() > a.size()) {
    return false;
  }
  return a.compare(0, b.size(), b) == 0;
}

template <typename strip_t>
static inline std::string rstrip(const std::string& a,
                                 const strip_t stripchars) {
  std::string::size_type p = a.find_last_not_of(stripchars);
  if (p == std::string::npos) {
    return "";
  }
  return a.substr(0, p + 1);
}

static inline std::string rstrip(const std::string& a) {
  std::string::size_type p = a.size();
  while (p && isspace(a[p - 1])) {
    p--;
  }
  if (!p) {
    return "";
  }
  return a.substr(0, p);
}

template <typename strip_t>
static inline std::string lstrip(const std::string& a,
                                 const strip_t stripchars) {
  std::string::size_type p = a.find_first_not_of(stripchars);
  if (p == std::string::npos) {
    return "";
  }
  return a.substr(p);
}

static inline std::string lstrip(const std::string& a) {
  std::string::size_type p = 0;
  while (p < a.size() && isspace(a[p])) {
    p++;
  }
  if (p == a.size()) {
    return "";
  }
  return a.substr(p);
}

template <typename strip_t>
static inline std::string strip(const std::string& a,
                                const strip_t stripchars) {
  std::string::size_type p = a.find_first_not_of(stripchars);
  if (p == std::string::npos) {
    return "";
  }
  std::string::size_type q = a.find_last_not_of(stripchars);
  return a.substr(p, q - p);
}

static inline std::string strip(const std::string& a) {
  std::string::size_type p = 0;
  while (p < a.size() && isspace(a[p])) {
    p++;
  }
  if (p == a.size()) {
    return "";
  }
  std::string::size_type q = a.size();
  while (q > p && isspace(a[q - 1])) {
    q--;
  }
  return a.substr(p, q - p);
}
}  // namespace str
