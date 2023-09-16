#ifndef __GAIJIN_EDITORCORE_EC_RECT_H__
#define __GAIJIN_EDITORCORE_EC_RECT_H__
#pragma once


struct EcRect
{
  int l, t, r, b;

  int operator!=(EcRect &r2)
  {
    if (l != r2.l || t != r2.t || r != r2.r || b != r2.b)
      return 1;

    return 0;
  }

  int operator==(EcRect &r2)
  {
    if (l == r2.l && t == r2.t && r == r2.r && b == r2.b)
      return 1;

    return 0;
  }

  int width() const { return r - l; }
  int height() const { return b - t; }
};


#endif