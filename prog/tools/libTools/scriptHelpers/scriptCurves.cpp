// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>

#include <propPanel/control/container.h>
#include "../propPanel/commonWindow/w_curve_math.h"

#include <scriptHelpers/tunedParams.h>
#include <math/dag_Point2.h>

#include <debug/dag_debug.h>


using namespace ScriptHelpers;


namespace ScriptHelpers
{
static Tab<String> typeNames(tmpmem);

class TunedCubicCurveParam : public TunedElement
{

public:
  static const int MAX_POINTS = 6;

  Point2 position[MAX_POINTS];
  Point2 lastPosition[MAX_POINTS];
  bool selection[MAX_POINTS];
  real coeficient[MAX_POINTS];
  int curveType;
  int ptCnt;

  E3DCOLOR color;

  int typePid, controlPid;

  PropPanel::ContainerPropertyControl *mPanel;

  TunedCubicCurveParam(const char *nm, E3DCOLOR c) : typePid(-1), controlPid(-1), mPanel(NULL)
  {
    if (!typeNames.size())
    {
      typeNames.push_back() = "Cubic polynom";
      typeNames.push_back() = "Hermite spline";
    }
    name = nm;
    color = c;
    initPoints();
  }

  ~TunedCubicCurveParam() {}

  void resetPropPanel() override { mPanel = nullptr; }
  virtual void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &ppcb)
  {
    mPanel = &ppcb;
    typePid = ++pid;
    ppcb.createCombo(typePid, String(128, "%s curve type", getName()), typeNames, curveType);
    controlPid = ++pid;
    ppcb.createCurveEdit(controlPid, getName());
    ppcb.setColor(controlPid, color);

    initCurveControl(ppcb);
  }

  void initCurveControl(PropPanel::ContainerPropertyControl &ppcb)
  {
    if (curveType == 0)
      ppcb.setInt(controlPid, PropPanel::CURVE_CUBICPOLYNOM_APP);
    else if (curveType == 1)
      ppcb.setInt(controlPid, PropPanel::CURVE_CUBIC_P_SPLINE_APP);

    if (curveType == 0 && ptCnt != 4)
    {
      ptCnt = 4;
      for (int i = 0; i < ptCnt; ++i)
        position[i] = Point2(i / real(ptCnt - 1), 1);
    }

    Tab<Point2> _points(tmpmem);
    for (int i = 0; i < ptCnt; ++i)
      _points.push_back(position[i]);

    ppcb.setBool(controlPid, true); // lock ends
    if (curveType == 0)
      ppcb.setMinMaxStep(controlPid, 4, 4, PropPanel::CURVE_MIN_MAX_POINTS);
    else if (curveType == 1)
      ppcb.setMinMaxStep(controlPid, 2, MAX_POINTS, PropPanel::CURVE_MIN_MAX_POINTS);

    ppcb.setMinMaxStep(controlPid, 0, 1, PropPanel::CURVE_MIN_MAX_X);
    ppcb.setMinMaxStep(controlPid, 0, 1, PropPanel::CURVE_MIN_MAX_Y);
    ppcb.setControlPoints(controlPid, _points);
  }
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

  virtual void getControlPoints(Tab<Point2> &points)
  {
    if ((controlPid > -1) && (mPanel))
      mPanel->getCurveCoefs(controlPid, points);
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

  //---------------------------------------------

  virtual TunedElement *cloneElem() { return new TunedCubicCurveParam(*this); }

  virtual int subElemCount() const { return 0; }
  virtual TunedElement *getSubElem(int index) const { return NULL; }


  virtual void getValues(int &pid, PropPanel::ContainerPropertyControl &panel)
  {
    ++pid;
    if (curveType != panel.getInt(typePid))
    {
      curveType = panel.getInt(typePid);
      initCurveControl(panel);
    }

    ++pid;

    //------------------------------------

    Tab<Point2> pts(tmpmem);
    getControlPoints(pts);

    ptCnt = pts.size();
    for (int i = 0; i < pts.size(); ++i)
      position[i] = pts[i];
  }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
  {
    Tab<Point2> segc(tmpmem);
    bool trans_x = true;
    if (mPanel)
      trans_x = mPanel->getCurveCubicCoefs(controlPid, segc);
    else
    {
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
      {
        for (int i = 0; i < ptCnt; i++)
          curve->addNewControlPoint(position[i]);
        trans_x = curve->getCoefs(segc);
      }
    }

    if (trans_x || segc.size() < 4 || segc.size() > 5 * 4 || (segc.size() & 3))
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

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
  {
    if (curveType)
      blk.setInt("curveType", curveType);
    for (int i = 0; i < ptCnt; ++i)
      blk.setPoint2(String(32, "pos%d", i), position[i]);
    for (int i = ptCnt; i < MAX_POINTS; ++i)
      blk.removeParam(String(32, "pos%d", i));
  }

  virtual void loadValues(const DataBlock &blk)
  {
    curveType = blk.getInt("curveType", 0);
    ptCnt = 0;
    for (int i = 0; i < MAX_POINTS; ++i)
      if (blk.paramExists(String(32, "pos%d", i)))
      {
        position[i] = blk.getPoint2(String(32, "pos%d", i), position[i]);
        position[i].x = clamp<float>(position[i].x, 0, 1);
        position[i].y = clamp<float>(position[i].y, 0, 1);
        ptCnt++;
      }
      else
        break;
    if (curveType == 0 && ptCnt != 4)
    {
      logerr("bad src data: curveType=%d ptCnt=%d", curveType, ptCnt);
      initPoints();
    }
    if (mPanel)
      initCurveControl(*mPanel);
  }
};


TunedElement *create_tuned_cubic_curve(const char *name, E3DCOLOR color) { return new TunedCubicCurveParam(name, color); }

}; // namespace ScriptHelpers
