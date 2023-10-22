#pragma once

#include "p_toolbar.h"
#include <propPanel2/c_panel_base.h>


// it is a fiction control - it created in CToolbar control
class CToolButton : public PropertyControlBase
{
public:
  CToolButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id);

  virtual unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_BOOL | CONTROL_CAPTION | CONTROL_BUTTON_PICTURES; }
  virtual unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_BOOL; }

  virtual bool getBoolValue() const;
  virtual void setBoolValue(bool value);
  virtual void setCaptionValue(const char value[]);
  virtual void setButtonPictureValues(const char *fname = NULL);


  virtual void setEnabled(bool enabled);
  virtual void setWidth(hdpi::Px w) {}
  virtual void moveTo(int x, int y) {}

private:
  CToolbar *mTParent;
};
