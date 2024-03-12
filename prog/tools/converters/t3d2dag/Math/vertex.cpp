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

#include "vertex.h"

float &Vertex::operator[](const int index) { return *(&x + index); }

void Vertex::operator+=(const Vertex &v)
{
  x += v.x;
  y += v.y;
  z += v.z;
}

void Vertex::operator-=(const Vertex &v)
{
  x -= v.x;
  y -= v.y;
  z -= v.z;
}

void Vertex::operator*=(const float scalar)
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
}
void Vertex::operator/=(const float dividend)
{
  x /= dividend;
  y /= dividend;
  z /= dividend;
}

bool operator==(const Vertex &v0, const Vertex &v1) { return (v0.x == v1.x && v0.y == v1.y && v0.z == v1.z); }

bool operator!=(const Vertex &v0, const Vertex &v1) { return (v0.x != v1.x || v0.y != v1.y || v0.z != v1.z); }

void Vertex::normalize()
{
  float invLen = 1.0f / sqrtf(x * x + y * y + z * z);
  x *= invLen;
  y *= invLen;
  z *= invLen;
}

void Vertex::fastNormalize()
{
  float invLen = rsqrtf(x * x + y * y + z * z);
  x *= invLen;
  y *= invLen;
  z *= invLen;
}

/* ------------------------------------------------------------ */

Vertex operator+(const Vertex &u, const Vertex &v) { return Vertex(u.x + v.x, u.y + v.y, u.z + v.z); }

Vertex operator-(const Vertex &u, const Vertex &v) { return Vertex(u.x - v.x, u.y - v.y, u.z - v.z); }

Vertex operator-(const Vertex &v) { return Vertex(-v.x, -v.y, -v.z); }

float operator*(const Vertex &u, const Vertex &v) { return u.x * v.x + u.y * v.y + u.z * v.z; }

Vertex operator*(const float scalar, const Vertex &v) { return Vertex(scalar * v.x, scalar * v.y, scalar * v.z); }

Vertex operator*(const Vertex &v, const float scalar) { return Vertex(scalar * v.x, scalar * v.y, scalar * v.z); }


Vertex operator/(const Vertex &v, const float dividend) { return Vertex(v.x / dividend, v.y / dividend, v.z / dividend); }


Vertex cross(const Vertex &u, const Vertex &v) { return Vertex(u.y * v.z - v.y * u.z, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x); }

float dist(const Vertex &u, const Vertex &v) { return sqrtf(sqrf(u.x - v.x) + sqrf(u.y - v.y) + sqrf(u.z - v.z)); }

float length(const Vertex &v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

float fastLength(const Vertex &v)
{
  float lsqr = v.x * v.x + v.y * v.y + v.z * v.z;
  return lsqr * rsqrtf(lsqr);
}

float lengthSqr(const Vertex &v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

Vertex normalize(const Vertex &v)
{
  float invLen = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  return v * invLen;
}

Vertex fastNormalize(const Vertex &v)
{
  float invLen = rsqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  return v * invLen;
}

unsigned int getRGBANormal(Vertex v)
{
  unsigned int norm;
  int tmp;
  float lenSqr = lengthSqr(v);
  if (lenSqr > 1)
    v *= rsqrtf(lenSqr);

  tmp = int((v.x + 1) * 127.5f);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  norm = tmp;
  tmp = int((v.y + 1) * 127.5f);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  norm |= tmp << 8;
  tmp = int((v.z + 1) * 127.5f);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  norm |= tmp << 16;
  return norm;
}

unsigned int getBGRANormal(Vertex v)
{
  unsigned int norm;
  int tmp;
  float lenSqr = lengthSqr(v);
  if (lenSqr > 1)
    v *= rsqrtf(lenSqr);

  tmp = int((v.z + 1) * 127.5f);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  norm = tmp;
  tmp = int((v.y + 1) * 127.5f);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  norm |= tmp << 8;
  tmp = int((v.x + 1) * 127.5f);
  if (tmp < 0)
    tmp = 0;
  if (tmp > 255)
    tmp = 255;
  norm |= tmp << 16;
  return norm;
}
