// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

namespace HumanInput
{
namespace sonypad
{

// Steam Input launched this process and remaps a pad of this vendor into a virtual Xbox controller
bool steam_input_remaps_vendor(uint16_t vid);

} // namespace sonypad
} // namespace HumanInput
