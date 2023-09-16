#include <gameMath/mathUtils.h>

#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>

float distance_to_triangle(const Point3 &p, const Point3 &a, const Point3 &b, const Point3 &c, Point3 &out_contact, Point3 &out_normal)
{
  Point3 edge0 = b - a;
  Point3 edge1 = c - a;
  out_normal = normalize(edge0 % edge1);
  float dist = p * out_normal - out_normal * a;
  out_contact = p - out_normal * dist;

  float ab = ((out_contact - a) % (b - a)) * out_normal;
  float bc = ((out_contact - b) % (c - b)) * out_normal;
  float ca = ((out_contact - c) % (a - c)) * out_normal;
  if ((ab > 0.f && bc > 0.f && ca > 0.f) || (ab < 0.f && bc < 0.f && ca < 0.f))
    return rabs(dist);

  float t;
  Point3 contact;
  dist = distanceToSeg(p, a, b, t, out_contact);
  float dist2 = distanceToSeg(p, b, c, t, contact);
  if (dist2 < dist)
  {
    dist = dist2;
    out_contact = contact;
  }
  dist2 = distanceToSeg(p, c, a, t, contact);
  if (dist2 < dist)
  {
    dist = dist2;
    out_contact = contact;
  }
  return dist;
}

bool p3_nonzero(const Point3 &p) { return float_nonzero(p.x) || float_nonzero(p.y) || float_nonzero(p.z); }

void get_random_point_on_sphere(float min_x, float max_x, int &rand_seed, Point3 &out_pnt)
{
  out_pnt.x = _rnd_float(rand_seed, min_x, max_x);
  float t = TWOPI * _frnd(rand_seed);
  float w = safe_sqrt(1.f - sqr(out_pnt.x));
  sincos(t, out_pnt.y, out_pnt.z);
  out_pnt.y *= w;
  out_pnt.z *= w;
}

float min(std::initializer_list<float> val_list)
{
  float result = VERY_BIG_NUMBER;
  for (float val : val_list)
    inplace_min(result, val);
  return result;
}

float max(std::initializer_list<float> val_list)
{
  float result = VERY_SMALL_NUMBER;
  for (float val : val_list)
    inplace_max(result, val);
  return result;
}
