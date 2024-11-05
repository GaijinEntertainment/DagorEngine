//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>


namespace dabfg
{

enum class Access : uint8_t
{
  UNKNOWN,
  READ_ONLY,
  READ_WRITE
};

} // namespace dabfg
