// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "curveControlStandalone.h"
#include "../scopedImguiBeginDisabled.h"
#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_dialogs.h>

namespace PropPanel
{

class CurveEditPropertyControl : public PropertyControlBase
{
public:
  CurveEditPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h + hdpi::_pxScaled(DEFAULT_CONTROL_HEIGHT) + hdpi::_pxScaled(2)),
    controlCaption(caption),
    curveControl(this, nullptr, x, y + hdpi::_pxS(DEFAULT_CONTROL_HEIGHT) + hdpi::_pxS(2), hdpi::_px(w), hdpi::_px(h))
  {
    G_ASSERT(mH > 0);
  }

  unsigned getTypeMaskForSet() const override
  {
    return CONTROL_CAPTION | CONTROL_DATA_TYPE_CONTROL_POINTS | CONTROL_DATA_TYPE_COLOR | CONTROL_DATA_TYPE_BOOL;
  }

  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_SPLINE_COEF; };

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setColorValue(E3DCOLOR value) override { curveControl.setColor(value); }

  // lock ends
  void setBoolValue(bool value) override { curveControl.setFixed(value); }

  // set approximation method
  void setIntValue(int value) override
  {
    // approximation

    switch (value & CONTROL_APROXIMATION_MASK)
    {
      case CURVE_LINEAR_APP: curveControl.setCB(new LinearCB()); break;

      case CURVE_CMR_APP: curveControl.setCB(new CatmullRomCBTest()); break;

      case CURVE_NURB_APP:
        // curveControl.setCB(new  NurbCBTest());
        break;

      case CURVE_QUAD_APP:
        // curveControl.setCB(new  QuadraticCBTest());
        break;

      case CURVE_CUBICPOLYNOM_APP: curveControl.setCB(new CubicPolynomCB()); break;

      case CURVE_CUBIC_P_SPLINE_APP: curveControl.setCB(new CubicPSplineCB()); break;
    }

    // cycled mode

    curveControl.setCycled(value & CONTROL_CYCLED_VALUES);
  }

  // set Y borders
  void setMinMaxStepValue(float min, float max, float step) override
  {
    switch ((int)floor(step))
    {
      case CURVE_MIN_MAX_POINTS: curveControl.setMinMax(floor(min), floor(max)); break;

      case CURVE_MIN_MAX_X: curveControl.setXMinMax(min, max); break;

      case CURVE_MIN_MAX_Y: curveControl.setYMinMax(min, max); break;
    }
  }

  // current X value line
  void setFloatValue(float value) override { curveControl.setCurValue(value); }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void getCurveCoefsValue(Tab<Point2> &points) const override { curveControl.getValue(points); }

  bool getCurveCubicCoefsValue(Tab<Point2> &xy_4c_per_seg) const override
  {
    const ICurveControlCallback *cb = curveControl.getCB();
    xy_4c_per_seg.clear();
    if (!cb)
      return false;
    return cb->getCoefs(xy_4c_per_seg);
  }

  void setControlPointsValue(Tab<Point2> &points) override { curveControl.setValue(points); }

  long onWcClipboardCopy(WindowBase *source, DataBlock &blk) override
  {
    blk.reset();
    blk.setStr("DataType", "Curve");
    Tab<Point2> points(tmpmem);
    getCurveCoefsValue(points);
    for (int i = 0; i < points.size(); ++i)
      blk.addPoint2("point", points[i]);
    return 1;
  }

  long onWcClipboardPaste(WindowBase *source, const DataBlock &blk) override
  {
    if (strcmp(blk.getStr("DataType", ""), "Curve") != 0)
      return 0;

    curveControl.setSelected(true);
    int result = 0;

    if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Replace curve points for \"%s\"?",
          controlCaption.c_str()) == wingw::MB_ID_YES)
    {
      Tab<Point2> points(tmpmem);
      int nameId = blk.getNameId("point");
      for (int i = 0; i < blk.paramCount(); ++i)
        if (blk.getParamNameId(i) == nameId)
          points.push_back(blk.getPoint2(i));
      setControlPointsValue(points);
      onWcChange(nullptr);
      result = 1;
    }

    curveControl.setSelected(false);
    return result;
  }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);

    const float width = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
    curveControl.updateImgui(width, mH);
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  CurveControlStandalone curveControl;
};

} // namespace PropPanel