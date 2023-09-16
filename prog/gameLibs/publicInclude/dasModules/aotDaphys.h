//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/aotDacoll.h>
#include <gamePhys/collision/collisionResponse.h>

MAKE_TYPE_FACTORY(ResolveContactParams, daphys::ResolveContactParams);
namespace bind_dascript
{
inline void daphys_resolve_penetration(DPoint3 &pos, Quat &quat, const das::TArray<gamephys::CollisionContactData> &contacts,
  double inv_mass, const DPoint3 &inv_moi, double dt, bool use_future_contacts, int num_iter, float linear_slop, float erp)
{
  using ContactSlice = dag::ConstSpan<gamephys::CollisionContactData>;
  ContactSlice contactsSlice(reinterpret_cast<const gamephys::CollisionContactData *>(contacts.data), contacts.size);
  daphys::resolve_penetration(pos, quat, contactsSlice, inv_mass, inv_moi, dt, use_future_contacts, num_iter, linear_slop, erp);
}

inline void daphys_resolve_contacts(const DPoint3 &pos, const Quat &quat, DPoint3 &vel, DPoint3 &omega,
  const das::TArray<gamephys::CollisionContactData> &contacts, double inv_mass, const DPoint3 &inv_moi,
  const daphys::ResolveContactParams &params, int num_iters)
{
  using ContactSlice = dag::ConstSpan<gamephys::CollisionContactData>;
  ContactSlice contactsSlice(reinterpret_cast<const gamephys::CollisionContactData *>(contacts.data), contacts.size);
  daphys::resolve_contacts(pos, quat, vel, omega, contactsSlice, inv_mass, inv_moi, params, num_iters);
}


} // namespace bind_dascript
