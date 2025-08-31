// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "EASTL/string.h"

namespace shc
{
class CompilationContext;
}

namespace assembly
{

eastl::string build_common_hardware_defines_hlsl(const shc::CompilationContext &ctx);
eastl::string build_hardware_defines_hlsl(const char *profile, bool use_halfs, const shc::CompilationContext &ctx);

#if _CROSS_TARGET_C2

#endif

} // namespace assembly