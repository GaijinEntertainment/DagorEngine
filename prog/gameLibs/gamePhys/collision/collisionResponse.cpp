// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_span.h>
#include <generic/dag_tab.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/collision/collisionInfo.h>
#include <memory/dag_framemem.h>
#include <EASTL/sort.h>

#include <gamePhys/collision/collisionResponse.h>

#define DEBUG_ENERGY 0

const float gamephys::CollisionInfoCompressed::maxSurfVel = 150.0f;

void daphys::resolve_penetration(DPoint3 &pos, Quat &ori, dag::ConstSpan<gamephys::CollisionContactData> contacts, double inv_mass,
  const DPoint3 &inv_moi, double /*dt*/, bool use_future_contacts, int num_iter, float linear_slop, float erp)
{
  if (contacts.empty())
    return;
  Tab<gamephys::SeqImpulseInfo> collisions(framemem_ptr());
  collisions.reserve(contacts.size());
  Point3 posFlt = Point3::xyz(pos);
  Quat invOrient = inverse(ori);
  for (int i = 0; i < contacts.size(); ++i)
  {
    const gamephys::CollisionContactData &contact = contacts[i];
    if (contact.depth >= 0.f && !use_future_contacts)
      continue;
    DPoint3 localNorm = normalize(dpoint3(invOrient * contact.wnormB));
    DPoint3 localPos = dpoint3(invOrient * (contact.wpos - posFlt));
    gamephys::SeqImpulseInfo &info = collisions.push_back();
    info.axis1 = contact.wnormB;
    info.w1 = localPos % localNorm;
    info.b = inv_mass + info.w1 * hadamard_product(info.w1, inv_moi);
    info.depth = contact.depth;
    info.appliedImpulse = info.fullAppliedImpulse = 0.f;
    info.type = gamephys::SeqImpulseInfo::TYPE_COLLISION;
    info.bounce = 0.0;
    info.friction = -1.0;
    info.contactIndex = i;
    info.impulseLimitIndex1 = info.impulseLimitIndex2 = -1;
  }

  if (collisions.empty())
    return;

  DPoint3 pseudoVel(0.0, 0.0, 0.0);
  DPoint3 pseudoOmega(0.0, 0.0, 0.0);
  for (int iter = 0; iter < num_iter; ++iter)
  {
    bool hadImpulse = false;
    for (gamephys::SeqImpulseInfo &info : collisions)
    {
      double a = info.axis1 * pseudoVel + info.w1 * pseudoOmega;
      double b = info.b;
      double lambda = safediv((-info.depth - linear_slop) * erp - a, b);
      double finalImpulse = max(info.appliedImpulse + lambda, 0.0);
      lambda = finalImpulse - info.appliedImpulse;
      if (!float_nonzero(lambda))
        continue;
      info.appliedImpulse += lambda;
      info.fullAppliedImpulse += lambda;
      hadImpulse = true;
      pseudoVel += info.axis1 * lambda * inv_mass;
      DPoint3 deltaOmega = info.w1 * lambda;
      pseudoOmega += hadamard_product(deltaOmega, inv_moi);
    }
    if (!hadImpulse) // preemptive exit
      break;
  }

  pos += pseudoVel;
  Quat orientInc = Quat(pseudoOmega, length(pseudoOmega));
  ori = ori * orientInc;
}

void daphys::resolve_simple_velocity(DPoint3 &vel, dag::ConstSpan<gamephys::CollisionContactData> contacts, int num_iter)
{
  if (contacts.empty())
    return;
  Tab<gamephys::SeqImpulseInfo> collisions(framemem_ptr());
  collisions.reserve(contacts.size());
  for (int i = 0; i < contacts.size(); ++i)
  {
    const gamephys::CollisionContactData &contact = contacts[i];
    if (contact.depth >= 0.f)
      continue; // not a penetration yet
    gamephys::SeqImpulseInfo &info = collisions.push_back();
    info.axis1 = contact.wnormB;
    info.b = 1.f;
    info.depth = contact.depth;
    info.appliedImpulse = info.fullAppliedImpulse = 0.f;
    info.type = gamephys::SeqImpulseInfo::TYPE_COLLISION;
    info.bounce = 0.0;
    info.friction = -1.0;
    info.contactIndex = i;
  }

  if (collisions.empty())
    return;

  DPoint3 addVel(0.0, 0.0, 0.0);
  for (int iter = 0; iter < num_iter; ++iter)
  {
    bool hadImpulse = false;
    for (int i = 0; i < collisions.size(); ++i)
    {
      gamephys::SeqImpulseInfo &info = collisions[i];
      double a = info.axis1 * (vel + addVel);
      double b = info.b;
      double lambda = safediv(-a, b);
      if (lambda < 0.0) // Think about accumulating lambdas, for more accurate (but with higher iterations needed) response
        continue;
      hadImpulse = true;
      addVel += info.axis1 * lambda;
    }
    if (!hadImpulse) // preemptive exit
      break;
  }

  vel += addVel;
}

