//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>


namespace dafg
{

enum class Access : uint8_t
{
  UNKNOWN = 0,
  READ_ONLY = 1u << 0,
  READ_WRITE = 1u << 1
};

const char *to_string(Access access);

} // namespace dafg
