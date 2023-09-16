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

#ifndef _VERTEX_H_
#define _VERTEX_H_

#include "myMath.h"

#define Vector Vertex

// The Vertex structure defines a single point in space or a vector
struct Vertex
{
  float x, y, z;
  Vertex() {}
  Vertex(const float ix, const float iy, const float iz)
  {
    x = ix;
    y = iy;
    z = iz;
  }
  operator const float *() const { return &x; }
  float &operator[](const int index);
  void operator+=(const Vertex &v);
  void operator-=(const Vertex &v);
  void operator*=(const float scalar);
  void operator/=(const float dividend);
  void normalize();
  void fastNormalize();
};

bool operator==(const Vertex &v0, const Vertex &v1);
bool operator!=(const Vertex &v0, const Vertex &v1);

Vertex operator+(const Vertex &u, const Vertex &v);
Vertex operator-(const Vertex &u, const Vertex &v);
Vertex operator-(const Vertex &v);
float operator*(const Vertex &u, const Vertex &v);
Vertex operator*(const float scalar, const Vertex &v);
Vertex operator*(const Vertex &v, const float scalar);
Vertex operator/(const Vertex &v, const float dividend);

Vertex cross(const Vertex &u, const Vertex &v);
float dist(const Vertex &u, const Vertex &v);
float length(const Vertex &v);
float fastLength(const Vertex &v);
float lengthSqr(const Vertex &v);
Vertex normalize(const Vertex &v);
Vertex fastNormalize(const Vertex &v);

unsigned int getRGBANormal(Vertex v);
unsigned int getBGRANormal(Vertex v);

#endif