#if DEBUG_ENERGY
static double calc_energy(double mass, const DPoint3 &moi, const DPoint3 &vel, const DPoint3 &omega)
{
  return 0.5 * mass * lengthSq(vel) + 0.5 * (sqr(moi.x * omega.x) + sqr(moi.y * omega.y) + sqr(moi.z * omega.z));
}
#endif

struct ContactSortPair
{
  float appliedImpulse;
  size_t idx;
};

void daphys::cache_solved_contacts(dag::ConstSpan<gamephys::SeqImpulseInfo> collisions, Tab<gamephys::CachedContact> &cached_contacts,
  const size_t max_count)
{
  // sort by applied impulse
  Tab<ContactSortPair> sortedContacts;
  for (size_t i = 0; i < collisions.size(); ++i)
  {
    const gamephys::SeqImpulseInfo &info = collisions[i];
    sortedContacts.push_back({float(info.appliedImpulse), i});
  }
  eastl::sort(sortedContacts.begin(), sortedContacts.end(), [](auto &a, auto &b) { return a.appliedImpulse > b.appliedImpulse; });
  // populate with N most powerful contacts
  for (size_t i = 0; i < min(max_count, size_t(sortedContacts.size())); ++i)
  {
    const size_t collIdx = sortedContacts[i].idx;
    const gamephys::SeqImpulseInfo &info = collisions[collIdx];

    gamephys::CachedContact &contact = cached_contacts.push_back();
    contact.wpos = info.wpos;
    contact.localPos = Point3::xyz(info.pos);
    contact.wnorm = Point3::xyz(info.axis1);
    contact.appliedImpulse = info.appliedImpulse;
    contact.depth = info.depth;
  }
}

void daphys::resolve_contacts(const DPoint3 &pos, const Quat &ori, DPoint3 &vel, DPoint3 &omega,
  dag::ConstSpan<gamephys::CollisionContactData> contacts, double inv_mass, const DPoint3 &inv_moi, const ResolveContactParams &params,
  int num_iter)
{
  Tab<gamephys::SeqImpulseInfo> collisions(framemem_ptr());
  daphys::convert_contacts_to_solver_data(pos, ori, contacts, collisions, inv_mass, inv_moi, params);
  daphys::energy_conservation_vel_patch(collisions, vel, omega, params);
  daphys::resolve_contacts(ori, vel, omega, collisions, inv_mass, inv_moi, params, num_iter);
}

