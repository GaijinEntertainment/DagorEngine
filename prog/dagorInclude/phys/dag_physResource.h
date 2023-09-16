//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_dynSceneRes.h>
#include <memory/dag_mem.h>
#include <math/dag_TMatrix.h>
#include <generic/dag_tab.h>
#include <generic/dag_DObject.h>
#include <util/dag_simpleString.h>

class GeomNodeTree;
class DynamicRenderableSceneLodsResource;


decl_dclass_and_id(PhysicsResource, DObject, 0xABEFB687) // DagorPhysicsResource
public:
  DAG_DECLARE_NEW(midmem)

  struct TmHelper;

  static PhysicsResource *loadResource(IGenLoad & crd, int flags);

  PhysicsResource();

  void load(IGenLoad & cb, int load_flags);


  bool getBodyTm(const char *name, TMatrix &tm);
  bool getTmHelperWtm(const char *name, TMatrix &wtm);


  struct TmHelper
  {
    TMatrix tm;
    SimpleString name;
  };


  struct SphColl
  {
    Point3 center;
    real radius;
    SimpleString materialName;
  };


  struct BoxColl
  {
    TMatrix tm;
    Point3 size;
    SimpleString materialName;
  };


  struct CapColl
  {
    Point3 center, extent;
    real radius;
    SimpleString materialName;
  };


  struct NodeAlignCtrl
  {
    SimpleString node0, node1, twist[3];
    float angDiff = 0;
  };

  struct Body
  {
    SimpleString name;

    TMatrix tm;
    TMatrix tmInvert;
    real mass;
    Point3 momj;
    SimpleString materialName;
    SimpleString aliasName;

    Tab<TmHelper> tmHelpers;
    Tab<SphColl> sphColl;
    Tab<BoxColl> boxColl;
    Tab<CapColl> capColl;


    Body() : tmHelpers(midmem), sphColl(midmem), boxColl(midmem), capColl(midmem) {}

    void load(IGenLoad &cb);

    int findTmHelper(const char *name);
  };

  struct RdBallJoint
  {
    int body1, body2;
    TMatrix tm;
    Point3 minLimit, maxLimit;
    real damping, twistDamping, stiffness;
    SimpleString name;
  };

  struct RdHingeJoint
  {
    int body1, body2;
    Point3 pos, axis, midAxis, xAxis;
    real angleLimit;
    real damping;
    real stiffness;
    SimpleString name;
  };

  struct RevoluteJoint
  {
    int body1, body2;
    Point3 pos, axis;
    real minAngle, maxAngle;
    real damping;

    real minRestitution, maxRestitution;
    real spring;
    real projAngle, projDistance;
    short projType;
    short flags;

    SimpleString name;
  };

  struct SphericalJoint
  {
    int body1, body2;
    Point3 pos, dir, axis;
    real minAngle, maxAngle;

    real minRestitution, maxRestitution;
    real swingValue, swingRestitution;

    real spring, damping;
    real swingSpring, swingDamping;
    real twistSpring, twistDamping;

    real projAngle, projDistance;
    short projType;
    short flags;

    SimpleString name;
  };

  const Tab<Body> &getBodies() const { return bodies; }
  dag::ConstSpan<NodeAlignCtrl> getNodeAlignCtrl() const { return nodeAlignCtrl; }

protected:
  friend class BulletPhysSystemInstance;
  friend class JoltPhysSystemInstance;

  Tab<Body> bodies;

  Tab<RdBallJoint> rdBallJoints;
  Tab<RdHingeJoint> rdHingeJoints;
  Tab<RevoluteJoint> revoluteJoints;
  Tab<SphericalJoint> sphericalJoints;
  Tab<NodeAlignCtrl> nodeAlignCtrl;
end_dclass_decl();


class DynamicPhysObjectData
{
public:
  PtrTab<DynamicRenderableSceneLodsResource> models;
  Ptr<PhysicsResource> physRes;
  GeomNodeTree *nodeTree;
  GeomNodeTree *skeleton;

  DynamicPhysObjectData() : models(midmem)
  {
    nodeTree = NULL;
    skeleton = nullptr;
  }
};
