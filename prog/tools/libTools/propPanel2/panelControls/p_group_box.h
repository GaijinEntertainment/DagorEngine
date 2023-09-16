#pragma once

#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_placement.h>


class CGroupBase : public PropertyContainerVert
{
public:
  CGroupBase(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, WindowControlBase *wc_rect, int id, int x,
    int y, int w, int h, const char caption[]);
  ~CGroupBase();

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return 0; }

  void setCaptionValue(const char value[]);
  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

  virtual WindowBase *getWindow();
  virtual bool isRealContainer() { return false; }

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onControlAdd(PropertyControlBase *control);
  virtual int getNextControlY(bool new_line = true);

private:
  WindowControlBase *mRect;
};


class CGroupBox : public CGroupBase
{
public:
  CGroupBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);
};