void daphys::resolve_contacts(const Quat &ori, DPoint3 &vel, DPoint3 &omega, Tab<gamephys::SeqImpulseInfo> &collisions,
  double inv_mass, const DPoint3 &inv_moi, const daphys::ResolveContactParams &params, int num_iter)
{
  if (collisions.empty())
    return;

  Quat invOrient = inverse(ori);
  for (int iter = 0; iter < num_iter; ++iter)
  {
    bool hadImpulse = false;
    for (gamephys::SeqImpulseInfo &info : collisions)
    {
      double a = info.axis1 * vel + info.w1 * omega + info.depth * params.baumgarteFactor;
      double b = info.b;
      double velAfterContact = max((-info.axis1 * vel * params.bounce) - params.minSpeedForBounce, 0.0);
      double lambda = safediv(velAfterContact - a, b);
      double finalImpulse = max(info.appliedImpulse + lambda, 0.0);
      lambda = finalImpulse - info.appliedImpulse;
      if (!float_nonzero(lambda))
        continue;
      info.appliedImpulse += lambda;
      info.fullAppliedImpulse += lambda;
      hadImpulse = true;
      DPoint3 curAddVel = info.axis1 * lambda * inv_mass;
      DPoint3 deltaOmega = info.w1 * lambda;
      DPoint3 curAddOmega = hadamard_product(deltaOmega, inv_moi);
      vel += curAddVel;
      omega += curAddOmega;
    }
    if (!hadImpulse) // preemptive exit
      break;
  }

  Tab<gamephys::SeqImpulseInfo> frictionConstraints(framemem_ptr());
  frictionConstraints.resize(collisions.size() * 2);
  int ci = 0;
  for (const auto &collision : collisions)
  {
    gamephys::SeqImpulseInfo &info1 = frictionConstraints[ci * 2];
    gamephys::SeqImpulseInfo &info2 = *(&info1 + 1);

    DPoint3 localPos = collision.pos;
    DPoint3 velProj = vel - (vel * collision.axis1) * collision.axis1;
    info1.axis1 = normalize(velProj);
    info2.axis1 = normalize(collision.axis1 % info1.axis1);
    DPoint3 localAxis1 = dpoint3(invOrient * info1.axis1);
    DPoint3 localAxis2 = dpoint3(invOrient * info2.axis1);
    info1.w1 = localPos % localAxis1;
    info2.w1 = localPos % localAxis2;
    info1.b = inv_mass + info1.w1 * hadamard_product(info1.w1, inv_moi);
    info2.b = inv_mass + info2.w1 * hadamard_product(info2.w1, inv_moi);
    info1.appliedImpulse = info1.fullAppliedImpulse = 0.0;
    info1.parentIndex = ci;
    info2.appliedImpulse = info2.fullAppliedImpulse = 0.0;
    info2.parentIndex = ci;
    ++ci;
  }

  for (int iter = 0; iter < num_iter; ++iter)
  {
    bool hadImpulse = false;
    for (int i = 0; i < frictionConstraints.size(); ++i)
    {
      gamephys::SeqImpulseInfo &info = frictionConstraints[i];
      gamephys::SeqImpulseInfo &parent = collisions[info.parentIndex];
      double a = info.axis1 * vel + info.w1 * omega;
      double b = info.b;
      double lambda = safediv(-a, b);
      const double minFriction = max(-parent.appliedImpulse, params.minFriction) * params.friction;
      const double maxFriction = min(parent.appliedImpulse, params.maxFriction) * params.friction;
      double finalImpulse = clamp(info.appliedImpulse + lambda, minFriction, maxFriction);
      lambda = finalImpulse - info.appliedImpulse;
      if (!float_nonzero(lambda))
        continue;
      info.appliedImpulse += lambda;
      info.fullAppliedImpulse += lambda;
      hadImpulse = true;
      DPoint3 curAddVel = info.axis1 * lambda * inv_mass;
      DPoint3 deltaOmega = info.w1 * lambda;
      DPoint3 curAddOmega = hadamard_product(deltaOmega, inv_moi);
      vel += curAddVel;
      omega += curAddOmega;
    }
    if (!hadImpulse) // preemptive exit
      break;
  }

#if DEBUG_ENERGY
  double mass = safeinv(inv_mass);
  DPoint3 moi(safeinv(inv_moi.x), safeinv(inv_moi.y), safeinv(inv_moi.z));
  double energy = calc_energy(mass, moi, vel, omega);
  double linEnergy = calc_energy(mass, moi, vel, DPoint3(0, 0, 0));
  double rotEnergy = calc_energy(mass, moi, DPoint3(0, 0, 0), omega);
  double newEnergy = calc_energy(mass, moi, vel + addVel, omega + addOmega);
  double linNewEnergy = calc_energy(mass, moi, vel + addVel, DPoint3(0, 0, 0));
  double rotNewEnergy = calc_energy(mass, moi, DPoint3(0, 0, 0), omega + addOmega);
  G_ASSERTF(newEnergy <= energy * 1.02 + 1e-4,
    "Gaining energy with prev energy equal to %f, new equal to %f, delta %f with"
    "vel " FMT_P3 " omega " FMT_P3 " gain vel " FMT_P3 " gain omega " FMT_P3 " mass %f moi " FMT_P3 "linDelta %f rotDelta %f",
    energy, newEnergy, newEnergy - energy, P3D(vel), P3D(omega), P3D(addVel), P3D(addOmega), mass, P3D(moi), linNewEnergy - linEnergy,
    rotNewEnergy - rotEnergy);
#endif
}


