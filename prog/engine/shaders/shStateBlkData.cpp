// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shStateBlk.h"
#include "shStateBlock.h"

volatile int shaders_internal::cached_state_block = eastl::to_underlying(ShaderStateBlockId::Invalid);
