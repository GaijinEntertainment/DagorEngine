// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <generic/dag_enumerate.h>
#include <EASTL/unordered_set.h>

#include "bvh_tools.h"

namespace bvh::ri
{

static eastl::unordered_set<ContextId> relem_changed_contexts;
static CallbackToken relem_changed_token;

static void impostor_callback(const RenderableInstanceLodsResource *resource)
{
  G_ASSERT(resource->isBakedImpostor());

  auto lodIx = resource->lods.size() - 1;
  auto &lod = resource->lods[lodIx];

  for (auto context_id : relem_changed_contexts)
    if (context_id->has(Features::RIFull))
    {
      auto bounding = BSphere3(resource->bsphCenter, resource->bsphRad);
      auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);
      G_ASSERT(elems.size() == 1);

      auto &params = resource->getImpostorParams();
      auto &textures = resource->getImpostorTextures();

      process_relems(context_id, elems, resource->getBvhId(), lodIx, nullptr, Point4::ONE, Point4::ZERO, bounding, true, &params,
        &textures);
    }
}

static void on_relem_changed(ContextId context_id, const RenderableInstanceLodsResource *resource, bool deleted, bool load_impostors)
{
  TIME_PROFILE(bvh::on_ri_relem_changed);

  if (resource->getBvhId() == 0)
    resource->setBvhId(++bvh_id_gen);

  bool hasBakedImpostor = resource->isBakedImpostor();

  for (auto [lodIx, lod] : enumerate(resource->lods))
  {
    int lastLod = hasBakedImpostor ? resource->lods.size() - 2 : resource->lods.size() - 1;
    if (context_id->has(Features::RIBaked) && lodIx < lastLod)
      continue;

    if (hasBakedImpostor && lodIx == resource->lods.size() - 1)
    {
      if (context_id->has(Features::RIFull))
      {
        auto &firstElem = lod.scene->getMesh()->getMesh()->getMesh()->getAllElems()[0];

        bool hasIndices;
        if (firstElem.vertexData->isRenderable(hasIndices))
        {
          if (load_impostors)
            impostor_callback(resource);

          continue;
        }
      }
    }

    auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

    if (deleted)
    {
      for (auto [elemIx, elem] : enumerate(elems))
        remove_mesh(context_id, make_relem_mesh_id(resource->getBvhId(), lodIx, elemIx));
    }
    else
    {
      auto bounding = BSphere3(resource->bsphCenter, resource->bsphRad);
      process_relems(context_id, elems, resource->getBvhId(), lodIx, nullptr, Point4::ONE, Point4::ZERO, bounding, true);
    }
  }
}

static void on_relem_changed_all(const RenderableInstanceLodsResource *resource, bool deleted, bool load_impostors)
{
  for (auto &contextId : relem_changed_contexts)
    on_relem_changed(contextId, resource, deleted, load_impostors);
}

void init()
{
  relem_changed_token = unitedvdata::riUnitedVdata.on_mesh_relems_updated.subscribe(
    [](const RenderableInstanceLodsResource *resource, bool deleted) { on_relem_changed_all(resource, deleted, false); });
  RenderableInstanceLodsResource::setImpostorTextureCallback(impostor_callback);
}

void teardown_ri_gen();

void teardown()
{
  relem_changed_token = CallbackToken();
  RenderableInstanceLodsResource::setImpostorTextureCallback(nullptr);

  teardown_ri_gen();
}

void init(ContextId context_id)
{
  if (context_id->has(Features::RIFull | Features::RIBaked))
  {
    relem_changed_contexts.insert(context_id);
    unitedvdata::riUnitedVdata.enumRElems(
      [](const RenderableInstanceLodsResource *resource) { on_relem_changed_all(resource, false, true); });
  }
}

void wait_ri_extra_instances_update(ContextId context_id);
void wait_ri_gen_instances_update(ContextId context_id);
void wait_cut_down_trees();

void on_unload_scene_ri_ex(ContextId context_id);

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::RIFull | Features::RIBaked))
    return;
  wait_ri_extra_instances_update(context_id);
  wait_ri_gen_instances_update(context_id);
  wait_cut_down_trees();
  for (auto &trees : context_id->uniqueTreeBuffers)
    for (auto &tree : trees.second.elems)
    {
      if (tree.second.buffer)
        context_id->releaseBuffer(tree.second.buffer.get());
      context_id->freeMetaRegion(tree.second.metaAllocId);
    }
  for (auto &trees : context_id->uniqueRiExtraTreeBuffers)
    for (auto &tree : trees.second)
    {
      if (tree.second.buffer)
        context_id->releaseBuffer(tree.second.buffer.get());
      context_id->freeMetaRegion(tree.second.metaAllocId);
    }
  for (auto &trees : context_id->uniqueRiExtraFlagBuffers)
    for (auto &tree : trees.second)
    {
      if (tree.second.buffer)
        context_id->releaseBuffer(tree.second.buffer.get());
      context_id->freeMetaRegion(tree.second.metaAllocId);
    }
  context_id->uniqueTreeBuffers.clear();
  context_id->freeUniqueTreeBLASes.clear();
  context_id->uniqueRiExtraTreeBuffers.clear();
  context_id->uniqueRiExtraFlagBuffers.clear();

  on_unload_scene_ri_ex(context_id);
}

void teardown(ContextId context_id)
{
  bvh::ri::on_unload_scene(context_id);
  relem_changed_contexts.erase(context_id);
}

} // namespace bvh::ri