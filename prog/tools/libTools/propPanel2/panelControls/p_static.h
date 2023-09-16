#pragma once

#include "../windowControls/w_simple_controls.h"
#include "../c_constants.h"
#include <propPanel2/c_panel_base.h>


class CStatic : public PropertyControlBase
{
public:
  CStatic(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, const char caption[],
    int h = DEFAULT_CONTROL_HEIGHT);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_STRING; }

  void setEnabled(bool enabled);
  void setCaptionValue(const char value[]);
  void setTextValue(const char value[]);
  int getTextValue(char *buffer, int buflen) const;
  void setBoolValue(bool value);


  void setWidth(unsigned w);
  void moveTo(int x, int y);

private:
  WStaticText mCaption;
};
