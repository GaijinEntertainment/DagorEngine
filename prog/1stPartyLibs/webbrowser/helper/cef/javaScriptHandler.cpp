#include "javaScriptHandler.h"


namespace cef
{

void JavaScriptHandler::OnBrowserCreated(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefDictionaryValue> extra_info)
{
  CefDictionaryValue::KeyList names;
  if (extra_info && extra_info->GetKeys(names))
    for (const auto & name : names)
      this->methods.push_back({name, extra_info->GetInt(name)});
}


void JavaScriptHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> /* frame */,
                                         CefRefPtr<CefV8Context> context)
{
  auto window = context->GetGlobal();

  if (methods.size())
  {
    CefRefPtr<CefV8Handler> caller = new JavaScriptCallback(browser, this->methods);

    for (const auto& m : methods)
      window->SetValue(m.name, CefV8Value::CreateFunction(m.name, caller),
        V8_PROPERTY_ATTRIBUTE_NONE);
  }
}


bool JavaScriptCallback::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> /* object */,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& /* retval */,
                                 CefString& exception)
{
  auto signature = std::find_if(this->methods.begin(), this->methods.end(),
                                [&name](const auto& v) { return v.name == name; });
  if (signature == this->methods.end())
    return false;

  if (arguments.size() == signature->argc)
  {
    auto msg = CefProcessMessage::Create(L"call");
    auto vals = msg->GetArgumentList();
    vals->SetString(0, name);
    for (int i = 0; i < arguments.size(); ++i)
      vals->SetString(i+1, arguments[i]->GetStringValue());

    this->browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
  }
  else
  {
    char errBuf[64] = { 0 };
    ::sprintf(errBuf, "%d parameters expected, got %llu", signature->argc, arguments.size());
    exception = errBuf;
  }

  return true;
}

} // namespace cef
