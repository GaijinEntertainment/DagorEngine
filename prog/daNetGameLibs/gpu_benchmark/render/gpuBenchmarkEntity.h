// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/graphicsAutodetect.h>

namespace gpubenchmark
{
enum class Selfdestruct
{
  NO,
  YES
};
GraphicsAutodetect *get_graphics_autodetect();
void make_graphics_autodetect_entity(Selfdestruct selfdestruct);
void destroy_graphics_autodetect_entity();
} // namespace gpubenchmark