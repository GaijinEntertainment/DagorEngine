//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_renderStateId.h>
#include <generic/dag_tab.h>
#include <shaders/dag_shaderState.h>


class ScriptedShaderElement;

enum class ShaderStateBlockId : uint32_t
{
  Invalid = 0
};

class GlobalVariableStates
{
  Tab<uint8_t> globIntervalNormValues;
  uint32_t generation = 0;

  friend void copy_current_global_variables_states(GlobalVariableStates &gv);
  friend int get_dynamic_variant_states(const GlobalVariableStates &global_variants_state, const ScriptedShaderElement &s,
    uint32_t &program, ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
    shaders::TexStateIdx &tex_state);

public:
  GlobalVariableStates(IMemAlloc *mem = defaultmem) : globIntervalNormValues(mem) {}
  bool empty() const { return globIntervalNormValues.empty(); }
  void clear() { clear_and_shrink(globIntervalNormValues); }
  void set_allocator(IMemAlloc *a) { dag::set_allocator(globIntervalNormValues, a); }
  IMemAlloc *get_allocator() const { return dag::get_allocator(globIntervalNormValues); }
};
// this will copy current globals variable states (intervals) so it can be used by get_dynamic_variant_states
void copy_current_global_variables_states(GlobalVariableStates &gv);

// uses explicit global variants state
int get_dynamic_variant_states(const GlobalVariableStates &global_variants_state, const ScriptedShaderElement &s, uint32_t &program,
  ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state);

// uses current global global variants state
int get_dynamic_variant_states(const ScriptedShaderElement &s, uint32_t &program, ShaderStateBlockId &state_index,
  shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state, shaders::TexStateIdx &tex_state);

inline int get_dynamic_variant_states(const GlobalVariableStates *global_variants_state, const ScriptedShaderElement &s,
  uint32_t &program, ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state)
{
  return global_variants_state
           ? get_dynamic_variant_states(*global_variants_state, s, program, state_index, render_state, const_state, tex_state)
           : get_dynamic_variant_states(s, program, state_index, render_state, const_state, tex_state);
}

int get_cached_dynamic_variant_states(const ScriptedShaderElement &s, dag::ConstSpan<int> cache, uint32_t &program,
  ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state);

// set states returned by get_dynamic_variant_states family
void set_states_for_variant(const ScriptedShaderElement &s, int curVariant, uint32_t program, ShaderStateBlockId state_index);

namespace shaders
{

struct CombinedDynVariantState
{
  uint32_t program;
  uint32_t variant;
  ShaderStateBlockId state_index;
  shaders::RenderStateId render_state;
  shaders::ConstStateIdx const_state;
  shaders::TexStateIdx tex_state;
};

inline CombinedDynVariantState get_dynamic_variant_state(const ScriptedShaderElement &s)
{
  CombinedDynVariantState result;
  result.variant =
    ::get_dynamic_variant_states(s, result.program, result.state_index, result.render_state, result.const_state, result.tex_state);
  return result;
}

} // namespace shaders
