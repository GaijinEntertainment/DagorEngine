//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepGui/wndCommon.h>
#include <sepGui/wndMenuInterface.h>
#include <libTools/util/hdpiUtil.h>

class IWndManager;
class IWndEmbeddedWindow;
class DataBlock;


class IWndManagerEventHandler
{
public:
  virtual void onInit(IWndManager *manager) = 0;
  virtual bool onClose() = 0;
  virtual void onDestroy() = 0;
};


class IWndManagerWindowHandler
{
public:
  virtual void *onWmCreateWindow(int type) = 0;
  virtual bool onWmDestroyWindow(void *window) = 0;
};


class IWndManager
{
public:
  virtual ~IWndManager(){};

  // general
  virtual int run(int width, int height, const char *caption, const char *icon = "", WindowSizeInit size = WSI_NORMAL) = 0;
  virtual void close() = 0;

  virtual bool loadLayout(const char *filename = NULL) = 0;
  virtual void saveLayout(const char *filename = NULL) = 0;

  virtual bool loadLayoutFromDataBlock(const DataBlock &data_block) = 0;
  virtual void saveLayoutToDataBlock(DataBlock &data_block) = 0;

  virtual void setMainWindowCaption(const char *caption) = 0;

  virtual void registerWindowHandler(IWndManagerWindowHandler *handler) = 0;
  virtual void unregisterWindowHandler(IWndManagerWindowHandler *handler) = 0;

  virtual void reset() = 0;
  virtual void show(WindowSizeInit size = WSI_NORMAL) = 0;

  virtual void clientToScreen(int &x, int &y) = 0;

  // windows
  virtual void *getMainWindow() const = 0;
  virtual void *getFirstWindow() const = 0;

  inline void *splitWindow(void *old_handle, void *new_handle, hdpi::Px new_wsize, WindowAlign new_walign = WA_RIGHT);
  virtual void *splitWindowF(void *old_handle, void *new_handle, float new_wsize = 0.5, WindowAlign new_walign = WA_RIGHT) = 0;
  inline void *splitNeighbourWindow(void *old_handle, void *new_handle, hdpi::Px new_wsize, WindowAlign new_walign = WA_RIGHT);
  virtual void *splitNeighbourWindowF(void *old_handle, void *new_handle, float new_wsize = 0.5,
    WindowAlign new_walign = WA_RIGHT) = 0;

  inline bool resizeWindow(void *handle, hdpi::Px new_wsize);
  virtual bool resizeWindowF(void *handle, float new_wsize) = 0;
  virtual bool fixWindow(void *handle, bool fix = true) = 0;
  virtual bool removeWindow(void *handle) = 0;

  virtual bool getMovableFlag(void *handle) = 0;

  virtual void setWindowType(void *handle, int type) = 0;
  virtual int getWindowType(void *handle) = 0;

  virtual void getWindowClientSize(void *handle, unsigned &width, unsigned &height) = 0;
  virtual bool getWindowPosSize(void *handle, int &x, int &y, unsigned &width, unsigned &height) = 0;
  virtual int getSplitterControlSize() const = 0;

  virtual void setHeader(void *handle, HeaderPos header_pos) = 0;
  virtual void setCaption(void *handle, const char *caption) = 0;

  virtual void setMinSize(void *handle, hdpi::Px width, hdpi::Px height) = 0;
  virtual void setMenuArea(void *handle, hdpi::Px width, hdpi::Px height) = 0;

  // menu
  virtual IMenu *getMainMenu() = 0;
  virtual IMenu *getMenu(void *handle) = 0;

  // creator
  static IWndManager *createManager(IWndManagerEventHandler *event_handler);

  virtual bool init3d(const char *drv_name = NULL, const DataBlock *blkTexStreaming = NULL) = 0;

  // accelerators
  virtual void addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl = false, bool alt = false, bool shift = false) = 0;
  virtual void addAcceleratorUp(unsigned cmd_id, unsigned v_key, bool ctrl = false, bool alt = false, bool shift = false) = 0;
  virtual void addViewportAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl = false, bool alt = false, bool shift = false,
    bool allow_repeat = false) = 0;
  virtual void clearAccelerators() = 0;
  virtual unsigned processImguiAccelerator() = 0;
  virtual unsigned processImguiViewportAccelerator() = 0;

  virtual void initCustomMouseCursors(const char *path) = 0;
  virtual void updateImguiMouseCursor() = 0;
};

inline void *IWndManager::splitWindow(void *old_handle, void *new_handle, hdpi::Px new_wsize, WindowAlign new_walign)
{
  int sz = _px(new_wsize);
  return splitWindowF(old_handle, new_handle, sz > 2 ? sz : 2, new_walign);
}
inline void *IWndManager::splitNeighbourWindow(void *old_handle, void *new_handle, hdpi::Px new_wsize, WindowAlign new_walign)
{
  int sz = _px(new_wsize);
  return splitNeighbourWindowF(old_handle, new_handle, sz > 2 ? sz : 2, new_walign);
}
inline bool IWndManager::resizeWindow(void *handle, hdpi::Px new_wsize)
{
  int sz = _px(new_wsize);
  return resizeWindowF(handle, sz > 2 ? sz : 2);
}
