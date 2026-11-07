// Copyright (C) Gaijin Games KFT.  All rights reserved.

struct Stub
{

  void shutdown() {}

  bool changeGamma(float) { return false; }
  void getDisplaySize(int &width, int &height, bool) { width = height = 0; }
  void getVideoModeList(Tab<String> &list) { list.clear(); }
  void getMonitors(Tab<String> &monitorNames) { monitorNames.clear(); }
  bool getMonitorInfo(const char *, String *, int *) { return false; }
  void getResolutionsFromMonitor(const char *, Tab<String> &resolutions) { resolutions.clear(); }
  void *getMainWindowPtrHandle() { return nullptr; }
  bool isMainWindow(void *) { return false; }
  void destroyMainWindow() {}
  bool initWindow(const char *, int, int, const linux_GUI::WindowCreationOptions &) { return false; }
  void getWindowPosition(void *, int &cx, int &cy) { cx = cy = 0; }
  void getWindowFramePosition(void *, int &x, int &y) { x = y = 0; }
  bool isWindowMaximized(void *) { return false; }
  void setTitle(const char *, const char *) {}
  void setTitleUTF8(const char *, const char *) {}
  int getScreenRefreshRate() { return 0; }
  void setFullscreenMode(bool) {}
  void processMessages() {}
  bool getWindowClientSize(void *, int &, int &) { return false; }
  bool getWindowScreenRect(void *, linux_GUI::RECT *, linux_GUI::RECT *) { return false; }
  bool getLastCursorPos(int *, int *, void *) { return false; }
  void setCursor(void *, const char *) {}
  void setCursorPosition(int, int, void *) {}
  void getAbsoluteCursorPosition(int &cx, int &cy) { cx = cy = 0; }
  bool getCursorDelta(int &, int &, void *) { return false; }
  void clipCursor() {}
  void unclipCursor() {}
  void hideCursor(bool) {}
  void *getNativeDisplay() { return nullptr; }
  void *getNativeWindow(void *) { return nullptr; }
  void lockWindow(void *) {}
  void unlockWindow(void *) {}
  bool getClipboardUTF8Text(char *, int) { return false; }
  bool setClipboardUTF8Text(const char *) { return false; }
};
