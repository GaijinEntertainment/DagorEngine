//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>
#include <util/dag_stdint.h>

class MspaceAlloc : public IMemAlloc
{
public:
  KRNLIMP static MspaceAlloc *create(void *base, size_t capacity, bool fixed);
  KRNLIMP void release(bool reinit);
  KRNLIMP void getStatistics(size_t &system_allocated, size_t &mem_allocated);

  // IMemAlloc interface
  virtual void destroy();
  virtual bool isEmpty();
  virtual size_t getSize(void *p);
  virtual void *alloc(size_t sz);
  virtual void *tryAlloc(size_t sz);
  virtual void *allocAligned(size_t sz, size_t alignment);
  virtual bool resizeInplace(void *p, size_t sz);
  virtual void *realloc(void *p, size_t sz);
  virtual void free(void *p);
  void freeAligned(void *p) override;

private:
  void *mBase, *mBaseEnd, *rmSpace;
  bool mFixed, mMemOwner;

  MspaceAlloc(void *base, size_t capacity, bool fixed);
  ~MspaceAlloc();
};

#include <supp/dag_undef_KRNLIMP.h>
