// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <windows.h>

#include "../log.h"

namespace ipc
{
typedef HANDLE pid_t;

class Process : public webbrowser::EnableLogging
{
  public:
    Process(const wchar_t* exe, webbrowser::ILogger& l)
      : webbrowser::EnableLogging(l),
        pid((pid_t)-1),
        executable(exe),
        channel(0)
    {
      /* VOID */
    }

    virtual ~Process() { /* VOID */ }

  public:
    bool spawn(unsigned short port,
               const char *user_agent,
               const char *res_path,
               const char *cache_path,
               const char *log_path,
               unsigned bg_color,
               bool external_popups,
               bool purge_cookies_on_startup,
               void* parent_wnd = NULL);
    bool isAlive() { return this->pid != INVALID_HANDLE_VALUE; }

  private:
    pid_t pid;

    const wchar_t* executable;
    unsigned short channel;
}; //class Process

} // namespace ipc
