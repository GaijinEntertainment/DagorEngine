// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "unsafe_call.h"

#if _TARGET_PC_WIN
#include <windows.h>
#endif

using namespace drv3d_vulkan;

bool drv3d_vulkan::execute_unsafe(UnsafeFunc func, void *user_data) noexcept
{
#if _TARGET_PC_WIN
  __try
  {
    func(user_data);
    return true;
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return false;
  }
#else
  func(user_data);
  return true;
#endif
}
