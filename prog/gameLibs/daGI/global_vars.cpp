#include <shaders/dag_shaders.h>
#include "global_vars.h"
namespace dagi
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

}; // namespace dagi