void daphys::energy_conservation_vel_patch(const Tab<gamephys::SeqImpulseInfo> &collisions, DPoint3 &vel, DPoint3 &omega,
  const ResolveContactParams &params)
{
  if (params.energyConservation >= 1.0)
    return;

  for (const gamephys::SeqImpulseInfo &info : collisions)
  {
    if (info.axis1.y <= params.noSleepAtTheSlopeCos)
      continue;
    vel = normalize(vel) * sqrtf(lengthSq(vel) * params.energyConservation);
    omega = normalize(omega) * sqrtf(lengthSq(omega) * params.energyConservation);
  }
}

void daphys::convert_contacts_to_solver_data(const DPoint3 &pos, const Quat &ori,
  dag::ConstSpan<gamephys::CollisionContactData> contacts, Tab<gamephys::SeqImpulseInfo> &collisions, double inv_mass,
  const DPoint3 &inv_moi, const ResolveContactParams &params)
{
  if (contacts.empty())
    return;
  collisions.reserve(contacts.size());
  Point3 posFlt = Point3::xyz(pos);
  Quat invOrient = inverse(ori);
  for (int i = 0; i < contacts.size(); ++i)
  {
    const gamephys::CollisionContactData &contact = contacts[i];
    if (contact.depth >= 0.f && !params.useFutureContacts)
      continue;
    DPoint3 localNorm = normalize(dpoint3(invOrient * contact.wnormB));
    DPoint3 localPos = dpoint3(invOrient * (contact.wpos - posFlt));
    gamephys::SeqImpulseInfo &info = collisions.push_back();
    info.pos = localPos;
    info.wpos = contact.wpos - contact.wnormB * contact.depth;
    info.axis1 = contact.wnormB;
    info.w1 = localPos % localNorm;
    info.b = inv_mass + info.w1 * hadamard_product(info.w1, inv_moi);
    info.depth = contact.depth;
    info.appliedImpulse = info.fullAppliedImpulse = 0.f;
    info.type = gamephys::SeqImpulseInfo::TYPE_COLLISION;
    info.bounce = 0.0;
    info.friction = -1.0;
    info.contactIndex = i;
    info.impulseLimitIndex1 = info.impulseLimitIndex2 = -1;
  }
}

void daphys::apply_cached_contacts(const DPoint3 &pos, const Quat &ori, DPoint3 &vel, DPoint3 &omega,
  dag::ConstSpan<gamephys::CachedContact> cached_contacts, Tab<gamephys::SeqImpulseInfo> &collisions, double inv_mass,
  const DPoint3 &inv_moi, const daphys::ResolveContactParams &params)
{
  if (params.warmstartingFactor <= 0.f || cached_contacts.empty())
    return;

  collisions.reserve(collisions.size() + cached_contacts.size());
  const Point3 posFlt = Point3::xyz(pos);
  const Quat invOrient = inverse(ori);
  constexpr float breakingThreshold = 0.1f;
  for (const gamephys::CachedContact &contact : cached_contacts)
  {
    const Point3 wpos = posFlt + ori * contact.localPos;
    const Point3 deltaPos = wpos - contact.wpos;
    if (lengthSq(deltaPos) >= sqr(breakingThreshold))
      continue;
    const double newDepth = deltaPos * contact.wnorm;
    // apply part of the impulse instantly
    const double lambda = contact.appliedImpulse * params.warmstartingFactor;

    DPoint3 localNorm = normalize(dpoint3(invOrient * contact.wnorm));
    DPoint3 localPos = dpoint3(contact.localPos);
    gamephys::SeqImpulseInfo &info = collisions.push_back();
    info.pos = localPos;
    info.wpos = contact.wpos;
    info.axis1 = contact.wnorm;
    info.w1 = localPos % localNorm;
    info.b = inv_mass + info.w1 * hadamard_product(info.w1, inv_moi);
    info.depth = newDepth;
    info.appliedImpulse = lambda;
    info.type = gamephys::SeqImpulseInfo::TYPE_COLLISION;
    info.bounce = 0.0;
    info.friction = -1.0;

    DPoint3 curAddVel = dpoint3(contact.wnorm) * lambda * inv_mass;
    DPoint3 deltaOmega = info.w1 * lambda;
    DPoint3 curAddOmega = hadamard_product(deltaOmega, inv_moi);
    vel += curAddVel;
    omega += curAddOmega;
  }
}

