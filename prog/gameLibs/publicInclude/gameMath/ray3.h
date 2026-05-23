//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_vecMathCompatibility.h>

struct Ray3
{
  vec3f start;
  vec3f dir;
  float length = 0.f;

  vec3f end() const { return v_madd(dir, v_splats(length), start); }

  Ray3() : start(v_zero()), dir(v_make_vec4f(1.f, 0.f, 0.f, 0.f)) {}
  Ray3(vec3f s, vec3f d, float l) : start(s), dir(d), length(l) {}
};

inline Ray3 operator*(mat44f_cref tm, const Ray3 &r)
{
  return {v_mat44_mul_vec3p(tm, r.start), v_norm3(v_mat44_mul_vec3v(tm, r.dir)), r.length};
}

inline Ray3 make_ray_from_segment(vec3f p1, vec3f p2)
{
  vec3f diff = v_sub(p2, p1);
  return {p1, v_norm3_safe(diff, v_zero()), v_extract_x(v_length3(diff))};
}

inline Ray3 make_ray_from_segment_and_t(vec3f p1, vec3f p2, float t) { return {p1, v_norm3_safe(v_sub(p2, p1), v_zero()), t}; }
