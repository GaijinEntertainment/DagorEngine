//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/collision/collisionInfo.h>
#include <rendInst/rendInstGen.h>

struct CachedCollisionObjectInfo : public gamephys::CollisionObjectInfo
{
  static constexpr float DEFAULT_TTL = 1.f / 3.f;

  rendinst::RendInstDesc riDesc;
  float timeToLive = DEFAULT_TTL;
  float originalThreshold;
  float thresImpulse;
  bool alive;
  float atTime;

  CachedCollisionObjectInfo(float impulse, const rendinst::RendInstDesc &ri_desc, float at_time) :
    riDesc(ri_desc), originalThreshold(impulse), thresImpulse(impulse), alive(true), atTime(at_time)
  {}
  ~CachedCollisionObjectInfo() = default;

  void *operator new(size_t, void *p, int) { return p; }
  void operator delete(void *p);

  virtual float onImpulse(float impulse, const Point3 & /*dir*/, const Point3 & /*pos*/, float /*point_vel*/,
    const Point3 & /*collision_normal*/, uint32_t flags = CIF_NONE, int32_t /* user_data */ = -1,
    gamephys::ImpulseLogFunc /*log_func*/ = nullptr, const char * /*actor_name*/ = nullptr) override
  {
    if (flags & CIF_NO_DAMAGE)
      return impulse;

    refreshTimeToLive();
    return 0.f;
  }

  float getRemainingImpulse() const override { return thresImpulse; }
  void refreshTimeToLive() { timeToLive = DEFAULT_TTL; }

  bool update(float dt)
  {
    if (!alive)
      return false;
    thresImpulse = min(originalThreshold, thresImpulse + originalThreshold * dt * 0.4f);
    timeToLive -= dt;
    return timeToLive > 0.f;
  }
};
