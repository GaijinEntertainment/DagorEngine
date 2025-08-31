// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/collision/collisionLib.h>
#include "phys/netPhys.h"
#include "phys/lagCompensation.h"
#include <memory/dag_framemem.h>

namespace bind_dascript
{

inline int getCurInterpDelayTicksPacked() { return PhysUpdateCtx::ctx.getCurInterpDelayTicksPacked(); }

inline bool test_obj_to_phys_collision(const CollisionObject &co_a, const das::float3x4 &tm_a, const ecs::EntityId eid_b)
{
  IPhysBase *testPhys = nullptr;
  BasePhysActor *testActor = get_phys_actor(eid_b);
  if (testActor)
    testPhys = &testActor->getPhys();

  if (testPhys == nullptr)
    return false;

  dag::ConstSpan<CollisionObject> testColl = testPhys->getCollisionObjects();
  uint64_t testCollActive = testPhys->getActiveCollisionObjectsBitMask();
  const TMatrix &testTm = testPhys->getCollisionObjectsMatrix();
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  return dacoll::test_pair_collision(make_span_const(&co_a, 1), ALL_COLLISION_OBJECTS, reinterpret_cast<const TMatrix &>(tm_a),
    testColl, testCollActive, testTm, contacts, dacoll::TestPairFlags::Default & ~dacoll::TestPairFlags::CheckInWorld);
}

inline float lag_compensation_time(
  ecs::EntityId avatar_eid, ecs::EntityId lc_eid, float at_time, int interp_delay_ticks_packed, float additional_interp_delay = 0.f)
{
  return ::lag_compensation_time(avatar_eid, lc_eid, at_time, interp_delay_ticks_packed, additional_interp_delay, nullptr);
}

inline void using_lag_compensation(
  float to_time, ecs::EntityId except_eid, const das::TBlock<void, void> &block, das::Context *context, das::LineInfoArg *at)
{
  get_lag_compensation().startLagCompensation(to_time, except_eid);
  context->invoke(block, nullptr, nullptr, at);
  get_lag_compensation().finishLagCompensation();
}

} // namespace bind_dascript