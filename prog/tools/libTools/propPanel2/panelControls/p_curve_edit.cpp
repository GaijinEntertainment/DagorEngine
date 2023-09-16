// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_curve_edit.h"
#include "../c_constants.h"
#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_dialogs.h>

CCurveEdit::CCurveEdit(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, h + DEFAULT_CONTROL_HEIGHT + 2),
  mCurve(this, parent->getWindow(), x, y + DEFAULT_CONTROL_HEIGHT + 2, w, h),
  mCaption(this, parent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT)
{
  mCaption.setTextValue(caption);
  setIntValue(CURVE_CMR_APP);
  initTooltip(&mCurve);
}


PropertyContainerControlBase *CCurveEdit::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createCurveEdit(id, caption, 0, true, new_line);
  return NULL;
}


void CCurveEdit::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }

void CCurveEdit::setColorValue(E3DCOLOR value) { mCurve.setColor(value); }


void CCurveEdit::setBoolValue(bool value) { mCurve.setFixed(value); }

void CCurveEdit::setIntValue(int value)
{
  // approximation

  switch (value & CONTROL_APROXIMATION_MASK)
  {
    case CURVE_LINEAR_APP: mCurve.setCB(new LinearCB()); break;

    case CURVE_CMR_APP: mCurve.setCB(new CatmullRomCBTest()); break;

    case CURVE_NURB_APP:
      // mCurve.setCB(new  NurbCBTest());
      break;

    case CURVE_QUAD_APP:
      // mCurve.setCB(new  QuadraticCBTest());
      break;

    case CURVE_CUBICPOLYNOM_APP: mCurve.setCB(new CubicPolynomCB()); break;

    case CURVE_CUBIC_P_SPLINE_APP: mCurve.setCB(new CubicPSplineCB()); break;
  }

  // cycled mode

  mCurve.setCycled(value & CONTROL_CYCLED_VALUES);
}

void CCurveEdit::setMinMaxStepValue(float min, float max, float step)
{
  switch ((int)floor(step))
  {
    case CURVE_MIN_MAX_POINTS: mCurve.setMinMax(floor(min), floor(max)); break;

    case CURVE_MIN_MAX_X: mCurve.setXMinMax(min, max); break;

    case CURVE_MIN_MAX_Y: mCurve.setYMinMax(min, max); break;
  }
}

void CCurveEdit::setFloatValue(float value) { mCurve.setCurValue(value); }


void CCurveEdit::getCurveCoefsValue(Tab<Point2> &points) const { mCurve.getValue(points); }

bool CCurveEdit::getCurveCubicCoefsValue(Tab<Point2> &xy_4c_per_seg) const
{
  const ICurveControlCallback *cb = mCurve.getCB();
  xy_4c_per_seg.clear();
  if (!cb)
    return false;
  return cb->getCoefs(xy_4c_per_seg);
}

void CCurveEdit::setControlPointsValue(Tab<Point2> &points) { mCurve.setValue(points); }


void CCurveEdit::setWidth(unsigned w)
{
  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(w, mCaption.getHeight());
  mCurve.resizeWindow(w, mCurve.getHeight());
}


void CCurveEdit::setHeight(unsigned h)
{
  PropertyControlBase::setHeight(h);
  mCurve.resizeWindow(mCurve.getWidth(), h - mCaption.getHeight());
}


void CCurveEdit::setEnabled(bool enabled)
{
  mCurve.setEnabled(enabled);
  mCaption.setEnabled(enabled);
}


void CCurveEdit::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mCurve.moveWindow(x, y + DEFAULT_CONTROL_HEIGHT + 2);
}


long CCurveEdit::onWcClipboardCopy(WindowBase *source, DataBlock &blk)
{
  blk.reset();
  blk.setStr("DataType", "Curve");
  Tab<Point2> points(tmpmem);
  getCurveCoefsValue(points);
  for (int i = 0; i < points.size(); ++i)
    blk.addPoint2("point", points[i]);
  return 1;
}

long CCurveEdit::onWcClipboardPaste(WindowBase *source, const DataBlock &blk)
{
  if (strcmp(blk.getStr("DataType", ""), "Curve") != 0)
    return 0;

  mCurve.setSelected(true);
  mCurve.refresh();
  int result = 0;
  char caption[128];
  mCaption.getTextValue(caption, 127);

  if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Replace curve points for \"%s\"?", caption) ==
      wingw::MB_ID_YES)
  {
    Tab<Point2> points(tmpmem);
    int nameId = blk.getNameId("point");
    for (int i = 0; i < blk.paramCount(); ++i)
      if (blk.getParamNameId(i) == nameId)
        points.push_back(blk.getPoint2(i));
    setControlPointsValue(points);
    onWcChange(&mCurve);
    result = 1;
  }

  mCurve.setSelected(false);
  return result;
}
