#include "client.h"

#include <unicodeString.h>
#include <assert.h>
#include "../endpoint.h"

#include <shtypes.h>


namespace cef
{
using UnicodeString = webbrowser::UnicodeString;

typedef HRESULT (WINAPI *GetScaleFactorForMonitorProc)(HMONITOR, DEVICE_SCALE_FACTOR*);

static GetScaleFactorForMonitorProc GetScaleFactorForMonitor = NULL;
static bool doInitScaleProc = true;

bool Client::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> /*frame*/,
                           const CefString& target_url,
                           const CefString& /*target_frame_name*/,
                           CefLifeSpanHandler::WindowOpenDisposition target_disposition,
                           bool /*user_gesture*/,
                           const CefPopupFeatures& /*popupFeatures*/,
                           CefWindowInfo& window_info,
                           CefRefPtr<CefClient>& /*client*/,
                           CefBrowserSettings& /*settings*/,
                           CefRefPtr<CefDictionaryValue>& /* extra_info */,
                           bool* /*no_javascript_access*/)
{
  this->endpoint->DBG("%s: disp %d, browser %d", __FUNCTION__, target_disposition, browser->GetIdentifier());
  switch (target_disposition)
  {
    case WOD_UNKNOWN:
    case WOD_CURRENT_TAB:
    case WOD_OFF_THE_RECORD:
      if (!target_url.empty())
        browser->GetMainFrame()->LoadURL(target_url);
      break;

    case WOD_NEW_WINDOW:
    case WOD_NEW_POPUP:
    case WOD_SINGLETON_TAB:
    case WOD_NEW_FOREGROUND_TAB:
    case WOD_NEW_BACKGROUND_TAB:
      if (handlePopupExternal) //Don't handle popup by CEF, just inform client about popup attempt
      {
        endpoint->onPopup(target_url.ToString().c_str());
        return true;
      }
      else
      {
        window_info.SetAsWindowless(this->windowHandle);
        return false; // Allow CEF to handle this
      }
      break;

    case WOD_SAVE_TO_DISK:
    case WOD_IGNORE_ACTION:
    default: // Do nothing
        break;
  }
  return true;
}


void Client::OnAfterCreated(CefRefPtr<CefBrowser> b)
{
  this->endpoint->DBG("%s: new %d, %lu already known", __FUNCTION__, b->GetIdentifier(), this->browsers.size());
  if (!this->browsers.active())
    this->endpoint->onBrowserCreated();
  else
  {
    this->endpoint->DBG("%s: deactivating %d", __FUNCTION__, this->browsers.active()->GetIdentifier());
    this->browsers.active()->GetHost()->SetFocus(false);
  }

  this->browsers.add(b);
  this->browsers.active()->GetHost()->WasResized(); // force OSR update
  this->endpoint->DBG("%s: new active %d, known %lu", __FUNCTION__, this->browsers.active()->GetIdentifier(), this->browsers.size());
}


void Client::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  this->endpoint->DBG("%s: %d, browsers left %lu", __FUNCTION__, browser->GetIdentifier(), this->browsers.size());
  this->browsers.remove(browser);
  this->reactivateBrowser();
  this->endpoint->DBG("%s: %lu browsers after", __FUNCTION__, this->browsers.size());
  if (!this->hasBrowser())
    this->endpoint->onShutdown();
}


bool Client::DoClose(CefRefPtr<CefBrowser> browser)
{
  return false;
}


void Client::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int /*httpStatusCode*/)
{
  if (!frame->IsMain() || !this->browsers.isActive(browser))
    return;

  webbrowser::IHandler::PageInfo pi;
  const std::string url = frame->GetURL().ToString();
  pi.url = url.c_str();
  pi.canGoBack = this->browsers.size() > 1 ? true : browser->CanGoBack();
  pi.canGoForward = browser->CanGoForward();
  this->endpoint->onPageLoaded(pi);
  this->browsers.active()->GetHost()->SetFocus(true);
  this->browsers.active()->GetHost()->Invalidate(PET_VIEW);
}


