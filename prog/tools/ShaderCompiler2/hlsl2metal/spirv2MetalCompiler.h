// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <spirv2Metal/spirv_msl.hpp>
#include "metalShaderType.h"
#include "../compileResult.h"

#include <spirv/compiler.h>

#include <util/dag_string.h>

class CompilerMSLlocal : public spirv_cross::CompilerMSL
{
public:
  CompilerMSLlocal(std::vector<uint32_t> spirv, bool use_ios_token);

  bool validate(std::stringstream &errors);

  CompileResult convertToMSL(CompileResult &compile_result, eastl::vector<spirv::ReflectionInfo> &resourceMap, const char *source,
    ShaderType shaderType, const char *shader_name, const char *entry, uint64_t shader_variant_hash, bool use_binary_msl);

  bool compileBinaryMSL(const String &mtl_src, std::vector<uint8_t> &mtl_bin, bool ios, bool raytracing, std::string_view &result);

private:
  void setOptions(bool has_raytracing);
  uint32_t getTypeSize(const spirv_cross::SPIRType &type, uint32_t &align) const;

  bool use_ios_token = false;
};
