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

#ifndef _QUATERNION_H_
#define _QUATERNION_H_

// #include "Matrix.h"

struct Quaternion
{
  float s, x, y, z;

  Quaternion() {}
  Quaternion(const float Wx, const float Wy);
  Quaternion(const float Wx, const float Wy, const float Wz);
  Quaternion(const float is, const float ix, const float iy, const float iz)
  {
    s = is;
    x = ix;
    y = iy;
    z = iz;
  }

  operator const float *() const { return &s; }

  void operator+=(const Quaternion &v);
  void operator-=(const Quaternion &v);
  void operator*=(const float scalar);
  void operator/=(const float dividend);
  void normalize();
  void fastNormalize();
};

Quaternion operator+(const Quaternion &u, const Quaternion &v);
Quaternion operator-(const Quaternion &u, const Quaternion &v);
Quaternion operator-(const Quaternion &v);
Quaternion operator*(const Quaternion &u, const Quaternion &v);
Quaternion operator*(const float scalar, const Quaternion &v);
Quaternion operator*(const Quaternion &v, const float scalar);
Quaternion operator/(const Quaternion &v, const float dividend);

Quaternion slerp(const Quaternion &q0, const Quaternion &q1, const float t);
Quaternion scerp(const Quaternion &q0, const Quaternion &q1, const Quaternion &q2, const Quaternion &q3, const float t);


#endif // _QUATERNION_H_
