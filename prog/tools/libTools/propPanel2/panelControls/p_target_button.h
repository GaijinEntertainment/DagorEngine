#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>

class CTargetButton : public BasicPropertyControl
{
public:
  CTargetButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_STRING; }

  void setTextValue(const char value[]);
  void setCaptionValue(const char value[]);
  int getTextValue(char *buffer, int buflen) const;

  void reset();

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

protected:
  virtual void onWcClick(WindowBase *source);

private:
  WStaticText mCaption;
  WButton mTargetButton;
  String mValue;
};
