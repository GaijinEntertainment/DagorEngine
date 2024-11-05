// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>

#include <drv_log_defs.h>

inline void check_texture_creation_args(uint32_t width, uint32_t height, uint32_t flags, const char *name)
{
  if (is_bc_texformat(flags) && !(width % 4 == 0 && height % 4 == 0))
    D3D_ERROR("BC texture format requires width and height to be multiple of 4, but got %u x %u for '%s'", width, height, name);
}
