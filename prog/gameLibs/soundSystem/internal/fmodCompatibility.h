#ifndef _DAGOR_GAMELIB_SOUNDSYSTEM_FMODCOMPATIBILITY_H_
#define _DAGOR_GAMELIB_SOUNDSYSTEM_FMODCOMPATIBILITY_H_
#pragma once

#include <fmod_studio_common.h>
#include <fmod_common.h>

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>

#include <soundSystem/varId.h>

namespace sndsys
{
template <class T>
inline const FMOD_VECTOR &as_fmod_vector(const T &point)
{
  G_ASSERT(!check_nan(point));
  G_STATIC_ASSERT(sizeof(point) >= sizeof(FMOD_VECTOR));
  return *reinterpret_cast<const FMOD_VECTOR *>(&point);
}

inline const Point3 &as_point3(const FMOD_VECTOR &vector)
{
  G_STATIC_ASSERT(sizeof(vector) >= sizeof(Point3));
  return *reinterpret_cast<const Point3 *>(&vector);
}

struct Attributes3D : public FMOD_3D_ATTRIBUTES
{
  Attributes3D()
  {
    position = ZERO<FMOD_VECTOR>();
    velocity = ZERO<FMOD_VECTOR>();
    forward = as_fmod_vector(Point3(0, 0, 1));
    up = as_fmod_vector(Point3(0, 1, 0));
  }

  Attributes3D(const Point3 &new_position)
  {
    position = as_fmod_vector(new_position);
    velocity = ZERO<FMOD_VECTOR>();
    forward = as_fmod_vector(Point3(0, 0, 1));
    up = as_fmod_vector(Point3(0, 1, 0));
  }

  Attributes3D(const Point3 &new_position, const Point3 &new_velocity, const Point3 &new_forward, const Point3 &new_up)
  {
    position = as_fmod_vector(new_position);
    velocity = as_fmod_vector(new_velocity);
    forward = as_fmod_vector(new_forward);
    up = as_fmod_vector(new_up);
  }

  Attributes3D(const TMatrix &tm, const Point3 &new_velocity)
  {
    position = as_fmod_vector(tm.getcol(3));
    velocity = as_fmod_vector(new_velocity);
    forward = as_fmod_vector(tm.getcol(2));
    up = as_fmod_vector(tm.getcol(1));
  }

  Attributes3D(const TMatrix4 &tm, const Point3 &new_velocity)
  {
    position = as_fmod_vector(tm.getrow(3));
    velocity = as_fmod_vector(new_velocity);
    forward = as_fmod_vector(tm.getrow(2));
    up = as_fmod_vector(tm.getrow(1));
  }
};

inline const VarId &as_var_id(const FMOD_STUDIO_PARAMETER_ID &id)
{
  G_STATIC_ASSERT(eastl::is_trivially_copyable<VarId>::value);
  G_STATIC_ASSERT(eastl::is_trivially_copyable<FMOD_STUDIO_PARAMETER_ID>::value);
  G_STATIC_ASSERT(sizeof(VarId) == sizeof(FMOD_STUDIO_PARAMETER_ID));
  return reinterpret_cast<const VarId &>(id);
}
inline const FMOD_STUDIO_PARAMETER_ID &as_fmod_param_id(const VarId &var_id)
{
  G_STATIC_ASSERT(eastl::is_trivially_copyable<VarId>::value);
  G_STATIC_ASSERT(eastl::is_trivially_copyable<FMOD_STUDIO_PARAMETER_ID>::value);
  G_STATIC_ASSERT(sizeof(VarId) == sizeof(FMOD_STUDIO_PARAMETER_ID));
  return reinterpret_cast<const FMOD_STUDIO_PARAMETER_ID &>(var_id);
}
inline const FMOD_STUDIO_PARAMETER_ID *as_fmod_param_arr(const VarId *var_id)
{
  G_STATIC_ASSERT(eastl::is_trivially_copyable<VarId>::value);
  G_STATIC_ASSERT(eastl::is_trivially_copyable<FMOD_STUDIO_PARAMETER_ID>::value);
  G_STATIC_ASSERT(sizeof(VarId) == sizeof(FMOD_STUDIO_PARAMETER_ID));
  return reinterpret_cast<const FMOD_STUDIO_PARAMETER_ID *>(var_id);
}
} // namespace sndsys

#endif
