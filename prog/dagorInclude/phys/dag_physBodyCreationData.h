//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physDecl.h>
#include <math/dag_Point3.h>

struct PhysBodyCreationData
{
  Point3 momentOfInertia = Point3(0, 0, 0);
  float friction = -1.f, restitution = -1.f, rollingFriction = -1.f; //< not applied when < 0
  float linearDamping = 0.2f, angularDamping = 0.2f;
  void *userPtr = nullptr;
  short materialId = 0; //< when < 0 (invalid) friction and restitution are not used from mat
  unsigned short group = 1u, mask = 0xFFFFu;
  bool useMotionState = true, autoMask = true, autoInertia = false;
  bool addToWorld = true, kinematic = false;
  bool allowFastInaccurateCollTm = true;
};
