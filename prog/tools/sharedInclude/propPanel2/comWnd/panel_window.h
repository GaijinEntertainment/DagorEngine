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
class IMenu;
class PanelWindowContextMenuEventHandler;


class CPanelWindow : public PropertyContainerVert, public IWndEmbeddedWindow
{
public:
  CPanelWindow(ControlEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]);
  ~CPanelWindow();

  virtual WindowBase *getWindow();
  virtual void *getWindowHandle();
  virtual void *getParentWindowHandle();
  unsigned getTypeMaskForSet() const;
  unsigned getTypeMaskForGet() const;

  void setCaptionValue(const char value[]);
  virtual void clear() override;
  virtual void setWidth(hdpi::Px w);
  virtual void setHeight(hdpi::Px h);
  virtual void moveTo(int x, int y);

  virtual int getScrollPos();
  virtual void setScrollPos(int pos);

  virtual void showPanel(bool visible);

  virtual void onChildResize(int id);

  // saving and loading state with datablock
  virtual int saveState(DataBlock &datablk);
  virtual int loadState(DataBlock &datablk);

  // IWndEmbeddedWindow
  virtual void onWmEmbeddedResize(int width, int height);
  virtual bool onWmEmbeddedMakingMovable(int &w, int &h) { return true; }

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onControlAdd(PropertyControlBase *control);
  virtual void onWcResize(WindowBase *source);
  virtual void onWcRightClick(WindowBase *source);
  virtual void onWcCommand(WindowBase *source, unsigned notify_code, unsigned elem_id);

protected:
  void scrollCheck();

  WWindow *mPanelWindow;
  WContainer *mPanel;
  void *mParentHandle;
  IMenu *contextMenu;
  PanelWindowContextMenuEventHandler *contextMenuEventHandler;
};
