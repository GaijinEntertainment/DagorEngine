#pragma once

#include "p_base.h"
#include "../windowControls/w_spin_edit.h"
#include <propPanel2/c_panel_base.h>
#include <math/dag_Point4.h>


class CPoint4 : public BasicPropertyControl
{
public:
  CPoint4(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, const char caption[],
    int prec);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_POINT4 | CONTROL_CAPTION | CONTROL_DATA_PREC; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_POINT4; }

  Point4 getPoint4Value() const;
  void setPoint4Value(Point4 value);
  void setPrecValue(int prec);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  int mSpinWidth;
  Point4 mValue;

  WStaticText mCaption;
  WFloatSpinEdit mSpin1, mSpin2, mSpin3, mSpin4;
};
