//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct PhysObjectUserData
{
  int physObjectType = 0;
};

#define MAKE_PHYS_OBJ_TYPE(ClassName, fourc)                                              \
  ClassName() { physObjectType = fourc; }                                                 \
  static ClassName *cast(PhysObjectUserData *ptr)                                         \
  {                                                                                       \
    return ptr && ptr->physObjectType == fourc ? static_cast<ClassName *>(ptr) : nullptr; \
  }
