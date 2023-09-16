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

#if defined(CMAKE_BUILD)
#include <carve/config.h>
#elif defined(XCODE_BUILD)
#include <carve/xcode_config.h>
#elif defined(_MSC_VER)
#include <carve/vcpp_config.h>
#else
#include <carve/config.h>
#endif

#if defined(WIN32)
#include <carve/win32.h>
#endif

#include <math.h>

#include <iomanip>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <carve/collection.hpp>

#include <carve/util.hpp>

#include <stdarg.h>

#define STR(x) #x
#define XSTR(x) STR(x)

/**
 * \brief Top level Carve namespace.
 */
namespace carve {
static struct noinit_t {
} NOINIT;

inline std::string fmtstring(const char* fmt, ...);

/**
 * \brief Base class for all Carve exceptions.
 */
struct exception {
 private:
  mutable std::string err;
  mutable std::ostringstream accum;

 public:
  exception(const std::string& e) : err(e), accum() {}
  exception() : err(), accum() {}
  exception(const exception& e) : err(e.str()), accum() {}
  exception& operator=(const exception& e) {
    if (this != &e) {
      err = e.str();
      accum.str("");
    }
    return *this;
  }

  const std::string& str() const {
    if (accum.str().size() > 0) {
      err += accum.str();
      accum.str("");
    }
    return err;
  }

  template <typename T>
  exception& operator<<(const T& t) {
    accum << t;
    return *this;
  }
};

template <typename iter_t,
          typename order_t =
              std::less<typename std::iterator_traits<iter_t>::value_type> >
struct index_sort {
  iter_t base;
  order_t order;
  index_sort(const iter_t& _base) : base(_base), order() {}
  index_sort(const iter_t& _base, const order_t& _order)
      : base(_base), order(_order) {}
  template <typename U>
  bool operator()(const U& a, const U& b) {
    return order(*(base + a), *(base + b));
  }
};

template <typename iter_t, typename order_t>
index_sort<iter_t, order_t> make_index_sort(const iter_t& base,
                                            const order_t& order) {
  return index_sort<iter_t, order_t>(base, order);
}

template <typename iter_t>
index_sort<iter_t> make_index_sort(const iter_t& base) {
  return index_sort<iter_t>(base);
}

enum RayIntersectionClass {
  RR_DEGENERATE = -2,
  RR_PARALLEL = -1,
  RR_NO_INTERSECTION = 0,
  RR_INTERSECTION = 1
};

enum LineIntersectionClass {
  COLINEAR = -1,
  NO_INTERSECTION = 0,
  INTERSECTION_LL = 1,
  INTERSECTION_PL = 2,
  INTERSECTION_LP = 3,
  INTERSECTION_PP = 4
};

enum PointClass {
  POINT_UNK = -2,
  POINT_OUT = -1,
  POINT_ON = 0,
  POINT_IN = 1,
  POINT_VERTEX = 2,
  POINT_EDGE = 3
};

enum IntersectionClass {
  INTERSECT_BAD = -1,
  INTERSECT_NONE = 0,
  INTERSECT_FACE = 1,
  INTERSECT_VERTEX = 2,
  INTERSECT_EDGE = 3,
  INTERSECT_PLANE = 4,
};

extern double EPSILON;
extern double EPSILON2;

static inline void setEpsilon(double ep) {
  EPSILON = ep;
  EPSILON2 = ep * ep;
}

template <typename T>
struct identity_t {
  typedef T argument_type;
  typedef T result_type;
  const T& operator()(const T& t) const { return t; }
};

struct hash_pair {
  template <typename pair_t>
  size_t operator()(const pair_t& pair) const {
    size_t r = std::hash<typename pair_t::first_type>{}(pair.first);
    size_t s = std::hash<typename pair_t::second_type>()(pair.second);
    return r ^ ((s >> 16) | (s << 16));
  }
};

template <typename iter_t>
inline bool is_sorted(iter_t first, iter_t last) {
  if (first == last) {
    return true;
  }

  iter_t iter = first;
  iter_t next = first;
  ++next;
  for (; next != last; iter = next, ++next) {
    if (*next < *iter) {
      return false;
    }
  }
  return true;
}

template <typename iter_t, typename pred_t>
inline bool is_sorted(iter_t first, iter_t last, pred_t pred) {
  if (first == last) {
    return true;
  }

  iter_t iter = first;
  iter_t next = first;
  ++next;
  for (; next != last; iter = next, ++next) {
    if (pred(*next, *iter)) {
      return false;
    }
  }
  return true;
}

inline double rangeSeparation(const std::pair<double, double>& a,
                              const std::pair<double, double>& b) {
  if (a.second < b.first) {
    return b.first - a.second;
  } else if (b.second < a.first) {
    return a.first - b.second;
  } else {
    return 0.0;
  }
}
}  // namespace carve

#if defined(_MSC_VER)
#define MACRO_BEGIN do {
#define MACRO_END                                           \
  __pragma(warning(push)) __pragma(warning(disable : 4127)) \
  }                                                         \
  while (0)                                                 \
  __pragma(warning(pop))
#else
#define MACRO_BEGIN do {
#define MACRO_END \
  }               \
  while (0)
#endif

#if !defined(CARVE_NODEBUG)
#define CARVE_ASSERT(x)                                                     \
  MACRO_BEGIN if (!(x)) throw carve::exception() << __FILE__ << ":"         \
                                                 << __LINE__ << "  " << #x; \
  MACRO_END
#else
#define CARVE_ASSERT(X)
#endif

#define CARVE_FAIL(x)                                                         \
  MACRO_BEGIN throw carve::exception() << __FILE__ << ":" << __LINE__ << "  " \
                                       << #x;                                 \
  MACRO_END
