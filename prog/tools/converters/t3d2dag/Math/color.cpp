/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#include "color.h"

Color::Color(const float red, const float green, const float blue)
{
  r = red;
  g = green;
  b = blue;
  a = 1.0f;
}

Color::Color(const float red, const float green, const float blue, const float alpha)
{
  r = red;
  g = green;
  b = blue;
  a = alpha;
}

unsigned int Color::getRGBA() const
{
  unsigned int col;
  int tmp;

  tmp = int(r * 255);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  col = tmp;
  tmp = int(g * 255);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  col |= tmp << 8;
  tmp = int(b * 255);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  col |= tmp << 16;
  tmp = int(a * 255);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  col |= tmp << 24;
  return col;
}

void Color::operator+=(const Color &v)
{
  r += v.r;
  g += v.g;
  b += v.b;
  a += v.a;
}

void Color::operator-=(const Color &v)
{
  r -= v.r;
  g -= v.g;
  b -= v.b;
  a -= v.a;
}


Color operator+(const Color &u, const Color &v) { return Color(u.r + v.r, u.g + v.g, u.b + v.b, u.a + v.a); }

Color operator-(const Color &u, const Color &v) { return Color(u.r - v.r, u.g - v.g, u.b - v.b, u.a - v.a); }

Color operator*(const Color &u, const Color &v) { return Color(u.r * v.r, u.g * v.g, u.b * v.b, u.a * v.a); }

Color operator*(const float scalar, const Color &u) { return Color(scalar * u.r, scalar * u.g, scalar * u.b, scalar * u.a); }

Color operator*(const Color &u, const float scalar) { return Color(scalar * u.r, scalar * u.g, scalar * u.b, scalar * u.a); }

Color operator/(const Color &u, const float diuidend) { return Color(u.r / diuidend, u.g / diuidend, u.b / diuidend, u.a / diuidend); }