static void apply_lambda(daphys::SolverBodyInfo &body, const DPoint3 &n, const DPoint3 &w, double lambda, DPoint3 &vel, DPoint3 &omega)
{
  vel += n * lambda * body.invMass;
  omega += hadamard_product(w * lambda, body.invMoi);
}

void daphys::contacts_to_solver_data(const SolverBodyInfo *lhs, const SolverBodyInfo *rhs,
  dag::ConstSpan<gamephys::CollisionContactData> contacts, Tab<gamephys::SeqImpulseInfo> &collisions)
{
  double linearImpulse =
    max(lhs ? safeinv(lhs->invMass) * length(lhs->vel) : 0.0, rhs ? safeinv(rhs->invMass) * length(rhs->vel) : 0.0);
  double secondMult = lhs && rhs ? -1.0 : 1.0; // have both bodies so we need to invert second axes
  for (int i = 0, cnt = contacts.size(); i < cnt; ++i)
  {
    const gamephys::CollisionContactData &contact = contacts[i];
    if (contact.depth >= 0.f)
      continue;
    if (contact.objectInfo && contact.objectInfo->isRICollision() && contact.objectInfo->getDestructionImpulse() < linearImpulse)
      continue; // skip RI which will be destroyed in a collision
    DPoint3 localNorm1 = lhs ? dpoint3(lhs->itm % contact.wnormB) : ZERO<DPoint3>();
    DPoint3 localNorm2 = rhs ? dpoint3(rhs->itm % contact.wnormB) * secondMult : ZERO<DPoint3>();

    DPoint3 localPos1 = lhs ? dpoint3(lhs->itm * contact.wpos - lhs->cog) : ZERO<DPoint3>();
    DPoint3 localPos2 = rhs ? dpoint3(rhs->itm * contact.wpos - rhs->cog) : ZERO<DPoint3>();

    int frictionIdx = collisions.size();
    {
      gamephys::SeqImpulseInfo &info = collisions.push_back();
      info.pos = contact.wpos;
      info.axis1 = lhs ? contact.wnormB : ZERO<DPoint3>();
      info.axis2 = rhs ? contact.wnormB * secondMult : ZERO<DPoint3>();
      info.w1 = localPos1 % localNorm1;
      info.w2 = localPos2 % localNorm2;
      info.b = (lhs ? lhs->invMass + info.w1 * hadamard_product(info.w1, lhs->invMoi) : 0.0) +
               (rhs ? rhs->invMass + info.w2 * hadamard_product(info.w2, rhs->invMoi) : 0.0);
      info.depth = contact.depth;
      info.appliedImpulse = info.fullAppliedImpulse = 0.f;
      info.type = gamephys::SeqImpulseInfo::TYPE_COLLISION;
      info.bounce = 0.0;
      info.friction = -1.0;
      info.contactIndex = i;
      info.impulseLimitIndex1 = info.impulseLimitIndex2 = -1;
      info.parentIndex = frictionIdx;
    }

    if (!lhs || !rhs) // add friction only for pairs (may be changed in future, just to not change any behaviour in WT for now)
      continue;

    {
      DPoint3 relVelLin = lhs->vel - rhs->vel;
      DPoint3 relVelRot = dpoint3(lhs->tm % (lhs->omega % localPos1) - rhs->tm % (rhs->omega % localPos2));
      DPoint3 relVel = relVelLin + relVelRot;
      gamephys::SeqImpulseInfo &fricInfo1 = collisions.push_back();
      const gamephys::SeqImpulseInfo &info = collisions[frictionIdx];
      fricInfo1.pos = info.pos;
      fricInfo1.axis1 = normalize(relVel - info.axis1 * (info.axis1 * relVel));
      fricInfo1.axis2 = -fricInfo1.axis1;
      fricInfo1.w1 = dpoint3((lhs->itm * fricInfo1.pos - lhs->cog) % (lhs->itm % fricInfo1.axis1));
      fricInfo1.w2 = dpoint3((rhs->itm * fricInfo1.pos - rhs->cog) % (rhs->itm % fricInfo1.axis2));
      fricInfo1.b = lhs->invMass + rhs->invMass + fricInfo1.w1 * hadamard_product(fricInfo1.w1, lhs->invMoi) +
                    fricInfo1.w2 * hadamard_product(fricInfo1.w2, rhs->invMoi);
      fricInfo1.depth = 0.f;
      fricInfo1.appliedImpulse = fricInfo1.fullAppliedImpulse = 0.f;
      fricInfo1.type = gamephys::SeqImpulseInfo::TYPE_FRICTION_1;
      fricInfo1.bounce = 0.0;
      fricInfo1.friction = -1.0;
      fricInfo1.contactIndex = i;
      fricInfo1.impulseLimitIndex1 = fricInfo1.impulseLimitIndex2 = -1;
      fricInfo1.parentIndex = frictionIdx;
    }

    {
      gamephys::SeqImpulseInfo &fricInfo2 = collisions.push_back();
      const gamephys::SeqImpulseInfo &info = collisions[frictionIdx];
      const gamephys::SeqImpulseInfo &fricInfo1 = *(&fricInfo2 - 1); //-V557
      fricInfo2.pos = info.pos;
      fricInfo2.axis1 = normalize(info.axis1 % fricInfo1.axis1);
      fricInfo2.axis2 = -fricInfo2.axis1;
      fricInfo2.w1 = dpoint3((lhs->itm * fricInfo2.pos - lhs->cog) % (lhs->itm % fricInfo2.axis1));
      fricInfo2.w2 = dpoint3((rhs->itm * fricInfo2.pos - rhs->cog) % (rhs->itm % fricInfo2.axis2));
      fricInfo2.b = lhs->invMass + rhs->invMass + fricInfo2.w1 * hadamard_product(fricInfo2.w1, lhs->invMoi) +
                    fricInfo2.w2 * hadamard_product(fricInfo2.w2, rhs->invMoi);
      fricInfo2.depth = 0.f;
      fricInfo2.appliedImpulse = fricInfo2.fullAppliedImpulse = 0.f;
      fricInfo2.type = gamephys::SeqImpulseInfo::TYPE_FRICTION_2;
      fricInfo2.bounce = 0.0;
      fricInfo2.friction = -1.0;
      fricInfo2.contactIndex = i;
      fricInfo2.impulseLimitIndex1 = fricInfo2.impulseLimitIndex2 = -1;
      fricInfo2.parentIndex = frictionIdx;
    }
  }
}

