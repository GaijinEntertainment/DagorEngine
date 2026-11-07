//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_capsule.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <EASTL/string.h>


namespace FastPhys
{
enum
{
  FILE_ID = _MAKE4C('fph3'),
};


enum
{
  AID_NULL,

  AID_INTEGRATE,

  AID_INIT_POINT,
  AID_MOVE_POINT_WITH_NODE,

  AID_SET_BONE_LENGTH,
  AID_MIN_MAX_LENGTH,
  AID_SIMPLE_BONE_CTRL,

  AID_CLIP_SPHERICAL,
  AID_CLIP_CYLINDRICAL,

  AID_ANGULAR_SPRING,

  AID_SET_BONE_LENGTH_WITH_CONE,

  AID_LOOK_AT_BONE_CTRL,

  AID_HINGE_CONSTRAINT,

  AID_CLIP_EXTERNAL,
};


struct ClippedLine
{
  int p1Index, p2Index;
  int numSegs;

  ClippedLine() {} //-V730
  ClippedLine(int i1, int i2, int n) : p1Index(i1), p2Index(i2), numSegs(n) {}
};

struct ClippedLink
{
  int pivotIndex, endIndex, virtIndex;

  ClippedLink() {} //-V730
  ClippedLink(int pivot, int end, int virt) : pivotIndex(pivot), endIndex(end), virtIndex(virt) {}
};


}; // namespace FastPhys

namespace FastPhysRender
{
void draw_sphere(float radius, float angle, float axisLength, E3DCOLOR color, bool is_selected = false);
void draw_cylinder(float radius, float angle, float axisLength, E3DCOLOR color, bool is_selected = false);
} // namespace FastPhysRender


class IGenLoad;
class GeomNodeTree;

class FastPhysSystem;


class FastPhysAction
{
public:
  virtual ~FastPhysAction() {}

  virtual void init(FastPhysSystem &system, GeomNodeTree &nodeTree) = 0;
  virtual void perform(FastPhysSystem &system) = 0;

  virtual void reset(FastPhysSystem &system) = 0;
  virtual void debugRender() {}
};


class FastPhysSystem
{
public:
  struct Point
  {
    Point3 pos, vel, acc;
    real gravity, damping, windK;
  };

  SmallTab<Point, MidmemAlloc> points;
  SmallTab<FastPhysAction *, MidmemAlloc> initActions, updateActions;

  real deltaTime;
  real windScale, windTurb, windPower;
  Point3 windVel;
  Point3 windPos;
  vec3f curWtmOfs, deltaWtmOfs;
  Point3 gravityDirection;

  bool useOwnerTm = false;
  TMatrix ownerTm = TMatrix::IDENT;

  // Gate honored by AnimcharBaseComponent::postRecalcWtm; when false, update() is skipped.
  bool enabled = true;

  Tab<TMatrix> clipBoxes;
  Tab<Capsule> clipCapsules;
  real clipFriction = 0;

  int firstIntegratePoint;

  FastPhysSystem();
  ~FastPhysSystem();

  void load(IGenLoad &cb);

  void init(GeomNodeTree &nodes);

  void update(real dt, vec3f skeleton_wofs = v_zero());

  void reset(vec3f skeleton_wofs = v_zero());

  void debugRender();

protected:
  FastPhysSystem(const FastPhysSystem &);

  FastPhysAction *loadAction(IGenLoad &cb);
};

FastPhysSystem *create_fast_phys_from_gameres(const char *res_name);
