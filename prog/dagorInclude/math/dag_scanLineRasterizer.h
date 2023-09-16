//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint4.h>
#include <generic/dag_carray.h>
#include <vecmath/dag_vecMath.h>
#include <EASTL/utility.h>

// Shader has one member shade(Pixels at, Interpolant)
// Interpolant has operator *(float), operator +(Inerpolator), operator -(Inerpolator)
// Pixels has operator +(int). And whatever needed to Shader. Typically, is a pointer to FB
struct EmptyInterpolant
{
  EmptyInterpolant &operator*(float) { return *this; }
  EmptyInterpolant operator+(const EmptyInterpolant &) const { return *this; }
  EmptyInterpolant operator+=(const EmptyInterpolant &) { return *this; }
  EmptyInterpolant operator-(const EmptyInterpolant &) const { return *this; }
  EmptyInterpolant operator-=(const EmptyInterpolant &) { return *this; }
};

enum
{
  SR_PRECISE_BITS = 8,
  SR_PRECISE_VALUE = 1 << SR_PRECISE_BITS
};
#define SR_PRECISE_VALUEF (float(SR_PRECISE_VALUE))

template <class Shader, class Interpolant, class Pixels, int Width, int Height>
class ScanLineRasterizer
{
public:
  Pixels pixels;
  void init(Pixels memory) { pixels = memory; }

  inline void draw_triangle_nocull(const Point2 *v, const Interpolant *interp) { draw_indexed_triangle_nocull(v, interp, 0, 1, 2); }

  inline void draw_indexed_triangle_nocull(const Point2 *v, const Interpolant *interp, int v0, int v1, int v2)
  {
    G_UNREFERENCED(v0);
    G_UNREFERENCED(v1);
    G_UNREFERENCED(v2);
    IPoint2 vi[3];
    vi[0] = ipoint2(v[0] * SR_PRECISE_VALUE);
    vi[1] = ipoint2(v[1] * SR_PRECISE_VALUE);
    vi[2] = ipoint2(v[2] * SR_PRECISE_VALUE);
    draw_indexed_trianglei<true, IPoint2>(vi, interp, 0, 1, 2);
  }

  template <bool noCull, class IntegerPoint>
  inline void draw_indexed_trianglei(const IntegerPoint *vi, const Interpolant *interp, int v0, int v1, int v2)
  {
    uint32_t cullMask = 0; // 1<<31
    if (vi[v0].y > vi[v1].y)
    {
      eastl::swap(v0, v1);
      cullMask ^= (1 << 31);
    }
    if (vi[v0].y > vi[v2].y)
    {
      eastl::swap(v0, v2);
      cullMask ^= (1 << 31);
    }
    if (vi[v1].y > vi[v2].y)
    {
      eastl::swap(v1, v2);
      cullMask ^= (1 << 31);
    }
    if (noCull)
      draw_ordered_indexed_triangle_nocull<IntegerPoint>(v0, v1, v2, vi, interp);
    else
      draw_ordered_indexed_triangle_cull<IntegerPoint>(cullMask, v0, v1, v2, vi, interp);
  }

  template <class IntegerPoint>
  inline void draw_ordered_indexed_triangle_cull(uint32_t cullMask, int top, int mid, int bot, const IntegerPoint *vi,
    const Interpolant *interp)
  {
    IPoint2 bt = IPoint2(vi[bot].x - vi[top].x, vi[bot].y - vi[top].y);
    IPoint2 mt = IPoint2(vi[mid].x - vi[top].x, vi[mid].y - vi[top].y);
    int val = (bt.x >> SR_PRECISE_BITS) * (mt.y >> SR_PRECISE_BITS) - (bt.y >> SR_PRECISE_BITS) * (mt.x >> SR_PRECISE_BITS); // no cull
    if ((val & (1 << 31)) != cullMask)
      return;
    if (val > 0)
      draw_ordered_triangle<0>(vi, interp, bt, mt, top, mid, bot);
    else
      draw_ordered_triangle<1>(vi, interp, bt, mt, top, mid, bot);
  }

