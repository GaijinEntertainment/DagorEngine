//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <sepGui/wndCommon.h>
#include <sepGui/wndMenuInterface.h>

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
  virtual IWndEmbeddedWindow *onWmCreateWindow(void *handle, int type) = 0;
  virtual bool onWmDestroyWindow(void *handle) = 0;
};


class IWndManager
{
public:
  virtual ~IWndManager(){};

  // general
  virtual int run(int width, int height, const char *caption, const char *icon = "", WindowSizeInit size = WSI_NORMAL) = 0;
  virtual void close() = 0;

  virtual void loadLayout(const char *filename = NULL) = 0;
  virtual void saveLayout(const char *filename = NULL) = 0;

  virtual void loadLayoutFromDataBlock(const DataBlock &data_block) = 0;
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

  virtual void *splitWindow(void *old_handle, void *new_handle, float new_window_size = 0.5,
    WindowAlign new_window_align = WA_RIGHT) = 0;
  virtual void *splitNeighbourWindow(void *old_handle, void *new_handle, float new_window_size = 0.5,
    WindowAlign new_window_align = WA_RIGHT) = 0;

  virtual bool resizeWindow(void *handle, float new_window_size) = 0;
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

  virtual void setMinSize(void *handle, int width, int height) = 0;
  virtual void setMenuArea(void *handle, int width, int height) = 0;

  // menu
  virtual IMenu *getMainMenu() = 0;
  virtual IMenu *getMenu(void *handle) = 0;

  // creator
  static IWndManager *createManager(IWndManagerEventHandler *event_handler);

  virtual bool init3d(const char *drv_name = NULL, const DataBlock *blkTexStreaming = NULL) = 0;

  // accelerators
  virtual void addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl = false, bool alt = false, bool shift = false) = 0;
  virtual void addAcceleratorUp(unsigned cmd_id, unsigned v_key, bool ctrl = false, bool alt = false, bool shift = false) = 0;
  virtual void clearAccelerators() = 0;
};
