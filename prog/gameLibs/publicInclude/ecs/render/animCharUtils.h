//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>

class ShaderMaterial;
namespace AnimV20
{
class AnimcharRendComponent;
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  eastl::function<void(ShaderMaterial *)> &&shader_var_setter);

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render, const char *shader_name,
  eastl::function<void(ShaderMaterial *)> &&shader_var_setter);

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  const eastl::vector<const char *, framemem_allocator> &shader_names_filter,
  eastl::function<void(ShaderMaterial *)> &&shader_var_setter);
