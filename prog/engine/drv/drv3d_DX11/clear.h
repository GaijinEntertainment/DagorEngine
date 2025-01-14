// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cinttypes>

namespace drv3d_dx11
{

void clear_slow(int write_mask, const float (&color_value)[4], float z_value, uint32_t stencil_value);

} // namespace drv3d_dx11
