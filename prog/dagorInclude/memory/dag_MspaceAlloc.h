//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

class MspaceAlloc final : public IMemAlloc
{
public:
  KRNLIMP static MspaceAlloc *create(size_t initial_size);
  static inline MspaceAlloc *create(void *, size_t cap, bool) { return create(cap); }

  // IMemAlloc interface
  KRNLIMP virtual void destroy();
  KRNLIMP bool isEmpty() override { return false; }
  KRNLIMP size_t getSize(void *p) override;
  KRNLIMP void *alloc(size_t sz) override;
  KRNLIMP void *tryAlloc(size_t sz) override;
  KRNLIMP void *allocAligned(size_t sz, size_t alignment) override;
  KRNLIMP bool resizeInplace(void *p, size_t sz) override;
  KRNLIMP void *realloc(void *p, size_t sz) override;
  KRNLIMP void free(void *p) override;
  KRNLIMP void freeAligned(void *p) override;

private:
  void *heap = nullptr;
  MspaceAlloc(void *h) : heap(h) {}
};

#include <supp/dag_undef_KRNLIMP.h>
