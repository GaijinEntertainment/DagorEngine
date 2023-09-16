#pragma once

#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_placement.h>


class CContainer : public PropertyContainerVert
{
public:
  CContainer(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    int interval);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return 0; }
  unsigned getTypeMaskForGet() const { return 0; }

  void moveTo(int x, int y);

  virtual WindowBase *getWindow();

  virtual void setEnabled(bool enabled);
  virtual void setWidth(unsigned w);

  virtual void onChildResize(int id);

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onControlAdd(PropertyControlBase *control);

private:
  WContainer mRect;
  bool mWResize;
};
