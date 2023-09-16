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

#include <carve/geom.hpp>
#include <carve/geom3d.hpp>
#include <carve/math_constants.hpp>

#include <sstream>

#include <math.h>

namespace carve {
namespace geom3d {

struct hash_vector_ptr {
  size_t operator()(const Vector* const& v) const { return (size_t)v; }
  size_t operator()(const std::pair<const Vector*, const Vector*>& v) const {
    size_t r = (size_t)v.first;
    size_t s = (size_t)v.second;
    return r ^ ((s >> 16) | (s << 16));
  }
};

struct vec_adapt_ident {
  const Vector& operator()(const Vector& v) const { return v; }
  Vector& operator()(Vector& v) const { return v; }
};

struct vec_adapt_ptr {
  const Vector& operator()(const Vector* const& v) const { return *v; }
  Vector& operator()(Vector*& v) const { return *v; }
};

struct vec_adapt_pair_first {
  template <typename pair_t>
  const Vector& operator()(const pair_t& v) const {
    return v.first;
  }
  template <typename pair_t>
  Vector& operator()(pair_t& v) const {
    return v.first;
  }
};

struct vec_adapt_pair_second {
  template <typename pair_t>
  const Vector& operator()(const pair_t& v) const {
    return v.second;
  }
  template <typename pair_t>
  Vector& operator()(pair_t& v) const {
    return v.second;
  }
};

template <typename adapt_t>
struct vec_cmp_lt_x {
  adapt_t adapt;
  vec_cmp_lt_x(adapt_t _adapt = adapt_t()) : adapt(_adapt) {}
  template <typename input_t>
  bool operator()(const input_t& a, const input_t& b) const {
    return adapt(a).x < adapt(b).x;
  }
};
template <typename adapt_t>
vec_cmp_lt_x<adapt_t> vec_lt_x(adapt_t& adapt) {
  return vec_cmp_lt_x<adapt_t>(adapt);
}

template <typename adapt_t>
struct vec_cmp_lt_y {
  adapt_t adapt;
  vec_cmp_lt_y(adapt_t _adapt = adapt_t()) : adapt(_adapt) {}
  template <typename input_t>
  bool operator()(const input_t& a, const input_t& b) const {
    return adapt(a).y < adapt(b).y;
  }
};
template <typename adapt_t>
vec_cmp_lt_y<adapt_t> vec_lt_y(adapt_t& adapt) {
  return vec_cmp_lt_y<adapt_t>(adapt);
}

template <typename adapt_t>
struct vec_cmp_lt_z {
  adapt_t adapt;
  vec_cmp_lt_z(adapt_t _adapt = adapt_t()) : adapt(_adapt) {}
  template <typename input_t>
  bool operator()(const input_t& a, const input_t& b) const {
    return adapt(a).z < adapt(b).z;
  }
};
template <typename adapt_t>
vec_cmp_lt_z<adapt_t> vec_lt_z(adapt_t& adapt) {
  return vec_cmp_lt_z<adapt_t>(adapt);
}

template <typename adapt_t>
struct vec_cmp_gt_x {
  adapt_t adapt;
  vec_cmp_gt_x(adapt_t _adapt = adapt_t()) : adapt(_adapt) {}
  template <typename input_t>
  bool operator()(const input_t& a, const input_t& b) const {
    return adapt(a).x > adapt(b).x;
  }
};
template <typename adapt_t>
vec_cmp_gt_x<adapt_t> vec_gt_x(adapt_t& adapt) {
  return vec_cmp_gt_x<adapt_t>(adapt);
}

template <typename adapt_t>
struct vec_cmp_gt_y {
  adapt_t adapt;
  vec_cmp_gt_y(adapt_t _adapt = adapt_t()) : adapt(_adapt) {}
  template <typename input_t>
  bool operator()(const input_t& a, const input_t& b) const {
    return adapt(a).y > adapt(b).y;
  }
};
template <typename adapt_t>
vec_cmp_gt_y<adapt_t> vec_gt_y(adapt_t& adapt) {
  return vec_cmp_gt_y<adapt_t>(adapt);
}

template <typename adapt_t>
struct vec_cmp_gt_z {
  adapt_t adapt;
  vec_cmp_gt_z(adapt_t _adapt = adapt_t()) : adapt(_adapt) {}
  template <typename input_t>
  bool operator()(const input_t& a, const input_t& b) const {
    return adapt(a).z > adapt(b).z;
  }
};
template <typename adapt_t>
vec_cmp_gt_z<adapt_t> vec_gt_z(adapt_t& adapt) {
  return vec_cmp_gt_z<adapt_t>(adapt);
}

template <typename iter_t, typename adapt_t>
void sortInDirectionOfRay(const Vector& ray_dir, iter_t begin, iter_t end,
                          adapt_t adapt) {
  switch (carve::geom::largestAxis(ray_dir)) {
    case 0:
      if (ray_dir.x > 0) {
        std::sort(begin, end, vec_lt_x(adapt));
      } else {
        std::sort(begin, end, vec_gt_x(adapt));
      }
      break;
    case 1:
      if (ray_dir.y > 0) {
        std::sort(begin, end, vec_lt_y(adapt));
      } else {
        std::sort(begin, end, vec_gt_y(adapt));
      }
      break;
    case 2:
      if (ray_dir.z > 0) {
        std::sort(begin, end, vec_lt_z(adapt));
      } else {
        std::sort(begin, end, vec_gt_z(adapt));
      }
      break;
  }
}
}  // namespace geom3d
}  // namespace carve
