// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../webbrowser.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


namespace webbrowser
{

class Log
{
  public:
    typedef enum { LL_DBG, LL_WRN, LL_INFO, LL_ERR } Level;

  public:
    Log(ILogger& l) : log(l) { /* VOID */ }
    virtual ~Log() { this->log.flush(); }

  public:
    void write(Level l, const char *fmt, va_list arg)
    {
      va_list cp;
      va_copy(cp, arg);
      int needed = vsnprintf(NULL, 0, fmt, cp);
      va_end(cp);

      if (needed < 0)
      {
        this->log.err("failed to format log message");
        this->log.err(fmt);
        return;
      }

      needed++; // Zero-terminator;
      char *buf = new char[needed];
      memset(buf, 0, needed);
      vsnprintf(buf, needed, fmt, arg);
      this->write(l, buf);
      delete [] buf;
    }

    void write(Level l, const char *msg)
    {
      switch (l)
      {
        case LL_DBG: this->log.dbg(msg); break;
        case LL_WRN: this->log.warn(msg); break;
        case LL_INFO: this->log.info(msg); break;
        case LL_ERR: this->log.err(msg); break;
      }
    }

  private:
    ILogger& log;
}; // class Log


struct EnableLogging
{
  EnableLogging(ILogger& l) : log(l) { /* VOID */ }
  virtual ~EnableLogging() { /* VOID */ }

#define ENABLE_LEVEL(LEVEL)                     \
  virtual void LEVEL(const char *fmt, ...)      \
  {                                             \
    va_list ap;                                 \
    va_start(ap, fmt);                          \
    this->log.write(Log::LL_##LEVEL, fmt, ap);  \
    va_end(ap);                                 \
  }
  ENABLE_LEVEL(DBG)
  ENABLE_LEVEL(WRN)
  ENABLE_LEVEL(INFO)
  ENABLE_LEVEL(ERR)
#undef ENABLE_LEVEL

  private:
    Log log;
}; // struct EnableLogging

} // namespace webbrowser
