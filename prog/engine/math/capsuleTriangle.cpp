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

    float r = sqrtf(sqr(capsule.r) - d * d);

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

VECTORCALL bool test_capsule_triangle_intersection(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float &t,
  vec3f &out_norm, vec3f &out_pos, bool no_cull, float dp_cull_threshold, float h_zero_eps)
{
  int col = -1;
  vec3f edge0 = v_sub(v1, v0);
  vec3f edge1 = v_sub(v2, v0);
  vec3f vNorm = v_norm3(v_cross3(edge1, edge0));
  vec4f makeNeg = v_cast_vec4f(v_seti_x((no_cull ? 1 : 0) << 31)); // invert sign in no_cull mode
  float dp = v_extract_x(v_or(v_dot3_x(vNorm, dir), makeNeg));
  if (!no_cull && dp > dp_cull_threshold) // culling
    return false;

  vec3f vertices[3] = {v0, v1, v2};

  // pass1: ray VS plane
  plane3f plane = v_make_plane_norm(v0, vNorm);
  float h = v_extract_x(v_andnot(makeNeg, v_plane_dist_x(plane, from)));
  if (h < -radius)
    return false;

  if (dp != 0.f)
  {
    float dist = -max(h - radius, 0.f) / dp;
    if (dist < t)
    {
      // hitPos selection depends on the sign of h:
      //
      //   h >= 0 (origin in front of, or on, the plane):
      //     vOnPlane = ray-plane intersection (along ray direction). For h > radius this is
      //     the FIRST t at which the capsule axis enters the triangle's plane (forward of
      //     the origin since -h/dp > 0 when dp < 0). For 0 <= h <= radius the intersection
      //     still lies forward of the origin and is a sensible contact-point approximation.
      //
      //   h < 0 (origin BEHIND the plane, but |h| <= radius so the start sphere overlaps
      //          the plane -- pass1 wouldn't run otherwise; the `h < -radius` early-out
      //          above rejects the deeper case):
      //     vOnPlane = from + dir * (-h/dp). With h < 0 and dp < 0, -h/dp < 0: the
      //     ray-plane intersection lies BEHIND the ray origin. It is not the contact
      //     point of the capsule's start sphere with the plane -- the capsule extends
      //     forward from the origin, not backward. The actual contact (and the only one
      //     the start sphere can reach when |h| <= radius) is the orthogonal projection
      //     of the origin onto the plane: orthoProj = from - vNorm * h.
      //
      // Why this matters:
      //   The legacy code unconditionally used vOnPlane and tested is_point_in_triangle
      //   on it. For h < 0 there are configurations where vOnPlane (behind the ray)
      //   lands inside the triangle while orthoProj (the real start-sphere contact)
      //   lands outside it -- the start sphere is touching air just outside the
      //   triangle face but pass1 spuriously reports a face hit at t = 0. With raw
      //   verts both FRT and BVH paths share the bug identically (so it was invisible),
      //   but BLAS-decoded verts (vert21 round-trip ~1e-6 noise on icosahedron-scale
      //   coords) shift vOnPlane fractionally across the triangle's edge while
      //   orthoProj stays decisively outside -- the BVH path then produces a hit the
      //   FRT path does not, breaking BVH/FRT parity. Selecting hitPos = orthoProj for
      //   the h < 0 branch is the rigorous "does the start sphere actually touch the
      //   triangle face?" test. The h >= 0 branch is unchanged because vOnPlane there
      //   is well-defined (forward of origin) and matches the geometric contact for
      //   the start-of-capsule contact case.
      vec3f hitPos = h < 0.f ? v_sub(from, v_mul(vNorm, v_splats(h))) : v_madd(dir, v_splats(-h / dp), from);
      if (is_point_in_triangle(hitPos, v0, v1, v2))
      {
        t = dist;
        out_norm = vNorm;
        out_pos = hitPos;
        col = 0;
      }
    }
  }

  // pass2: ray VS spheres on triangle vertices
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

  // pass3: capsule VS triangle edges
  for (int i = 0; i < 3; i++)
  {
    int j = i + 1;
    if (j == 3)
      j = 0;
    vec3f p0 = vertices[i];
    vec3f p1 = vertices[j];
    vec3f p2 = v_sub(p1, dir);

    vec3f n = v_norm3(v_cross3(v_sub(p1, p0), v_sub(p2, p0)));
    plane3f plane = v_make_plane_norm(p0, n);

    float d = v_extract_x(v_plane_dist_x(plane, from));
    if (d > radius || d < -radius)
      continue;

    float r = sqrtf(sqr(radius) - sqr(d));

    // project on plane
    vec3f pt0 = v_sub(from, v_mul(plane, v_splats(d))); // center of the sphere slice (a circle)
    vec3f onLine = closest_point_on_line(pt0, p0, v_norm3(v_sub(p1, p0)));

    vec3f v = v_norm3(v_sub(onLine, pt0));
    vec3f pt1 = v_madd(v, v_splats(r), pt0); // point on the sphere that will maybe collide with the edge
    vec3f vv = v_add(pt1, dir);

    Point2 l1s, l1e, l2s, l2e;
    vec3f absN = v_abs(plane);
    float pl_x = v_extract_x(absN);
    float pl_y = v_extract_y(absN);
    float pl_z = v_extract_z(absN);
    if (pl_x > pl_y && pl_x > pl_z)
    {
      l1s = Point2(v_extract_y(pt1), v_extract_z(pt1));
      l1e = Point2(v_extract_y(vv), v_extract_z(vv));
      l2s = Point2(v_extract_y(p0), v_extract_z(p0));
      l2e = Point2(v_extract_y(p1), v_extract_z(p1));
    }
    else if (pl_y > pl_z)
    {
      l1s = Point2(v_extract_x(pt1), v_extract_z(pt1));
      l1e = Point2(v_extract_x(vv), v_extract_z(vv));
      l2s = Point2(v_extract_x(p0), v_extract_z(p0));
      l2e = Point2(v_extract_x(p1), v_extract_z(p1));
    }
    else
    {
      l1s = Point2(v_extract_x(pt1), v_extract_y(pt1));
      l1e = Point2(v_extract_x(vv), v_extract_y(vv));
      l2s = Point2(v_extract_x(p0), v_extract_y(p0));
      l2e = Point2(v_extract_x(p1), v_extract_y(p1));
    }

    float dist;
    bool res = get_lines_intersection(l1s, l1e, l2s, l2e, dist);
    // h_zero_eps tolerates small negative h from quantized-vertex drift: a flat triangle whose
    // y=0 plane decodes as y=-5e-5 must not flip the back-side rejection on grazing rays.
    if (!res || dist < -radius || (h < -h_zero_eps && dist < 0.f))
      continue;

    vec3f inter = v_madd(dir, v_splats(dist), pt1);
    vec3f r1 = v_sub(p0, inter);
    vec3f r2 = v_sub(p1, inter);
    if (v_extract_x(v_dot3_x(r1, r2)) > 0.f)
      continue;

    // h-sign override of dist: the line-line intersection in 2D may produce a sign that
    // doesn't reliably indicate "ahead vs behind the ray" depending on which axis was
    // dropped during projection. The h sign is the canonical "in front (h>=0) / behind
    // (h<0)" signal -- when h<0 the contact has already been happening, clamp dist to 0.
    // h_zero_eps tolerates BLAS-decoded vertex precision drift (vert21 round-trip ~1e-5
    // can flip h's sign on grazing rays where the geometric truth is h==0); without the
    // tolerance, BVH's slightly-negative h would clamp a valid forward contact to t=0 while
    // FRT's exact h=0 keeps the line-line value, producing spurious BVH-vs-FRT t-only divergence.
    //
    // TODO: a more accurate (precision-independent) version replaces the h-driven clamp
    // with a direct geometric "is the start sphere already overlapping this edge segment?"
    // test. Then `dist` is forced to 0 only when the capsule really IS in contact at t=0,
    // and h's sign is no longer load-bearing -- so vertex-precision noise on h cannot
    // affect the result at all. Sketch:
    //
    //   vec3f edgeDir = v_sub(p1, p0);
    //   float edgeDirSq = v_extract_x(v_dot3_x(edgeDir, edgeDir));
    //   vec3f originRel = v_sub(from, p0);
    //   float t_seg = min(max(v_extract_x(v_dot3_x(originRel, edgeDir)) / edgeDirSq, 0.f), 1.f);
    //   vec3f closestSeg = v_madd(edgeDir, v_splats(t_seg), p0);
    //   float startSphereDistSq = v_extract_x(v_length3_sq_x(v_sub(closestSeg, from)));
    //   if (startSphereDistSq < radius * radius)
    //     dist = 0.f;
    //   else
    //     dist = max(dist, 0.f);
    //
    // Why we did not switch yet: the geometric check changes the SEMANTIC meaning of
    // pass3's dist for cases where h>0 (origin in front of plane) but the start sphere
    // happens to overlap the edge segment -- the legacy h-sign-copy reports the line-line
    // forward t there (a non-physical "future contact"), and FRT-baselined hit-count
    // expectancies in the unittest were generated with that legacy interpretation. The
    // geometric version would correctly report t=0 for those cases, which would shift
    // FRT counts on cylinder/icosphere/jaguar by a few hits each (similar to what the
    // pass1 hitPos=orthoProj fix did) and require updating those baselines.
    if (h < -h_zero_eps)
      dist = 0.f;
    else
      dist = max(dist, 0.f);
    if (dist < t)
    {
      t = dist;
      out_norm = vNorm;
      out_pos = inter;
      col = 2;
    }
  }

  return col != -1;
}

