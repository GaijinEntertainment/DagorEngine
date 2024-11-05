// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderVarsUtils.h>


namespace rendinst::render
{

struct OpaqueGlobalDynVarsPolicy
{
  GlobalVariableStates *globalVarsState = nullptr;

  int getStates(const ScriptedShaderElement &shelem, uint32_t &prog, uint32_t &state, shaders::RenderStateId &rstate,
    shaders::ConstStateIdx &cstate, shaders::TexStateIdx &tstate)
  {
    return get_dynamic_variant_states(*globalVarsState, shelem, prog, state, rstate, cstate, tstate);
  }
};

} // namespace rendinst::render
