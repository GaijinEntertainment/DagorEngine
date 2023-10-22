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
#include <EASTL/unique_ptr.h>


class PhysSystemInstance
{
public:
  // Note: if `tm` is null then bodies wont be added to phys world
  PhysSystemInstance(PhysicsResource *resource, PhysWorld *world, const TMatrix *tm, void *userData, uint16_t fgroup = 0,
    uint16_t fmask = 0);
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
  PhysBody *getBody(int index) const { return bodies[index].body.get(); }


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
    eastl::unique_ptr<PhysBody> body;
    Tab<TmHelper> tmHelpers;

    Body(PhysicsResource::Body &res_body, PhysWorld *world, const TMatrix *tm, void *userData, uint16_t fgroup, uint16_t fmask,
      bool has_joints);

    void updateTms(PhysicsResource::Body &res_body, const TMatrix &scale_tm);
  };


  TMatrix scaleTm;

  Tab<Body> bodies;

  Tab<PhysJoint *> joints;

  static void phys_jolt_setup_collision_groups_for_joints(int grpId, Tab<Body> &b, PhysicsResource *r);
};
DAG_DECLARE_RELOCATABLE(PhysSystemInstance::Body);
