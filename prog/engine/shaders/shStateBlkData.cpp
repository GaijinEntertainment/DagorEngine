// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shStateBlk.h"
#include "shStateBlock.h"

ska::flat_hash_set<ScriptedShaderMaterial *> shaders_internal::shader_mats;
ska::flat_hash_set<ScriptedShaderElement *> shaders_internal::shader_mat_elems;

volatile int shaders_internal::cached_state_block = eastl::to_underlying(ShaderStateBlockId::Invalid);
