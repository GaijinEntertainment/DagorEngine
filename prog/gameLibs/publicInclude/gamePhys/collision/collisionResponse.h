//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/collision/collisionInfo.h>
#include <gamePhys/collision/solverBodyInfo.h>


namespace daphys
{
struct ResolveContactParams
{
  bool useFutureContacts = false;

  double friction = 0.0;
  double minFriction = -VERY_BIG_NUMBER;
  double maxFriction = VERY_BIG_NUMBER;

  double bounce = 0.0;
  double minSpeedForBounce = 0.0;

  double energyConservation = 1.0;
  double noSleepAtTheSlopeCos = -1.0;

  double baumgarteFactor = 0.0;

  double warmstartingFactor = 0.0;
};

void resolve_penetration(DPoint3 &pos, Quat &ori, dag::ConstSpan<gamephys::CollisionContactData> contacts, double inv_mass,
  const DPoint3 &inv_moi, double dt, bool use_future_contacts, int num_iter = 5, float linear_slop = 0.01, float erp = 1.f);
void resolve_simple_velocity(DPoint3 &vel, dag::ConstSpan<gamephys::CollisionContactData> contacts, int num_iter = 5);

void resolve_contacts(const DPoint3 &pos, const Quat &ori, DPoint3 &vel, DPoint3 &omega,
  dag::ConstSpan<gamephys::CollisionContactData> contacts, double inv_mass, const DPoint3 &inv_moi, const ResolveContactParams &params,
  int num_iter = 5);
void resolve_contacts(const Quat &ori, DPoint3 &vel, DPoint3 &omega, Tab<gamephys::SeqImpulseInfo> &collisions, double inv_mass,
  const DPoint3 &inv_moi, const daphys::ResolveContactParams &params, int num_iter);

void convert_contacts_to_solver_data(const DPoint3 &pos, const Quat &ori, dag::ConstSpan<gamephys::CollisionContactData> contacts,
  Tab<gamephys::SeqImpulseInfo> &collisions, double inv_mass, const DPoint3 &inv_moi, const ResolveContactParams &params);

void apply_cached_contacts(const DPoint3 &pos, const Quat &ori, DPoint3 &vel, DPoint3 &omega,
  dag::ConstSpan<gamephys::CachedContact> cached_contacts, Tab<gamephys::SeqImpulseInfo> &collisions, double inv_mass,
  const DPoint3 &inv_moi, const daphys::ResolveContactParams &params);

void cache_solved_contacts(dag::ConstSpan<gamephys::SeqImpulseInfo> collisions, Tab<gamephys::CachedContact> &cached_contacts,
  const size_t max_count);

void energy_conservation_vel_patch(const Tab<gamephys::SeqImpulseInfo> &collisions, DPoint3 &vel, DPoint3 &omega,
  const ResolveContactParams &params);

void contacts_to_solver_data(const SolverBodyInfo *lhs, const SolverBodyInfo *rhs,
  dag::ConstSpan<gamephys::CollisionContactData> contacts, Tab<gamephys::SeqImpulseInfo> &collisions);
bool resolve_pair_velocity(SolverBodyInfo &lhs, SolverBodyInfo &rhs, dag::Span<gamephys::SeqImpulseInfo> collisions,
  double fric_k = 0.7, double bounce = 0.0, int num_iter = 5);
void resolve_pair_penetration(SolverBodyInfo &lhs, SolverBodyInfo &rhs, dag::ConstSpan<gamephys::SeqImpulseInfo> collisions,
  double erp = 2.0, int num_iter = 5);
} // namespace daphys
