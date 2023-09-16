#include "app.h"

#include <wrapper/cef_helpers.h>
#include <cef_version.h>
#include <cef_command_line.h>

#include "client.h"
#include "../endpoint.h"

#include <windows.h> // for ::GetCommandLineW()

#include <assert.h>


namespace cef
{

App::App()
{
  CefRefPtr<CefCommandLine> cl = CefCommandLine::CreateCommandLine();
  cl->InitFromString(::GetCommandLineW());
  this->name = cl->GetSwitchValue("lwb-name");
  this->jsHandler = new JavaScriptHandler;
}


void App::OnBeforeCommandLineProcessing(const CefString& process_type,
                                        CefRefPtr<CefCommandLine> cl)
{
  cl->AppendSwitch("disable-surfaces");
  cl->AppendSwitch("enable-system-flash");
  cl->AppendSwitch("always-authorize-plugins");
  cl->AppendSwitch("enable-npapi");

  if (!this->name.empty())
  {
    const char prefix[] = "Chrome/" MAKE_STRING(CHROME_VERSION_MAJOR)
                                "." MAKE_STRING(CHROME_VERSION_MINOR)
                                "." MAKE_STRING(CHROME_VERSION_BUILD)
                                "." MAKE_STRING(CHROME_VERSION_PATCH) " ";
    cl->AppendSwitchWithValue("user-agent-product", prefix + this->name);
  }
}


void App::OnContextInitialized()
{
  CEF_REQUIRE_UI_THREAD();

  this->endpoint->onInitialized();
}

} // namespace cef
