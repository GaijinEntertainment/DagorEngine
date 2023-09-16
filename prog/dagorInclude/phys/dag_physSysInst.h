//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <generic/dag_tab.h>
#include <phys/dag_physResource.h>
#include <phys/dag_physDecl.h>


class PhysSystemInstance
{
public:
  DAG_DECLARE_NEW(midmem)

  PhysSystemInstance(PhysicsResource *resource, PhysWorld *world, void *userData, real sleep_threshold = 8.0);
  PhysSystemInstance(const PhysSystemInstance &) = delete;
  ~PhysSystemInstance();

  TMatrix *getTmHelper(const char *name);

  PhysBody *getBody(const char *name) const;

  SimpleString getBodyMaterialName(PhysBody *body) const;

  void updateTms();


  bool setBodyTmByTmHelper(const char *helper_name, const TMatrix &wtm);
  bool setBodyTmAndVelByTmHelper(const char *helper_name, const TMatrix &wtm_prev, const TMatrix &wtm, real dt);


  void resetTm(const TMatrix &tm);


  PhysicsResource *getPhysicsResource() const { return resource; }

  int getBodyCount() const { return bodies.size(); }
  PhysBody *getBody(int index) const { return bodies[index].body; }


  void setGroupAndLayerMask(unsigned group, unsigned mask);

  void removeJoint(const char *name);
  bool removeJoint(const char *body1, const char *body2);

  int findBodyIdByName(const char *name) const;

protected:
  Ptr<PhysicsResource> resource;

  struct TmHelper
  {
    TMatrix wtm;
  };


  struct Body
  {
    PhysBody *body;
    Tab<TmHelper> tmHelpers;


    Body();
    Body(const Body &) = delete;
    ~Body();

    void init(PhysicsResource::Body &res_body, PhysWorld *world, void *userData, bool has_joints);

    void updateTms(PhysicsResource::Body &res_body, const TMatrix &scale_tm);
  };


  TMatrix scaleTm;

  Tab<Body> bodies;

  Tab<PhysJoint *> joints;

  static void phys_jolt_setup_collision_groups_for_joints(int grpId, Tab<Body> &b, PhysicsResource *r);
};
DAG_DECLARE_RELOCATABLE(PhysSystemInstance::Body);
