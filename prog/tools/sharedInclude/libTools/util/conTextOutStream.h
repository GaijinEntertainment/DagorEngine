//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <libtools/util/iLogWriter.h>
#include <util/dag_string.h>


struct ConTextOutStream : public IGenSave
{
  ILogWriter &log;
  String ln;

  ConTextOutStream(ILogWriter &l) : log(l) {}
  ~ConTextOutStream() { flush(); }

  virtual void write(const void *_ptr, int size)
  {
    const char *ptr = (const char *)_ptr;

    if ((size == 1 && memcmp(ptr, "\n", size) == 0) || (size == 2 && memcmp(ptr, "\r\n", size) == 0))
    {
      log.addMessage(log.NOTE, ln);
      ln.clear();
      return;
    }
    while (const char *p = (const char *)memchr(ptr, '\n', size))
    {
      ln.append((const char *)ptr, (p > ptr && p[-1] == '\r') ? p - 1 - ptr : p - ptr);
      log.addMessage(log.NOTE, ln);
      ln.clear();
      size -= p - ptr + 1;
      ptr = p + 1;
    }
    if (size)
      ln.append((const char *)ptr, size);
  }

  virtual int tell() { return 0; }
  virtual void seekto(int) {}
  virtual void seektoend(int /*ofs*/ = 0) {}

  virtual const char *getTargetName() { return "consoleTextOut://"; }

  virtual void beginBlock() {}
  virtual void endBlock(unsigned /*block_flg*/ = 0) {}
  virtual int getBlockLevel() { return 0; }

  /// Flush stream
  virtual void flush()
  {
    if (ln.length())
      log.addMessage(log.NOTE, ln);
    ln.clear();
  }
};
