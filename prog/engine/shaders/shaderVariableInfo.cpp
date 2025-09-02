// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderVariableInfo.h>


ShaderVariableInfo &ShaderVariableInfo::operator=(int v_id)
{
  data = nullptr;
  var_id = v_id;
  resolve();
  return *this;
}
