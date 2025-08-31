//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_statDrv.h>

namespace bind_dascript
{
void instrument_function(das::Context &ctx, das::SimFunction *FNPTR, bool isInstrumenting, uint64_t userData);
void daProfiler_resolve_path(const char *fname, const das::TBlock<void, const das::TTemporary<const char *>> &blk,
  das::Context *context, das::LineInfoArg *at);

inline void profile_block(const char *name, const das::TBlock<void> &block, das::Context *context, das::LineInfoArg *at)
{
  TIME_PROFILE_NAME(das, name);
  context->invoke(block, nullptr, nullptr, at);
}

inline void profile_gpu_block(const char *name, const das::TBlock<void> &block, das::Context *context, das::LineInfoArg *at)
{
  TIME_D3D_PROFILE_NAME(das, name);
  context->invoke(block, nullptr, nullptr, at);
}

} // namespace bind_dascript