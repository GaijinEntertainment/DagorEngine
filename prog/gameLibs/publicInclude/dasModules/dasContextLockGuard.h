//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>

namespace bind_dascript
{

class ContextLockGuard
{
  das::Context *ctx;

public:
  ContextLockGuard(das::Context &ctx_) : ctx(&ctx_) { ctx_.lock(); }
  ContextLockGuard(const ContextLockGuard &) { G_FAST_ASSERT(0); } //-V730 Note: eastl instantiates copy-ctor even if only move is used
  ContextLockGuard(ContextLockGuard &&other) : ctx(other.ctx) { other.ctx = nullptr; }
  ContextLockGuard &operator=(const ContextLockGuard &) = delete;
  ~ContextLockGuard() { ctx ? (void)ctx->unlock() : (void)0; }
};

}; // namespace bind_dascript
