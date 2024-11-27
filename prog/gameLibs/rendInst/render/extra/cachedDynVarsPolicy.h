// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "riExtraRendererT.h"

#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_dynVariantsCache.h>
#include <memory/dag_framemem.h>


namespace rendinst::render
{

struct CachedDynVarsPolicy
{
  DynVariantsCache<framemem_allocator> dynVarCache;

  int getStates(const ScriptedShaderElement &shelem, uint32_t &prog, ShaderStateBlockId &state, shaders::RenderStateId &rstate,
    shaders::ConstStateIdx &cstate, shaders::TexStateIdx &tstate)
  {
    return get_cached_dynamic_variant_states(shelem, dynVarCache.getCache(), prog, state, rstate, cstate, tstate);
  }
};

} // namespace rendinst::render
