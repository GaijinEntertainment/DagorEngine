#pragma once

#include "p_base.h"
#include "../windowControls/w_spin_edit.h"
#include <propPanel2/c_panel_base.h>


class CPoint2 : public BasicPropertyControl
{
public:
  CPoint2(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int prec);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_POINT2 | CONTROL_CAPTION | CONTROL_DATA_PREC; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_POINT2; }

  Point2 getPoint2Value() const;
  void setPoint2Value(Point2 value);
  void setPrecValue(int prec);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  int mSpinWidth;
  Point2 mValue;

  WStaticText mCaption;
  WFloatSpinEdit mSpin1, mSpin2;
};
