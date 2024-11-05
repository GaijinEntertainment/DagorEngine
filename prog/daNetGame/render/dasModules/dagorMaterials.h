// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasShaders.h>

#include <ecs/anim/anim.h>
#include <ecs/render/animCharUtils.h>
#include <shaders/dag_dynSceneRes.h>

namespace bind_dascript
{
inline bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  const das::TBlock<void, das::TTemporary<ShaderMaterial>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  return ::recreate_material_with_new_params(animchar_render, [&](ShaderMaterial *mat) {
    vec4f arg = das::cast<ShaderMaterial *>::from(mat);
    context->invoke(block, &arg, nullptr, at);
  });
}

inline bool recreate_material_with_new_params_1(AnimV20::AnimcharRendComponent &animchar_render,
  const char *shader_name,
  const das::TBlock<void, das::TTemporary<ShaderMaterial>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  return ::recreate_material_with_new_params(animchar_render, shader_name, [&](ShaderMaterial *mat) {
    vec4f arg = das::cast<ShaderMaterial *>::from(mat);
    context->invoke(block, &arg, nullptr, at);
  });
}

inline bool recreate_material_with_new_params_2(AnimV20::AnimcharRendComponent &animchar_render,
  const das::TArray<char *> &shader_names,
  const das::TBlock<void, das::TTemporary<ShaderMaterial>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  eastl::vector<const char *, framemem_allocator> names(shader_names.size);
  for (int i = 0; i < shader_names.size; i++)
    names[i] = shader_names[i];
  return ::recreate_material_with_new_params(animchar_render, names, [&](ShaderMaterial *mat) {
    vec4f arg = das::cast<ShaderMaterial *>::from(mat);
    context->invoke(block, &arg, nullptr, at);
  });
}

inline void scene_lods_gather_mats(const DynamicRenderableSceneLodsResource &scene_lods,
  int lod,
  const das::TBlock<void, das::TTemporary<const das::TArray<ShaderMaterial *>>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  Tab<ShaderMaterial *> mats(framemem_ptr());
  scene_lods.gatherLodUsedMat(mats, lod);

  das::Array arr;
  arr.data = (char *)mats.data();
  arr.size = uint32_t(mats.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript