//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>
#include <util/dag_globDef.h>
#include <string.h>

class DebugTextStream final : public IGenSave
{
  char buffer[2 << 10];
  int used = 0;
  const char *prefix;

  void print()
  {
    debug("%s\n%s", prefix, buffer);
    prefix = "";
  }

public:
  DebugTextStream(const char *p = "") : prefix(p) {}
  DebugTextStream(const DebugTextStream &) = delete;
  DebugTextStream &operator=(const DebugTextStream &) = delete;
  ~DebugTextStream()
  {
    buffer[used] = 0;
    if (used)
      print();
  }

  void write(const void *void_ptr, int size) override
  {
    const char *ptr = reinterpret_cast<const char *>(void_ptr);
    while (true)
    {
      int s = min<int>(sizeof(buffer) - 1 - used, size);
      ::memcpy(buffer + used, ptr, s);
      used += s;
      if (s == size)
        return;

      char *endl = buffer + used;
      for (char *p = buffer + used - 1; p >= buffer; --p)
        if (*p == '\n')
        {
          endl = p;
          break;
        }

      *endl = 0;
      print();

      const char *tail = endl + 1;
      for (const char *p = tail; p < buffer + used; ++p)
        buffer[p - tail] = *p;
      used = max<int>(buffer + used - tail, 0);

      ptr += s;
      size -= s;
    }
  }

  int tell(void) override { return 0; }
  void seekto(int) override {}
  void seektoend(int) override {}
  const char *getTargetName() override { return "DebugTextStream"; }
  void beginBlock() override {}
  void endBlock(unsigned) override {}
  int getBlockLevel(void) override { return 0; }
  void flush(void) override {}
};
