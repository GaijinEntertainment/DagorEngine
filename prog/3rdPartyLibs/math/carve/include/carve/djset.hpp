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
#include <vector>

namespace carve {
namespace djset {

class djset {
 protected:
  struct elem {
    size_t parent, rank;
    elem(size_t p, size_t r) : parent(p), rank(r) {}
    elem() {}
  };

  std::vector<elem> set;
  size_t n_sets;

 public:
  djset() : set(), n_sets(0) {}

  djset(size_t N) {
    n_sets = N;
    set.reserve(N);
    for (size_t i = 0; i < N; ++i) {
      set.push_back(elem(i, 0));
    }
  }

  void init(size_t N) {
    if (N == set.size()) {
      for (size_t i = 0; i < N; ++i) {
        set[i] = elem(i, 0);
      }
      n_sets = N;
    } else {
      djset temp(N);
      std::swap(set, temp.set);
      std::swap(n_sets, temp.n_sets);
    }
  }

  size_t count() const { return n_sets; }

  size_t find_set_head(size_t a) {
    if (a == set[a].parent) {
      return a;
    }

    size_t a_head = a;
    while (set[a_head].parent != a_head) {
      a_head = set[a_head].parent;
    }
    set[a].parent = a_head;
    return a_head;
  }

  bool same_set(size_t a, size_t b) {
    return find_set_head(a) == find_set_head(b);
  }

  void merge_sets(size_t a, size_t b) {
    a = find_set_head(a);
    b = find_set_head(b);
    if (a != b) {
      n_sets--;
      if (set[a].rank < set[b].rank) {
        set[a].parent = b;
      } else if (set[b].rank < set[a].rank) {
        set[b].parent = a;
      } else {
        set[a].rank++;
        set[b].parent = a;
      }
    }
  }

  void get_index_to_set(std::vector<size_t>& index_set,
                        std::vector<size_t>& set_size) {
    index_set.clear();
    index_set.resize(set.size(), n_sets);
    set_size.clear();
    set_size.resize(n_sets, 0);

    size_t c = 0;
    for (size_t i = 0; i < set.size(); ++i) {
      size_t s = find_set_head(i);
      if (index_set[s] == n_sets) {
        index_set[s] = c++;
      }
      index_set[i] = index_set[s];
      set_size[index_set[s]]++;
    }
  }

  template <typename in_iter_t, typename out_collection_t>
  void collate(in_iter_t in, out_collection_t& out) {
    std::vector<size_t> set_id(set.size(), n_sets);
    out.clear();
    out.resize(n_sets);
    size_t c = 0;
    for (size_t i = 0; i < set.size(); ++i) {
      size_t s = find_set_head(i);
      if (set_id[s] == n_sets) {
        set_id[s] = c++;
      }
      s = set_id[s];
      std::insert_iterator<typename out_collection_t::value_type> j(
          out[s], out[s].end());
      *j = *in++;
    }
  }
};
}  // namespace djset
}  // namespace carve
