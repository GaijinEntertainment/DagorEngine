// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include "math/dag_capsuleTriangle.h"
#include "math/dag_capsuleTriangleCached.h"
#include "math/dag_math3d.h"
#include "math/dag_math2d.h"
#include "math/dag_plane3.h"
#include "math/dag_rayIntersectSphere.h"
#include "math/dag_mathUtils.h"

bool clipCapsuleTriangle(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const TriangleFace &f)
{
  // simple capsule clip test: serves to drop evident cases
  real _d0, _d1, _dmin, _dmax;

  {
    // Real capsule case
    real fd = -(f.vp[0] * f.n);
    _d0 = (c.a * f.n) + fd;
    _d1 = (c.b * f.n) + fd;

    const real d1_GE_d0 = fabsf(_d1) - fabsf(_d0);
    _dmin = fsel(d1_GE_d0, _d0, _d1);
    _dmax = fsel(d1_GE_d0, _d1, _d0);

    if ((_dmax >= 0 && _dmin <= c.r) || (_dmax < 0 && _dmin >= -c.r))
      ; // has collision
    else
      return false;
  }

  real t0 = 0, t1 = 1;
  int e;
  for (e = 0; e < 3; ++e)
  {
    Point3 en = normalize(f.n % (f.vp[e == 2 ? 0 : e + 1] - f.vp[e]));
    real ed = -(en * f.vp[e]);
    real d0 = en * c.a + ed;
    real d1 = en * c.b + ed;
    if (d0 < 0)
    {
      if (d1 < 0)
      {
        if (d0 > d1)
        {
          if (d0 < -c.r)
          {
            return false;
          }
        }
        else
        {
          if (d1 < -c.r)
          {
            return false;
          }
        }
        break;
      }
      real t = d0 / (d0 - d1);
      if (t > t0)
        t0 = t;
    }
    else if (d1 < 0)
    {
      real t = d0 / (d0 - d1);
      if (t < t1)
        t1 = t;
    }
  }


  if (e < 3 || t0 > t1)
  {
    Point3 cwdir = c.b - c.a;
    float clen = cwdir.length();
    cwdir *= safeinv(clen);

    // capsule line is outside, find nearest edge/vertex
    bool changed = false;
    for (e = 0; e < 3; ++e)
    {
      Point3 wdir;
      Point3 wp0 = f.vp[e];
      wdir = f.vp[e == 2 ? 0 : e + 1] - wp0;

      real l = length(wdir);
      if (l != 0)
        wdir /= l;
      real k = wdir * cwdir;
      Point3 dp = c.a - wp0;
      Point3 h = wdir - cwdir * k;
      real num = dp * h, denum = 1 - k * k;
      real t;
      if (denum == 0)
        t = .5f;
      else
        t = num / denum;
      if (t < 0)
        t = 0;
      else if (t > l)
        t = l;
      real t2 = ((wdir * t + wp0) - c.a) * cwdir;
      if (t2 < 0)
        t2 = 0;
      else if (t2 > clen)
        t2 = clen;
      t = ((cwdir * t2 + c.a) - wp0) * wdir;
      if (t < 0)
        t = 0;
      else if (t > l)
        t = l;
      Point3 p1, p2;
      real dl = length((p1 = wdir * t + wp0) - (p2 = cwdir * t2 + c.a));
      real d = dl - c.r;
      if (d < 0)
      {
        if (d < md)
        {
          md = d;
          if (dl != 0)
            dp = (p2 - p1) / dl;
          else
            dp = normalize(wdir % cwdir);
          cp2 = p1;
          cp1 = p2 - dp * c.r;
          changed = true;
        }
      }
    }
    return changed;
  }

  bool ret = false;

  real d0 = _d0, d1 = _d1;
  real dmin = (d1 - d0) * t0 + d0;
  real dmax = (d1 - d0) * t1 + d0;
  // DEBUG_CTX("dmin=%.3f dmax=%.3f, c.r=%.3f md=%.3f, t0=%.3f, t1=%.3f", dmin, dmax, c.r, md, t0, t1);
  if (dmin <= -c.r)
  {
    if (dmax <= -c.r)
    {
      return false;
    }
  }
  else if (dmin >= c.r)
  {
    if (dmax >= c.r)
    {
      return false;
    }
    real d = dmax - c.r;
    if (d < md)
    {
      md = d;
      cp1 = (c.b - c.a) * t1 + c.a;
      ret = true;
    }
    else
      return false;
  }
  if (dmin < dmax)
  {
    real d = dmin - c.r;
    if (d < md)
    {
      md = d;
      cp1 = (c.b - c.a) * t0 + c.a;
      ret = true;
    }
    else if (!ret)
      return false;
  }
  else
  {
    real d = dmax - c.r;
    if (d < md)
    {
      md = d;
      cp1 = (c.b - c.a) * t1 + c.a;
      ret = true;
    }
    else if (!ret)
      return false;
  }

  // if (!ret)
  //   __asm ud2;
  cp1 -= (f.n * c.r);
  cp2 = cp1 - (f.n * md);
  return true;
}