void Client::OnLoadError(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefLoadHandler::ErrorCode errorCode,
                         const CefString& errorText,
                         const CefString& failedUrl)
{
  this->endpoint->DBG("%s: [%d] %s returned error %d:\n%s\n",
                      __FUNCTION__,
                      browser->GetIdentifier(),
                      failedUrl.ToString().c_str(),
                      errorCode,
                      errorText.ToString().c_str());
  if (errorCode == ERR_ABORTED)
    return;

  if (frame->IsMain() && this->browsers.isActive(browser))
  {
    webbrowser::IHandler::PageInfo pi;
    const std::string url = frame->GetURL().ToString();
    pi.url = url.c_str();
    const std::string error = errorText.ToString();
    pi.error = error.c_str();
    pi.isErrorRecoverable = errorCode == ERR_CACHE_MISS;
    this->endpoint->onPageError(pi);
  }
}


void Client::OnLoadStart(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         TransitionType /*transition_type*/)
{
  if (!frame->IsMain() || !this->browsers.isActive(browser))
    return;

  if (this->cefScale > 1.0)
    browser->GetHost()->SetZoomLevel(cefScale);

  webbrowser::IHandler::PageInfo pi;
  const std::string url = frame->GetURL().ToString();
  pi.url = url.c_str();
  pi.id = browser->GetIdentifier();
  this->endpoint->onPageCreated(pi);
}


void Client::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                  bool isLoading,
                                  bool /*canGoBack*/,
                                  bool /*canGoForward*/)
{
  if (isLoading || !this->browsers.isActive(browser)) // Workaround missing OnLoadEnd for some OnLoadStart calls
    return;

  webbrowser::IHandler::PageInfo pi;
  const std::string url = browser->GetMainFrame()->GetURL().ToString();
  pi.url = url.c_str();
  pi.canGoBack = this->browsers.size() > 1 ? true : browser->CanGoBack();
  pi.canGoForward = browser->CanGoForward();
  this->endpoint->onPageLoaded(pi);
}


void Client::GetViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect)
{
  rect.Set(0, 0, this->width, this->height);
}


void Client::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
  if (!show && this->browsers.isActive(browser))
  {
    this->popup.Set(0, 0, 0, 0);
    free(this->popupTextureCache);
    this->popupTextureCache = NULL;
  }
}


void Client::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& r)
{
  if (this->browsers.isActive(browser))
  {
    this->popup = r;
    size_t texSize = this->popup.width * this->popup.height * 4;
    this->popupTextureCache = (uint8_t*)realloc(this->popupTextureCache, texSize);
  }
}


static void copy_rect(uint8_t *dst, int rowSize, uint8_t *src, const CefRect& r)
{
  uint8_t *texStart = dst + r.y * rowSize;
  for (int i = 0; i < r.height; ++i)
  {
    uint8_t *row = texStart + i * rowSize + r.x * 4;
    memcpy(row, (uint8_t*)src + i * r.width * 4, r.width * 4);
  }
}


void Client::OnPaint(CefRefPtr<CefBrowser> browser,
                     PaintElementType type,
                     const RectList& dirty,
                     const void *buf,
                     int w,
                     int h)
{
  if (!this->browsers.isActive(browser))
    return;

  const int rowSize = 4 * this->width;

  if (type == PET_VIEW)
  {
    if (w > this->width || h > this->height)
      return; // discard, we were probably resized.

    if ((w == this->width && h == this->height) ||
        (dirty.size() == 1 && dirty[0] == CefRect(0, 0, this->width, this->height)))
      memcpy(this->texture, buf, w * h * 4);
    else
      for (auto i = dirty.begin(); i != dirty.end(); ++i)
        copy_rect(this->texture, rowSize, (uint8_t*)buf, *i);
  }
  else if (type == PET_POPUP && this->popupTextureCache)
  {
    memcpy(this->popupTextureCache, buf, w * h * 4);
  }

  if (this->popupTextureCache)
    copy_rect(this->texture, rowSize, this->popupTextureCache, this->popup);

  this->endpoint->onPaint(this->texture, width, height);
}


void Client::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                       CefRequestHandler::TerminationStatus status)
{
  switch (status)
  {
    case TS_ABNORMAL_TERMINATION:
      this->endpoint->onBrowserError(webbrowser::WB_ERR_HELPER_TERMINATED);
      break;
    case TS_PROCESS_WAS_KILLED:
      this->endpoint->onBrowserError(webbrowser::WB_ERR_HELPER_KILLED);
      break;
    case TS_PROCESS_CRASHED:
      this->endpoint->onBrowserError(webbrowser::WB_ERR_HELPER_CRASHED);
      break;
    default: break;
  }
}


