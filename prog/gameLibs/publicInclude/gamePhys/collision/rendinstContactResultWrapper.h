//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <memory/dag_framemem.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>

#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/contactResultWrapper.h>

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

  float addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB,
    gamephys::CollisionObjectInfo *obj_info);
};
