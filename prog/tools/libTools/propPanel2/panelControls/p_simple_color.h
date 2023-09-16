#pragma once

#include "p_base.h"
#include "../windowControls/w_color_button.h"
#include <propPanel2/c_panel_base.h>


class CSimpleColor : public BasicPropertyControl
{
public:
  CSimpleColor(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
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
  void setWidth(unsigned w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  int mButtonWidth;
  E3DCOLOR mColor;

  WStaticText mCaption;
  WColorButton mButton;
};
