// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include "bvh_ri_common.h"
#include <shaders/dag_shaderResUnitedData.h>
#include <generic/dag_enumerate.h>
#include <rendInst/rendInstGen.h>
#include <EASTL/unordered_set.h>
#include <gameRes/dag_resourceNameResolve.h>

#include "bvh_tools.h"

namespace bvh::ri
{

void init_ex();
void teardown_ex();

static constexpr float bvh_ri_lod_range_exception_threshold = 10000.0f;

static eastl::unordered_set<ContextId> relem_changed_contexts;
static CallbackToken relem_changed_token;
static CallbackToken relem_prechanged_token;
static float ri_lod_dist_bias = 0.0f;

static int ri_single_lod_filter_max_faces = 0;
static float ri_single_lod_filter_max_range = 0;

struct BrokenAssetDetails
{
  const RenderableInstanceLodsResource *resource;
  float range;
  int faceCnt;
};
// deferred logging because of its name not being ready at the moment of detection
static eastl::vector<BrokenAssetDetails> brokenAssets;

static String get_resource_name(const RenderableInstanceLodsResource *resource)
{
  String name;
  if (!resolve_game_resource_name(name, resource))
    name = "<unknown>";
  return name;
}

static int calc_face_cnt(const RenderableInstanceLodsResource *resource)
{
  int faceCnt = 0;
  for (auto &lod : resource->lods)
    for (auto &e : lod.scene->getMesh()->getMesh()->getMesh()->getAllElems())
      faceCnt += e.numf;
  return faceCnt;
}

static bool filter_broken_lod_resources(const RenderableInstanceLodsResource *resource, bool log_broken_assets)
{
  G_ASSERT_RETURN(resource->lods.size() > 0, true);
  const bool filterBrokenAssets = ri_single_lod_filter_max_faces > 0 && ri_single_lod_filter_max_range > 0;
  if (filterBrokenAssets && resource->lods.size() == 1)
  {
    float range = resource->lods[0].range;
    if (range >= bvh_ri_lod_range_exception_threshold)
      return true;
    int faceCnt = calc_face_cnt(resource);
    if (range > ri_single_lod_filter_max_range && faceCnt > ri_single_lod_filter_max_faces)
    {
      if (log_broken_assets)
        brokenAssets.push_back({resource, range, faceCnt});
      return false;
    }
  }
  return true;
}

static bool filter_lod_resources(const RenderableInstanceLodsResource *resource, bool log_broken_assets)
{
  // can add more filters here if necessary
  return filter_broken_lod_resources(resource, log_broken_assets);
}

void debug_update()
{
#if DAGOR_DBGLEVEL > 0
  for (auto asset : brokenAssets)
  {
    String resName = get_resource_name(asset.resource);
    logwarn("BVH RI: resource %s has a single lod!! (range = %f, faceCnt = %d)", resName.data(), asset.range, asset.faceCnt);
  }
#endif
  brokenAssets.clear();
}

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

      if (textures.albedo_alpha != BAD_TEXTUREID)
        process_relems(context_id, "impostor", elems, resource->getBvhId(), lodIx, eastl::nullopt, Point4::ONE, Point4::ZERO, bounding,
          true, PerInstanceDataUse::ALLOWED, &params, &textures);
    }
}

inline const char *get_tag(unsigned lod)
{
  switch (lod)
  {
    case 0: return "ri_lod0";
    case 1: return "ri_lod1";
    case 2: return "ri_lod2";
    case 3: return "ri_lod3";
    case 4: return "ri_lod4";
    default: return "ri_lod4+";
  }
}

static void on_relem_changed(ContextId context_id, const RenderableInstanceLodsResource *resource, bool deleted, int upper_lod,
  bool pre_change_event)
{
  TIME_PROFILE(bvh::on_ri_relem_changed);

  if (!filter_lod_resources(resource, pre_change_event))
    return;

  if (resource->getBvhId() == 0)
  {
    resource->setBvhId(bvh_id_gen.add_fetch(1));
    resource->updateBvhMinLod(bvh::ri::get_ri_lod_dist_bias());
  }

  const int bestLod = resource->getBvhMinLod();

  if (deleted)
  {
    for (auto [lodIx, lod] : enumerate(make_span_const(resource->lods)))
    {
      if (lodIx >= upper_lod)
        break;

      auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      for (int elemIx = 0; elemIx < elems.size(); elemIx++)
        remove_object(context_id, make_relem_mesh_id(resource->getBvhId(), lodIx, elemIx));
    }
  }
  else if (pre_change_event)
  {
    for (auto [lodIx, lod] : enumerate(make_span_const(resource->lods)))
    {
      if (lodIx < bestLod)
        continue;

      auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      for (auto [i, elem] : enumerate(elems))
      {
        const auto meshId = make_relem_mesh_id(resource->getBvhId(), lodIx, i);
        before_object_action(context_id, meshId);
      }
    }
  }
  else
  {
    for (auto [lodIx, lod] : enumerate(make_span_const(resource->lods)))
    {
      if (resource->isBakedImpostor() && lodIx == resource->lods.size() - 1)
      {
        auto &firstElem = lod.scene->getMesh()->getMesh()->getMesh()->getAllElems()[0];
        bool hasIndices;
        if (firstElem.vertexData->isRenderable(hasIndices))
          impostor_callback(resource);
      }
      else
      {
        if (lodIx < bestLod)
          continue;

        auto elems = lod.scene->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);
        auto bounding = BSphere3(resource->bsphCenter, resource->bsphRad);
        process_relems(context_id, get_tag(lodIx), elems, resource->getBvhId(), lodIx,
          resource->isUsedAsDagdp() ? eastl::make_optional<const BufferProcessor *>(nullptr) : eastl::nullopt, Point4::ONE,
          Point4::ZERO, bounding, true, resource->isUsedAsDagdp() ? PerInstanceDataUse::FORCE_OFF : PerInstanceDataUse::ALLOWED);
      }
    }
  }
}

