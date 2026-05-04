//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <debug/dag_assert.h>

namespace bind_dascript
{

struct FramememStackAllocator : public das::StackAllocator
{
  FramememStackAllocator(uint32_t size);
  ~FramememStackAllocator();
};

struct SharedFramememStack : public FramememStackAllocator
{
  SharedFramememStack() : FramememStackAllocator(16 << 10)
  {
    G_FAST_ASSERT(!*das::SharedStackGuard::lastContextStack);
    *das::SharedStackGuard::lastContextStack = this;
  }
  SharedFramememStack(const SharedFramememStack &) = delete;
  ~SharedFramememStack()
  {
    G_FAST_ASSERT(*das::SharedStackGuard::lastContextStack == this);
    *das::SharedStackGuard::lastContextStack = nullptr;
  }
};

} // namespace bind_dascript

namespace das
{
class SharedFramememStackGuard : protected bind_dascript::FramememStackAllocator, public SharedStackGuard
{
public:
  SharedFramememStackGuard(Context &ctx) :
    bind_dascript::FramememStackAllocator(*SharedStackGuard::lastContextStack ? 0 : (16 << 10)), // Note: no alloc if already created
    SharedStackGuard(ctx, *this)
  {}
};
} // namespace das
