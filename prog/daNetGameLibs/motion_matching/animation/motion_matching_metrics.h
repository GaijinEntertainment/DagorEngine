// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "motion_matching_features.h"

__forceinline float feature_distance_metric(
  const vec3f *__restrict current, const vec3f *__restrict next, const vec3f *__restrict weights, int count)
{
  vec4f result = v_zero();
  for (int i = 0; i < count; i++)
  {
    vec4f dif = v_mul(v_sub(current[i], next[i]), weights[i]);
    result = v_add(result, v_dot4(dif, dif));
  }
  return v_extract_x(result);
}

__forceinline bool feature_metric_need_update(vec4f &__restrict best_result,
  const vec3f *__restrict current,
  const vec3f *__restrict next,
  const vec3f *__restrict weights,
  float bias,
  int count)
{
  vec4f result = v_splats(bias);
  for (int i = 0; i < count; i++)
  {
    vec4f dif = v_mul(v_sub(current[i], next[i]), weights[i]);
    result = v_add(result, v_dot4(dif, dif));
    if (v_test_vec_x_ge(result, best_result))
    {
      return false;
    }
  }
  best_result = result;
  return true;
}

__forceinline bool feature_bounded_metric_need_update(vec4f best_result,
  const vec3f *__restrict current,
  const vec3f *__restrict bounds_min,
  const vec3f *__restrict bounds_max,
  const vec3f *__restrict weights,
  float bias,
  int count)
{
  vec4f result = v_splats(bias);
  for (int i = 0; i < count; i++)
  {
    vec4f dif = v_sub(current[i], v_clamp(current[i], bounds_min[i], bounds_max[i]));
    dif = v_mul(dif, weights[i]);
    result = v_add(result, v_dot4(dif, dif));
    if (v_test_vec_x_ge(result, best_result))
    {
      return false;
    }
  }
  return true;
}


__forceinline float path_metric(const vec3f *current, const vec3f *next, const vec3f *weights, int trajectory)
{
  return feature_distance_metric(current, next, weights, trajectory * TRAJECTORY_DIMENSION);
}