bool clipCapsuleTriangleCached(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, TriangleFaceCached &f, int &ready_stage)
{
  // simple capsule clip test: serves to drop evident cases
  real _d0, _d1, _dmin, _dmax;

  {
    // Real capsule case
    _d0 = (c.a * f.n) + f.fd;
    _d1 = (c.b * f.n) + f.fd;


    if (fabsf(_d0) <= fabsf(_d1))
    {
      _dmin = _d0;
      _dmax = _d1;
    }
    else
    {
      _dmin = _d1;
      _dmax = _d0;
    }


    if ((_dmax >= 0 && _dmin <= c.r) || (_dmax < 0 && _dmin >= -c.r))
      ; // has collision
    else
      return false;
  }

  if (ready_stage < 2)
  {
    f.precalc();
    ready_stage = 2;
  }

  real t0 = 0, t1 = 1;
  int e;
  for (e = 0; e < 3; ++e)
  {
    real d0 = f.en[e] * c.a + f.ed[e];
    real d1 = f.en[e] * c.b + f.ed[e];
    if (d0 < 0)
    {
      if (d1 < 0)
      {
        if (d0 > d1)
        {
          if (d0 < -c.r)
          {
            return false;
          }
        }
        else
        {
          if (d1 < -c.r)
          {
            return false;
          }
        }
        break;
      }
      real t = d0 / (d0 - d1);
      if (t > t0)
        t0 = t;
    }
    else if (d1 < 0)
    {
      real t = d0 / (d0 - d1);
      if (t < t1)
        t1 = t;
    }
  }


  if (e < 3 || t0 > t1)
  {
    Point3 cwdir = c.b - c.a;
    float clen = cwdir.length();
    cwdir *= safeinv(clen);

    // capsule line is outside, find nearest edge/vertex
    bool changed = false;
    for (e = 0; e < 3; ++e)
    {
      Point3 wdir;
      Point3 wp0 = f.vp[e];
      wdir = f.vp[(e + 1) % 3] - wp0;

      real l = length(wdir);
      if (l != 0)
        wdir /= l;
      real k = wdir * cwdir;
      Point3 dp = c.a - wp0;
      Point3 h = wdir - cwdir * k;
      real num = dp * h, denum = 1 - k * k;
      real t;
      if (denum == 0)
        t = .5f;
      else
        t = num / denum;
      if (t < 0)
        t = 0;
      else if (t > l)
        t = l;
      real t2 = ((wdir * t + wp0) - c.a) * cwdir;
      if (t2 < 0)
        t2 = 0;
      else if (t2 > clen)
        t2 = clen;
      t = ((cwdir * t2 + c.a) - wp0) * wdir;
      if (t < 0)
        t = 0;
      else if (t > l)
        t = l;
      Point3 p1, p2;
      real dl = length((p1 = wdir * t + wp0) - (p2 = cwdir * t2 + c.a));
      real d = dl - c.r;
      if (d < 0)
      {
        if (d < md)
        {
          md = d;
          if (dl != 0)
            dp = (p2 - p1) / dl;
          else
            dp = normalize(wdir % cwdir);
          cp2 = p1;
          cp1 = p2 - dp * c.r;
          changed = true;
        }
      }
    }
    return changed;
  }

  bool ret = false;

  real d0 = _d0, d1 = _d1;
  real dmin = (d1 - d0) * t0 + d0;
  real dmax = (d1 - d0) * t1 + d0;
  // DEBUG_CTX("dmin=%.3f dmax=%.3f, c.r=%.3f md=%.3f, t0=%.3f, t1=%.3f", dmin, dmax, c.r, md, t0, t1);
  if (dmin <= -c.r)
  {
    if (dmax <= -c.r)
    {
      return false;
    }
  }
  else if (dmin >= c.r)
  {
    if (dmax >= c.r)
    {
      return false;
    }
    real d = dmax - c.r;
    if (d < md)
    {
      md = d;
      cp1 = (c.b - c.a) * t1 + c.a;
      ret = true;
    }
    else
      return false;
  }
  if (dmin < dmax)
  {
    real d = dmin - c.r;
    if (d < md)
    {
      md = d;
      cp1 = (c.b - c.a) * t0 + c.a;
      ret = true;
    }
    else if (!ret)
      return false;
  }
  else
  {
    real d = dmax - c.r;
    if (d < md)
    {
      md = d;
      cp1 = (c.b - c.a) * t1 + c.a;
      ret = true;
    }
    else if (!ret)
      return false;
  }

  // if (!ret)
  //   __asm ud2;
  cp1 -= (f.n * c.r);
  cp2 = cp1 - (f.n * md);
  return true;
}

