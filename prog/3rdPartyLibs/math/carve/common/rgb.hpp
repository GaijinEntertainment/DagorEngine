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

#include <functional>

struct cRGB {
  typedef float value_type;
  value_type r, g, b;

  cRGB() : r(0), g(0), b(0) {}

  cRGB(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
};

struct cRGBA {
  typedef float value_type;
  value_type r, g, b, a;

  cRGBA() : r(0), g(0), b(0), a(1) {}

  cRGBA(float _r, float _g, float _b, float _a = 1.0f)
      : r(_r), g(_g), b(_b), a(_a) {}

  cRGBA(const cRGB& rgb) : r(rgb.r), g(rgb.g), b(rgb.b), a(1) {}
};

static inline cRGB operator+(const cRGB& a, const cRGB& b) {
  return cRGB(a.r + b.r, a.g + b.g, a.b + b.b);
}
static inline cRGB& operator+=(cRGB& a, const cRGB& b) {
  a.r += b.r;
  a.g += b.g;
  a.b += b.b;
  return a;
}

static inline cRGB operator*(float s, const cRGB& a) {
  return cRGB(s * a.r, s * a.g, s * a.b);
}

static inline cRGBA operator+(const cRGBA& a, const cRGBA& b) {
  return cRGBA(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}
static inline cRGBA& operator+=(cRGBA& a, const cRGBA& b) {
  a.r += b.r;
  a.g += b.g;
  a.b += b.b;
  a.a += b.a;
  return a;
}

static inline cRGBA operator*(float s, const cRGBA& a) {
  return cRGBA(s * a.r, s * a.g, s * a.b, s * a.a);
}

static inline cRGB HSV2RGB(float H, float S, float V) {
  H = 6.0f * H;
  if (S < 5.0e-6) {
    cRGB(V, V, V);
  } else {
    int i = (int)H;
    float f = H - i;
    float p1 = V * (1.0f - S);
    float p2 = V * (1.0f - S * f);
    float p3 = V * (1.0f - S * (1.0f - f));
    switch (i) {
      case 0:
        return cRGB(V, p3, p1);
      case 1:
        return cRGB(p2, V, p1);
      case 2:
        return cRGB(p1, V, p3);
      case 3:
        return cRGB(p1, p2, V);
      case 4:
        return cRGB(p3, p1, V);
      case 5:
        return cRGB(V, p1, p2);
    }
  }
  return cRGB(0, 0, 0);
}

struct colour_clamp_t {
  cRGB operator()(const cRGB& c) const {
    return cRGB(std::min(std::max(c.r, 0.0f), 1.0f),
                std::min(std::max(c.g, 0.0f), 1.0f),
                std::min(std::max(c.b, 0.0f), 1.0f));
  }
  cRGBA operator()(const cRGBA& c) const {
    return cRGBA(std::min(std::max(c.r, 0.0f), 1.0f),
                 std::min(std::max(c.g, 0.0f), 1.0f),
                 std::min(std::max(c.b, 0.0f), 1.0f),
                 std::min(std::max(c.a, 0.0f), 1.0f));
  }
};
