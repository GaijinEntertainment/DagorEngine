#pragma once

#include <debug/dag_log.h>
#include <webbrowser/webbrowser.h>

namespace webbrowser
{

struct Logger : public webbrowser::ILogger
{
  virtual void err(const char *msg) { logerr("[BRWS] %s", msg); }
  virtual void warn(const char *msg) { logwarn("[BRWS] %s", msg); }
  virtual void info(const char *msg) { logdbg("[BRWS] %s", msg); }
  virtual void dbg(const char *msg) { logdbg("[BRWS] %s", msg); }
  virtual void flush() {} // VOID
};                        // class ILogger

} // namespace webbrowser
