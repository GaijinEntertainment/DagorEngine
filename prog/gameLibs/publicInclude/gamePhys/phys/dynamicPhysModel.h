//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/common/loc.h>
#include <generic/dag_tab.h>

namespace gamephys
{
struct CollisionContactData;

class DynamicPhysModel
{
  bool active;
  float timeFromCollision;

public:
  enum PhysType
  {
    E_ANGULAR_SPRING = 0,  // just angular spring
    E_FALL_WITH_FIXED_POS, // these will fall, while some of the positions are fixed
    E_FORCED_POS_BY_COLLISION,
    E_PHYS_OBJECT
  };

  Loc location;
  Loc originalLoc;
  Point3 velocity;
  Point3 omega;

  float mass;
  Point3 momentOfInertia;
  Point3 centerOfGravity;

  PhysType physType;

  Tab<CollisionContactData> contacts;
  DynamicPhysModel(DynamicPhysModel &&);
  DynamicPhysModel &operator=(DynamicPhysModel &&);
  ~DynamicPhysModel();
  DynamicPhysModel() = default;
  DynamicPhysModel(const TMatrix &tm, float bodyMass, const Point3 &moment, const Point3 &CoG, PhysType phys_type);

  void applyImpulse(const Point3 &impulse, const Point3 &arm, Point3 &outVel, Point3 &outOmega, float invMass,
    const Point3 &invMomentOfInertia);

  void update(float dt);

  bool isActive() const { return active; }
};
}; // namespace gamephys
