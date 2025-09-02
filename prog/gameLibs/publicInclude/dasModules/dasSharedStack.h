//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>

namespace bind_dascript
{

struct FramememStackAllocator : public das::StackAllocator
{
  FramememStackAllocator(uint32_t size);
  ~FramememStackAllocator();
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
