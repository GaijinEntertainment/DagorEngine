//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

struct PhysObjectUserData
{
  int physObjectType = 0;
};

#define MAKE_PHYS_OBJ_TYPE(ClassName, fourc)                                              \
  ClassName()                                                                             \
  {                                                                                       \
    physObjectType = fourc;                                                               \
  }                                                                                       \
  static ClassName *cast(PhysObjectUserData *ptr)                                         \
  {                                                                                       \
    return ptr && ptr->physObjectType == fourc ? static_cast<ClassName *>(ptr) : nullptr; \
  }
