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

#include "quaternion.h"
#include "myMath.h"

Quaternion::Quaternion(const float Wx, const float Wy)
{
  float cx = cosf(Wx * 0.5f), sx = sinf(Wx * 0.5f), cy = cosf(Wy * 0.5f), sy = sinf(Wy * 0.5f);


  s = cx * cy;
  x = -cy * sx;
  y = cx * sy;
  z = sx * sy;
}

Quaternion::Quaternion(const float Wx, const float Wy, const float Wz)
{
  float cx = cosf(Wx * 0.5f), sx = sinf(Wx * 0.5f), cy = cosf(Wy * 0.5f), sy = sinf(Wy * 0.5f), cz = cosf(Wz * 0.5f),
        sz = sinf(Wz * 0.5f);

  s = cx * cy * cz + sx * sy * sz;
  x = -sx * cy * cz - cx * sy * sz;
  y = cx * sy * cz - sx * cy * sz;
  z = sx * sy * cz - cx * cy * sz;
}

void Quaternion::operator+=(const Quaternion &v)
{
  s += v.s;
  x += v.x;
  y += v.y;
  z += v.z;
}

void Quaternion::operator-=(const Quaternion &v)
{
  s -= v.s;
  x -= v.x;
  y -= v.y;
  z -= v.z;
}

void Quaternion::operator*=(const float scalar)
{
  s *= scalar;
  x *= scalar;
  y *= scalar;
  z *= scalar;
}

void Quaternion::operator/=(const float dividend)
{
  s /= dividend;
  x /= dividend;
  y /= dividend;
  z /= dividend;
}

void Quaternion::normalize()
{
  float invLen = 1.0f / sqrtf(s * s + x * x + y * y + z * z);
  s *= invLen;
  x *= invLen;
  y *= invLen;
  z *= invLen;
}

void Quaternion::fastNormalize()
{
  float invLen = rsqrtf(s * s + x * x + y * y + z * z);
  s *= invLen;
  x *= invLen;
  y *= invLen;
  z *= invLen;
}


Quaternion operator+(const Quaternion &u, const Quaternion &v) { return Quaternion(u.s + v.s, u.x + v.x, u.y + v.y, u.z + v.z); }

Quaternion operator-(const Quaternion &u, const Quaternion &v) { return Quaternion(u.s - v.s, u.x - v.x, u.y - v.y, u.z - v.z); }

Quaternion operator-(const Quaternion &v) { return Quaternion(-v.s, -v.x, -v.y, -v.z); }

Quaternion operator*(const Quaternion &u, const Quaternion &v)
{
  return Quaternion(u.s * v.s - u.x * v.x - u.y * v.y - u.z * v.z, u.s * v.x + u.x * v.s + u.y * v.z - u.z * v.y,
    u.s * v.y + u.y * v.s + u.z * v.x - u.x * v.z, u.s * v.z + u.z * v.s + u.x * v.y - u.y * v.x);
}

Quaternion operator*(const float scalar, const Quaternion &v)
{
  return Quaternion(v.s * scalar, v.x * scalar, v.y * scalar, v.z * scalar);
}

Quaternion operator*(const Quaternion &v, const float scalar)
{
  return Quaternion(v.s * scalar, v.x * scalar, v.y * scalar, v.z * scalar);
}

Quaternion operator/(const Quaternion &v, const float dividend)
{
  return Quaternion(v.s / dividend, v.x / dividend, v.y / dividend, v.z / dividend);
}


Quaternion slerp(const Quaternion &q0, const Quaternion &q1, const float t)
{
  float cosTheta = q0.s * q1.s + q0.x * q1.x + q0.y * q1.y + q0.z * q1.z;

  if (fabsf(1 - cosTheta) < 0.001f)
  {
    return q0 * (1 - t) + q1 * t;
  }
  else
  {
    float theta = acosf(cosTheta);
    return (q0 * sinf((1 - t) * theta) + q1 * sinf(t * theta)) / sinf(theta);
  }
}


Quaternion scerp(const Quaternion &q0, const Quaternion &q1, const Quaternion &q2, const Quaternion &q3, const float t)
{
  return slerp(slerp(q1, q2, t), slerp(q0, q3, t), 2 * t * (1 - t));
}
