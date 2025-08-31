//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/fastPhysData/fp_data.h>

class FpdPoint;


class FpdInitPointAction : public FpdAction
{
public:
  FpdPoint *pointObj;

public:
  FpdInitPointAction(FpdPoint *o) : pointObj(o) { actionName = "initPt"; }

  FpdObject *getObject() override;
  void save(DataBlock &blk, const GeomNodeTree &tree) override {}
  bool load(const DataBlock &blk, IFpdLoad &loader) override { return true; }
  void exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp) override;
};


class FpdLinkPointAction : public FpdAction
{
public:
  FpdPoint *pointObj;

public:
  FpdLinkPointAction(FpdPoint *o) : pointObj(o) { actionName = "linkPt"; }

  FpdObject *getObject() override;
  void save(DataBlock &blk, const GeomNodeTree &tree) override {}
  bool load(const DataBlock &blk, IFpdLoad &loader) override { return true; }
  void exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp) override;
};


class FpdPoint : public FpdObject
{
public:
  static constexpr unsigned HUID = 0x4B8A49AEu; // FpdPoint

  const mat44f *nodeWtm = nullptr;
  dag::Index16 node;
  short int groupId;

  real gravity, damping, windK;
  vec3f localPos;

  Ptr<FpdInitPointAction> initAction;
  Ptr<FpdLinkPointAction> linkAction;

public:
  FpdPoint();

  void save(DataBlock &blk, const GeomNodeTree &tree) override;
  bool load(const DataBlock &blk, IFpdLoad &loader) override;

  void initActions(FpdContainerAction *init_a, FpdContainerAction *upd_a, IFpdLoad &ld) override;
  void getActions(Tab<FpdAction *> &actions) override;

  void exportFastPhys(mkbindump::BinDumpSaveCB &cwr);

  // local pos handling
  void recalcPos()
  {
    if (!nodeWtm)
      FpdObject::setPos(as_point3(&localPos));
    else
    {
      Point3_vec4 p;
      v_st(&p, v_mat44_mul_vec3p(*nodeWtm, localPos));
      FpdObject::setPos(p);
    }
  }

  bool setPos(const Point3 &p) override
  {
    if (nodeWtm)
    {
      mat44f itm;
      v_mat44_inverse43(itm, *nodeWtm);
      localPos = v_mat44_mul_vec3p(itm, v_ldu(&p.x));
    }
    else
      as_point3(&localPos) = p;

    return FpdObject::setPos(p);
  }

  bool isSubOf(DClassID id) override { return id == FpdPoint::HUID || FpdObject::isSubOf(id); }
};
