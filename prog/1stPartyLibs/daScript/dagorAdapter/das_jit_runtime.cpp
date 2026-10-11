// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>

static bool jit_runtime_enabled = false;

void das_pull_jit_runtime()
{
#if defined(DAS_JIT_RUNTIME)
  jit_runtime_enabled = true;
  NEED_MODULE(Module_DASBIND)
  NEED_MODULE(Module_Jit)
#endif
}

bool das_jit_runtime_enabled() { return jit_runtime_enabled; }
