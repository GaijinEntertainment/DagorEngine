#pragma once
#include <cef_app.h>

#include "javaScriptHandler.h"
#include "../enableEndpoint.h"


namespace cef
{

class App : public CefApp,
            public CefBrowserProcessHandler,
            public ipc::EnableEndpoint
{
  public:
    App();

  public: /** CefApp interface **/
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return jsHandler; }

    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                       CefRefPtr<CefCommandLine> cl) override;

  public: /** CefBrowserProcessHandler interface **/
    virtual void OnContextInitialized() override;

  private:
    std::string name;
    CefRefPtr<JavaScriptHandler> jsHandler;

  IMPLEMENT_REFCOUNTING(App);
}; // class App

} // namespace cef

