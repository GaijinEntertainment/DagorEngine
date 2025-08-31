// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cppStcodeAssembly.h"
#include <shaders/shExprTypes.h>
#include <shaders/shFunc.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace stcode
{

inline ShaderVarType shexpr_value_to_shadervar_type(shexpr::ValueType shvt, bool is_integer)
{
  switch (shvt)
  {
    case shexpr::VT_REAL: return is_integer ? SHVT_INT : SHVT_REAL;
    case shexpr::VT_COLOR4: return is_integer ? SHVT_INT4 : SHVT_COLOR4;
    case shexpr::VT_TEXTURE: return SHVT_TEXTURE;
    case shexpr::VT_BUFFER: return SHVT_BUFFER;
    default: G_ASSERT_RETURN(0, SHVT_INT);
  }
}

inline const char *shadervar_type_to_stcode_type(ShaderVarType shvt)
{
  switch (shvt)
  {
    case SHVT_INT: return "int32_t";
    case SHVT_REAL: return "float";
    case SHVT_INT4: return "int4";
    case SHVT_COLOR4: return "float4";
    case SHVT_FLOAT4X4: return "float4x4";
    case SHVT_BUFFER:
    case SHVT_TEXTURE:
    case SHVT_SAMPLER:
    case SHVT_TLAS:
      return "void"; // void * to d3d::SamplerHandle or RaytraceTopAccelerationStructure * as global, invalid as local. For tex/buf
                     // points to a shader_internal::Tex/Buf structure.

    default: G_ASSERT_RETURN(0, nullptr);
  }
}

inline const char *value_type_to_stcode_type(shexpr::ValueType vt, bool is_integer)
{
  return shadervar_type_to_stcode_type(shexpr_value_to_shadervar_type(vt, is_integer));
}

inline int shader_stage_to_idx(ShaderStage stage)
{
  switch (stage)
  {
    case STAGE_CS:
    case STAGE_PS: return 0;
    case STAGE_VS: return 1;
    default: G_ASSERT_RETURN(0, -1);
  }
}

inline eastl::string extract_shader_name_from_path(const char *fn)
{
  const eastl::string_view path(fn);
  auto revIt = eastl::find(path.crbegin(), path.crend(), '/');

  const char *begin = revIt.base();
  const char *end = strstr(begin, ".dshl");
  if (!end)
    end = path.cend();

  return eastl::string(begin, end);
}

} // namespace stcode
