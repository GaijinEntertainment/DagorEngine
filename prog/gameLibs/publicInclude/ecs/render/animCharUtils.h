//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/vector.h>
#include <generic/dag_functionRef.h>
#include <generic/dag_span.h>
#include <memory/dag_framemem.h>

class ShaderMaterial;
namespace AnimV20
{
class AnimcharRendComponent;
}

// the callables are only invoked during the call, never stored
bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  dag::FunctionRef<bool(const ShaderMaterial *) const> material_filter,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter);

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter);

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render, const char *shader_name,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter);

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  const eastl::vector<const char *, framemem_allocator> &shader_names_filter,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter);

// per-node copies of rigid mesh materials; unknown and repeated names are skipped
// name_idx indexes node_names; false when nothing was copied
bool recreate_node_materials(AnimV20::AnimcharRendComponent &animchar_render, dag::ConstSpan<const char *> node_names,
  dag::FunctionRef<void(int name_idx, ShaderMaterial *) const> shader_var_setter);
