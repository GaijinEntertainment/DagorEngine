// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsShaderVars.h"

#define VAR(a, opt) int a##VarId = -1;
CLOUDS_VARS_LIST
#undef VAR
