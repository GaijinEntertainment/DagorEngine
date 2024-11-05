// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifdef IOSTREAMH
#include <iostream>
using namespace std;
#endif

namespace delaunay
{

class Vec3
{
protected:
  real elt[3];

  inline void copy(const Vec3 &v);

public:
  DAG_DECLARE_NEW(delmem)

  // Standard constructors
  Vec3(real x = 0, real y = 0, real z = 0)
  {
    elt[0] = x;
    elt[1] = y;
    elt[2] = z;
  }
  Vec3(const Vec2 &v, real z)
  {
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = z;
  }
  Vec3(const Vec3 &v) { copy(v); }
  Vec3(const real *v)
  {
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
  }

  // Access methods
  real &operator()(int i) { return elt[i]; }
  const real &operator()(int i) const { return elt[i]; }
  real &operator[](int i) { return elt[i]; }
  const real &operator[](int i) const { return elt[i]; }

  // Assignment methods
  inline Vec3 &operator=(const Vec3 &v);
  inline Vec3 &operator+=(const Vec3 &v);
  inline Vec3 &operator-=(const Vec3 &v);
  inline Vec3 &operator*=(real s);
  inline Vec3 &operator/=(real s);

  // Arithmetic methods
  inline Vec3 operator+(const Vec3 &v) const;
  inline Vec3 operator-(const Vec3 &v) const;
  inline Vec3 operator-() const;

  inline Vec3 operator*(real s) const;
  inline Vec3 operator/(real s) const;
  inline real operator*(const Vec3 &v) const;
  inline Vec3 operator^(const Vec3 &v) const;


#ifdef IOSTREAMH
  // Input/Output methods
  friend ostream &operator<<(ostream &, const Vec3 &);
  friend istream &operator>>(istream &, Vec3 &);
#endif

  // Additional vector methods
  inline real length();
  inline real norm();
  inline real norm2();

  inline real unitize();
};


inline void Vec3::copy(const Vec3 &v)
{
  elt[0] = v.elt[0];
  elt[1] = v.elt[1];
  elt[2] = v.elt[2];
}

inline Vec3 &Vec3::operator=(const Vec3 &v)
{
  copy(v);
  return *this;
}

inline Vec3 &Vec3::operator+=(const Vec3 &v)
{
  elt[0] += v[0];
  elt[1] += v[1];
  elt[2] += v[2];
  return *this;
}

inline Vec3 &Vec3::operator-=(const Vec3 &v)
{
  elt[0] -= v[0];
  elt[1] -= v[1];
  elt[2] -= v[2];
  return *this;
}

inline Vec3 &Vec3::operator*=(real s)
{
  elt[0] *= s;
  elt[1] *= s;
  elt[2] *= s;
  return *this;
}

inline Vec3 &Vec3::operator/=(real s)
{
  elt[0] /= s;
  elt[1] /= s;
  elt[2] /= s;
  return *this;
}

///////////////////////

inline Vec3 Vec3::operator+(const Vec3 &v) const
{
  Vec3 w(elt[0] + v[0], elt[1] + v[1], elt[2] + v[2]);
  return w;
}

inline Vec3 Vec3::operator-(const Vec3 &v) const
{
  Vec3 w(elt[0] - v[0], elt[1] - v[1], elt[2] - v[2]);
  return w;
}

inline Vec3 Vec3::operator-() const { return Vec3(-elt[0], -elt[1], -elt[2]); }

inline Vec3 Vec3::operator*(real s) const
{
  Vec3 w(elt[0] * s, elt[1] * s, elt[2] * s);
  return w;
}

inline Vec3 Vec3::operator/(real s) const
{
  Vec3 w(elt[0] / s, elt[1] / s, elt[2] / s);
  return w;
}

inline real Vec3::operator*(const Vec3 &v) const { return elt[0] * v[0] + elt[1] * v[1] + elt[2] * v[2]; }

inline Vec3 Vec3::operator^(const Vec3 &v) const
{
  Vec3 w(elt[1] * v[2] - v[1] * elt[2], -elt[0] * v[2] + v[0] * elt[2], elt[0] * v[1] - v[0] * elt[1]);
  return w;
}

inline real Vec3::length() { return norm(); }

inline real Vec3::norm() { return sqrt(elt[0] * elt[0] + elt[1] * elt[1] + elt[2] * elt[2]); }

inline real Vec3::norm2() { return elt[0] * elt[0] + elt[1] * elt[1] + elt[2] * elt[2]; }

inline real Vec3::unitize()
{
  real l = norm();
  if (l != 1.0)
    (*this) /= l;
  return l;
}

}; // namespace delaunay
