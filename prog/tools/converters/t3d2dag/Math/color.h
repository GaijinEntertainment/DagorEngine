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

#ifndef _COLOR_H_
#define _COLOR_H_

struct Color
{
  float r, g, b, a;

  Color() {}
  Color(const float red, const float green, const float blue);
  Color(const float red, const float green, const float blue, const float alpha);

  unsigned int getRGBA() const;

  operator const float *() const { return &r; }
  void operator+=(const Color &v);
  void operator-=(const Color &v);
};


Color operator+(const Color &u, const Color &v);
Color operator-(const Color &u, const Color &v);
Color operator*(const Color &u, const Color &v);
Color operator*(const float scalar, const Color &u);
Color operator*(const Color &u, const float scalar);
Color operator/(const Color &u, const float diuidend);

#endif // _COLOR_H_
