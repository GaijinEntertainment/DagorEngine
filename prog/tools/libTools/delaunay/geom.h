#ifndef GEOM_INCLUDED // -*- C++ -*-
#define GEOM_INCLUDED

////////////////////////////////////////////////////////////////////////
//
// Define some basic types and values
//
////////////////////////////////////////////////////////////////////////

#ifdef SAFETY
#include <assert.h>
#endif

namespace delaunay
{

typedef double real;
// inline int rint(float a) {return a;}
#define EPS  1e-6
#define EPS2 (EPS * EPS)
}; // namespace delaunay

#include "supp/dag_math.h"
#include "vec2.h"
#include "vec3.h"

namespace delaunay
{

typedef int boolean;

enum Axis
{
  X,
  Y,
  Z,
  W
};
enum Side
{
  Left = -1,
  On = 0,
  Right = 1
};

#ifndef NULL
#define NULL 0
#endif

#ifndef True
#define True  1
#define False 0
#endif

class Labelled
{
public:
  int token;
};


////////////////////////////////////////////////////////////////////////
//
// Here we define some useful geometric functions
//
////////////////////////////////////////////////////////////////////////

//
// triArea returns TWICE the area of the oriented triangle ABC.
// The area is positive when ABC is oriented counterclockwise.
inline real triArea(const Vec2 &a, const Vec2 &b, const Vec2 &c)
{
  return (b[X] - a[X]) * (c[Y] - a[Y]) - (b[Y] - a[Y]) * (c[X] - a[X]);
}

inline boolean ccw(const Vec2 &a, const Vec2 &b, const Vec2 &c) { return triArea(a, b, c) > 0; }

inline boolean rightOf(const Vec2 &x, const Vec2 &org, const Vec2 &dest) { return ccw(x, dest, org); }

inline boolean leftOf(const Vec2 &x, const Vec2 &org, const Vec2 &dest) { return ccw(x, org, dest); }

// Returns True if the point d is inside the circle defined by the
// points a, b, c. See Guibas and Stolfi (1985) p.107.
//
inline boolean inCircle(const Vec2 &a, const Vec2 &b, const Vec2 &c, const Vec2 &d)
{
  return (a[0] * a[0] + a[1] * a[1]) * triArea(b, c, d) - (b[0] * b[0] + b[1] * b[1]) * triArea(a, c, d) +
           (c[0] * c[0] + c[1] * c[1]) * triArea(a, b, d) - (d[0] * d[0] + d[1] * d[1]) * triArea(a, b, c) >
         EPS;
}


class Plane
{
public:
  DAG_DECLARE_NEW(delmem)

  real a, b, c;

  Plane() {}
  Plane(const Vec3 &p, const Vec3 &q, const Vec3 &r) { init(p, q, r); }

  inline void init(const Vec3 &p, const Vec3 &q, const Vec3 &r);

  real operator()(real x, real y) const { return a * x + b * y + c; }
  real operator()(int x, int y) const { return a * x + b * y + c; }
};

inline void Plane::init(const Vec3 &p, const Vec3 &q, const Vec3 &r)
// find the plane z=ax+by+c passing through three points p,q,r
{
  // We explicitly declare these (rather than putting them in a
  // Vector) so that they can be allocated into registers.
  real ux = q[X] - p[X], uy = q[Y] - p[Y], uz = q[Z] - p[Z];
  real vx = r[X] - p[X], vy = r[Y] - p[Y], vz = r[Z] - p[Z];
  real den = ux * vy - uy * vx;

  a = (uz * vy - uy * vz) / den;
  b = (ux * vz - uz * vx) / den;
  c = p[Z] - a * p[X] - b * p[Y];
}


class Line
{

private:
  real a, b, c;

public:
  DAG_DECLARE_NEW(delmem)

  Line(const Vec2 &p, const Vec2 &q)
  {
    Vec2 t = q - p;
    real l = t.length();
#ifdef SAFETY
    assert(l != 0);
#endif
    a = t[Y] / l;
    b = -t[X] / l;
    c = -(a * p[X] + b * p[Y]);
  }

  inline real eval(const Vec2 &p) const { return (a * p[X] + b * p[Y] + c); }

  inline Side classify(const Vec2 &p) const
  {
    real d = eval(p);

    if (d < -EPS)
      return Left;
    else if (d > EPS)
      return Right;
    else
      return On;
  }

  inline Vec2 intersect(const Line &l) const
  {
    Vec2 p;
    intersect(l, p);
    return p;
  }

  inline void intersect(const Line &l, Vec2 &p) const
  {
    real den = a * l.b - b * l.a;
#ifdef SAFETY
    assert(den != 0);
#endif
    p[X] = (b * l.c - c * l.b) / den;
    p[Y] = (c * l.a - a * l.c) / den;
  }

#ifdef IOSTREAMH
  friend ostream &operator<<(ostream &, const Line &);
#endif
};

#ifdef IOSTREAMH
inline ostream &operator<<(ostream &out, const Line &l) { return out << "Line(a=" << l.a << " b=" << l.b << " c=" << l.c << ")"; }
#endif

}; // namespace delaunay

#endif
