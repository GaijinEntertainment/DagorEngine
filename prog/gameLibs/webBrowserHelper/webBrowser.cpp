// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_log.h>
#include <webbrowser/webbrowser.h>
#include <webBrowserHelper/webBrowser.h>

#include "doubleBuffer.h"
#include "logger.h"
#include "sqEventDispatch.h"


namespace webbrowser
{

WebBrowserHelper browser_helper;
webbrowser::Logger logger;

WebBrowserHelper *get_helper() { return &browser_helper; }

void WebBrowserHelper::onInitialized()
{
  debug("[BRWS] %s", __FUNCTION__);
  dispatch_sq_event(BROWSER_EVENT_INITIALIZED, this->url);
  if (!this->url.empty() && this->browser)
    this->browser->go(this->url.str());
}


void WebBrowserHelper::onBrowserCreated()
{
  debug("[BRWS] %s", __FUNCTION__);
  dispatch_sq_event(BROWSER_EVENT_BEGIN_LOADING_FRAME, this->url.str());
}


void WebBrowserHelper::onBrowserExited() { debug("[BRWS] %s", __FUNCTION__); }
void WebBrowserHelper::onBrowserError(webbrowser::WebBrowserError e)
{
  SimpleString errStr;
  switch (e)
  {
    case webbrowser::WB_ERR_HELPER_SPAWN: errStr = "error.helper.spawn"; break;
    case webbrowser::WB_ERR_HELPER_INIT: errStr = "error.helper.init"; break;
    case webbrowser::WB_ERR_HELPER_LOST: errStr = "error.helper.lost"; break;
    case webbrowser::WB_ERR_HELPER_TERMINATED: errStr = "error.helper.term"; break;
    case webbrowser::WB_ERR_HELPER_KILLED: errStr = "error.helper.killed"; break;
    case webbrowser::WB_ERR_HELPER_CRASHED: errStr = "error.helper.crashed"; break;
    default: errStr = "error.unspecified"; break;
  }
  logwarn("[BRWS] %s: %s", __FUNCTION__, errStr);
  dispatch_sq_event(BROWSER_EVENT_BROWSER_CRASHED, this->url.str(), NULL, false, errStr.str());
}


void WebBrowserHelper::applyPageInfo(const PageInfo &pi)
{
  if (*pi.url)
    this->url = pi.url;
  if (*pi.title)
    this->title = pi.title;
}


void WebBrowserHelper::onPageCreated(const PageInfo &pi)
{
  debug("[BRWS] %s: %s", __FUNCTION__, *pi.url ? pi.url : "<empty>");
  applyPageInfo(pi);
  dispatch_sq_event(BROWSER_EVENT_BEGIN_LOADING_FRAME, this->url.str(), this->title.str(), pi.canGoBack);
}


void WebBrowserHelper::onPageAborted(const PageInfo &pi)
{
  debug("[BRWS] %s: %s", __FUNCTION__, *pi.url ? pi.url : "<empty>");
  applyPageInfo(pi);
  dispatch_sq_event(BROWSER_EVENT_FINISH_LOADING_FRAME, this->url.str(), this->title.str(), pi.canGoBack);
}


void WebBrowserHelper::onPageLoaded(const PageInfo &pi)
{
  debug("[BRWS] %s: %s", __FUNCTION__, *pi.url ? pi.url : "<empty>");
  applyPageInfo(pi);
  dispatch_sq_event(BROWSER_EVENT_FINISH_LOADING_FRAME, this->url.str(), this->title.str(), pi.canGoBack);
}


void WebBrowserHelper::onPageError(const PageInfo &pi)
{
  SimpleString err((pi.error && *pi.error) ? pi.error : "Unspecified error");
  debug("[BRWS] %s: %s", __FUNCTION__, pi.isErrorRecoverable ? "cache miss" : err);
  if (!pi.error)
    dispatch_sq_event(BROWSER_EVENT_FINISH_LOADING_FRAME, this->url.str(), this->title.str(), pi.canGoBack);
  else if (pi.isErrorRecoverable)
    dispatch_sq_event(BROWSER_EVENT_NEED_RESEND_FRAME, this->url.str(), this->title.str(), pi.canGoBack);
  else
    dispatch_sq_event(BROWSER_EVENT_FAIL_LOADING_FRAME, this->url.str(), this->title.str(), pi.canGoBack, err);
}


void WebBrowserHelper::onPaint(const void *b, unsigned short w, unsigned short h) { this->tex_buf->update((const uint8_t *)b, w, h); }


void WebBrowserHelper::init(const Configuration &cfg)
{
  if (!webbrowser::is_available())
  {
    debug("[BRWS] %s: browser not available", __FUNCTION__);
    return;
  }
  else
    debug("[BRWS] %s as %s at %s (%d ports)", __FUNCTION__, cfg.userAgent, cfg.resourcesBaseDir, cfg.availablePorts.size());

  props = new webbrowser::IBrowser::Properties();
  tex_buf = new DoubleBuffer();
  config = cfg;

  props->userAgent = config.userAgent;
  props->logPath = config.logPath;
  props->cachePath = config.cachePath;
  props->resourcesDir = config.resourcesBaseDir;
  props->acceptLanguages = config.acceptLanguages;
  props->availablePorts = config.availablePorts.data();
  props->availablePortsCount = config.availablePorts.size();
  props->purgeCookiesOnStartup = config.purgeCookiesOnStartup;
}


bool WebBrowserHelper::createBrowserWindow()
{
  if (browser)
  {
    logerr("Trying to create second instance of a browser on the same helper.");
    return false;
  }

  if (!props)
  {
    logerr("Tried to create browser but props were not initialized.");
    return false;
  }

  browser = create(*props, *this, logger);

  if (browser)
  {
    browser->open();
    return true;
  }
  else
  {
    logwarn("[BRWS] Could not create browser");
    return false;
  }
}


void WebBrowserHelper::update(uint16_t x, uint16_t y)
{
  if (!browser)
    return;

  browser->tick();
  if (tex_buf->size() == Point2(0, 0))
  {
    browser->resize(x, y);
    tex_buf->resize(x, y);
  }
}


void WebBrowserHelper::destroyBrowserWindow()
{
  tex_buf->resize(0, 0);
  if (browser)
  {
    browser->close();
    del_it(browser);
  }
}


void WebBrowserHelper::shutdown()
{
  debug("[BRWS] shutting down helper");
  if (!props)
    return; // nothing to shutdown, browser not initialized

  del_it(props);
  del_it(tex_buf);
  if (browser)
    browser->close();
  del_it(browser);
}


bool WebBrowserHelper::hasTexture() { return !tex_buf->empty(); }


TEXTUREID WebBrowserHelper::getTexture()
{
  tex_buf->swap();
  return tex_buf->getActiveTextureId();
}

d3d::SamplerHandle WebBrowserHelper::getSampler() const { return tex_buf->getActiveSampler(); }

void WebBrowserHelper::goBack()
{
  if (browser)
    browser->back();
}


void WebBrowserHelper::go(const char *u)
{
  debug("[BRWS] %s: %s", __FUNCTION__, u);

  this->url = u;

  if (!this->url.empty() && browser)
    browser->go(this->url.str());
}


void WebBrowserHelper::reload()
{
  if (browser)
    browser->reload();
}


void WebBrowserHelper::onWindowMethodCall(const char *name, const char *p1, const char *p2)
{
  debug("[BRWS] %s", __FUNCTION__);
  dispatch_js_call_sq_event(name, p1, p2);
}

} // namespace webbrowser
