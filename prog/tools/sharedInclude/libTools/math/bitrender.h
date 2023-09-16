#ifndef __DAGOR_BITRENDER_H
#define __DAGOR_BITRENDER_H
#pragma once


#include <math/dag_mathBase.h>

#define FLT_ERROR 1e-4f

template <class Shader>
class Bitrender
{
public:
  void init(int wd, int ht, Shader *sh)
  {
    width = wd;
    height = ht;
    shader = sh;
  }

  inline void draw_indexed_triangle(const Point2 *v, int v0, int v1, int v2)
  {
    if (v[v0].y > v[v1].y)
      swap(v0, v1);
    if (v[v0].y > v[v2].y)
      swap(v0, v2);
    if (v[v1].y > v[v2].y)
      swap(v1, v2);
    draw_ordered_indexed_triangle(v0, v1, v2, v);
  }

  inline void draw_triangle(const Point2 *v) { draw_indexed_triangle(v, 0, 1, 2); }

private:
  int width, height;
  Shader *shader;

  template <class T>
  static inline void swap(T &a, T &b)
  {
    T c = a;
    a = b;
    b = c;
  }

  template <int left>
  inline void scan(int y, float x[2])
  {
    int xs = (int)floorf(x[left]), xe = (int)floorf(x[1 - left]);
    if (xe < 0 || xs > width || xs == xe)
      return;
    xe = min(xe, width);
    xs = max(xs, 0);
    shader->shade(y, xs, xe - xs);
  }

  template <int left>
  inline bool scanY(int starty, int endy, float x[2], float dx[2])
  {
    if (endy < 0)
    {
      int ht = (endy - starty);
      x[1] += dx[1] * ht;
      return true;
    }
    if (starty >= height)
      return false;
    if (endy > height)
      endy = height;
    if (starty < 0)
    {
      float sy = -float(starty);
      x[0] += dx[0] * sy;
      x[1] += dx[1] * sy;
      starty = 0;
    }

    for (int y = starty; y < endy; y++)
    {
      scan<left>(y, x);
      x[0] += dx[0];
      x[1] += dx[1];
    }
    return true;
  }

  template <int left>
  inline void draw_ordered_triangle(const Point2 *v, const Point2 &bt, const Point2 &mt, int top, int mid, int bot)
  {
    int starty, endy;
    float dx[2], x[2];
    float invBTY = fsel(bt.y - FLT_ERROR, 1.0f / bt.y, 1.0f);
    dx[1] = bt.x * invBTY;

    x[0] = x[1] = v[top].x;

    endy = (int)floorf(v[mid].y);
    starty = (int)floorf(v[top].y);
    if (starty != endy)
    {
      float invMTY = fsel(mt.y - FLT_ERROR, 1.0f / mt.y, 1.0f);
      dx[0] = mt.x * invMTY;

      if (!scanY<left>(starty, endy, x, dx))
        return;
    }

    Point2 bm = v[bot] - v[mid];
    float invBMY = fsel(bm.y - FLT_ERROR, 1.0f / bm.y, 1.0f);
    dx[0] = bm.x * invBMY;
    x[0] = v[mid].x;

    starty = endy;
    endy = (int)floorf(v[bot].y);
    if (starty != endy)
      scanY<left>(starty, endy, x, dx);
  }

  inline void draw_ordered_indexed_triangle(int top, int mid, int bot, const Point2 *v)
  {
    Point2 bt = v[bot] - v[top];
    Point2 mt = v[mid] - v[top];
    float val = bt.x * mt.y - bt.y * mt.x;
    if (val > 0)
      draw_ordered_triangle<0>(v, bt, mt, top, mid, bot);
    else
      draw_ordered_triangle<1>(v, bt, mt, top, mid, bot);
  }
};

#undef FLT_ERROR

#endif
