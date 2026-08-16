//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animPostBlendCtrl.h>
#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <util/dag_index16.h>
#include <util/dag_simpleString.h>

class DataBlock;
class GeomNodeTree;

namespace AnimV20
{
//
// Controller to lock foot (toe actually) at some point in world space when leg is on the ground in animation
//
class FootLockerIKCtrl : public AnimPostBlendCtrl
{
public:
  struct LegStaticData
  {
    SimpleString hip, knee, ankle, toe;
    float needLockParamThreshold = 0.2;
    int needLockParamId = -1;
  };

  Tab<LegStaticData> legsNodeNames;
  float unlockViscosity = 0;
  float maxReachScale = 1;
  float unlockRadius = 0;
  float unlockWhenUnreachableRadius = 0;
  float toeNodeHeight = 0;
  float ankleNodeHeight = 0;

  float maxFootUp = 0;
  float maxFootDown = 0;
  float maxToeMoveUp = 0;
  float maxToeMoveDown = 0;
  float footRaisingSpeed = 0;
  float footInclineViscosity = 0;
  float maxAnkleAnlgeCos = 0;
  float maxHipMoveDown = 0;
  float hipMoveViscosity = 0;
  float proceduralStepHeight = 0;
  float proceduralStepMinDistance = 0;
  float proceduralStepSpeed = 0;
  float proceduralStepCooldown = 0;
  Point2 disableDistanceRange = Point2::ZERO;

  int legsDataVarId = -1;
  int hipMoveDownVarId = -1;
  int allowProceduralStepVarId = -1;
  int proceduralStepTimerVarId = -1;
  int distanceFromCameraVarId = -1;

  bool useHeightmapQueries = false;

  struct LegData
  {
    dag::Index16 toeNodeId;
    dag::Index16 ankleNodeId;
    dag::Index16 kneeNodeId;
    dag::Index16 hipNodeId;

    Point3 lockedPosition;
    Point3 posOffset;
    float ankleVerticalMove = 0;
    float ankleTargetMove = 0;

    int toeHeightmapQueryId = -1;
    float toeDisplacement = 0;
    int ankleHeightmapQueryId = -1;
    float ankleDisplacement = 0;

    bool isLocked = false;
    bool needLock = false; // updated outside
    bool isProceduralStepActive = false;
    bool inited = false;
  };
  FootLockerIKCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) {}

  virtual void destroy() {}
  virtual void setDefaultState(AnimGraphStateHolder & /*st*/) {}
  virtual void clearAllocatedMemory(AnimGraphStateHolder &);

  virtual void init(AnimGraphStateHolder &st, const GeomNodeTree &tree);
  virtual void process(AnimGraphStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx);
  virtual void advance(AnimGraphStateHolder & /*st*/, real /*dt*/) {}

  const char *class_name() const override { return "FootLockerIKCtrl"; }
  virtual bool isSubOf(DClassID id) { return id == FootLockerIKCtrlCID || AnimPostBlendCtrl::isSubOf(id); }

  static void createNode(AnimationGraph &graph, const DataBlock &blk);
};
} // namespace AnimV20
