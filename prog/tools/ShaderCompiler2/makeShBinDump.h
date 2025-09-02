// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cppStcodeAssembly.h"

namespace shc
{
class CompilationContext;
}

struct BindumpPackingFlagsBits
{
  static constexpr uint32_t NONE = 0;
  static constexpr uint32_t SHADER_GROUPS = 1;
  static constexpr uint32_t WHOLE_BINARY = 1 << 1;
};
using BindumpPackingFlags = uint32_t;

bool make_scripted_shaders_dump(const char *dump_name, const char *cache_filename, bool strip_shaders_and_stcode,
  BindumpPackingFlags packing_flags, const shc::CompilationContext &ctx);
