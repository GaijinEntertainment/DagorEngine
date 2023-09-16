//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathUtils.h>
#include <gamePhys/phys/treeDestr.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>

struct WrapperRendinstContactResultCB;

struct WrapperRendInstCollisionImplCB : public rendinst::RendInstCollisionCB
{
  WrapperRendinstContactResultCB &resultCallback;
  CollisionObject objA;
  bool shouldProvideCollisionInfo;
  float atTime;
  bool restoreImpulse = false;

  WrapperRendInstCollisionImplCB(const CollisionObject &obj, WrapperRendinstContactResultCB &result_callback, bool provide_coll_info,
    float at_time, bool restore_imp = false) :
    objA(obj),
    resultCallback(result_callback),
    shouldProvideCollisionInfo(provide_coll_info),
    atTime(at_time),
    restoreImpulse(restore_imp)
  {}

  virtual CollisionObject processCollisionInstance(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info,
    CollisionObject &alternative_obj, TMatrix &normalized_tm);

  virtual void addCollisionCheck(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) override;
  virtual void addTreeCheck(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) override;
};

template <typename ContactCallbackType>
struct WrapperRendInstCollisionCB : WrapperRendInstCollisionImplCB
{
  ContactCallbackType &contactCallback;

  WrapperRendInstCollisionCB(const CollisionObject &obj, ContactCallbackType &callback, bool provide_coll_info, float at_time,
    bool restore_imp = false) :
    WrapperRendInstCollisionImplCB(obj, callback, provide_coll_info, at_time, restore_imp), contactCallback(callback)
  {}

  CollisionObject processCollisionInstance(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info,
    CollisionObject &alternative_obj, TMatrix &normalized_tm) final
  {
    CollisionObject obj = WrapperRendInstCollisionImplCB::processCollisionInstance(coll_info, alternative_obj, normalized_tm);

    contactCallback.collMatId = rendinst::getRIGenMaterialId(coll_info.desc);
    contactCallback.originalRIPos = normalized_tm.getcol(3);

    return obj;
  }
};
