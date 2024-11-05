// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/functional.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>

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
  void run(const char *caption, const char *icon, FileDropHandler file_drop_handler = nullptr);

  intptr_t windowProc(void *h_wnd, unsigned msg, void *w_param, void *l_param);

private:
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
