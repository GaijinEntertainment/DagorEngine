#pragma once

#include "riExtraRendererT.h"

#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_dynVariantsCache.h>
#include <memory/dag_framemem.h>


namespace rendinst
{

struct CachedDynVarsPolicy
{
  DynVariantsCache<framemem_allocator> dynVarCache;

  int getStates(const ScriptedShaderElement &shelem, uint32_t &prog, uint32_t &state, shaders::RenderStateId &rstate, uint32_t &cstate,
    uint32_t &tstate)
  {
    return get_cached_dynamic_variant_states(shelem, dynVarCache.getCache(), prog, state, rstate, cstate, tstate);
  }
};

} // namespace rendinst
