// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/geomMeshHelper.h>
#include <util/dag_simpleString.h>


struct PhysSysObject
{
  struct Momj
  {
    real mass;
    Point3 cmPos;
    Matrix3 momj;
  };

  enum
  {
    COLL_NONE,
    COLL_BOX,
    COLL_SPHERE,
    COLL_CAPSULE,
  };

  enum
  {
    MASS_NONE,
    MASS_BOX,
    MASS_SPHERE,
    MASS_MESH,
  };

  GeomMeshHelper mesh;

  TMatrix matrix;
  int collType, massType;
  bool useMass;
  real massDens;
  SimpleString materialName;
  SimpleString helperName;

  PhysSysObject(const char *hlp_nm) : helperName(hlp_nm), collType(COLL_NONE), massType(MASS_NONE), useMass(false), massDens(1e3) {}
  bool load(Mesh &m, const TMatrix &wtm, const DataBlock &blk);
  bool calcMomj(Momj &momj);

  bool getBoxCollision(BBox3 &box);
  bool getSphereCollision(Point3 &center, real &radius);
  bool getCapsuleCollision(Point3 &center, Point3 &extent, real &radius, const Point3 &scale);
};


struct PhysSysBodyObject
{
  TMatrix matrix;
  real bodyMass;
  Point3 bodyMomj;
  SimpleString materialName, name, ref;
  bool momjInIdentMatrix, forceCmPosX0, forceCmPosY0, forceCmPosZ0;

  void calcMomjAndTm(dag::ConstSpan<PhysSysObject *> objs);
  void calcBodyMomjAndTm(dag::ConstSpan<PhysSysObject::Momj> objs, bool force_ident_tm, bool force_cm_posx_0, bool force_cm_posy_0,
    bool force_cm_posz_0);
};
