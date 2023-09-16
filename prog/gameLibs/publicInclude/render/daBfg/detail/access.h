//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>


namespace dabfg
{

enum class Access : uint32_t
{
  UNKNOWN,
  READ_ONLY,
  READ_WRITE
};

} // namespace dabfg