static void consume_impulse_reserve(daphys::SolverBodyInfo &lhs, daphys::SolverBodyInfo &rhs, const gamephys::SeqImpulseInfo &info,
  double &out_lambda)
{
  const double commonImpulseLimit = min(lhs.commonImpulseLimit, rhs.commonImpulseLimit);
  double lambdaAbs = min(fabs(out_lambda), commonImpulseLimit);
  if (info.impulseLimitIndex1 >= 0 && info.impulseLimitIndex2 >= 0)
  {
    const double impulseLimit =
      min(commonImpulseLimit, min(lhs.impulseLimits[info.impulseLimitIndex1], rhs.impulseLimits[info.impulseLimitIndex2]));
    lambdaAbs = min(lambdaAbs, impulseLimit);
    lhs.impulseLimits[info.impulseLimitIndex1] = max(lhs.impulseLimits[info.impulseLimitIndex1] - lambdaAbs, 0.0);
    rhs.impulseLimits[info.impulseLimitIndex2] = max(rhs.impulseLimits[info.impulseLimitIndex2] - lambdaAbs, 0.0);
  }
  else if (info.impulseLimitIndex1 >= 0)
  {
    double &impulseLimit = lhs.impulseLimits[info.impulseLimitIndex1];
    lambdaAbs = min(lambdaAbs, impulseLimit);
    impulseLimit = max(impulseLimit - lambdaAbs, 0.0);
  }
  else if (info.impulseLimitIndex2 >= 0)
  {
    double &impulseLimit = rhs.impulseLimits[info.impulseLimitIndex2];
    lambdaAbs = min(lambdaAbs, impulseLimit);
    impulseLimit = max(impulseLimit - lambdaAbs, 0.0);
  }
  lhs.commonImpulseLimit = max(lhs.commonImpulseLimit - lambdaAbs, 0.0);
  rhs.commonImpulseLimit = max(rhs.commonImpulseLimit - lambdaAbs, 0.0);
  out_lambda = copysign(lambdaAbs, out_lambda);
}

