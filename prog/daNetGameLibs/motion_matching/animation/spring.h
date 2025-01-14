// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>

constexpr float PIf = 3.14159265358979323846f;
constexpr float LN2f = 0.69314718056f;

VECTORCALL static inline quat4f v_quat_abs(quat4f q) { return v_sel(q, v_neg(q), v_perm_wwww(q)); }

static inline quat4f quat_exp(vec3f v, vec4f eps = V_C_EPS_VAL)
{
  vec4f halfangle = v_length3(v);

  if (v_test_vec_x_ge(eps, halfangle)) // eps >= halfangle
  {
    return v_norm4(v_perm_xyzd(v, V_C_ONE)); // quat_normalize(quat(v.x, v.y, v.z, 1.f));
  }
  else
  {
    vec4f s, c;
    v_sincos(halfangle, s, c);
    return v_perm_xyzd(v_div(v_mul(v, s), halfangle), c); // quat(v * s / halfangle, c)
  }
}

//--------------------------------------
static inline vec3f quat_log(quat4f q, vec4f eps = V_C_EPS_VAL)
{
  vec4f length = v_length3(q);

  vec3f v = v_perm_xyzd(q, v_zero());

  if (v_test_vec_x_ge(eps, length)) // eps >= length
  {
    return v;
  }
  else
  {
    vec4f halfangle = v_acos(v_clamp(v_perm_wwww(q), v_neg(V_C_ONE), V_C_ONE)); // acosf(clampf(q.w, -1.0f, 1.0f));
    return v_mul(halfangle, v_div(v, length));                                  // halfangle * (v / length);
  }
}

static inline quat4f quat_from_scaled_angle_axis(vec3f v, vec4f eps = V_C_EPS_VAL)
{
  return quat_exp(v_mul(v, V_C_HALF), eps); // quat_exp(v / 2.0f, eps);
}

static inline vec3f quat_to_scaled_angle_axis(quat4f q, vec4f eps = V_C_EPS_VAL)
{
  return v_mul(V_C_TWO, quat_log(q, eps)); // 2.0f * quat_log(q, eps);
}

static inline vec3f damper_exact_v3(vec3f x, vec3f g, float halflife, float dt, float eps = 1e-5f)
{
  float t = 1.0f - expf(-(LN2f * dt) / (halflife + eps));
  return v_lerp_vec4f(v_splats(t), x, g);
}

static inline quat4f damper_exact_q(quat4f x, quat4f g, float halflife, float dt, float eps = 1e-5f)
{
  float t = 1.0f - expf(-(LN2f * dt) / (halflife + eps));
  return v_quat_qslerp(t, x, g);
}

static inline vec3f damp_adjustment_exact_v3(vec3f x, float halflife, float dt, float eps = 1e-5f)
{
  float t = expf(-(LN2f * dt) / (halflife + eps));
  return v_mul(x, v_splats(t));
}

static inline quat4f damp_adjustment_exact_q(quat4f x, float halflife, float dt, float eps = 1e-5f)
{
  float t = expf(-(LN2f * dt) / (halflife + eps));
  return v_quat_qslerp(t, V_C_UNIT_0001, x); // V_C_UNIT_0001 is quat()
}

//--------------------------------------

static inline float halflife_to_damping(float halflife, float eps = 1e-5f) { return (4.0f * LN2f) / (halflife + eps); }

static inline float damping_to_halflife(float damping, float eps = 1e-5f) { return (4.0f * LN2f) / (damping + eps); }

static inline float frequency_to_stiffness(float frequency)
{
  float x = 2.0f * PIf * frequency;
  return x * x;
}

static inline float stiffness_to_frequency(float stiffness) { return sqrtf(stiffness) / (2.0f * PIf); }

//--------------------------------------

static inline void simple_spring_damper_exact(float &x, float &v, const float x_goal, const float halflife, const float dt)
{
  float y = halflife_to_damping(halflife) / 2.0f;
  float j0 = x - x_goal;
  float j1 = v + j0 * y;
  float eydt = expf(-y * dt);

  x = eydt * (j0 + j1 * dt) + x_goal;
  v = eydt * (v - j1 * y * dt);
}

static inline void simple_spring_damper_exact_v3(vec3f &x, vec3f &v, const vec3f x_goal, const float halflife, const float dt)
{
  float y = halflife_to_damping(halflife) / 2.0f;
  vec4f eydt = v_splats(expf(-y * dt));

  vec3f j0 = v_sub(x, x_goal);                 // x - x_goal;
  vec3f j1 = v_add(v, v_mul(j0, v_splats(y))); // v + j0*y;

  x = v_add(v_mul(eydt, v_add(j0, v_mul(j1, v_splats(dt)))), x_goal); // eydt*(j0 + j1*dt) + x_goal;
  v = v_mul(eydt, v_sub(v, v_mul(j1, v_splats(y * dt))));             // eydt*(v - j1*y*dt);
}

static inline void simple_spring_damper_exact_q(quat4f &x, vec3f &v, const quat4f x_goal, const float halflife, const float dt)
{
  float y = halflife_to_damping(halflife) / 2.0f;
  vec4f eydt = v_splats(expf(-y * dt));

  vec3f j0 = quat_to_scaled_angle_axis(v_quat_abs(v_quat_mul_quat(x, v_quat_conjugate(x_goal))));
  vec3f j1 = v_add(v, v_mul(j0, v_splats(y))); // v + j0*y;

  vec3f axis = v_mul(eydt, v_add(j0, v_mul(j1, v_splats(dt)))); // eydt*(j0 + j1*dt))
  x = v_quat_mul_quat(quat_from_scaled_angle_axis(axis), x_goal);
  v = v_mul(eydt, v_sub(v, v_mul(j1, v_splats(y * dt)))); // eydt*(v - j1*y*dt);
}

//--------------------------------------

static inline void decay_spring_damper_exact(float &x, float &v, const float halflife, const float dt)
{
  float y = halflife_to_damping(halflife) / 2.0f;
  float j1 = v + x * y;
  float eydt = expf(-y * dt);

  x = eydt * (x + j1 * dt);
  v = eydt * (v - j1 * y * dt);
}

static inline void decay_spring_damper_exact_v3(vec3f &x, vec3f &v, const float halflife, const float dt)
{
  float y = halflife_to_damping(halflife) / 2.0f;
  vec3f j1 = v_add(v, v_mul(x, v_splats(y))); // v + x*y;
  vec4f eydt = v_splats(expf(-y * dt));

  x = v_mul(eydt, v_add(x, v_mul(j1, v_splats(dt))));     // eydt*(x + j1*dt);
  v = v_mul(eydt, v_sub(v, v_mul(j1, v_splats(y * dt)))); // eydt*(v - j1*y*dt);
}

static inline void decay_spring_damper_exact_q(quat4f &x, vec3f &v, const float halflife, const float dt)
{
  float y = halflife_to_damping(halflife) / 2.0f;

  vec3f j0 = quat_to_scaled_angle_axis(x);
  vec3f j1 = v_add(v, v_mul(j0, v_splats(y))); // v + j0*y;

  vec4f eydt = v_splats(expf(-y * dt));

  vec3f axis = v_mul(eydt, v_add(j0, v_mul(j1, v_splats(dt)))); // eydt*(j0 + j1*dt))
  x = quat_from_scaled_angle_axis(axis);
  v = v_mul(eydt, v_sub(v, v_mul(j1, v_splats(y * dt)))); // eydt*(v - j1*y*dt);
}
