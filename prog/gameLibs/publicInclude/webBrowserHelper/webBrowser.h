//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <3d/dag_texMgr.h>

#include <webbrowser/webbrowser.h>

namespace webbrowser
{

class DoubleBuffer;

enum EventType
{
  BROWSER_EVENT_INITIALIZED,
  BROWSER_EVENT_DOCUMENT_READY,
  BROWSER_EVENT_FAIL_LOADING_FRAME,
  BROWSER_EVENT_CANT_DOWNLOAD,
  BROWSER_EVENT_FINISH_LOADING_FRAME,
  BROWSER_EVENT_BEGIN_LOADING_FRAME,
  BROWSER_EVENT_NEED_RESEND_FRAME,
  BROWSER_EVENT_BROWSER_CRASHED,
};

struct Configuration
{
  SimpleString resourcesBaseDir;
  SimpleString cachePath;
  SimpleString userAgent;
  SimpleString acceptLanguages;
  SimpleString logPath;
  Tab<short> availablePorts;
  bool purgeCookiesOnStartup = true;
}; // struct Configuration

class WebBrowserHelper : public webbrowser::IHandler
{
public:
  IBrowser *getBrowser() { return browser; }
  void init(const Configuration &cfg);
  bool createBrowserWindow();
  void update(uint16_t x, uint16_t y);
  void destroyBrowserWindow();
  void shutdown();

  bool browserExists() { return browser != nullptr; }
  bool canEnable() { return props != nullptr; }

  bool hasTexture();
  TEXTUREID getTexture();
  d3d::SamplerHandle getSampler() const;

  void go(const char *url);
  void goBack();
  void reload();

  const char *getUrl() { return url.str(); }
  const char *getTitle() { return title.str(); }

  // IHandler
public:
  void onInitialized();
  void onBrowserCreated();
  void onBrowserExited();
  void onBrowserError(webbrowser::WebBrowserError e);
  void applyPageInfo(const PageInfo &pi);
  void onPageCreated(const PageInfo &pi);
  void onPageAborted(const PageInfo &pi);
  void onPageLoaded(const PageInfo &pi);
  void onPageError(const PageInfo &pi);
  void onPaint(const void *b, unsigned short w, unsigned short h);
  void onPopup(const char *) {}
  void onWindowMethodCall(const char *name, const char *p1, const char *p2);

private:
  Configuration config;

  IBrowser *browser = nullptr;
  IBrowser::Properties *props = nullptr;

  DoubleBuffer *tex_buf = nullptr;

  SimpleString url;
  SimpleString title;
};

WebBrowserHelper *get_helper();

} // namespace webbrowser
