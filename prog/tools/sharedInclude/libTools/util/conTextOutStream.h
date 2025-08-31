//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <libTools/util/iLogWriter.h>
#include <util/dag_string.h>


struct ConTextOutStream : public IGenSave
{
  ILogWriter &log;
  String ln;

  ConTextOutStream(ILogWriter &l) : log(l) {}
  ~ConTextOutStream() override { flush(); }

  void write(const void *_ptr, int size) override
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

  int tell() override { return 0; }
  void seekto(int) override {}
  void seektoend(int /*ofs*/ = 0) override {}

  const char *getTargetName() override { return "consoleTextOut://"; }

  void beginBlock() override {}
  void endBlock(unsigned /*block_flg*/ = 0) override {}
  int getBlockLevel() override { return 0; }

  /// Flush stream
  void flush() override
  {
    if (ln.length())
      log.addMessage(log.NOTE, ln);
    ln.clear();
  }
};
