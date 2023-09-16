#pragma once

#include "../windowControls/w_tab.h"
#include <propPanel2/c_panel_base.h>


class CTabPanel : public PropertyContainerControlBase
{
public:
  CTabPanel(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  // Creates
  virtual PropertyContainerControlBase *createTabPage(int id, const char caption[]);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_INT; }

  virtual int getIntValue() const;
  virtual void setIntValue(int value);

  virtual void setWidth(unsigned w);
  virtual void moveTo(int x, int y);
  virtual void onChildResize(int id);

  virtual WindowBase *getWindow();

  virtual void onWcChange(WindowBase *source);

  void setPageCaption(int id, const char caption[]);
  virtual void clear();

protected:
  virtual void resizeControl(unsigned w, unsigned h);
  virtual void onControlAdd(PropertyControlBase *control);

private:
  void onPageActivated();

  WTabPanel mTab;
};
