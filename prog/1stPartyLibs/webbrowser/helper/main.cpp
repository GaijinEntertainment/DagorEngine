// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cef/app.h"
#include <cef_version.h>
#include "endpoint.h"
#include "version.inc.c"


int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE /*hPrevInstance*/,
                      PWSTR /*lpCmdLine*/,
                      int /*nCmdShow*/)
{
  CefEnableHighDPISupport();

  CefMainArgs args(hInstance);
  CefRefPtr<cef::App> app(new cef::App);

  int exit_code = CefExecuteProcess(args, app.get(), NULL);
  if (exit_code >= 0)
    return exit_code; // subprocess terminated

  // main helper process
  ipc::Endpoint ep(app.get());
  if (!ep.init(args))
    return 1;

  const char chromeVer[] = "Chrome/" MAKE_STRING(CHROME_VERSION_MAJOR)
                                 "." MAKE_STRING(CHROME_VERSION_MINOR)
                                 "." MAKE_STRING(CHROME_VERSION_BUILD)
                                 "." MAKE_STRING(CHROME_VERSION_PATCH);
  ep.DBG("Running %s built on %s %s", chromeVer, WBHP_BUILD_DATE, WBHP_BUILD_TIME);
  ep.run();
  ep.DBG("DONE!");

  //This is brutal, but otherwise cefprocess.exe fail in libcef.dll under Windows 7
  //inside one of CEF atexit() callbacks outside of our SEH handler
  OSVERSIONINFOEXW vi = { sizeof(vi) };
  ::GetVersionExW((OSVERSIONINFOW*)&vi);

  if (vi.wProductType == VER_NT_WORKSTATION && vi.dwMajorVersion == 6 && vi.dwMinorVersion == 1)
    ::ExitProcess(0); //No atexit() handlers under Windows 7. Sorry, CEF

  return 0;
}
