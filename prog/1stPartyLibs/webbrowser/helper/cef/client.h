#pragma once

#include <cef_client.h>
#include <cef_life_span_handler.h>
#include <cef_load_handler.h>
#include <cef_render_handler.h>
#include <cef_display_handler.h>

#include "../enableEndpoint.h"
#include "browserList.h"


namespace cef
{

class App;

class Client : public CefClient,
               public CefLifeSpanHandler,
               public CefLoadHandler,
               public CefRenderHandler,
               public CefRequestHandler,
               public CefDisplayHandler,
               public CefDragHandler,
               public CefContextMenuHandler,
               public ipc::EnableEndpoint
{
  public:
    ~Client()
    {
      free(this->texture);
      free(this->popupTextureCache);
    }

  public: /** CefClient interface **/
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
    {
      return CefRefPtr<CefLifeSpanHandler>(this);
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override
    {
      return CefRefPtr<CefLoadHandler>(this);
    }

    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override
    {
      return CefRefPtr<CefRenderHandler>(this);
    }

    virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override
    {
      return CefRefPtr<CefRequestHandler>(this);
    }

    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
    {
      return CefRefPtr<CefDisplayHandler>(this);
    }

    virtual CefRefPtr<CefDragHandler> GetDragHandler() override
    {
      return CefRefPtr<CefDragHandler>(this);
    }

    virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override
    {
      return CefRefPtr<CefContextMenuHandler>(this);
    }

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) override;

  public: /** CefDragHandler **/
    virtual bool OnDragEnter(CefRefPtr<CefBrowser>,
                             CefRefPtr<CefDragData>,
                             CefDragHandler::DragOperationsMask) override
    {
      return true; //No drag&drop in browser window
    }

  public: /** CefLifeSpanHandler interface **/
    virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString& target_url,
                               const CefString& target_frame_name,
                               CefLifeSpanHandler::WindowOpenDisposition target_disposition,
                               bool user_gesture,
                               const CefPopupFeatures& popupFeatures,
                               CefWindowInfo& windowInfo,
                               CefRefPtr<CefClient>& client,
                               CefBrowserSettings& settings,
                               CefRefPtr<CefDictionaryValue>& extra_info,
                               bool* no_javascript_access) override;
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;

  public: /** CefLoadHandler interface **/
    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           int httpStatusCode) override;
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefLoadHandler::ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl) override;
    virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             TransitionType transition_type) override;
    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                      bool isLoading,
                                      bool canGoBack,
                                      bool canGoForward) override;

  public: /** CefRenderHandler interface **/
    virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;
    virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& r) override;
    virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                         PaintElementType type,
                         const RectList& dirtyRects,
                         const void *buf,
                         int width,
                         int height) override;

  public: /** CefRequestHandler interface **/
    virtual void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                           CefRequestHandler::TerminationStatus status) override;

  public: /** CefDisplayHandler interface **/
  virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString& title) override;

  public: /** CefContextMenuHandler **/
    virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) override;

    virtual bool OnContextMenuCommand(CefRefPtr<CefBrowser>,
                                      CefRefPtr<CefFrame>,
                                      CefRefPtr<CefContextMenuParams>,
                                      int,
                                      EventFlags) override
    {
      return false;
    }

  public:
    bool hasBrowser() { return this->browsers.size() != 0; }
    void go(const char *url) { if (this->hasBrowser()) this->browsers.active()->GetMainFrame()->LoadURL(url); }
    void reload() { if (this->hasBrowser()) this->browsers.active()->Reload(); }
    void back();
    void forward() { if (this->hasBrowser()) this->browsers.active()->GoForward(); }
    void stop() { if (this->hasBrowser()) this->browsers.active()->StopLoad(); }
    void show() { if (this->hasBrowser()) this->browsers.active()->GetHost()->WasHidden(false); }
    void hide() { if (this->hasBrowser()) this->browsers.active()->GetHost()->WasHidden(true); }
    void resize(short w, short h)
    {
      this->width = w;
      this->height = h;
      this->texture = (uint8_t*)realloc(this->texture, this->width * this->height * 4);
      if (this->hasBrowser())
        this->browsers.active()->GetHost()->WasResized();
    }

  public:
    void sendMouseMove(const CefMouseEvent& e)
    {
      if (this->hasBrowser())
        this->browsers.active()->GetHost()->SendMouseMoveEvent(e, false);
    }

    void sendMouseWheel(const CefMouseEvent& e, short dx, short dy)
    {
      if (this->hasBrowser())
        this->browsers.active()->GetHost()->SendMouseWheelEvent(e, dx, dy);
    }

    void sendMouseButton(const CefMouseEvent& e, short btn, short isUp)
    {
      if (this->hasBrowser())
        this->browsers.active()->GetHost()->SendMouseClickEvent(e, (CefBrowserHost::MouseButtonType)btn, isUp, 1);
    }

    void sendKeyEvent(const CefKeyEvent& e)
    {
      if (this->hasBrowser())
        this->browsers.active()->GetHost()->SendKeyEvent(e);
    }

  public:
    void shutdown() { browsers.closeAll(); }

    void setWindowHandle(cef_window_handle_t h);
    void setPopupsExternal(bool handle_external) { handlePopupExternal = handle_external; }

  private:
    BrowserList browsers;
    short width = 0;
    short height = 0;
    uint8_t *texture = NULL;
    CefRect popup;
    uint8_t *popupTextureCache = NULL;
    cef_window_handle_t windowHandle = 0;
    float cefScale = 1.0;
    bool handlePopupExternal = false;

  private:
    void reactivateBrowser();


  IMPLEMENT_REFCOUNTING(Client);
}; // class Client

} // namespace cef
