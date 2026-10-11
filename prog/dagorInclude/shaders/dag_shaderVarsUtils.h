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

// C++ requires this overload of get_dynamic_variant_states to be declared static before friending it
class GlobalVariableStates;
static int get_dynamic_variant_states(const GlobalVariableStates &global_variants_state, const ScriptedShaderElement &, uint32_t &,
  ShaderStateBlockId &, shaders::RenderStateId &, shaders::ConstStateIdx &, shaders::TexStateIdx &, uint32_t &);

class GlobalVariableStates
{
  Tab<uint8_t> globIntervalNormValues;
  uint32_t generation = 0;

  friend void copy_current_global_variables_states(GlobalVariableStates &gv);
  friend int get_dynamic_variant_states(const GlobalVariableStates &global_variants_state, const ScriptedShaderElement &s,
    uint32_t &program, ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
    shaders::TexStateIdx &tex_state, uint32_t &variant_code);

public:
  GlobalVariableStates(IMemAlloc *mem = defaultmem) : globIntervalNormValues(mem) {}
  bool empty() const { return globIntervalNormValues.empty(); }
  void clear() { clear_and_shrink(globIntervalNormValues); }
  void set_allocator(IMemAlloc *a) { dag::set_allocator(globIntervalNormValues, a); }
  IMemAlloc *get_allocator() const { return dag::get_allocator(globIntervalNormValues); }
};
// this will copy current globals variable states (intervals) so it can be used by get_dynamic_variant_states
void copy_current_global_variables_states(GlobalVariableStates &gv);

namespace shaders
{

struct CombinedDynVariantState
{
  uint32_t program = uint32_t(-1);
  int32_t variant = -1;
  ShaderStateBlockId state_index = ShaderStateBlockId::Invalid;
  shaders::RenderStateId render_state = shaders::RenderStateId::Invalid;
  shaders::ConstStateIdx const_state = shaders::ConstStateIdx::Invalid;
  shaders::TexStateIdx tex_state = shaders::TexStateIdx::Invalid;
#if DAGOR_DBGLEVEL > 0
  uint32_t variantCode = uint32_t(-1);
#endif
};

} // namespace shaders

// uses explicit global variants state
shaders::CombinedDynVariantState get_dynamic_variant_states(const GlobalVariableStates &global_variants_state,
  const ScriptedShaderElement &s);

// uses current global global variants state
shaders::CombinedDynVariantState get_dynamic_variant_states(const ScriptedShaderElement &s);

inline shaders::CombinedDynVariantState get_dynamic_variant_states(const GlobalVariableStates *global_variants_state,
  const ScriptedShaderElement &s)
{
  return global_variants_state ? get_dynamic_variant_states(*global_variants_state, s) : get_dynamic_variant_states(s);
}

shaders::CombinedDynVariantState get_cached_dynamic_variant_states(const ScriptedShaderElement &s, dag::ConstSpan<int> cache);

// set states returned by get_dynamic_variant_states family
void set_states_for_variant(const ScriptedShaderElement &s, const shaders::CombinedDynVariantState &state);

inline bool is_valid(const shaders::CombinedDynVariantState &dv_state) { return dv_state.variant >= 0; }

// True when the dynamic variant selection of s reads the global shader var var_id
bool dynamic_variant_depends_on_global_var(const ScriptedShaderElement &s, int var_id);