  template <class IntegerPoint>
  inline void draw_ordered_indexed_triangle_nocull(int top, int mid, int bot, const IntegerPoint *vi, const Interpolant *interp)
  {
    IPoint2 bt = IPoint2(vi[bot].x - vi[top].x, vi[bot].y - vi[top].y);
    IPoint2 mt = IPoint2(vi[mid].x - vi[top].x, vi[mid].y - vi[top].y);
    int val = (bt.x >> SR_PRECISE_BITS) * (mt.y >> SR_PRECISE_BITS) - (bt.y >> SR_PRECISE_BITS) * (mt.x >> SR_PRECISE_BITS); // no cull
    if (val > 0)
      draw_ordered_triangle<0>(vi, interp, bt, mt, top, mid, bot);
    else
      draw_ordered_triangle<1>(vi, interp, bt, mt, top, mid, bot);
  }

private:
  template <int left>
  inline void scan(int x[2], Interpolant z[2], Pixels pix)
  {
    int xs = x[left] >> SR_PRECISE_BITS, xe = x[1 - left] >> SR_PRECISE_BITS;
    if (xe < 0 || xs >= Width || xs == xe)
      return;
    xe = min(xe, Width);

    float invX = (x[1 - left] == x[left]) ? 1.0f : SR_PRECISE_VALUEF / (x[1] - x[0]);
    Interpolant dz = (z[1] - z[0]) * invX;
    Interpolant z0 = z[left];

    if (xs < 0)
    {
      z0 -= dz * xs;
      xs = 0;
    }
    pix += xs;
    // Shader::shade(pix, xe-xs);
    for (int ix = xs; ix < xe; ++ix, pix += 1, z0 += dz)
      Shader::shade(pix, z0);
  }

  template <int left>
  inline bool scanY(int starty, int endy, int x[2], int dx[2], Interpolant z[2], Interpolant dz[2], Pixels &pix)
  {
    if (endy < 0)
    {
      int ht = (endy - starty);
      pix += Width * ht;
      z[1] += dz[1] * ht;
      x[1] += dx[1] * ht;
      return true;
    }
    // if (starty >= Height)
    //   return false;
    if (endy > Height)
      endy = Height;
    if (starty < 0)
    {
      x[0] -= dx[0] * starty;
      x[1] -= dx[1] * starty;
      pix += -starty * Width;
      float sy = -float(starty);
      z[0] = z[0] + dz[0] * sy;
      z[1] = z[1] + dz[1] * sy;
      starty = 0;
    }

    int ht = (endy - starty);
    for (int y = 0; y < ht; y++)
    {
      scan<left>(x, z, pix);
      x[0] += dx[0];
      x[1] += dx[1];
      pix += Width;

      z[0] = z[0] + dz[0];
      z[1] = z[1] + dz[1];
    }
    return true;
  }

  template <int left, class IntegerPoint>
  inline void draw_ordered_triangle(const IntegerPoint *v, const Interpolant *interp, const IPoint2 &bt, const IPoint2 &mt, int top,
    int mid, int bot)
  {
    int starty, endy;
    starty = v[top].y >> SR_PRECISE_BITS;
    if (starty >= Height || v[bot].y < 0)
      return;

    int dx[2], x[2];
    Interpolant z[2], dz[2];
    dx[1] = bt.y == 0 ? bt.x : bt.x * SR_PRECISE_VALUE / bt.y;
    x[0] = x[1] = v[top].x;

    float invBTY = bt.y == 0 ? 1.0f : SR_PRECISE_VALUEF / bt.y;
    dz[1] = (interp[bot] - interp[top]) * invBTY;
    z[0] = z[1] = interp[top];

    endy = v[mid].y >> SR_PRECISE_BITS;
    Pixels pix = pixels;
    pix += starty * Width;
    if (starty != endy)
    {
      dx[0] = mt.y == 0 ? mt.x : mt.x * SR_PRECISE_VALUE / mt.y;

      float invMTY = mt.y == 0 ? 1.0f : SR_PRECISE_VALUEF / mt.y;
      dz[0] = (interp[mid] - interp[top]) * invMTY;

      scanY<left>(starty, endy, x, dx, z, dz, pix);
    }
    /////////////////////////////
    IPoint2 bm = IPoint2(v[bot].x - v[mid].x, v[bot].y - v[mid].y);
    dx[0] = bm.y == 0 ? bm.x : bm.x * SR_PRECISE_VALUE / bm.y;
    x[0] = v[mid].x;

    float invBMY = bm.y == 0 ? 1.0f : SR_PRECISE_VALUEF / bm.y;
    dz[0] = (interp[bot] - interp[mid]) * invBMY;
    z[0] = interp[mid];

    starty = endy;
    endy = v[bot].y >> SR_PRECISE_BITS;
    pix = pixels;
    pix += starty * Width;
    if (starty != endy)
      scanY<left>(starty, endy, x, dx, z, dz, pix);
  }
};

