//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <phys/dag_physSysInst.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>
#include <phys/dag_physUserData.h>
#include <animChar/dag_animCharacter2.h>
#include <generic/dag_smallTab.h>


class PhysRagdoll final : public AnimV20::IAnimCharPostController
{
public:
  PhysRagdoll();
  ~PhysRagdoll();

  void update(real dt, GeomNodeTree &tree, AnimV20::AnimcharBaseComponent *ac) override;
  bool overridesBlender() const override { return true; }

  static PhysRagdoll *create(PhysicsResource *phys, PhysWorld *world);


  void init(PhysicsResource *phys, PhysWorld *world);

  void resizeCollMap(int size);

  void addCollNode(int node_id, int body_id);

  void setStartAddLinVel(const Point3 &add_vel);

  void setRecalcWtms(bool flag) { recalcWtms = flag; }

  // Create instance.
  void startRagdoll(int interact_layer = 1, int interact_mask = 0xffffffff, const GeomNodeTree *tree = nullptr);

  // Destroy instance, removing physical bodies from physical world.
  void endRagdoll();

  // sets ccd mode
  void setContinuousCollisionMode(bool mode);

  // dynamic clipout
  void setDynamicClipout(const Point3 &toPos, float timeout);

  // sets overrides
  void setOverrideVel(const Point3 &vel);
  void setOverrideOmega(const Point3 &omega);
  void applyImpulse(int node_id, const Point3 &pos, const Point3 &impulse);

  PhysSystemInstance *getPhysSys() const { return physSys; }

  PhysicsResource *getPhysRes() const { return physRes; }

  PhysWorld *getPhysWorld() const { return physWorld; }

  bool isCanApplyNodeImpulse() const { return !nodeBody.empty(); }

  bool wakeUp(); // Return false if was not started

protected:
  static void onPreSolve(PhysBody *body, void *other, const Point3 &pos, Point3 &norm, IPhysContactData *&contact_data);
  void initNodeHelpers(const GeomNodeTree &tree, dag::Span<TMatrix> last_node_tms = {});
  void setBodiesTm(const GeomNodeTree &tree, dag::Span<TMatrix> last_node_tms = {}, float dt = 0.f);

  struct NodeAlignCtrl
  {
    dag::Index16 node0Id, node1Id, twistId[3];
    int16_t twistCnt = 0;
    float angDiff = 0;
  };
  Ptr<PhysicsResource> physRes;
  PhysSystemInstance *physSys;
  PhysWorld *physWorld;
  PhysObjectUserData ud = {_MAKE4C('PRGD')};
  Point3 addVelImpulse;
  Point3 dynamicClipoutPos;
  float dynamicClipoutT;
  Point3 overrideVelocity;
  Point3 overrideOmega;

  SmallTab<TMatrix *, MidmemAlloc> nodeHelpers;
  SmallTab<TMatrix, TmpmemAlloc> lastNodeTms;
  Tab<NodeAlignCtrl> nodeAlignCtrl;
  Tab<int> nodeBody;

  bool recalcWtms;
  bool ccdMode;
  bool shouldOverrideVelocity;
  bool shouldOverrideOmega;
};
