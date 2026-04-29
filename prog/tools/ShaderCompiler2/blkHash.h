// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/shader_layout.h>
#include <debug/dag_assert.h>

using BlkHashBytes = eastl::array<uint8_t, 20>;

G_STATIC_ASSERT(sizeof(BlkHashBytes) == sizeof(shader_layout::ScriptedShadersBinDumpCompressedHeader{}.buildBlkHash));

inline eastl::string blk_hash_string(const auto &bytes) // auto so that it works with array too
{
  eastl::string res;
  for (uint8_t byte : bytes)
    res.append_sprintf("%02x", byte);
  return res;
}
