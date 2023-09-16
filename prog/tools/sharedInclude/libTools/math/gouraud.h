#ifndef __DAGOR_GOUARUD_H
#define __DAGOR_GOUARUD_H
#pragma once
#include <math/dag_mathBase.h>

#define FLT_ERROR 1e-4f
// Shader has one member shade(Pixels at, Interpolatant)
// Interpolatant has operator *(float), operator +(Inerpolator), operator -(Inerpolator)
// Pixels has operator +(int). And whatever needed to Shader. Typically, is a pointer to FB
template <class Shader, class Interpolatant, class Pixels>
class Gouraud
{
public:
  void init(Pixels memory, int wd, int ht)
  {
    pixels = memory;
    width = wd;
    height = ht;
  }
  inline void draw_indexed_triangle(const Point2 *v, const Interpolatant *interp, int v0, int v1, int v2)
  {
    if (v[v0].y > v[v1].y)
      swap(v0, v1);
    if (v[v0].y > v[v2].y)
      swap(v0, v2);
    if (v[v1].y > v[v2].y)
      swap(v1, v2);
    draw_ordered_indexed_triangle(v0, v1, v2, v, interp);
  }


  inline void draw_triangle(const Point2 *v, const Interpolatant *interp) { draw_indexed_triangle(v, interp, 0, 1, 2); }

private:
  Pixels pixels;
  int width, height;

  template <class T>
  void inline swap(T &a, T &b)
  {
    T c = a;
    a = b;
    b = c;
  }

  template <int left>
  inline void scan(float x[2], Interpolatant z[2], Pixels pix)
  {
    int xs = (int)floorf(x[left]), xe = (int)floorf(x[1 - left]);
    if (xe < 0 || xs > width || xs == xe)
      return;
    xe = min(xe, width);
    float invX = fsel(x[1 - left] - x[left] - FLT_ERROR, 1.0f / (x[1] - x[0]), 1.0f);
    Interpolatant dz = (z[1] - z[0]) * invX;
    Interpolatant z0 = z[left];
    if (xs < 0)
    {
      z0 -= dz * xs;
      xs = 0;
    }
    pix += xs;
    for (int x = xs; x < xe; ++x, pix += 1, z0 += dz)
    {
      Shader::shade(pix, z0);
      /*//pix->b=(int)z0;
      int yo = (pix-pixels)/width;
      int xo = (pix-pixels)%width;
      if (xo <0 || yo<0 || xo>=width || yo>=height)
        printf("a\n");
        ;//printf("%d %d\n", xo, yo);*/
    }
  }

  template <int left>
  inline bool scanY(int starty, int endy, float x[2], float dx[2], Interpolatant z[2], Interpolatant dz[2], Pixels &pix)
  {
    if (endy < 0)
    {
      int ht = (endy - starty);
      pix += width * ht;
      z[1] += dz[1] * ht;
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
      z[0] = z[0] + dz[0] * sy;
      z[1] = z[1] + dz[1] * sy;
      x[0] += dx[0] * sy;
      x[1] += dx[1] * sy;
      pix += -starty * width;
      starty = 0;
    }

    int ht = (endy - starty);
    /*if (OPENMP && ht>3)
    {
      #pragma omp parallel for
        for (int y = 0; y<ht; y++)
        {
          float yf = float(y);
          float x2[2] = {x[0]+dx[0]*yf, x[1]+dx[1]*yf};
          Interpolatant z2[2] = {z[0]+dz[0]*yf, z[1]+dz[1]*yf};
          scan<left>(x2, z2, pix+y*width);
        }
        pix = pix + width*ht;
        float htf = float(ht);
        z[0] = z[0] + htf*dz[0];
        z[1] = z[1] + htf*dz[1];
        x[0] += htf*dx[0];
        x[1] += htf*dx[1];
    } else
    {*/
    for (int y = 0; y < ht; y++)
    {
      scan<left>(x, z, pix);
      z[0] = z[0] + dz[0];
      z[1] = z[1] + dz[1];
      x[0] += dx[0];
      x[1] += dx[1];
      pix += width;
    }
    //}
    return true;
  }

  template <int left>
  inline void draw_ordered_triangle(const Point2 *v, const Interpolatant *interp, const Point2 &bt, const Point2 &mt, int top, int mid,
    int bot)
  {
    int starty, endy;
    float dx[2], x[2];
    Interpolatant z[2], dz[2];
    float invBTY = fsel(bt.y - FLT_ERROR, 1.0f / bt.y, 1.0f);
    dx[1] = bt.x * invBTY;
    dz[1] = (interp[bot] - interp[top]) * invBTY;

    x[0] = x[1] = v[top].x;
    z[0] = z[1] = interp[top];

    endy = (int)floorf(v[mid].y);
    starty = (int)floorf(v[top].y);
    Pixels pix = pixels;
    pix += starty * width;
    if (starty != endy)
    {
      float invMTY = fsel(mt.y - FLT_ERROR, 1.0f / mt.y, 1.0f);
      dx[0] = mt.x * invMTY;
      dz[0] = (interp[mid] - interp[top]) * invMTY;

      if (!scanY<left>(starty, endy, x, dx, z, dz, pix))
        return;
    }
    /////////////////////////////
    Point2 bm = v[bot] - v[mid];
    float invBMY = fsel(bm.y - FLT_ERROR, 1.0f / bm.y, 1.0f);
    dx[0] = bm.x * invBMY;
    x[0] = v[mid].x;

    dz[0] = (interp[bot] - interp[mid]) * invBMY;
    z[0] = interp[mid];
    starty = endy;
    endy = (int)floorf(v[bot].y);
    if (starty != endy)
      scanY<left>(starty, endy, x, dx, z, dz, pix);
  }

  inline void draw_ordered_indexed_triangle(int top, int mid, int bot, const Point2 *v, const Interpolatant *interp)
  {
    Point2 bt = v[bot] - v[top];
    Point2 mt = v[mid] - v[top];
    float val = bt.x * mt.y - bt.y * mt.x;
    if (val > 0)
      draw_ordered_triangle<0>(v, interp, bt, mt, top, mid, bot);
    else
      draw_ordered_triangle<1>(v, interp, bt, mt, top, mid, bot);
  }
};

#undef FLT_ERROR

#endif