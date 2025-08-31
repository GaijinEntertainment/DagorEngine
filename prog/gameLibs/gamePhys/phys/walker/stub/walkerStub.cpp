// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanControlState.h>
void HumanControlState::setWalkDir(const Point2 &) { G_ASSERT(0); }
void HumanControlState::setWorldWalkDir(const Point2 &, const Point2 &) { G_ASSERT(0); }
void HumanControlState::setWishShootLookDir(const Point3 &) { G_ASSERT(0); }
void HumanControlState::setWishShootDir(const Point3 &) { G_ASSERT(0); }
void HumanControlState::setWishLookDir(const Point3 &) { G_ASSERT(0); }

#include <gamePhys/phys/walker/humanPhys.h>
HumanPhysState::HumanPhysState() { G_ASSERT(0); }
bool HumanPhysState::deserialize(const danet::BitStream &, IPhysBase &) { G_ASSERT_RETURN(false, false); }

template class PhysicsBase<HumanPhysState, HumanControlState, CommonPhysPartialState>;

void HumanPhysState::resetStamina(float) { G_ASSERT(0); }

void HumanPhys::restoreStamina(float, float) { G_ASSERT(0); }

bool HumanPhys::canStartSprint() const { G_ASSERT_RETURN(false, false); }

float HumanPhys::getJumpStaminaDrain() const { G_ASSERT_RETURN(false, 0.0f); }

float HumanPhys::getClimbStaminaDrain() const { G_ASSERT_RETURN(false, 0.0f); }

Point3 HumanPhys::calcGunPos(class TMatrix const &, float, float, float, PrecomputedPresetMode) const
{
  G_ASSERT_RETURN(false, ZERO<Point3>());
}
TMatrix HumanPhys::calcGunTm(class TMatrix const &, float, float, float, PrecomputedPresetMode) const
{
  G_ASSERT_RETURN(false, ZERO<TMatrix>());
}
Point3 HumanPhys::calcCcdPos() const { G_ASSERT_RETURN(false, ZERO<Point3>()); }
Point3 HumanPhys::calcGravityPivot() const { G_ASSERT_RETURN(false, ZERO<Point3>()); }
bool HumanPhys::processCcdOffset(TMatrix &, const Point3 &, const Point3 &, float, float, bool, const Point3 &)
{
  G_ASSERT_RETURN(false, false);
}
bool HumanPhys::isAiming() const { G_ASSERT_RETURN(false, false); }

void HumanPhys::setWeaponLen(float, int) { G_ASSERT(0); }
void HumanPhys::tryClimbing(bool, bool, const Point3 &, float) { G_ASSERT(0); }
void HumanPhys::resetClimbing() { G_ASSERT(0); }
Point3 HumanPhys::calcCollCenter() const { G_ASSERT_RETURN(false, ZERO<Point3>()); }

#include <gamePhys/phys/walker/humanAnimState.h>
HumanAnimState::StateResult HumanAnimState::updateState(HumanStatePos, HumanStateMove, StateJump, HumanStateUpperBody,
  HumanAnimStateFlags)
{
  G_ASSERT_RETURN(false, HumanAnimState::StateResult());
}
