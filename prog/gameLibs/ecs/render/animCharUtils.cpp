// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/anim.h>
#include <ecs/render/animCharUtils.h>
#include <shaders/dag_dynSceneRes.h>
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>

static void commit_cloned_lods(AnimV20::AnimcharRendComponent &animchar_render, DynamicRenderableSceneInstance *scene,
  DynamicRenderableSceneLodsResource *new_res, dag::Span<ShaderMaterial *> new_mats)
{
  DynamicRenderableSceneLodsResource::finalizeDuplicateMaterial(new_mats);
  new_res->updateShaderElems();
  scene->changeLodsResource(new_res);
  animchar_render.updateLodResFromSceneInstance();
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  dag::FunctionRef<bool(const ShaderMaterial *) const> material_filter,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter)
{
  DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  if (scene == nullptr)
    return false;
  Tab<ShaderMaterial *> matList(framemem_ptr());
  scene->getLodsResource()->gatherUsedMat(matList);
  Ptr<DynamicRenderableSceneLodsResource> newRes = scene->cloneLodsResource();
  if (!newRes)
    return false;

  Tab<ShaderMaterial *> oldMatList(framemem_ptr()), newMatList(framemem_ptr());
  for (ShaderMaterial *mat : matList)
  {
    if (!material_filter(mat))
      continue;
    int matCount = newMatList.size();
    newRes->duplicateMat(mat, oldMatList, newMatList); // should add new pointer on material to newMatList
    if (matCount + 1 != newMatList.size())
      continue;
    shader_var_setter(newMatList.back());
  }

  commit_cloned_lods(animchar_render, scene, newRes, make_span(newMatList));
  return true;
}

bool recreate_node_materials(AnimV20::AnimcharRendComponent &animchar_render, dag::ConstSpan<const char *> node_names,
  dag::FunctionRef<void(int name_idx, ShaderMaterial *) const> shader_var_setter)
{
  DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  if (scene == nullptr)
    return false;
  // node ids are shared by all LODs; resolve them before the clone so unknown names cost nothing
  Tab<int> nodeIds(framemem_ptr());
  nodeIds.reserve(node_names.size());
  bool anyNode = false;
  for (const char *name : node_names)
  {
    int nodeId = scene->getNodeId(name);
    // a repeated node keeps the copies of its first entry: a second pass would free them
    if (nodeId >= 0 && find_value_idx(nodeIds, nodeId) >= 0)
      nodeId = -1;
    nodeIds.push_back(nodeId);
    anyNode |= nodeId >= 0;
  }
  if (!anyNode)
    return false;
  Ptr<DynamicRenderableSceneLodsResource> newRes = scene->cloneLodsResource();
  if (!newRes)
    return false;

  Tab<ShaderMaterial *> oldMatList(framemem_ptr()), newMatList(framemem_ptr()), allNewMats(framemem_ptr());
  for (int nameIdx = 0; nameIdx < nodeIds.size(); nameIdx++)
  {
    if (nodeIds[nameIdx] < 0)
      continue;
    // per node lists, so two nodes that share a material get separate copies
    oldMatList.clear();
    newMatList.clear();
    for (DynamicRenderableSceneLodsResource::Lod &lod : newRes->lods)
      for (DynamicRenderableSceneResource::RigidObject &ro : lod.scene->getRigids())
        if (ShaderMesh *mesh = ro.nodeId == nodeIds[nameIdx] ? ro.mesh->getMesh() : nullptr)
          // an element that duplicateMat already switched holds a copy, so it is skipped
          for (const ShaderMesh::RElem &elem : mesh->getAllElems())
            if (elem.mat && find_value_idx(newMatList, elem.mat.get()) < 0)
              mesh->duplicateMat(elem.mat.get(), oldMatList, newMatList);
    for (ShaderMaterial *mat : newMatList)
    {
      shader_var_setter(nameIdx, mat);
      allNewMats.push_back(mat);
    }
  }
  if (allNewMats.empty())
    return false;

  commit_cloned_lods(animchar_render, scene, newRes, make_span(allNewMats));
  return true;
}

static bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  dag::FunctionRef<bool(const char *) const> shader_name_filter, dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter)
{
  return recreate_material_with_new_params(
    animchar_render, [&shader_name_filter](const ShaderMaterial *m) { return shader_name_filter(m->getShaderClassName()); },
    shader_var_setter);
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter)
{
  return recreate_material_with_new_params(animchar_render, [](const char *) { return true; }, shader_var_setter);
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render, const char *shader_name,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter)
{
  return recreate_material_with_new_params(
    animchar_render, [shader_name](const char *name) { return strcmp(shader_name, name) == 0; }, shader_var_setter);
}

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &animchar_render,
  const eastl::vector<const char *, framemem_allocator> &shader_names_filter,
  dag::FunctionRef<void(ShaderMaterial *) const> shader_var_setter)
{
  return recreate_material_with_new_params(
    animchar_render,
    [&](const ShaderMaterial *m) {
      const char *name = m->getShaderClassName();
      for (const char *shader_name : shader_names_filter)
        if (strcmp(shader_name, name) == 0)
          return true;
      return false;
    },
    shader_var_setter);
}