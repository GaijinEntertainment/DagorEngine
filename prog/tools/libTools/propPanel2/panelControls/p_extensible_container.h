#pragma once

#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_ext_buttons.h"
#include <propPanel2/c_panel_placement.h>

class CExtensible : public PropertyContainerVert
{
public:
  CExtensible(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_STRING | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_INT; }

  int getIntValue() const;
  void setIntValue(int value);
  virtual void setTextValue(const char value[]);
  virtual int getCaptionValue(char *buffer, int buflen) const;

  void moveTo(int x, int y);
  virtual WindowBase *getWindow();
  virtual void setEnabled(bool enabled);
  virtual void setWidth(unsigned w);

  virtual void onChildResize(int id);
  virtual bool isRealContainer() { return false; }

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onControlAdd(PropertyControlBase *control);
  virtual int getNextControlX(bool new_line = true);
  virtual unsigned getClientWidth();

  virtual void onWcClick(WindowBase *source);

private:
  WContainer mRect;
  WPlusButton mPlusButton;
  WExtButtons mQuartButtons;
  mutable int mButtonStatus;
  bool mWResize;
};