void Client::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
  if (!this->browsers.isActive(browser))
    return;

  this->endpoint->DBG("%s: %s", __FUNCTION__, title.ToString().c_str());
  this->browsers.setTitle(browser, title.ToString().c_str());
  webbrowser::IHandler::PageInfo pi;
  const std::string url = browser->GetMainFrame()->GetURL().ToString();
  pi.url = url.c_str();
  const std::string myTitle = title.ToString();
  pi.title = myTitle.c_str();
  pi.canGoBack = this->browsers.size() > 1 ? true : browser->CanGoBack();
  pi.canGoForward = browser->CanGoForward();
  this->endpoint->onPageLoaded(pi);
}


void Client::OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                                 CefRefPtr<CefContextMenuParams>,
                                 CefRefPtr<CefMenuModel> model)
{
  model->Clear();
}


void Client::back()
{
  if (!this->browsers.active())
    return;

  if (this->browsers.active()->CanGoBack())
    this->browsers.active()->GoBack();
  else if (this->browsers.size() > 1)
    this->browsers.active()->GetHost()->CloseBrowser(true);
}


void Client::reactivateBrowser()
{
  if (!this->hasBrowser())
    return;

  this->endpoint->DBG("%s: activating %d", __FUNCTION__, this->browsers.active()->GetIdentifier());
  this->browsers.active()->GetHost()->SetFocus(true);
  this->browsers.active()->GetHost()->Invalidate(PET_VIEW);

  webbrowser::IHandler::PageInfo pi;
  const std::string url = this->browsers.active()->GetMainFrame()->GetURL().ToString();
  pi.url = url.c_str();
  pi.id = this->browsers.active()->GetIdentifier();
  pi.title = this->browsers.getActiveTitle();
  pi.canGoBack = this->browsers.size() > 1 ? true : this->browsers.active()->CanGoBack();
  pi.canGoForward = this->browsers.active()->CanGoForward();
  this->endpoint->onPageCreated(pi);
  this->endpoint->onPageLoaded(pi);
}


bool Client::OnProcessMessageReceived(CefRefPtr<CefBrowser> /* browser */,
                                      CefRefPtr<CefFrame> /* frame */,
                                      CefProcessId /* source_process */,
                                      CefRefPtr<CefProcessMessage> message)
{
  if (message->GetName() == L"call")
  {
    CefRefPtr<CefListValue> vals = message->GetArgumentList();

    UnicodeString name(L"%s", vals->GetString(0).c_str());
    UnicodeString p1(L"%s", vals->GetString(1).c_str());
    UnicodeString p2(L"%s", vals->GetString(2).c_str());

    this->endpoint->DBG("%s: Call window.%s() in %d", __FUNCTION__,
      name.utf8(), this->browsers.active()->GetIdentifier());

    this->endpoint->onWindowMethodCall(name.utf8(), p1.utf8(), p2.utf8());
    return true;
  }

  return false;
}


static void initScaleProc()
{
  doInitScaleProc = false;

  HMODULE api = ::LoadLibraryA("shcore.dll");

  if (!api) //Windows prior Windows 8.1
  {
    GetScaleFactorForMonitor = NULL;
    return;
  }

  GetScaleFactorForMonitor =
    (GetScaleFactorForMonitorProc)::GetProcAddress(api, "GetScaleFactorForMonitor");

  if (!GetScaleFactorForMonitor)
  {
    ::FreeLibrary(api);
    return;
  }
}


void Client::setWindowHandle(cef_window_handle_t h)
{
  this->windowHandle = h;
  this->cefScale = 1.0;

  if (doInitScaleProc)
    initScaleProc();

  if (!GetScaleFactorForMonitor)
    return;

  HMONITOR moni = ::MonitorFromWindow(h, MONITOR_DEFAULTTOPRIMARY);

  DEVICE_SCALE_FACTOR scaleInt;
  if (GetScaleFactorForMonitor(moni, &scaleInt) == S_OK)
    if (scaleInt > 100)
    {
      //CEF use scale exponent +20% per scale level, so 2.0 means 120%, 3.0 means 144% etc
      this->cefScale = logf((float)scaleInt / 100.0) / logf(1.2);
    }
}


} // namespace cef