bool daphys::resolve_pair_velocity(SolverBodyInfo &lhs, SolverBodyInfo &rhs, dag::Span<gamephys::SeqImpulseInfo> collisions,
  double fric_k, double bounce_k, int num_iter, BodyInnerConstraints l_constraints, BodyInnerConstraints r_constraints)
{
  bool hadCollision = false;
  for (int iteration = 0; iteration < num_iter; ++iteration)
  {
    bool hadImpulse = false;
    for (gamephys::SeqImpulseInfo &info : collisions)
    {
      DPoint3 vel1 = lhs.vel + lhs.addVel;
      DPoint3 vel2 = rhs.vel + rhs.addVel;
      DPoint3 omega1 = lhs.omega + lhs.addOmega;
      DPoint3 omega2 = rhs.omega + rhs.addOmega;

      DPoint3 n1 = info.axis1;
      DPoint3 n2 = info.axis2;
      DPoint3 w1 = info.w1;
      DPoint3 w2 = info.w2;

      double a1 = n1 * vel1 + w1 * omega1;
      double a2 = n2 * vel2 + w2 * omega2;
      double a = a1 + a2;
      const double bounce = info.bounce != 0.0 ? info.bounce : bounce_k;
      a *= (1.0 + bounce);

      double b = info.b;
      double lambda = -safediv(a, b);
      if (lambda < 0.0 && info.type == gamephys::SeqImpulseInfo::TYPE_COLLISION)
        continue;

      info.fullAppliedImpulse += lambda;

      if (info.type > gamephys::SeqImpulseInfo::TYPE_COLLISION && info.parentIndex >= 0)
      {
        const double friction = info.friction > 0.0 ? info.friction : fric_k;
        const double frictionImpulseLimit = collisions[info.parentIndex].appliedImpulse * friction;
        const double wishImpulse = clamp(info.appliedImpulse + lambda, -frictionImpulseLimit, frictionImpulseLimit);
        lambda = wishImpulse - info.appliedImpulse;
        info.fullAppliedImpulse = clamp(info.fullAppliedImpulse, -frictionImpulseLimit, frictionImpulseLimit);
      }

      hadImpulse = true;
      hadCollision = true;
      consume_impulse_reserve(lhs, rhs, info, lambda);
      apply_lambda(lhs, n1, w1, lambda, lhs.addVel, lhs.addOmega);
      apply_lambda(rhs, n2, w2, lambda, rhs.addVel, rhs.addOmega);
      info.appliedImpulse += lambda;
    }

    if (l_constraints)
      l_constraints(lhs);
    if (r_constraints)
      r_constraints(rhs);

    if (!hadImpulse)
      break;
  }
  return hadCollision;
}

void daphys::resolve_pair_penetration(SolverBodyInfo &lhs, SolverBodyInfo &rhs, dag::ConstSpan<gamephys::SeqImpulseInfo> collisions,
  double erp, int num_iter, BodyInnerConstraints l_constraints, BodyInnerConstraints r_constraints)
{
  for (int iteration = 0; iteration < num_iter; ++iteration)
  {
    bool hadImpulse = false;
    for (const gamephys::SeqImpulseInfo &info : collisions)
    {
      if (info.type != gamephys::SeqImpulseInfo::TYPE_COLLISION)
        continue;
      DPoint3 vel1 = lhs.vel + lhs.addVel + lhs.pseudoVel;
      DPoint3 vel2 = rhs.vel + rhs.addVel + rhs.pseudoVel;
      DPoint3 omega1 = lhs.omega + lhs.addOmega + lhs.pseudoOmega;
      DPoint3 omega2 = rhs.omega + rhs.addOmega + rhs.pseudoOmega;

      DPoint3 n1 = info.axis1;
      DPoint3 n2 = info.axis2;
      DPoint3 w1 = info.w1;
      DPoint3 w2 = info.w2;

      double a = n1 * vel1 + w1 * omega1 + n2 * vel2 + w2 * omega2;
      double b = info.b;
      double lambda = safediv(erp * rabs(info.depth) - a, b);
      if (lambda <= 0.0)
        continue;
      hadImpulse = true;

      apply_lambda(lhs, n1, w1, lambda, lhs.pseudoVel, lhs.pseudoOmega);
      apply_lambda(rhs, n2, w2, lambda, rhs.pseudoVel, rhs.pseudoOmega);
    }

    if (l_constraints)
      l_constraints(lhs);
    if (r_constraints)
      r_constraints(rhs);

    if (!hadImpulse)
      break;
  }
}
