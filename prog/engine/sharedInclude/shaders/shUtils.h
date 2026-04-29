// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  some shader util functions
************************************************************************/

#include "shader_layout.h"
#include <drv/3d/dag_shaderModelVersion.h>

namespace shaderbindump
{
using VarList = bindump::Mapper<shader_layout::VarList>;
}
using ScriptedShadersBinDump = bindump::Mapper<shader_layout::ScriptedShadersBinDump>;

class ShaderVarsState;

namespace ShUtils
{
const char *fsh_version(d3d::shadermodel::Version vertex_shader_model);
const char *channel_type_name(int t);
const char *channel_usage_name(int u);
const char *shader_var_type_name(int t);

const char *fsh_tokname(int t);
const char *vpr_tokname(int t);
const char *shcod_tokname(int t);

const char *fshVerToString(int fsh);

// dump code table
void shcod_dump(dag::ConstSpan<int> cod, const shaderbindump::VarList *globals = nullptr,
  const ShaderVarsState *globals_state = nullptr, const shaderbindump::VarList *locals = nullptr,
  const ScriptedShadersBinDump *dump = nullptr, dag::ConstSpan<uint32_t> stVarMap = {}, bool embrace_dump = true);
} // namespace ShUtils