#undef FLT_ERROR


template <int Width, int Height>
class RenderSWNoInterp
{
public:
  enum
  {
    WIDTH = Width,
    HEIGHT = Height
  };
  typedef uint8_t DepthType;
  struct DepthFrameBuffer
  {
    DepthFrameBuffer() : fixedDepth(1) {}
    DepthFrameBuffer(DepthType *d) : depth(d), fixedDepth(1) {}
    DepthType *depth;
    DepthType fixedDepth;
    inline void operator+=(int ofs) { depth += ofs; }
  };

  EmptyInterpolant empty;

  RenderSWNoInterp()
  {
    resetDepth();
    scanline.init(DepthFrameBuffer(depth.data()));
    to_screen = v_make_vec4f(0.5f * (WIDTH << SR_PRECISE_BITS), 0.5f * (HEIGHT << SR_PRECISE_BITS), 0, 0);
  }
  ~RenderSWNoInterp() {}

  void resetDepth() { mem_set_ff(depth); }

  static inline void shade(const DepthFrameBuffer &pixel, const EmptyInterpolant &)
  {
    if (*pixel.depth > pixel.fixedDepth)
      *pixel.depth = pixel.fixedDepth;
  }
  ScanLineRasterizer<RenderSWNoInterp, EmptyInterpolant, DepthFrameBuffer, Width, Height> scanline;

  void triangle_scanline(const Point2 *v, DepthType new_depth)
  {
    scanline.pixels.fixedDepth = new_depth;
    scanline.draw_triangle_nocull(v, &empty);
  }
  template <bool noCull>
  void triangle_scanline(const IPoint4 *vi, int i0, int i1, int i2)
  {
    scanline.pixels.fixedDepth = max(max(vi[i0].w, vi[i1].w), vi[i2].w);
    scanline.template draw_indexed_trianglei<noCull, IPoint4>(vi, &empty, i0, i1, i2);
  }

  template <bool noCull, class Index>
  void renderMeshInternal(IPoint4 *__restrict transformedVert, mat44f_cref globtm, const vec4f *__restrict vertex, int vertCnt,
    const Index *__restrict indices, int numTri)
  {
    for (int i = 0; i < vertCnt; ++i)
    {
      vec4f transVert = v_mat44_mul_vec3p(globtm, vertex[i]);
      vec4f w = v_splat_w(transVert);
      transVert = v_div(transVert, w);
      if (v_test_vec_x_gt_0(w))
      {
        transVert = v_madd(transVert, to_screen, to_screen);
        vec4i transformedI = v_cvt_roundi(transVert);
        transformedI = v_seli(v_cast_vec4i(w), transformedI, V_CI_MASK1100);
        v_sti(&transformedVert[i], transformedI);
      }
      else
      {
        v_sti(&transformedVert[i], v_cast_vec4i(v_zero()));
      }
    }
    for (int i = 0; i < numTri; i++, indices += 3)
    {
      if (!transformedVert[indices[0]].w | !transformedVert[indices[1]].w | !transformedVert[indices[2]].w)
        continue;

      triangle_scanline<noCull>(transformedVert, indices[0], indices[1], indices[2]);
    }
  }
  template <class Index>
  void renderMesh(mat44f_cref globtm, const vec4f *__restrict vertex, int vertCnt, const Index *__restrict indices, int numTri)
  {
    transformed.resize(vertCnt);
    return renderMeshInternal<true, Index>(transformed.data(), globtm, vertex, vertCnt, indices, numTri);
  }
  const DepthType *getDepth() const { return depth.data(); }
  bool checkPixel(int x, int y) const { return depth[y * WIDTH + x] < 255; }
  bool checkPixelSafe(int x, int y) const { return x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT && checkPixel(x, y); }

protected:
  vec3f to_screen;
  carray<DepthType, WIDTH * HEIGHT> depth;
  Tab<IPoint4> transformed;
};
