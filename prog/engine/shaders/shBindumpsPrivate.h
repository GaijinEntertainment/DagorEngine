// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shBindumps.h>
#include <hash/BLAKE3/blake3.h>
#include <generic/dag_functionRef.h>
#include "shadersBinaryData.h"

ScriptedShadersBinDump const &get_shaders_dump(ShaderBindumpHandle hnd);
ScriptedShadersBinDumpOwner const &get_shaders_dump_owner(ShaderBindumpHandle hnd);

inline ScriptedShadersBinDumpOwner &get_shaders_dump_owner_mut(ShaderBindumpHandle hnd)
{
  return const_cast<ScriptedShadersBinDumpOwner &>(get_shaders_dump_owner(hnd));
}

void iterate_all_shader_dumps(dag::FunctionRef<void(ScriptedShadersBinDumpOwner &)> cb, bool do_secondary_exp = true);
void iterate_all_additional_shader_dumps(dag::FunctionRef<void(ScriptedShadersBinDumpOwner &)> cb);

// Reinitializes it inplace on the same loaded data. To be called with iterate_all_additional_shader_dumps on main dump reload. should
// be called while already holding d3d::GpuAutoLock
bool reinit_shaders_bindump(ScriptedShadersBinDumpOwner &dest, ScriptedShadersGlobalData const &global_link_data);


static constexpr size_t BLAKE3_DIGEST_LENGTH = 32;

inline auto make_hash_log_string(const uint8_t (&hash)[BLAKE3_DIGEST_LENGTH])
{
  eastl::string res{};
  for (uint8_t byte : hash)
    res.append_sprintf("%02x", byte);
  return res;
}

struct Blake3Csum
{
  uint8_t data[BLAKE3_DIGEST_LENGTH];
};

inline Blake3Csum blake3_csum(const unsigned char *input, size_t len)
{
  Blake3Csum res{};
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, input, len);
  blake3_hasher_finalize(&hasher, res.data, sizeof(res.data));
  return res;
}
