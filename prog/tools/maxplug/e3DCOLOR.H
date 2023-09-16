// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __DAGOR_E3DCOLOR_H__
#define __DAGOR_E3DCOLOR_H__

#pragma once


/// @addtogroup baseclasses
/// @{


/// @addtogroup colors
/// @{


/// @file
/// Defines E3DCOLOR type and corresponding macros.

#ifdef __Dagor_Script__
#define unsigned int
#define E3DCOLOR unsigned
#else
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

  E3DCOLOR() {}
  E3DCOLOR(unsigned int c) { u = c; }
  E3DCOLOR(unsigned char R, unsigned char G, unsigned char B, unsigned char A = 255)
  {
    b = B;
    g = G;
    r = R;
    a = A;
  }
  operator unsigned int() { return u; }
#if defined _XBOX
#pragma warning(disable : 858) // type qualifier on return type is meaningless
  operator const unsigned int() const { return u; }
#pragma warning(default : 858) // type qualifier on return type is meaningless
#else
  operator const unsigned int() const { return u; }
#endif
};
#endif

#define E3DCOLOR_GETA(rgb)                ((rgb) >> 24)
#define E3DCOLOR_GETR(rgb)                (((rgb) >> 16) & 0xff)
#define E3DCOLOR_GETG(rgb)                (((rgb) >> 8) & 0xff)
#define E3DCOLOR_GETB(rgb)                ((rgb)&0xff)
#define E3DCOLOR_MAKE(r, g, b, a)         ((E3DCOLOR)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define E3DCOLOR_MAKE_SWAPPED(r, g, b, a) ((E3DCOLOR)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))

#define E3DCOLOR_SETA(rgb, v) ((rgb) = ((rgb)&0x00FFFFFF) | ((v) << 24))
#define E3DCOLOR_SETR(rgb, v) ((rgb) = ((rgb)&0xFF00FFFF) | ((v) << 16))
#define E3DCOLOR_SETG(rgb, v) ((rgb) = ((rgb)&0xFFFF00FF) | ((v) << 8))
#define E3DCOLOR_SETB(rgb, v) ((rgb) = ((rgb)&0xFFFFFF00) | ((v)))

#ifdef __Dagor_Script__

inline void SWAP_RB(E3DCOLOR &c)
{
  int a = E3DCOLOR_GETB(c);
  E3DCOLOR_SETB(c, E3DCOLOR_GETR(c));
  E3DCOLOR_SETR(c, a);
}
#else

inline void SWAP_RB(E3DCOLOR &c)
{
  unsigned char a = c.b;
  c.b = c.r;
  c.r = a;
}

#endif

/// @}

/// @}

#endif