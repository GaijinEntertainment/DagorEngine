//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/collision/collisionInfo.h>
#include <gamePhys/collision/contactData.h>
#include <phys/dag_physDebug.h>
#include <scene/dag_physMat.h>
#include <math/dag_mathUtils.h>

struct WrapperContactResultCB
{
  typedef ::PhysObjectUserData obj_user_data_t;
  typedef ::gamephys::CollisionContactData contact_data_t;
  int collMatId = -1;
  int checkMatId = -1;
  float collapseContactThreshold = 0.f;

  WrapperContactResultCB(Tab<gamephys::CollisionContactData> &stor, int check_mat_id = -1, float collapse_thres = 0.f) :
    contacts(stor), checkMatId(check_mat_id), collapseContactThreshold(collapse_thres)
  {}

  void setContactData(const gamephys::CollisionContactData &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB,
    gamephys::CollisionObjectInfo *obj_info, gamephys::CollisionContactData &dest_c)
  {
    dest_c.depth = cp.depth;
    dest_c.wpos = cp.wpos;
    dest_c.wposB = cp.wposB;
    dest_c.wnormB = cp.wnormB;
    dest_c.posA = cp.posA;
    dest_c.posB = cp.posB;
    if (auto *ptr = collMatId < 0 ? CollisionObjectUserData::cast(userPtrB) : nullptr)
      dest_c.matId = ptr->matId;
    else
      dest_c.matId = collMatId;

    dest_c.userPtrA = userPtrA;
    dest_c.userPtrB = userPtrB;
    dest_c.objectInfo = obj_info;
  }

  float addSingleResult(contact_data_t &cp, obj_user_data_t *userPtrA, obj_user_data_t *userPtrB,
    gamephys::CollisionObjectInfo *obj_info)
  {
    if (collapseContactThreshold > 0.f)
      for (gamephys::CollisionContactData &c : contacts)
        if (lengthSq(c.wpos - cp.wpos) < sqr(collapseContactThreshold) && c.userPtrA == userPtrA)
        {
          // update this one and move out
          if (c.depth > cp.depth)
            setContactData(cp, userPtrA, userPtrB, obj_info, c);
          return 0;
        }
    setContactData(cp, userPtrA, userPtrB, obj_info, contacts.push_back());
    return 0;
  }

#if DAGOR_DBGLEVEL > 0
  static void visualDebugForSingleResult(const PhysBody *bodyA, const PhysBody *bodyB, const contact_data_t &c)
  {
    PhysWorld *physWorld = dacoll::get_phys_world();
    if (physdbg::isInDebugMode(physWorld))
    {
      physdbg::renderOneBody(physWorld, bodyA);
      physdbg::renderOneBody(physWorld, bodyB);
    }

    // TODO: Make global collision debug flags
    G_UNUSED(c);
    /*
    if (::should_draw_collision)
    {
      draw_debug_box_buffered(BBox3(c.wpos, rabs(c.depth)));
      draw_debug_line_buffered(c.wpos, c.wpos + c.wnormB * 10.f);
    }
    */
  }
#else
  static void visualDebugForSingleResult(...) {}
#endif

  bool needsCollision(obj_user_data_t *userPtrB, bool /*is_static*/) const
  {
    if (checkMatId < 0)
      return true;
    if (auto *ptr = CollisionObjectUserData::cast(userPtrB))
      if (ptr->matId >= 0)
        return PhysMat::isMaterialsCollide(checkMatId, ptr->matId);
    return true;
  }

  Tab<gamephys::CollisionContactData> &contacts;
};

struct MatConvexCallback
{
  int convexMatId = -1;
  dag::ConstSpan<CollisionObject> ignoreObjs;

  bool needsCollision(const PhysBody *objB, void *userDataB) const
  {
    for (const auto &ignObj : ignoreObjs)
      if (objB == ignObj.body)
        return false;
    if (convexMatId >= 0)
      if (CollisionObjectUserData *ptr = CollisionObjectUserData::cast((PhysObjectUserData *)userDataB))
        if (ptr->matId >= 0)
          return PhysMat::isMaterialsCollide(convexMatId, ptr->matId);
    return true;
  }
};

struct MatAndGroupConvexCallback : public MatConvexCallback
{
  int collisionFilterGroup;
  int collisionFilterMask;
  bool needsCollision(const PhysBody *objB, void *userDataB) const
  {
    if (!MatConvexCallback::needsCollision(objB, userDataB))
      return false;
    bool collides = (objB->getGroupMask() & collisionFilterMask) != 0;
    collides = collides && (collisionFilterGroup & objB->getInteractionLayer());
    return collides;
  }
};
