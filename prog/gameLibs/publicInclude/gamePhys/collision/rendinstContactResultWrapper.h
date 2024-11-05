//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <memory/dag_framemem.h>
#include <phys/dag_physDecl.h>

#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/contactResultWrapper.h>

namespace gamephys
{
struct CollisionContactData;
};

struct RendinstCollisionUserInfo;

struct WrapperRendinstContactResultCB : public WrapperContactResultCB
{
  Point3 originalRIPos;
  uint32_t frameNo = 0;
  bool processTreeBehaviour = true;
  struct RiGenDamage
  {
    Point3 pos, axis;
  };
  Tab<RiGenDamage> riGenDamage{framemem_ptr()};

  WrapperRendinstContactResultCB(Tab<gamephys::CollisionContactData> &stor, uint32_t frame_no, int check_mat_id = -1,
    bool process_tree_behaviour = true) :
    WrapperContactResultCB(stor, check_mat_id), frameNo(frame_no), processTreeBehaviour(process_tree_behaviour)
  {}
  void applyRiGenDamage() const;
  ~WrapperRendinstContactResultCB()
  {
    if (!riGenDamage.empty())
      applyRiGenDamage();
  }

  void addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB);
  void addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB,
    const RendinstCollisionUserInfo *userInfo, gamephys::CollisionObjectInfo *obj_info = nullptr);
};