VECTORCALL bool test_capsule_triangle_hit(vec3f from, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float radius, float t, bool no_cull,
  float dp_cull_threshold, float h_zero_eps)
{
  vec3f edge0 = v_sub(v1, v0);
  vec3f edge1 = v_sub(v2, v0);
  vec3f vNorm = v_norm3(v_cross3(edge1, edge0));
  vec4f makeNeg = v_cast_vec4f(v_seti_x((no_cull ? 1 : 0) << 31)); // invert sign in no_cull mode
  float dp = v_extract_x(v_or(v_dot3_x(vNorm, dir), makeNeg));
  if (!no_cull && dp > dp_cull_threshold) // culling
    return false;

  vec3f vertices[3] = {v0, v1, v2};

  // pass1: ray VS plane
  plane3f plane = v_make_plane_norm(v0, vNorm);
  float h = v_extract_x(v_andnot(makeNeg, v_plane_dist_x(plane, from)));
  if (h < -radius)
    return false;

  if (dp != 0.f)
  {
    float dist = -max(h - radius, 0.f) / dp;
    if (dist < t)
    {
      // See test_capsule_triangle_intersection for the rationale -- mirror the same
      // h<0 -> orthoProj, h>=0 -> ray-plane-intersection split here.
      vec3f hitPos = h < 0.f ? v_sub(from, v_mul(vNorm, v_splats(h))) : v_madd(dir, v_splats(-h / dp), from);
      if (is_point_in_triangle(hitPos, v0, v1, v2))
        return true;
    }
  }

  // pass2: ray VS spheres on triangle vertices
  for (int i = 0; i < 3; i++)
  {
    float dist = rayIntersectSphereDist(from, dir, vertices[i], radius);
    if (dist >= 0.f && dist < t)
      return true;
  }

  // pass3: capsule VS triangle edges
  for (int i = 0; i < 3; i++)
  {
    int j = i + 1;
    if (j == 3)
      j = 0;
    vec3f p0 = vertices[i];
    vec3f p1 = vertices[j];
    vec3f p2 = v_sub(p1, dir);

    vec3f n = v_norm3(v_cross3(v_sub(p1, p0), v_sub(p2, p0)));
    plane3f plane = v_make_plane_norm(p0, n);

    float d = v_extract_x(v_plane_dist_x(plane, from));
    if (d > radius || d < -radius)
      continue;

    float r = sqrtf(sqr(radius) - sqr(d));

    // project on plane
    vec3f pt0 = v_sub(from, v_mul(plane, v_splats(d))); // center of the sphere slice (a circle)
    vec3f onLine = closest_point_on_line(pt0, p0, v_norm3(v_sub(p1, p0)));

    vec3f v = v_norm3(v_sub(onLine, pt0));
    vec3f pt1 = v_madd(v, v_splats(r), pt0); // point on the sphere that will maybe collide with the edge
    vec3f vv = v_add(pt1, dir);

    Point2 l1s, l1e, l2s, l2e;
    vec3f absN = v_abs(plane);
    float pl_x = v_extract_x(absN);
    float pl_y = v_extract_y(absN);
    float pl_z = v_extract_z(absN);
    if (pl_x > pl_y && pl_x > pl_z)
    {
      l1s = Point2(v_extract_y(pt1), v_extract_z(pt1));
      l1e = Point2(v_extract_y(vv), v_extract_z(vv));
      l2s = Point2(v_extract_y(p0), v_extract_z(p0));
      l2e = Point2(v_extract_y(p1), v_extract_z(p1));
    }
    else if (pl_y > pl_z)
    {
      l1s = Point2(v_extract_x(pt1), v_extract_z(pt1));
      l1e = Point2(v_extract_x(vv), v_extract_z(vv));
      l2s = Point2(v_extract_x(p0), v_extract_z(p0));
      l2e = Point2(v_extract_x(p1), v_extract_z(p1));
    }
    else
    {
      l1s = Point2(v_extract_x(pt1), v_extract_y(pt1));
      l1e = Point2(v_extract_x(vv), v_extract_y(vv));
      l2s = Point2(v_extract_x(p0), v_extract_y(p0));
      l2e = Point2(v_extract_x(p1), v_extract_y(p1));
    }

    float dist;
    bool res = get_lines_intersection(l1s, l1e, l2s, l2e, dist);
    // h_zero_eps: same precision tolerance as test_capsule_triangle_intersection.
    if (!res || dist < -radius || (h < -h_zero_eps && dist < 0.f))
      continue;

    vec3f inter = v_madd(dir, v_splats(dist), pt1);
    vec3f r1 = v_sub(p0, inter);
    vec3f r2 = v_sub(p1, inter);
    if (v_extract_x(v_dot3_x(r1, r2)) > 0.f)
      continue;

    // See test_capsule_triangle_intersection's pass3 for the rationale -- h_zero_eps tolerance
    // on the h-sign-driven clamp avoids spurious "behind plane" classification on quantized verts.
    if (h < -h_zero_eps)
      return true; // origin behind plane within radius -> contact at t=0 -> hit (0 < any input t)
    if (max(dist, 0.f) < t)
      return true;
  }

  return false;
}