// TODO: replace by vectorized version with optimizations and fixed intersections on capsule start
bool test_capsule_triangle_intersection(const Capsule &capsule, const TriangleFace &tf, float &t, Point3 &isect_pos)
{
  t = length(capsule.b - capsule.a);
  Point3 wdir = (capsule.b - capsule.a) * safeinv(t);
  if (dot(tf.n, wdir) > -0.001f)
    return false;

  int col = -1;

  // pass1: ray VS plane
  Plane3 plane(tf.n, tf.vp[0]);
  float h = plane.distance(capsule.a);
  if (h < -capsule.r)
    return false;

  if (h > capsule.r)
  {
    h -= capsule.r;
    float dp = dot(tf.n, wdir);
    if (dp != 0.f)
    {
      float dist = -h / dp;
      if (dist < t)
      {
        Point3 onPlane = capsule.a + wdir * dist;
        if (is_point_in_triangle(onPlane, tf.vp[0], tf.vp[1], tf.vp[2]))
        {
          t = dist;
          isect_pos = onPlane;
          col = 0;
        }
      }
    }
  }

  // pass2: ray VS spheres on triangle vertices
  for (int i = 0; i < 3; i++)
  {
    float dist = rayIntersectSphereDist(capsule.a, wdir, tf.vp[i], capsule.r);
    if (dist >= 0.f && dist < t)
    {
      t = dist;
      isect_pos = tf.vp[i];
      col = 1;
    }
  }

  // pass3: capsule VS triangle edges
  for (int i = 0; i < 3; i++)
  {
    Point3 edge0 = tf.vp[i];
    int j = i + 1;
    if (j == 3)
      j = 0;
    Point3 edge1 = tf.vp[j];
    Plane3 plane;
    plane.setNormalized(edge0, edge1, edge1 - wdir);
    float d = plane.distance(capsule.a);
    if (d > capsule.r || d < -capsule.r)
      continue;

    float r = sqrtf((capsule.r - d) * (capsule.r + d));

    Point3 pt0 = plane.project(capsule.a); // center of the sphere slice (a circle)
    Point3 onLine = closest_pt_on_line(pt0, edge0, edge1);
    Point3 v = normalize(onLine - pt0);
    Point3 pt1 = v * r + pt0; // point on the sphere that will maybe collide with the edge

    int a0 = 0, a1 = 1;
    float pl_x = fabsf(plane.n.x);
    float pl_y = fabsf(plane.n.y);
    float pl_z = fabsf(plane.n.z);
    if (pl_x > pl_y && pl_x > pl_z)
    {
      a0 = 1;
      a1 = 2;
    }
    else
    {
      if (pl_y > pl_z)
      {
        a0 = 0;
        a1 = 2;
      }
    }

    Point3 vv = pt1 + wdir;

    float dist;
    bool res = get_lines_intersection(Point2(pt1[a0], pt1[a1]), Point2(vv[a0], vv[a1]), Point2(edge0[a0], edge0[a1]),
      Point2(edge1[a0], edge1[a1]), dist);
    if (!res || dist < 0)
      continue;

    Point3 inter = pt1 + wdir * dist;
    Point3 r1 = edge0 - inter;
    Point3 r2 = edge1 - inter;
    if (dot(r1, r2) > 0.f)
      continue;

    if (dist < t)
    {
      t = dist;
      isect_pos = inter;
      col = 2;
    }
  }

  return col != -1;
}

