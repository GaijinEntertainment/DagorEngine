//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/fastPhysData/fp_data.h>

class FpdBone;


class FpdBoneConstraintAction : public FpdAction
{
public:
  FpdBone *boneObj;

  int type;
  real minLen, maxLen;
  real damping;

  Point3 limitAxis;
  float limitAngle;

  enum
  {
    DIRECTIONAL,
    LENCONSTR,
    HINGE,
  };

public:
  FpdBoneConstraintAction(FpdBone *o) :
    boneObj(o), type(DIRECTIONAL), minLen(100), maxLen(100), damping(0), limitAxis(1, 0, 0), limitAngle(0)
  {
    actionName = "boneConstr";
  }

  FpdObject *getObject() override;
  void save(DataBlock &blk, const GeomNodeTree &tree) override;
  bool load(const DataBlock &blk, IFpdLoad &loader) override;
  void exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp) override;
};


class FpdBoneAngularSpringAction : public FpdAction
{
public:
  FpdBone *boneObj;

  real springK;
  Point3 localAxis, orgAxis;

public:
  FpdBoneAngularSpringAction(FpdBone *o) : boneObj(o), springK(0), localAxis(1, 0, 0), orgAxis(1, 0, 0) { actionName = "boneAngSpr"; }

  FpdObject *getObject() override;
  void save(DataBlock &blk, const GeomNodeTree &tree) override;
  bool load(const DataBlock &blk, IFpdLoad &loader) override;
  void exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp) override;
};


class FpdBoneControllerAction : public FpdAction
{
public:
  FpdBone *boneObj;
  const mat44f *nodeWtm = nullptr, *nodeTm = nullptr;
  Point3 localBoneAxis;
  Matrix3 orgTm;
  dag::Index16 node;
  bool useLookAtMode;

public:
  FpdBoneControllerAction(FpdBone *o) : boneObj(o), localBoneAxis(1, 0, 0), useLookAtMode(false)
  {
    actionName = "boneCtrl";
    orgTm.identity();
  }

  FpdObject *getObject() override;
  void save(DataBlock &blk, const GeomNodeTree &tree) override;
  bool load(const DataBlock &blk, IFpdLoad &loader) override;
  void exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp) override;

  static int getAxisIndex(const Point3 &dir);
  void findNode(IFpdLoad &ld);
};


class FpdBone : public FpdObject
{
public:
  static constexpr unsigned HUID = 0x5B69184Fu; // FpdBone

  SimpleString point1Name, point2Name;

  Ptr<FpdBoneConstraintAction> constrAction;
  Ptr<FpdBoneControllerAction> ctrlAction;
  Ptr<FpdBoneAngularSpringAction> angSprAction;

public:
  FpdBone();

  void save(DataBlock &blk, const GeomNodeTree &tree) override;
  bool load(const DataBlock &blk, IFpdLoad &loader) override;

  void initActions(FpdContainerAction *init_a, FpdContainerAction *upd_a, IFpdLoad &ld) override;
  void getActions(Tab<FpdAction *> &actions) override;

  int getPoints(FpdPoint *&p1, FpdPoint *&p2, IFpdExport &exp) const;
  int getPointsLd(FpdPoint *&p1, FpdPoint *&p2, IFpdLoad &ld) const;

  bool isSubOf(DClassID id) override { return id == FpdBone::HUID || FpdObject::isSubOf(id); }
};
