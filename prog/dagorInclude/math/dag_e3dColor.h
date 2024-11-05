//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>

/// @addtogroup baseclasses
/// @{


/// @addtogroup colors
/// @{


/// @file
/// Defines E3DCOLOR type and corresponding macros.

/// 32-bit BGRA color (8 bits per component).
struct E3DCOLOR
{
  union
  {
    unsigned int u;
    struct
    {
      unsigned char b, g, r, a;
    };
  };

  E3DCOLOR() = default;
  E3DCOLOR(unsigned int c) { u = c; }
  E3DCOLOR(unsigned char R, unsigned char G, unsigned char B, unsigned char A = 255)
  {
    b = B;
    g = G;
    r = R;
    a = A;
  }
  operator unsigned int() const { return u; }
};

#define E3DCOLOR_MAKE(r, g, b, a)         ((E3DCOLOR)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define E3DCOLOR_MAKE_SWAPPED(r, g, b, a) ((E3DCOLOR)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))

inline void SWAP_RB(E3DCOLOR &c)
{
  unsigned char a = c.b;
  c.b = c.r;
  c.r = a;
}

// NTSC brightness Weights: r=.299 g=.587 b=.114
inline float brightness(E3DCOLOR c) { return (c.r * .299f + c.g * .587f + c.b * .114) / 255.0f; }

/// @}

/// @}

inline E3DCOLOR e3dcolor_lerp(E3DCOLOR a, E3DCOLOR b, float t)
{
  return E3DCOLOR_MAKE((int)lerp((float)a.r, (float)b.r, t), (int)lerp((float)a.g, (float)b.g, t),
    (int)lerp((float)a.b, (float)b.b, t), (int)lerp((float)a.a, (float)b.a, t));
}

inline E3DCOLOR e3dcolorLerp(E3DCOLOR a, E3DCOLOR b, float t) // deprecated
{
  return e3dcolor_lerp(a, b, t);
}

inline E3DCOLOR e3dcolor_mul(E3DCOLOR a, E3DCOLOR b)
{
  return E3DCOLOR_MAKE((int)a.r * b.r / 255, (int)a.g * b.g / 255, (int)a.b * b.b / 255, (int)a.a * b.a / 255);
}
