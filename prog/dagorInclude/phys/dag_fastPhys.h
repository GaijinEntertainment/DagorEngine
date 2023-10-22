//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <generic/dag_smallTab.h>
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
};


struct ClippedLine
{
  int p1Index, p2Index;
  int numSegs;

  ClippedLine() {} //-V730
  ClippedLine(int i1, int i2, int n) : p1Index(i1), p2Index(i2), numSegs(n) {}
};

void toggleDebugAnimChar(eastl::string &str);
bool checkDebugAnimChar(eastl::string &str);
void resetDebugAnimChars();

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
  virtual void debugRender(){};
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
