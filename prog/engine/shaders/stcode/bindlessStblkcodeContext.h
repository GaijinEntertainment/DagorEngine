// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../shStateBlock.h"

struct BindlessStblkcodeContext
{
  eastl::fixed_vector<BindlessConstParams, 6, /*overflow*/ true, framemem_allocator> bindlessResources{};
  added_bindless_textures_t addedBindlessTextures{};
};