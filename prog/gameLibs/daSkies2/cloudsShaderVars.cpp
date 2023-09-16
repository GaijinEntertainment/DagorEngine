#include "cloudsShaderVars.h"

#define VAR(a, opt) int a##VarId = -1;
CLOUDS_VARS_LIST
#undef VAR
