// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/functional.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <math/dag_e3dColor.h>

class IWndManager;
class IWndManagerEventHandler;
class String;

class EditorMainWindow
{
public:
  // It must return true if the drag and drop has been handled.
  typedef eastl::function<bool(const dag::Vector<String> &files)> FileDropHandler;

  explicit EditorMainWindow(IWndManagerEventHandler &event_handler);

  // file_drop_handler: if it is set then the main window will accept drag and dropped files from Windows.
  // bg_color: background color of the window (can be seen before d3d starts drawing)
  void run(const char *caption, const char *icon, FileDropHandler file_drop_handler = nullptr,
    E3DCOLOR bg_color = E3DCOLOR(229, 229, 229));

  intptr_t windowProc(void *h_wnd, unsigned msg, void *w_param, void *l_param);

private:
  IWndManager *createWindowManager();
  void onMainWindowCreated();
  void onClose();
  void close();

  bool onMouseButtonDown(int button);
  bool onMouseButtonUp(int button);

  IWndManagerEventHandler &eventHandler;
  void *mainHwnd = nullptr;
  eastl::unique_ptr<IWndManager> wndManager;
  FileDropHandler fileDropHandler;
};
