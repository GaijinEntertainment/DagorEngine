#pragma once

#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_toolbar.h"
#include "../c_constants.h"
#include <propPanel2/c_panel_base.h>


class CToolbar : public PropertyContainerControlBase
{
public:
  CToolbar(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, const char *caption);

  virtual ~CToolbar();

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return 0; }
  unsigned getTypeMaskForGet() const { return 0; }

  void setEnabled(bool enabled);
  virtual void setWidth(unsigned w);
  void moveTo(int x, int y);

  // Toolbar

  virtual bool setButtonEnabled(int id, bool enabled);
  virtual bool setButtonValue(int id, bool value);
  virtual bool getButtonValue(int id);
  virtual void setButtonCaption(int id, const char *caption);
  virtual void setButtonPictures(int id, const char *fname = NULL);

  // container

  virtual void createCheckBox(int id, const char caption[], bool value = false, bool enabled = true, bool new_line = true);
  virtual void createButton(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual void createSeparator(int id, bool new_line = true);
  virtual void createRadio(int id, const char caption[], bool enabled = true, bool new_line = true);

  virtual void clear();

  virtual void onChildResize(int id){};
  virtual void onWcClick(WindowBase *source);
  virtual void onWcResize(WindowBase *source);

protected:
  virtual void resizeControl(unsigned w, unsigned h){};

  virtual void addControl(PropertyControlBase *pcontrol, bool new_line = true);
  virtual void onControlAdd(PropertyControlBase *control){};
  virtual void addToolControl(int id);

  virtual int getNextControlX(bool new_line = true) { return 0; }
  virtual int getNextControlY(bool new_line = true) { return 0; }

private:
  WContainer mContainer;
  WToolbar mToolbar;
};
