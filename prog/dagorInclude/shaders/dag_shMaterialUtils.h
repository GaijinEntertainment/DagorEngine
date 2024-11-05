//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaderMatData.h>
#include <shaders/dag_shaderCommon.h>
#include <dag/dag_vector.h>
#include <shaders/shader_layout.h>

namespace shaderbindump
{
using ShaderClass = bindump::Mapper<shader_layout::ShaderClass>;
}

struct ShaderVarDesc
{
  SimpleString name;
  ShaderVarType type;
  ShaderMatData::VarValue value;
};

dag::Vector<ShaderVarDesc> get_shclass_script_vars_list(const char *shclass_name);
unsigned get_shclass_used_tex_mask(const shaderbindump::ShaderClass *shclass);
unsigned get_shclass_used_tex_mask(const char *shclass_name);
