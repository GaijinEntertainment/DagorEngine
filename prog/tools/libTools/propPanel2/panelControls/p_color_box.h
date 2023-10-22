#pragma once

#include "p_base.h"
#include "../windowControls/w_color_button.h"
#include "../windowControls/w_spin_edit.h"
#include <propPanel2/c_panel_base.h>


class CColorBox : public BasicPropertyControl
{
public:
  CColorBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_COLOR | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_COLOR; }

  E3DCOLOR getColorValue() const { return mColor; }
  void setColorValue(E3DCOLOR value);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

  void updateInterface(char changes);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  int mSpinWidth;
  E3DCOLOR mColor;

  WStaticText mCaption;
  WSpinEdit mRSpin, mGSpin, mBSpin, mASpin;
  WColorButton mButton;
};
