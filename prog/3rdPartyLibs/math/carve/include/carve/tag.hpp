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

#include <carve/carve.hpp>

namespace carve {

class tagable {
 private:
  static int s_count;

 protected:
  mutable int __tag;

 public:
  tagable(const tagable&) : __tag(s_count - 1) {}
  tagable& operator=(const tagable&) { return *this; }

  tagable() : __tag(s_count - 1) {}

  void tag() const { __tag = s_count; }
  void untag() const { __tag = s_count - 1; }
  bool is_tagged() const { return __tag == s_count; }
  bool tag_once() const {
    if (__tag == s_count) {
      return false;
    }
    __tag = s_count;
    return true;
  }

  static void tag_begin() { s_count++; }
};
}  // namespace carve
