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

namespace carve {
namespace colour {
static inline void HSV2RGB(float H, float S, float V, float& r, float& g,
                           float& b) {
  H = 6.0f * H;
  if (S < 5.0e-6) {
    r = g = b = V;
    return;
  } else {
    int i = (int)H;
    float f = H - i;
    float p1 = V * (1.0f - S);
    float p2 = V * (1.0f - S * f);
    float p3 = V * (1.0f - S * (1.0f - f));
    switch (i) {
      case 0:
        r = V;
        g = p3;
        b = p1;
        return;
      case 1:
        r = p2;
        g = V;
        b = p1;
        return;
      case 2:
        r = p1;
        g = V;
        b = p3;
        return;
      case 3:
        r = p1;
        g = p2;
        b = V;
        return;
      case 4:
        r = p3;
        g = p1;
        b = V;
        return;
      case 5:
        r = V;
        g = p1;
        b = p2;
        return;
    }
  }
  r = g = b = 0.0;
}
}  // namespace colour
}  // namespace carve
