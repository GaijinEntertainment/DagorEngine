// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/hdpiUtil.h>

class IWndManager;
class DataBlock;
typedef unsigned int ImGuiID;
typedef int ImGuiKeyChord;

enum WindowSizeInit
{
  WSI_NORMAL,
  WSI_MINIMIZED,
  WSI_MAXIMIZED,
};


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
  virtual ~IWndManager() {}

  // general
  virtual int run(int width, int height, const char *caption, const char *icon = "", WindowSizeInit size = WSI_NORMAL) = 0;
  virtual void close() = 0;

  virtual bool loadLayout(const char *filename = NULL) = 0;
  virtual void saveLayout(const char *filename = NULL) = 0;

  virtual void setMainWindowCaption(const char *caption) = 0;

  virtual void registerWindowHandler(IWndManagerWindowHandler *handler) = 0;
  virtual void unregisterWindowHandler(IWndManagerWindowHandler *handler) = 0;

  virtual void reset() = 0;
  virtual void show(WindowSizeInit size = WSI_NORMAL) = 0;

  // windows
  virtual void *getMainWindow() const = 0;

  virtual bool removeWindow(void *handle) = 0;

  virtual void setWindowType(void *handle, int type) = 0;

  virtual void getWindowClientSize(void *handle, unsigned &width, unsigned &height) = 0;
  virtual bool getWindowPosSize(void *handle, int &x, int &y, unsigned &width, unsigned &height) = 0;

  virtual void setMenuArea(void *handle, hdpi::Px width, hdpi::Px height) = 0;

  // creator
  static IWndManager *createManager(IWndManagerEventHandler *event_handler);

  virtual bool init3d(const char *drv_name = NULL, const DataBlock *blkTexStreaming = NULL) = 0;

  // accelerators
  virtual void addAccelerator(unsigned cmd_id, ImGuiKeyChord key_chord) = 0;
  virtual void addAcceleratorUp(unsigned cmd_id, ImGuiKeyChord key_chord) = 0;
  virtual void addViewportAccelerator(unsigned cmd_id, ImGuiKeyChord key_chord, bool allow_repeat = false) = 0;
  virtual void clearAccelerators() = 0;
  virtual unsigned processImguiAccelerator() = 0;
  virtual unsigned processImguiViewportAccelerator(ImGuiID viewport_id) = 0;

  virtual void initCustomMouseCursors(const char *path) = 0;
  virtual void updateImguiMouseCursor() = 0;
};
