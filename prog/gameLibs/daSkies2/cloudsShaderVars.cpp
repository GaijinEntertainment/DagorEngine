// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsShaderVars.h"
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>

#define VAR(a, opt) int a##VarId = -1;
CLOUDS_VARS_LIST
#undef VAR

bool clouds_checkerboard_compiled()
{
  // callable before any clouds instance initializes the var ids (static quality query)
  if (clouds_checkerboardVarId < 0)
    clouds_checkerboardVarId = ::get_shader_variable_id("clouds_checkerboard", true);
  return VariableMap::isVariablePresent(clouds_checkerboardVarId) && !ShaderGlobal::is_var_assumed(clouds_checkerboardVarId);
}
