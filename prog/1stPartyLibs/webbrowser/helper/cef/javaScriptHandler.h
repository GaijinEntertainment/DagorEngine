// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include <vector>

#include <cef_v8.h>
#include <cef_render_process_handler.h>

namespace cef
{
struct WindowMethodSignature
{
  CefString name;
  int argc;
};


class JavaScriptHandler : public CefRenderProcessHandler
{
  public: /** CefRenderProcessHandler interface **/
    void OnBrowserCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefDictionaryValue> extra_info) override;

    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override;

  private:
    std::vector<WindowMethodSignature> methods;

  IMPLEMENT_REFCOUNTING(JavaScriptHandler);
}; // class JavaScriptHandler


class JavaScriptCallback : public CefV8Handler
{
  public:
    JavaScriptCallback(const CefRefPtr<CefBrowser>& b, const std::vector<WindowMethodSignature>& m):
      browser(b), methods(m) {}

  public: /** CefV8Handler interface **/
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override;


  private:
    CefRefPtr<CefBrowser> browser;
    const std::vector<WindowMethodSignature>& methods;

  IMPLEMENT_REFCOUNTING(JavaScriptCallback);
};

} // namespace cef
