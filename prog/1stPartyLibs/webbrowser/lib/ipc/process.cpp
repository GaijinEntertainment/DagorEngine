// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "process.h"
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <Shlwapi.h>

#include "../unicodeString.h"


namespace ipc
{

using UnicodeString = webbrowser::UnicodeString;


bool Process::spawn(unsigned short port,
                    const char *product,
                    const char *res_path,
                    const char *cache_path,
                    const char *log_path,
                    unsigned bg_color,
                    bool external_popups,
                    bool purge_cookies_on_startup,
                    void* parent_wnd)
{
  assert(!this->isAlive());
  assert(this->executable && *this->executable);
  assert(port);

  HWND hndl = (HWND)parent_wnd;
  bool offScreen = !parent_wnd;

  if (!hndl || !::IsWindow(hndl))
    hndl = ::GetActiveWindow();

  UnicodeString args("--lwb-port=%d --lwb-hndl=0x%p --lwb-name=\"%s\" --lwb-bg=0x%08x "
#if DAGOR_DBGLEVEL > 0
                     "--log-severity=verbose "
#else
                     "--log-severity=disable "
#endif
                     "--resources-dir-path=\"%s\" "
                     "--locales-dir-path=\"%s\\Locales\" "
                     "--cache-path=\"%s\" "
                     "--log-file=\"%s\" "
                     "%s%s%s",
                     port, hndl, product, bg_color,
                     res_path,
                     res_path,
                     cache_path,
                     log_path,
                     purge_cookies_on_startup ? "" : "--keep-cookies=1 ",
                     offScreen ? "" : "--lwb-gdi=1 ",
                     external_popups ? "--lwb-extpop=1 " : "");

  UnicodeString cmd(L"%ls %ls", executable, args.wide());
  if (!cmd)
  {
    ERR("%s: could not format child command");
    return false;
  }
  DBG("%s: starting '%s'", __FUNCTION__, cmd.utf8());

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  if (::CreateProcessW(NULL, const_cast<wchar_t*>(cmd.wide()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
  {
    DBG("spawned %S: %s", this->executable, pi.hProcess == INVALID_HANDLE_VALUE ? "invalid" : "valid");
    this->pid = pi.hProcess;
    this->channel = port;
    int inited = ::WaitForInputIdle(pi.hProcess, INFINITE /*100*/);
    DBG("waited for child %d", inited);
    DWORD exited = 0;
    if (::GetExitCodeProcess(pi.hProcess, &exited) && exited != STILL_ACTIVE)
    {
      DBG("child exit code: 0x%x", exited);
      this->pid = INVALID_HANDLE_VALUE;
    }
  }
  else
    ERR("could not spawn: %d", GetLastError());

  return this->isAlive();
}


} // namespace ipc