static void on_relem_changed_all(const RenderableInstanceLodsResource *resource, bool deleted, int upper_lod, bool pre_change_event)
{
  for (auto &contextId : relem_changed_contexts)
    on_relem_changed(contextId, resource, deleted, upper_lod, pre_change_event);
}

void init(int single_lod_filter_max_faces, float single_lod_filter_max_range, float lod_dist_bias)
{
  ri_single_lod_filter_max_faces = single_lod_filter_max_faces;
  ri_single_lod_filter_max_range = single_lod_filter_max_range;
  ri_lod_dist_bias = lod_dist_bias;
  relem_prechanged_token = unitedvdata::riUnitedVdata.on_mesh_relems_about_to_be_updated.subscribe(
    [](const RenderableInstanceLodsResource *resource, bool deleted, int upper_lod) {
      on_relem_changed_all(resource, deleted, upper_lod, true);
    });
  relem_changed_token = unitedvdata::riUnitedVdata.on_mesh_relems_updated.subscribe(
    [](const RenderableInstanceLodsResource *resource, bool deleted, int upper_lod) {
      if (!deleted)
        on_relem_changed_all(resource, deleted, upper_lod, false);
    });
  rendinst::set_impostor_iexture_callback(impostor_callback);
  init_ex();
}

void teardown_ri_gen();

void teardown(bool device_reset)
{
  brokenAssets.clear();

  if (!device_reset)
  {
    unitedvdata::riUnitedVdata.availableRElemsAccessor([](dag::Span<RenderableInstanceLodsResource *> resources) {
      for (RenderableInstanceLodsResource *resource : resources)
        resource->setBvhId(0);
    });
  }

  relem_changed_token = CallbackToken();
  relem_prechanged_token = CallbackToken();
  rendinst::set_impostor_iexture_callback(nullptr);

  teardown_ri_gen();
  teardown_ex();
}

void init(ContextId context_id)
{
  if (context_id->has(Features::AnyRI))
  {
    relem_changed_contexts.insert(context_id);
    unitedvdata::riUnitedVdata.availableRElemsAccessor([](dag::Span<RenderableInstanceLodsResource *> resources) {
      for (RenderableInstanceLodsResource *resource : resources)
        on_relem_changed_all(resource, false, 0, false);
    });
  }
}

void wait_ri_extra_instances_update(ContextId context_id);
void wait_ri_gen_instances_update(ContextId context_id);

void on_scene_loaded_ri_ex(ContextId context_id);
void on_unload_scene_ri_ex(ContextId context_id);

void on_scene_loaded(ContextId context_id) { on_scene_loaded_ri_ex(context_id); }

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::AnyRI))
    return;
  wait_ri_extra_instances_update(context_id);
  wait_ri_gen_instances_update(context_id);
  for (auto &cnt : context_id->treeAnimIndexCount)
    cnt = 0;
  for (auto &lod : context_id->uniqueTreeBuffers)
    for (auto &trees : lod)
      for (auto &tree : trees.second.elems)
        context_id->freeMetaRegion(tree.second.metaAllocId);
  for (auto &lod : context_id->uniqueRiExtraTreeBuffers)
    for (auto &trees : lod)
      for (auto &tree : trees.second.elems)
        context_id->freeMetaRegion(tree.second.metaAllocId);
  for (auto &trees : context_id->uniqueRiExtraFlagBuffers)
    for (auto &tree : trees.second)
      context_id->freeMetaRegion(tree.second.metaAllocId);
  for (auto &lod : context_id->uniqueTreeBuffers)
    lod.clear();
  context_id->freeUniqueTreeBLASes.clear();
  context_id->freeUniqueRiExtraTreeBLASes.clear();
  for (auto &lod : context_id->uniqueRiExtraTreeBuffers)
    lod.clear();
  context_id->uniqueRiExtraFlagBuffers.clear();

  for (auto &tree : context_id->stationaryTreeBuffers)
    if (tree.second.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
      context_id->freeMetaRegion(tree.second.metaAllocId);
  context_id->stationaryTreeBuffers.clear();

  on_unload_scene_ri_ex(context_id);
}

void teardown(ContextId context_id)
{
  bvh::ri::on_unload_scene(context_id);
  relem_changed_contexts.erase(context_id);
}

void readdRendinst(ContextId context_id, const RenderableInstanceLodsResource *resource)
{
  on_relem_changed(context_id, resource, true, 8, true);
  on_relem_changed(context_id, resource, true, 8, false);
  on_relem_changed(context_id, resource, false, 8, true);
  on_relem_changed(context_id, resource, false, 8, false);
}

float get_ri_lod_dist_bias() { return ri_lod_dist_bias; }

} // namespace bvh::ri