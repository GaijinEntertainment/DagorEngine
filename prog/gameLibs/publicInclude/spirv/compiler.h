//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "compiled_meta_data.h"

#include <glslang/Public/ShaderLang.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <vector>

namespace spirv
{

// reflection info for resource remapping, used for metal
struct ReflectionInfo
{
  enum class Type
  {
    ConstantBuffer,
    Buffer,
    StructuredBuffer,
    TlasBuffer,
    Texture,
    Sampler
  };

  eastl::string name;
  Type type = Type::ConstantBuffer;
  int binding = -1;
  int set = -1;
  bool uav = false;
};

struct CompileToSpirVResult
{
  // hash and version is not set, only metadata
  spirv::ShaderHeader header;
  spirv::ComputeShaderInfo computeShaderInfo;
  // using std vector here to void additional copy (glslang -> spirv only accepts std::vector as target)
  // if this is empty, compilation failed
  std::vector<unsigned int> byteCode;
  // contains info logs from compiler, linker and io remapper
  eastl::vector<eastl::string> infoLog;
  // reflection info to remap resoure bindings
  eastl::vector<ReflectionInfo> reflection;
};
enum class CompileFlags : uint32_t
{
  NONE = 0,
  // embed glsl into spir-v as debug info
  DEBUG_BUILD = 1 << 0,
  // if set varyings are used as is and no location lookup is done
  // used for in place shaders for debug
  VARYING_PASS_THROUGH = 1 << 1,
  ENABLE_BINDLESS_SUPPORT = 1 << 2,
  // outputs reflection info
  OUTPUT_REFLECTION = 1 << 3,
  // forces descriptor set indices based on resource type
  OVERWRITE_DESCRIPTOR_SETS = 1 << 4,
  // force relaxed precision be converted to float16
  ENABLE_HALFS = 1 << 5
};
inline CompileFlags &operator|=(CompileFlags &self, CompileFlags o)
{
  reinterpret_cast<uint32_t &>(self) |= static_cast<uint32_t>(o);
  return self;
}
inline CompileFlags &operator^=(CompileFlags &self, CompileFlags o)
{
  reinterpret_cast<uint32_t &>(self) ^= static_cast<uint32_t>(o);
  return self;
}
inline CompileFlags &operator&=(CompileFlags &self, CompileFlags o)
{
  reinterpret_cast<uint32_t &>(self) &= static_cast<uint32_t>(o);
  return self;
}
inline CompileFlags operator|(CompileFlags l, CompileFlags r)
{
  return static_cast<CompileFlags>(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}
inline CompileFlags operator^(CompileFlags l, CompileFlags r)
{
  return static_cast<CompileFlags>(static_cast<uint32_t>(l) ^ static_cast<uint32_t>(r));
}
inline CompileFlags operator&(CompileFlags l, CompileFlags r)
{
  return static_cast<CompileFlags>(static_cast<uint32_t>(l) & static_cast<uint32_t>(r));
}
CompileToSpirVResult compileGLSL(dag::ConstSpan<const char *> sources, EShLanguage target, CompileFlags flags);
CompileToSpirVResult compileHLSL(dag::ConstSpan<const char *> sources, const char *entry, EShLanguage target, CompileFlags flags);
CompileToSpirVResult compileHLSL_DXC(dag::ConstSpan<char> source, const char *entry, const char *profile, CompileFlags flags,
  const eastl::vector<eastl::string_view> &disabledSpirvOptims);
} // namespace spirv
