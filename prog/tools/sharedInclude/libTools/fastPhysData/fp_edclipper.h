//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/fastPhysData/fp_data.h>
#include <generic/dag_tab.h>
#include <math/dag_Matrix3.h>

class FpdClipper;


class FpdClipPointsAction : public FpdAction
{
public:
  FpdClipper *clipObj;

public:
  FpdClipPointsAction(FpdClipper *o) : clipObj(o) { actionName = "clipPt"; }

  FpdObject *getObject() override;
  void save(DataBlock &blk, const GeomNodeTree &tree) override {}
  bool load(const DataBlock &blk, IFpdLoad &loader) override { return true; }
  void exportAction(mkbindump::BinDumpSaveCB &cwr, IFpdExport &exp) override;
};


class FpdClipper : public FpdObject
{
public:
  static constexpr unsigned HUID = 0x11825EDBu; // FpdClipper

  const mat44f *nodeWtm = nullptr;
  TMatrix localTm;

  real radius;
  real axisLength;
  real angle; // in degrees
  real lineSegLen;
  short int clipType;
  dag::Index16 node;

  Tab<SimpleString> pointNames;
  int selectedPoint;

  Tab<SimpleString> lineNames;
  int selectedLine;

  Ptr<FpdClipPointsAction> clipAction;


  enum
  {
    CLIP_SPHERICAL,
    CLIP_CYLINDRICAL,
  };


public:
  FpdClipper();

  void save(DataBlock &blk, const GeomNodeTree &tree) override;
  bool load(const DataBlock &blk, IFpdLoad &loader) override;

  void initActions(FpdContainerAction *init_a, FpdContainerAction *upd_a, IFpdLoad &ld) override;
  void getActions(Tab<FpdAction *> &actions) override;

  int findPoint(const char *name);
  void addPoint(const char *name);
  bool removePoint(const char *name);

  int findLine(const char *name);
  void addLine(const char *name);
  bool removeLine(const char *name);

  int calcLineSegs(real len);

  // local tm handling
  void recalcTm()
  {
    if (!nodeWtm)
      FpdObject::setPos(localTm.getcol(3));
    else
    {
      TMatrix tm;
      GeomNodeTree::mat44f_to_TMatrix(*nodeWtm, tm);
      tm = tm * localTm;
      FpdObject::setPos(tm.getcol(3));
    }
  }

  bool setPos(const Point3 &p) override
  {
    if (nodeWtm)
    {
      Point3_vec4 lp;
      mat44f itm;

      v_mat44_inverse43(itm, *nodeWtm);
      v_st(&lp.x, v_mat44_mul_vec3p(itm, v_ldu(&p.x)));

      localTm.setcol(3, lp);
    }
    else
      localTm.setcol(3, p);

    return FpdObject::setPos(p);
  }
  bool setMatrix(const Matrix3 &tm) override
  {
    if (nodeWtm)
    {
      Point3_vec4 lp[4];
      mat44f itm;

      for (int i = 0; i < 3; i++)
        lp[i] = tm.getcol(i);
      v_mat44_inverse43(itm, *nodeWtm);
      for (int i = 0; i < 3; i++)
        v_st(&lp[i].x, v_mat44_mul_vec3v(itm, v_ld(&lp[i].x)));

      for (int i = 0; i < 3; i++)
        localTm.setcol(i, lp[i]);
    }
    else
      for (int i = 0; i < 3; i++)
        localTm.setcol(i, tm.getcol(i));

    return true;
  }
  bool getMatrix(Matrix3 &tm) const override
  {
    if (nodeWtm)
    {
      TMatrix ntm;
      GeomNodeTree::mat44f_to_TMatrix(*nodeWtm, ntm);
      for (int i = 0; i < 3; i++)
        tm.setcol(i, ntm % localTm.getcol(i));
    }
    else
      for (int i = 0; i < 3; i++)
        tm.setcol(i, localTm.getcol(i));

    return true;
  }

  bool isSubOf(DClassID id) override { return id == FpdClipper::HUID || FpdObject::isSubOf(id); }
};
