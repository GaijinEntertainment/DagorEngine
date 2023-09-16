#pragma once
#include <string>
#include <vector>

#include <cef_v8.h>
#include <cef_render_process_handler.h>

namespace cef
{

class JavaScriptHandler : public CefV8Handler,
                          public CefRenderProcessHandler
{
  public: /** CefV8Handler interface **/
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override;

  public: /** CefRenderProcessHandler interface **/
    void OnBrowserCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefDictionaryValue> extra_info) override;

    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override;

  private:
    struct WindowMethodSignature
    {
      CefString name;
      int argc;
    };

    std::vector<WindowMethodSignature> methods;
    CefRefPtr<CefBrowser> browser;

  IMPLEMENT_REFCOUNTING(JavaScriptHandler);
}; // class JavaScriptHandler

} // namespace cef
