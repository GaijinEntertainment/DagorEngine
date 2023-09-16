#pragma once

#include <ioSys/dag_baseIo.h>
class IHashCalc : public IBaseSave
{
public:
  uint64_t hash = 14695981039346656037LU;
  void write(const void *ptr, int size)
  {
    const uint64_t prime = 1099511628211;
    for (int i = 0; i < size; ++i)
      hash = (hash * prime) ^ (((const uint8_t *)ptr)[i]);
  }
  int tell(void) { return 0; }
  void seekto(int) {}
  void seektoend(int) {}
  virtual const char *getTargetName() { return "(hash)"; }
  virtual void flush()
  { /*noop*/
  }
};
