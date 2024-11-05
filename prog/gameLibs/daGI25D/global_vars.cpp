// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "global_vars.h"
#include <debug/dag_log.h>
namespace dagi25d
{
#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR

void init_global_vars()
{
#define VAR(a)         \
  if (!bool(a##VarId)) \
    logerr("variable %s is not optional, but not present", a##VarId.getName());
  GLOBAL_VARS_LIST
#undef VAR
}

}; // namespace dagi25d