VECTORCALL static bool capsule_triangle_edge_intersection(vec3f from, vec3f dir, vec3f p0, vec3f p1, float radius, float &t,
  vec3f &out_pos)
{
  vec4d start = vd_cvt_from_vec4f(p0);
  vec4d edge = vd_sub(vd_cvt_from_vec4f(p1), start);
  double edgeLenSq = vd_extract_x(vd_dot3_x(edge, edge));
  if (edgeLenSq == 0.)
    return false;
  vec4d offset = vd_sub(vd_cvt_from_vec4f(from), start);
  double along = vd_extract_x(vd_dot3_x(offset, edge)) / edgeLenSq;
  vec4d radial = vd_sub(offset, vd_mul(edge, vd_splats(along)));
  double radiusSq = double(radius) * radius;
  double c = vd_extract_x(vd_dot3_x(radial, radial)) - radiusSq;
  float dist = 0.f;
  if (c > 0.)
  {
    vec4d direction = vd_cvt_from_vec4f(dir);
    double alongDir = vd_extract_x(vd_dot3_x(direction, edge)) / edgeLenSq;
    vec4d radialDir = vd_sub(direction, vd_mul(edge, vd_splats(alongDir)));
    double a = vd_extract_x(vd_dot3_x(radialDir, radialDir));
    double b = vd_extract_x(vd_dot3_x(radial, radialDir));
    if (b >= 0.)
      return false;
    vec4d cross = vd_cross3(radial, radialDir);
    double discriminant = a * radiusSq - vd_extract_x(vd_dot3_x(cross, cross));
    if (discriminant < 0.)
      return false;
    dist = float(c / (-b + sqrt(discriminant)));
    along += alongDir * dist;
  }
  if (dist >= t || along < 0. || along > 1.)
    return false;
  out_pos = vd_cvt_to_vec4f(vd_add(start, vd_mul(edge, vd_splats(along))));
  t = dist;
  return true;
}

VECTORCALL static bool capsule_triangle_face_intersection(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, vec3f norm,
  float radius, float h, float dp, float &t, vec3f &out_pos)
{
  if (!v_test_xyz_finite(norm))
    return false;
  float dist = 0.f;
  if (h > radius)
  {
    if (dp >= 0.f)
      return false;
    dist = (radius - h) / dp;
  }
  else if (h < -radius)
  {
    if (dp <= 0.f)
      return false;
    dist = (-radius - h) / dp;
  }
  if (dist >= t)
    return false;
  vec3f center = v_madd(dir, v_splats(dist), from);
  vec3f pos = v_sub(center, v_mul(norm, v_dot3(v_sub(center, v0), norm)));
  if (!is_point_in_triangle(pos, v0, v1, v2))
    return false;
  out_pos = pos;
  t = dist;
  return true;
}

VECTORCALL bool test_capsule_triangle_intersection(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float &t,
  vec3f &out_norm, vec3f &out_pos, bool no_cull, float dp_cull_threshold)
{
  int col = -1;
  vec3f edge0 = v_sub(v1, v0);
  vec3f edge1 = v_sub(v2, v0);
  vec3f vNorm = v_norm3(v_cross3(edge1, edge0));
  float dp = v_extract_x(v_dot3_x(vNorm, dir));
  if (!no_cull && dp > dp_cull_threshold) // culling
    return false;

  vec3f vertices[3] = {v0, v1, v2};

  plane3f plane = v_make_plane_norm(v0, vNorm);
  float h = v_extract_x(v_plane_dist_x(plane, from));
  if (!no_cull && h < -radius)
    return false;
  if (capsule_triangle_face_intersection(from, dir, v0, v1, v2, vNorm, radius, h, dp, t, out_pos))
  {
    out_norm = vNorm;
    col = 0;
  }

  for (int i = 0; i < 3; i++)
  {
    float dist = rayIntersectSphereDist(from, dir, vertices[i], radius);
    if (dist >= 0.f && dist < t)
    {
      t = dist;
      out_norm = vNorm;
      out_pos = vertices[i];
      col = 1;
    }
  }

  for (int i = 0; i < 3; i++)
    if (capsule_triangle_edge_intersection(from, dir, vertices[i], vertices[(i + 1) % 3], radius, t, out_pos))
    {
      out_norm = vNorm;
      col = 2;
    }

  return col != -1;
}

VECTORCALL bool test_capsule_triangle_hit(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float t, bool no_cull,
  float dp_cull_threshold)
{
  vec3f edge0 = v_sub(v1, v0);
  vec3f edge1 = v_sub(v2, v0);
  vec3f vNorm = v_norm3(v_cross3(edge1, edge0));
  float dp = v_extract_x(v_dot3_x(vNorm, dir));
  if (!no_cull && dp > dp_cull_threshold) // culling
    return false;

  vec3f vertices[3] = {v0, v1, v2};

  plane3f plane = v_make_plane_norm(v0, vNorm);
  float h = v_extract_x(v_plane_dist_x(plane, from));
  if (!no_cull && h < -radius)
    return false;
  vec3f pos;
  if (capsule_triangle_face_intersection(from, dir, v0, v1, v2, vNorm, radius, h, dp, t, pos))
    return true;

  for (int i = 0; i < 3; i++)
  {
    float dist = rayIntersectSphereDist(from, dir, vertices[i], radius);
    if (dist >= 0.f && dist < t)
      return true;
  }

  for (int i = 0; i < 3; i++)
    if (capsule_triangle_edge_intersection(from, dir, vertices[i], vertices[(i + 1) % 3], radius, t, pos))
      return true;

  return false;
}
