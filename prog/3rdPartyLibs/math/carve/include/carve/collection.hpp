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

namespace carve {

template <typename set_t>
class set_insert_iterator
    : public std::iterator<std::output_iterator_tag, void, void, void, void> {
 protected:
  set_t* set;

 public:
  set_insert_iterator(set_t& s) : set(&s) {}

  set_insert_iterator& operator=(typename set_t::const_reference value) {
    set->insert(value);
    return *this;
  }

  set_insert_iterator& operator*() { return *this; }
  set_insert_iterator& operator++() { return *this; }
  set_insert_iterator& operator++(int) { return *this; }
};

template <typename set_t>
inline set_insert_iterator<set_t> set_inserter(set_t& s) {
  return set_insert_iterator<set_t>(s);
}
}  // namespace carve
