//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/rendInstGen.h>
#include <ecs/rendInst/riExtra.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <util/dag_delayedAction.h>

inline bool move_ri_extra_tm_ex(rendinst::riex_handle_t handle, const TMatrix &transform, const Point3 &vel, const Point3 &omega,
  bool do_not_lock)
{
  mat44f m;
  v_mat44_make_from_43cu_unsafe(m, transform.m[0]);
  if (rendinst::moveRIGenExtra44(handle, m, /*moved*/ true, do_not_lock))
  {
    dacoll::move_ri_instance(rendinst::RendInstDesc(handle), vel, omega);
    return true;
  }
  else // lock failed
    return false;
}

struct MoveRiExtraDA final : public DelayedAction
{
  rendinst::riex_handle_t handle;
  TMatrix tm;
  Point3 vel;
  Point3 omega;
  MoveRiExtraDA(rendinst::riex_handle_t h, const TMatrix &t, const Point3 &v, const Point3 &o) : handle(h), tm(t), vel(v), omega(o) {}
  void performAction() override
  {
    if (rendinst::isRiGenExtraValid(handle)) // rendinst might been destroyed since addition of this DA
      move_ri_extra_tm_ex(handle, tm, vel, omega, /*do_not_lock*/ false);
  }
};

inline void move_ri_extra_tm_with_motion(rendinst::riex_handle_t handle, const TMatrix &transform, const Point3 &vel,
  const Point3 &omega)
{
  if (!move_ri_extra_tm_ex(handle, transform, vel, omega, /*do_not_lock*/ true)) // if lock failed try again in delayed action
    add_delayed_action(new MoveRiExtraDA(handle, transform, vel, omega));
}

inline void move_ri_extra_tm(rendinst::riex_handle_t handle, const TMatrix &transform)
{
  move_ri_extra_tm_with_motion(handle, transform, ZERO<Point3>(), ZERO<Point3>());
}
