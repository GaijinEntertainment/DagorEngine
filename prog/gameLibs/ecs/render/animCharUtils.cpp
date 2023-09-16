#include <ecs/anim/anim.h>
#include <memory/dag_framemem.h>

static bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  const eastl::function<bool(const char *)> &shader_name_filter, const eastl::function<void(ShaderMaterial *)> &shader_var_setter)
{
  auto scene = animchar_render.getSceneInstance();
  if (scene == nullptr)
    return false;
  Tab<ShaderMaterial *> matList(framemem_ptr());
  scene->getLodsResource()->gatherUsedMat(matList);
  DynamicRenderableSceneLodsResource *newRes = scene->cloneLodsResource();
  if (newRes == nullptr)
    return false;

  Tab<ShaderMaterial *> oldMatList(framemem_ptr()), newMatList(framemem_ptr());
  for (ShaderMaterial *mat : matList)
  {
    if (!shader_name_filter(mat->getShaderClassName()))
      continue;
    int matCount = newMatList.size();
    newRes->duplicateMat(mat, oldMatList, newMatList); // should add new pointer on material to newMatList
    if (matCount + 1 != newMatList.size())
      continue;
    shader_var_setter(newMatList.back());
  }

  DynamicRenderableSceneLodsResource::finalizeDuplicateMaterial(make_span(newMatList));
  newRes->updateShaderElems();
  animchar_render.updateLodResFromSceneInstance();

  return true;
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  eastl::function<void(ShaderMaterial *)> &&shader_var_setter)
{
  return recreate_material_with_new_params(
    animchar_render, [](const char *) { return true; }, eastl::move(shader_var_setter));
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render, const char *shader_name,
  eastl::function<void(ShaderMaterial *)> &&shader_var_setter)
{
  return recreate_material_with_new_params(
    animchar_render, [shader_name](const char *name) { return strcmp(shader_name, name) == 0; }, eastl::move(shader_var_setter));
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  const eastl::vector<const char *, framemem_allocator> &shader_names_filter,
  eastl::function<void(ShaderMaterial *)> &&shader_var_setter)
{
  return recreate_material_with_new_params(
    animchar_render,
    [&](const char *name) {
      for (const char *shader_name : shader_names_filter)
        if (strcmp(shader_name, name) == 0)
          return true;
      return false;
    },
    eastl::move(shader_var_setter));
}