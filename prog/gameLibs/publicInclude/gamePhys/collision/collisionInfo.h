//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <generic/dag_carray.h>

namespace PhysMat
{
struct MaterialData;
};

namespace gamephys
{
struct CollisionObjectInfo
{
  float collisionHardness; // used for damage multiplier

  CollisionObjectInfo() : collisionHardness(1.f) {}

  virtual ~CollisionObjectInfo() {}

  virtual float onImpulse(float impulse, const Point3 & /*dir*/, const Point3 & /*pos*/, float /*point_vel*/,
    int32_t /* user_data */ = -1)
  {
    return impulse;
  }

  virtual float getDestructionImpulse() const { return 0.f; }
  virtual float getRemainingImpulse() const { return 0.f; }

  virtual bool isRICollision() const { return false; }
  virtual bool isTreeCollision() const { return false; }
};

struct CollisionInfo
{
  int idx;
  float d;
  float appliedImpulse;
  DPoint3 PnT;
  bool isGear;
  bool isWater;

  DPoint3 normal;
  DPoint3 forward;
  DPoint3 left;
  DPoint3 surfVel;

  CollisionObjectInfo *collObjInfo;
  const PhysMat::MaterialData *matData;
};

struct SeqImpulseInfo
{
  DPoint3 pos;  // Only for damage
  DPoint3 wpos; // only for warmstarting
  DPoint3 axis1;
  DPoint3 axis2;
  DPoint3 w1; // w for first object
  DPoint3 w2; // w for second object
  double b;
  double depth;
  double appliedImpulse;
  double fullAppliedImpulse;
  int impulseLimitIndex1;
  int impulseLimitIndex2;
  double bounce;
  double friction;
  int contactIndex;
  int parentIndex;
  enum Type
  {
    TYPE_COLLISION,
    TYPE_FRICTION_1, // flags that it shouldn't go to pseudo velocities and damage
    TYPE_FRICTION_2  // flags that it shouldn't go to pseudo velocities and damage
  } type;
};

struct CachedContact
{
  Point3 wpos;
  Point3 localPos;
  Point3 wnorm;

  float appliedImpulse;
  float depth;
};

struct CollisionInfoCompressed
{
  static const float maxSurfVel;
  int16_t d256;
  uint8_t collisionCloudPointIndex;
  carray<uint8_t, 3> localNormalCompressed;
  unsigned int surfVelCompressed;
};

struct SceneCollisionInfoCompressed
{
  carray<int16_t, 3> localPointCompressed;
  carray<uint8_t, 3> localNormalCompressed;
  int16_t depth;
  uint8_t collisionObjectIndex;
};
}; // namespace gamephys
