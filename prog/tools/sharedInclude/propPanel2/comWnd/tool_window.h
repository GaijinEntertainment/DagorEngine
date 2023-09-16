//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/c_panel_placement.h>
#include <sepGui/wndEmbeddedWindow.h>

class WWindow;
class WContainer;


class CToolWindow : public PropertyContainerHorz, public IWndEmbeddedWindow
{
public:
  CToolWindow(ControlEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[]);
  ~CToolWindow();

  virtual WindowBase *getWindow();
  virtual void *getWindowHandle();
  virtual void *getParentWindowHandle();
  int getPanelWidth() const;
  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION; };
  unsigned getTypeMaskForGet() const { return 0; };

  void setCaptionValue(const char value[]);
  virtual void setWidth(unsigned w);
  virtual void setHeight(unsigned h);
  virtual void moveTo(int x, int y);

  virtual void showPanel(bool visible);

  virtual void onChildResize(int id);

  // IWndEmbeddedWindow
  virtual void onWmEmbeddedResize(int width, int height);
  virtual bool onWmEmbeddedMakingMovable(int &width, int &height);

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onControlAdd(PropertyControlBase *control);
  virtual void onWcResize(WindowBase *source);

private:
  void scrollCheck();

  WWindow *mPanelWindow;
  WContainer *mPanel;
  void *mParentHandle;
};
