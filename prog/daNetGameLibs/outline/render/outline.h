// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentType.h>

enum OutlineMode
{
  ZTEST_FAIL,
  ZTEST_PASS,
  NO_ZTEST,
  OVERRIDES_COUNT
};

ECS_DECLARE_RELOCATABLE_TYPE(OutlineMode);
