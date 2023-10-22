// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_curve.h"
#include <propPanel2/c_panel_base.h>

class CCurveEdit : public BasicPropertyControl
{
public:
  CCurveEdit(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, hdpi::Px h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const
  {
    return CONTROL_CAPTION | CONTROL_DATA_TYPE_CONTROL_POINTS | CONTROL_DATA_TYPE_COLOR | CONTROL_DATA_TYPE_BOOL;
  };
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_SPLINE_COEF; };

  void setCaptionValue(const char value[]);
  void setColorValue(E3DCOLOR value);
  void setBoolValue(bool value);                             // lock ends
  void setIntValue(int value);                               // set approximation method
  void setMinMaxStepValue(float min, float max, float step); // set Y borders
  void setFloatValue(float value);                           // current X value line
  void setEnabled(bool enabled);
  void getCurveCoefsValue(Tab<Point2> &points) const;
  void setControlPointsValue(Tab<Point2> &points);
  bool getCurveCubicCoefsValue(Tab<Point2> &xy_4c_per_seg) const;

  void setWidth(hdpi::Px w);
  void setHeight(hdpi::Px h);
  void moveTo(int x, int y);

  long onWcClipboardCopy(WindowBase *source, DataBlock &blk);
  long onWcClipboardPaste(WindowBase *source, const DataBlock &blk);

private:
  WStaticText mCaption;
  WCurveControl mCurve;
};
