// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>

#if _TARGET_C4

#endif

inline eastl::string get_platform_uid()
{
#if _TARGET_C4

#else
  return eastl::string{};
#endif
}
