// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <phys/dag_physUserData.h>
#include <rendInst/rendInstDesc.h>
#include <math/dag_bounds3.h>

namespace gamephys
{
struct CollisionObjectInfo;
}

struct RendinstCollisionUserInfo : public PhysObjectUserData
{
  rendinst::RendInstDesc desc;
  BBox3 bbox;
  TMatrix tm;
  bool isTree = false;
  bool immortal = false;
  bool isDestr = false;
  bool bushBehaviour = false;
  bool treeBehaviour = false;
  mutable bool collided = false;
  gamephys::CollisionObjectInfo *objInfoData = nullptr;
  void *prevUserPtrB = nullptr;

  RendinstCollisionUserInfo(const rendinst::RendInstDesc &d) : PhysObjectUserData{_MAKE4C('RIUD')}, desc(d) {}
};
