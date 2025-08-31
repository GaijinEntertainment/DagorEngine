// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>

#include "tunedParams.h"
#include <math/dag_curveParams.h>
#include <math/dag_Point2.h>
#include "../../../../libTools/propPanel/commonWindow/w_curve_math.cpp"

#include <debug/dag_debug.h>


using namespace ScriptHelpers;


namespace ScriptHelpers
{

class TunedCubicCurveParam : public TunedElement
{
public:
  Point2 position[CubicCurveSampler::MAX_POINTS];
  Point2 lastPosition[CubicCurveSampler::MAX_POINTS];
  bool selection[CubicCurveSampler::MAX_POINTS];
  real coeficient[CubicCurveSampler::MAX_POINTS];
  int curveType;
  int ptCnt;

  E3DCOLOR color;

  void initPoints()
  {
    curveType = 0;
    ptCnt = 4;
    memset(position, 0, sizeof(position));
    memset(lastPosition, 0, sizeof(lastPosition));
    memset(selection, 0, sizeof(selection));
    memset(coeficient, 0, sizeof(coeficient));

    for (int i = 0; i < ptCnt; ++i)
    {
      real x = i / real(ptCnt - 1);
      position[i] = Point2(x, 1);
      selection[i] = false;
    }
  }

  void addBasis(int i0, int i1, int i2, int i3)
  {
    real x0 = position[i0].x;
    real x1 = position[i1].x;
    real x2 = position[i2].x;
    real x3 = position[i3].x;

    real k = position[i0].y / ((x0 - x1) * (x0 - x2) * (x0 - x3));

    coeficient[0] -= x1 * x2 * x3 * k;
    coeficient[1] += (x1 * x2 + x2 * x3 + x3 * x1) * k;
    coeficient[2] -= (x1 + x2 + x3) * k;
    coeficient[3] += k;
  }

  void calcCoef()
  {
    memset(coeficient, 0, sizeof(coeficient));
    addBasis(0, 1, 2, 3);
    addBasis(1, 2, 3, 0);
    addBasis(2, 3, 0, 1);
    addBasis(3, 0, 1, 2);
  }


  TunedCubicCurveParam(const char *nm, E3DCOLOR c)
  {
    name = nm;
    color = c;
    initPoints();
  }

  ~TunedCubicCurveParam() override {}

  TunedElement *cloneElem() override { return new TunedCubicCurveParam(*this); }

  int subElemCount() const override { return 0; }
  TunedElement *getSubElem(int index) const override { return NULL; }

  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
  {
    Tab<Point2> segc(tmpmem);
    PropPanel::ICurveControlCallback *curve = NULL;
    PropPanel::CubicPolynomCB curve0;
    PropPanel::CubicPSplineCB curve1;

    if (curveType == 0)
      curve = &curve0;
    else if (curveType == 1)
      curve = &curve1;

    if (!curve)
      logerr("invalid curveType=%d", curveType);
    else
      for (int i = 0; i < ptCnt; i++)
        curve->addNewControlPoint(position[i]);
    bool trans_x = curve ? curve->getCoefs(segc) : false;

    if (trans_x || segc.size() < 4 || segc.size() > (CubicCurveSampler::MAX_POINTS - 1) * 4 || (segc.size() & 3))
    {
      logerr("incorrect spline: trans_x=%d cc.count=%d", trans_x, segc.size());
      cwr.writeZeroes(4 * 4);
      return;
    }

    if (segc.size() == 4)
    {
      if (fabs(segc[0 * 4 + 1].y) + fabs(segc[0 * 4 + 2].y) + fabs(segc[0 * 4 + 3].y) < 1e-6)
      {
        cwr.writeInt32e(0xFFFFFFFF);
        cwr.writeInt32e(0);
        cwr.writeReal(segc[0 * 4 + 0].y);
        return;
      }
    }
    else
    {
      G_ASSERT(segc.size() == (ptCnt - 1) * 4);

      cwr.writeInt32e(0xFFFFFFFF);
      cwr.writeInt32e(ptCnt - 1);
      for (int i = 1; i + 1 < ptCnt; i++)
        cwr.writeReal(position[i].x);
    }
    for (int i = 0; i < segc.size(); i++)
      cwr.writeReal(segc[i].y);
  }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    if (curveType)
      blk.setInt("curveType", curveType);
    for (int i = 0; i < ptCnt; ++i)
      blk.setPoint2(String(32, "pos%d", i), position[i]);
    for (int i = ptCnt; i < CubicCurveSampler::MAX_POINTS; ++i)
      blk.removeParam(String(32, "pos%d", i));
  }

  void loadValues(const DataBlock &blk) override
  {
    curveType = blk.getInt("curveType", 0);
    ptCnt = 0;
    for (int i = 0; i < CubicCurveSampler::MAX_POINTS; ++i)
      if (blk.paramExists(String(32, "pos%d", i)))
      {
        position[i] = blk.getPoint2(String(32, "pos%d", i), position[i]);
        ptCnt++;
      }
      else
        break;
    if (curveType == 0 && ptCnt != 4)
    {
      logerr("bad src data: curveType=%d ptCnt=%d", curveType, ptCnt);
      initPoints();
    }
  }
};


TunedElement *create_tuned_cubic_curve(const char *name, E3DCOLOR color) { return new TunedCubicCurveParam(name, color); }

}; // namespace ScriptHelpers
