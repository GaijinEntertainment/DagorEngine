//
// Dagor Engine 6.5 - Web Browser Library
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>


namespace webbrowser
{
namespace io
{

struct MouseMove { short x, y; };
struct MouseWheel { short x, y, dx, dy; };
struct MouseButton
{
  enum Id
  {
    LEFT = 0,
    MIDDLE,
    RIGHT
  };

  short x, y;
  Id btn;
  bool isUp;
};

struct Key
{
  uintptr_t winKey;
  intptr_t nativeKey;
  bool isSysKey;
  enum Modifier
  {
    NONE = 0x00,
    CAPS = 1 << 0,
    SHIFT = 1 << 1,
    CTRL = 1 << 2,
    ALT = 1 << 3
  };
  unsigned short modifiers;

  enum Event
  {
    PRESS = 0,
    RELEASE,
    CHAR
  };
  Event event;
};

} // namespace io


enum WebBrowserError
{
  WB_ERR_NONE = 0,
  WB_ERR_HELPER_SPAWN,
  WB_ERR_HELPER_INIT,
  WB_ERR_HELPER_LOST,
  WB_ERR_HELPER_TERMINATED,
  WB_ERR_HELPER_KILLED,
  WB_ERR_HELPER_CRASHED
};


class IBrowser
{
  public:
    virtual ~IBrowser() { /* VOID */ }

    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool tick() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void go(const char *url) = 0;
    virtual void back() = 0;
    virtual void forward() = 0;
    virtual void reload() = 0;
    virtual void stop() = 0;

    virtual void consume(const io::MouseMove& event) = 0;
    virtual void consume(const io::MouseWheel& event) = 0;
    virtual void consume(const io::MouseButton& event) = 0;
    virtual void consume(const io::Key& event) = 0;

    virtual void resize(unsigned short w, unsigned short h) = 0;

    virtual void addWindowMethod(const char* name, unsigned params_cnt) = 0;

  public:
    struct Properties
    {
      const char *userAgent = NULL;
      const char *logPath = NULL;
      const char *cachePath = NULL;
      const char *resourcesDir = NULL;
      const char *acceptLanguages = NULL;
      const short *availablePorts = NULL;
      short availablePortsCount = 0;
      void* parentWnd = NULL; //NULL to render offscreen, otherwise render to this HWND
      unsigned backgroundColor = 0xFFFFFFFF;
      bool externalPopups = false;
      bool purgeCookiesOnStartup = true;
    }; // struct Properties
}; // class Client


class IHandler
{
  public:
    struct PageInfo
    {
      uint32_t id = 0;
      const char *title = nullptr;
      const char *url = nullptr;
      const char *error = nullptr;
      bool canGoBack = false;
      bool canGoForward = false;
      bool isErrorRecoverable = false;
    }; // struct PageInfo

  public:
    virtual ~IHandler() { /* VOID */ }

    virtual void onInitialized() = 0;

    virtual void onBrowserCreated() = 0;
    virtual void onBrowserExited() = 0;

    virtual void onBrowserError(WebBrowserError e) = 0;

    virtual void onPageCreated(const PageInfo& pi) = 0;
    virtual void onPageAborted(const PageInfo& pi) = 0;
    virtual void onPageLoaded(const PageInfo& pi) = 0;
    virtual void onPageError(const PageInfo& pi) = 0;

    virtual void onPopup(const char* url) = 0; //Will be called only if externalPopups = true

    virtual void onPaint(const void *buf, unsigned short w, unsigned short h) = 0;

    virtual void onWindowMethodCall(const char* name, const char* p1, const char* p2) = 0;
}; // class IHandler


struct ILogger
{
  virtual ~ILogger() { /* VOID */ }

  virtual void err(const char *msg) = 0;
  virtual void warn(const char *msg) = 0;
  virtual void info(const char *msg) = 0;
  virtual void dbg(const char *msg) = 0;
  virtual void flush() = 0;
}; // class ILogger


IBrowser* create(const IBrowser::Properties& props,
                 IHandler& handler,
                 ILogger& logger);

bool is_available();

void set_cef_folder(const wchar_t* path);

} // namespace webbrowser
