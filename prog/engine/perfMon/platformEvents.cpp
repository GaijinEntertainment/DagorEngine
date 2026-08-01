// Copyright (C) Gaijin Games KFT.  All rights reserved.

// #include <perfMon/dag_pix.h>

#if _TARGET_PC_WIN || _TARGET_XBOX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#if USE_PIX
#include <pix3.h>
#endif
#if USE_NVTX
#include <nvtx3/nvtx3.hpp>
#endif


void BEGIN_CPU_EVENT([[maybe_unused]] const char *name)
{
#if USE_PIX
  PIXBeginEvent(0, name);
#endif

#if USE_NVTX
  nvtxRangePushA(name);
#endif
}

void END_CPU_EVENT()
{
#if USE_PIX
  PIXEndEvent();
#endif
#if USE_NVTX
  nvtxRangePop();
#endif
}
