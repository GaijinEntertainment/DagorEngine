// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <render/dynmodelRenderer.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <generic/dag_enumerate.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <drv/3d/dag_lock.h>
#include <util/dag_convar.h>
#include <gameRes/dag_resourceNameResolve.h>

#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"
#include "bvh_color_from_pos.h"

#include <math/dag_hlsl_floatx.h>
#include <render/decals/planar_decals_params.hlsli>

#if DAGOR_DBGLEVEL > 0
extern bool bvh_dyn_range_enable;
extern float bvh_dyn_range;
#endif

namespace bvh::dyn
{

static constexpr float bvh_dyn_lod_range_exception_threshold = 10000.0f;

static eastl::unordered_set<ContextId> relem_changed_contexts;
static CallbackToken relem_changed_token;
static constexpr float bvh_force_anim_distance = 30.0f;

static bool bvh_decals = false;

static bool bvh_discard_destr_assets = false;

static int dyn_single_lod_filter_max_faces = 0;
static float dyn_single_lod_filter_max_range = 0;


void wait_tidy_up_skins();


struct BrokenAssetDetails
{
  const DynamicRenderableSceneLodsResource *resource;
  float range;
  int faceCnt;
};
// deferred logging because of its name not being ready at the moment of detection
static eastl::vector<BrokenAssetDetails> brokenAssets;

static String get_resource_name(const DynamicRenderableSceneLodsResource *resource)
{
  String name;
  if (!resolve_game_resource_name(name, resource))
    name = "<unknown>";
  return name;
}

static int calc_face_cnt(const DynamicRenderableSceneLodsResource *resource)
{
  int faceCnt = 0;
  for (auto &lod : resource->lods)
  {
    for (auto &r : lod.scene->getRigidsConst())
      for (auto &e : r.mesh->getMesh()->getAllElems())
        faceCnt += e.numf;
    for (auto &s : lod.scene->getSkins())
      for (auto &e : s->getMesh()->getShaderMesh().getAllElems())
        faceCnt += e.numf;
  }
  return faceCnt;
}

static bool filter_broken_lod_resources(const DynamicRenderableSceneLodsResource *resource)
{
  G_ASSERT_RETURN(resource->lods.size() > 0, true);
  const bool filterBrokenAssets = dyn_single_lod_filter_max_faces > 0 && dyn_single_lod_filter_max_range > 0;
  if (filterBrokenAssets && resource->lods.size() == 1)
  {
    float range = resource->lods[0].range;
    if (range >= bvh_dyn_lod_range_exception_threshold)
      return true;
    int faceCnt = calc_face_cnt(resource);
    if (range > dyn_single_lod_filter_max_range && faceCnt > dyn_single_lod_filter_max_faces)
    {
      brokenAssets.push_back({resource, range, faceCnt});
      return false;
    }
  }
  return true;
}

static bool filter_lod_resources(const DynamicRenderableSceneLodsResource *resource)
{
  if (resource->getStaticFlags() & DynamicRenderableSceneLodsResource::SF_XRAY)
    return false;
  if (bvh_discard_destr_assets && resource->getStaticFlags() & DynamicRenderableSceneLodsResource::SF_DESTR)
    return false;
  return filter_broken_lod_resources(resource);
}

void debug_update()
{
#if DAGOR_DBGLEVEL > 0
  for (auto asset : brokenAssets)
  {
    String resName = get_resource_name(asset.resource);
    logwarn("BVH DYN: resource %s has a single lod!! (range = %f, faceCnt = %d)", resName.data(), asset.range, asset.faceCnt);
  }
#endif
  brokenAssets.clear();
}

inline const char *get_tag(const DynamicRenderableSceneLodsResource *resource, bool rigid, unsigned lod)
{
  bool isDestr = resource->getStaticFlags() & DynamicRenderableSceneLodsResource::SF_DESTR;
  if (rigid)
  {
    if (isDestr)
    {
      switch (lod)
      {
        case 0: return "rigid_destr_lod0";
        case 1: return "rigid_destr_lod1";
        case 2: return "rigid_destr_lod2";
        case 3: return "rigid_destr_lod3";
        case 4: return "rigid_destr_lod4";
        default: return "rigid_destr_lod4+";
      }
    }
    else
    {
      switch (lod)
      {
        case 0: return "rigid_lod0";
        case 1: return "rigid_lod1";
        case 2: return "rigid_lod2";
        case 3: return "rigid_lod3";
        case 4: return "rigid_lod4";
        default: return "rigid_lod4+";
      }
    }
  }
  else
  {
    if (isDestr)
    {
      switch (lod)
      {
        case 0: return "skinned_destr_lod0";
        case 1: return "skinned_destr_lod1";
        case 2: return "skinned_destr_lod2";
        case 3: return "skinned_destr_lod3";
        case 4: return "skinned_destr_lod4";
        default: return "skinned_destr_lod4+";
      }
    }
    else
    {
      switch (lod)
      {
        case 0: return "skinned_lod0";
        case 1: return "skinned_lod1";
        case 2: return "skinned_lod2";
        case 3: return "skinned_lod3";
        case 4: return "skinned_lod4";
        default: return "skinned_lod4+";
      }
    }
  }
}

static void on_relem_changed(ContextId context_id, const DynamicRenderableSceneLodsResource *resource, bool deleted, int upper_lod)
{
  TIME_PROFILE(bvh::on_dyn_relem_changed);
  DA_PROFILE_TAG(bvh::on_dyn_relem_changed, deleted ? "delete" : "modify");

  if (!filter_lod_resources(resource))
    return;

  if (resource->getBvhId() == 0)
    resource->setBvhId(bvh_id_gen.add_fetch(1));

  auto bvhId = resource->getBvhId();

  BSphere3 bounding;
  bounding = resource->getLocalBoundingBox();

  Point4 posMul, posAdd;
  if (resource->isBoundPackUsed())
  {
    posMul = *((Point4 *)resource->bpC255);
    posAdd = *((Point4 *)resource->bpC254);
  }
  else
  {
    posMul = Point4(1.f, 1.f, 1.f, 0.0f);
    posAdd = Point4(0.f, 0.f, 0.f, 0.0f);
  }

  for (auto [lodIx, lod] : enumerate(resource->lods))
  {
    if (deleted && lodIx >= upper_lod)
      break;

    TIME_PROFILE(lod);

    auto rigidCount = lod.scene->getRigidsConst().size();
    dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> rigids = lod.scene->getRigidsConst();
    auto lodIxLocal = lodIx;

    lod.scene->getMeshes(
      [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
        if (!mesh)
          return;
        if (!context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull))
          return;

        if (context_id->has(Features::DynrendRigidBaked) && lodIxLocal < (resource->lods.size() - 1))
          return;

        TIME_PROFILE(rigid);

        // Check if rigid has a unique id (unique rigid ids are non-zero and have the same id in lo-word and hi-word
        uint32_t uniqueRefId = rigids[rigid_no].uniqueRefId & 0xffff;
        if (uniqueRefId && ((rigids[rigid_no].uniqueRefId >> 16) != uniqueRefId))
          return;

        G_UNUSED(node_id);
        G_UNUSED(radius);
        auto elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);
        for (auto [elemIx, elem] : enumerate(elems))
        {
          TIME_PROFILE(elem);
          const auto elemId = make_dyn_elem_id(rigid_no, elemIx);
          const uint64_t objectId = make_relem_mesh_id(bvhId, lodIxLocal, elemId);

          if (deleted)
            remove_object(context_id, objectId);
          else
          {
            auto isPacked = elem.mat->isPositionPacked();
            auto posMulBatch = isPacked ? posMul : Point4(1.f, 1.f, 1.f, 0.0f);
            auto posAddBatch = isPacked ? posAdd : Point4(0.f, 0.f, 0.f, 0.0f);
            if (auto meshInfo = process_relem(context_id, elem, eastl::nullopt, posMulBatch, posAddBatch, bounding, false,
                  PerInstanceDataUse::ALLOWED))
            {
              bool isAnimated = meshInfo->vertexProcessor && !meshInfo->vertexProcessor->isOneTimeOnly();
              add_object(context_id, objectId, {{eastl::move(meshInfo.value())}, isAnimated, get_tag(resource, true, lodIx)});
            }
          }
        }
      },
      [&](const ShaderSkinnedMesh *mesh, int node_id, int skin_no) {
        if (!context_id->has(Features::DynrendSkinnedFull))
          return;

        TIME_PROFILE(skinned);

        G_UNUSED(node_id);
        auto elems = mesh->getShaderMesh().getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);
        for (auto [elemIx, elem] : enumerate(elems))
        {
          TIME_PROFILE(elem);

          auto elemId = make_dyn_elem_id(skin_no + rigidCount, elemIx);
          const uint64_t objectId = make_relem_mesh_id(bvhId, lodIxLocal, elemId);

          if (deleted)
            remove_object(context_id, objectId);
          else
          {
            if (auto meshInfo = process_relem(context_id, elem,
                  use_batched_skinned_vertex_processor ? &ProcessorInstances::getSkinnedVertexProcessorBatched()
                                                       : &ProcessorInstances::getSkinnedVertexProcessor(),
                  posMul, posAdd, bounding, false, PerInstanceDataUse::ALLOWED))
            {
              bool isAnimated = meshInfo->vertexProcessor && !meshInfo->vertexProcessor->isOneTimeOnly();
              add_object(context_id, objectId, {{eastl::move(meshInfo.value())}, isAnimated, get_tag(resource, true, lodIx)});
            }
          }
        }
      });
  }
}

static void on_relem_changed_all(const DynamicRenderableSceneLodsResource *resource, bool deleted, int upper_lod)
{
  G_ASSERT_RETURN(resource, );

  // If this is not the original resource, don't process it.
  // The instance will use the original so we spare a lot of memory and processing.
  if (resource->getFirstOriginal() != resource)
    return;

  eastl::optional<d3d::GpuAutoLock> optionalGpuLock;
  if (is_main_thread())
    optionalGpuLock.emplace();

  for (auto &contextId : relem_changed_contexts)
    on_relem_changed(contextId, resource, deleted, upper_lod);
}

void wait_dynrend_instances();

void init(int single_lod_filter_max_faces, float single_lod_filter_max_range, bool discard_destr_assets)
{
  dyn_single_lod_filter_max_faces = single_lod_filter_max_faces;
  dyn_single_lod_filter_max_range = single_lod_filter_max_range;
  bvh_discard_destr_assets = discard_destr_assets;
  relem_changed_token = unitedvdata::dmUnitedVdata.on_mesh_relems_updated.subscribe(on_relem_changed_all);
}

void teardown(bool device_reset, bool zero_bvh_ids)
{
  brokenAssets.clear();

  if (!device_reset && zero_bvh_ids)
  {
    unitedvdata::dmUnitedVdata.availableRElemsAccessor([](dag::Span<DynamicRenderableSceneLodsResource *> resources) {
      for (DynamicRenderableSceneLodsResource *resource : resources)
        resource->setBvhId(0);
    });
  }

  relem_changed_token = CallbackToken();
}

void init(ContextId context_id)
{
  if (context_id->has(Features::AnyDynrend))
  {
    relem_changed_contexts.insert(context_id);
    unitedvdata::dmUnitedVdata.availableRElemsAccessor([](dag::Span<DynamicRenderableSceneLodsResource *> resources) {
      for (DynamicRenderableSceneLodsResource *resource : resources)
        on_relem_changed_all(resource, false, 0);
    });
  }
}

void enable_dynamic_planar_decals(bool enable) { bvh_decals = enable; }

void teardown(ContextId context_id)
{
  on_unload_scene(context_id);
  relem_changed_contexts.erase(context_id);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::AnyDynrend))
    return;

  wait_dynrend_instances();
  wait_tidy_up_skins();

  for (auto &buffers : context_id->uniqueSkinBuffers)
    for (auto &buffer : buffers.second.elems)
    {
      if (buffer.second.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
      {
        const auto &meta = context_id->meshMetaAllocator.get(buffer.second.metaAllocId)[0];
        if (meta.materialType & MeshMeta::bvhMaterialUseInstanceTextures)
        {
          context_id->releaseTexture(meta.albedoTextureIndex);
          context_id->releaseTexture(meta.alphaTextureIndex);
          context_id->releaseTexture(meta.normalTextureIndex);
          context_id->releaseTexture(meta.extraTextureIndex);
        }
      }
      context_id->freeMetaRegion(buffer.second.metaAllocId);
    }
  context_id->uniqueSkinBuffers.clear();

  for (auto &buffers : context_id->uniqueHeliRotorBuffers)
    for (auto &buffer : buffers.second)
      context_id->freeMetaRegion(buffer.second.metaAllocId);
  context_id->uniqueHeliRotorBuffers.clear();

  for (auto &buffers : context_id->uniqueDeformedBuffers)
    for (auto &buffer : buffers.second)
      context_id->freeMetaRegion(buffer.second.metaAllocId);
  context_id->uniqueDeformedBuffers.clear();

  context_id->decalDataHolder.reset();
  context_id->decalDataHolderCursor = 0;
  context_id->decalDataHolderMap.clear();
  if (context_id->decalDataHolderBindlessSlot)
  {
    d3d::free_bindless_resource_range(D3DResourceType::SBUF, context_id->decalDataHolderBindlessSlot, 1);
    context_id->decalDataHolderBindlessSlot = 0;
  }

  context_id->initialNodesHolder.reset();
  context_id->initialNodes.clear();
  context_id->initialNodes.shrink_to_fit();
  if (context_id->initialNodesHolderBindlessSlot)
  {
    d3d::free_bindless_resource_range(D3DResourceType::SBUF, context_id->initialNodesHolderBindlessSlot, 1);
    context_id->initialNodesHolderBindlessSlot = 0;
  }
}

struct IterCtx
{
  ContextId contextId;
  Point3 viewPosition;
  bool noShadow;
  int count;
  dynrend::ContextId dynrendContextId;
};

bool has_camo(ShaderMesh::RElem &elem)
{
  return strcmp(elem.mat->getShaderClassName(), "dynamic_masked_tank") == 0 ||
         strcmp(elem.mat->getShaderClassName(), "dynamic_masked_ship") == 0 ||
         strcmp(elem.mat->getShaderClassName(), "dynamic_masked_track") == 0 ||
         strcmp(elem.mat->getShaderClassName(), "dynamic_tank_net_atest") == 0;
}

inline bool is_deformed(ShaderMesh::RElem &elem) { return strcmp(elem.mat->getShaderClassName(), "dynamic_deformed") == 0; }

static eastl::tuple<bool, uint32_t> hold_camo_tex(bvh::ContextId bvh_context_id, TEXTUREID camoTexture,
  bool forceRefreshSrvsWhenLoaded)
{
  auto tql = get_managed_res_cur_tql(camoTexture);
  if (tql != TQL_stub)
  {
    uint32_t camoTexBindless = MeshMeta::INVALID_TEXTURE;
    if (camoTexture != BAD_TEXTUREID)
    {
      auto iter = bvh_context_id->camoTextures.find((uint32_t)camoTexture);
      if (iter == bvh_context_id->camoTextures.end())
      {
        uint32_t texIndex = MeshMeta::INVALID_TEXTURE;
        bvh_context_id->holdTexture(camoTexture, texIndex, forceRefreshSrvsWhenLoaded);
        camoTexBindless = texIndex;
        bvh_context_id->camoTextures.emplace((uint32_t)camoTexture, camoTexBindless);
      }
      else
        camoTexBindless = iter->second;

      return {true, camoTexBindless};
    }
  }
  return {false, MeshMeta::INVALID_TEXTURE};
}

static int hold_decal_data(bvh::ContextId bvh_context_id, Sbuffer *decal_buffer)
{
  static constexpr int elemDwordCount = sizeof(PlanarDecalsParamsSet) / 4;
  static constexpr int upsize = 32;

  auto iter = bvh_context_id->decalDataHolderMap.find(decal_buffer);
  if (iter != bvh_context_id->decalDataHolderMap.end())
    return iter->second;

  int oldSize = bvh_context_id->decalDataHolder ? bvh_context_id->decalDataHolder->getNumElements() / elemDwordCount : 0;
  if (!oldSize || bvh_context_id->decalDataHolderCursor >= oldSize)
  {
    int newSize = oldSize + upsize;

    UniqueBVHBuffer newBuffer;
    newBuffer.reset(
      d3d::buffers::create_persistent_sr_byte_address(newSize * elemDwordCount, "bvh_decal_data", d3d::buffers::Init::No, RESTAG_BVH));

    if (bvh_context_id->decalDataHolder)
      bvh_context_id->decalDataHolder->copyTo(newBuffer.get());

    bvh_context_id->decalDataHolder.swap(newBuffer);

    if (!bvh_context_id->decalDataHolderBindlessSlot)
      bvh_context_id->decalDataHolderBindlessSlot = d3d::allocate_bindless_resource_range(D3DResourceType::SBUF, 1);
    d3d::update_bindless_resource(D3DResourceType::SBUF, bvh_context_id->decalDataHolderBindlessSlot,
      bvh_context_id->decalDataHolder.get());
  }

  decal_buffer->copyTo(bvh_context_id->decalDataHolder.get(), bvh_context_id->decalDataHolderCursor * elemDwordCount * 4, 0,
    sizeof(PlanarDecalsParamsSet));

  int index = bvh_context_id->decalDataHolderCursor;
  ++bvh_context_id->decalDataHolderCursor;
  bvh_context_id->decalDataHolderMap.insert({decal_buffer, index});
  return index;
}

static int hold_initial_node_tm(mat44f_cref initial_node_tm, bvh::ContextId bvh_context_id)
{
  mat43f t;
  v_mat44_transpose_to_mat43(t, initial_node_tm);
  bvh_context_id->initialNodes.push_back(t);
  return bvh_context_id->initialNodes.size() - 1;
}

// The compiler breaks if this function is inlined. Cause shadows to flicker as in 6-95028 in WT_Nightly
DAGOR_NOINLINE
void pos_fix(bool relative_to_camera, TMatrix &tm, const DynamicRenderableSceneInstance &inst, IterCtx *ctx)
{
  if (!relative_to_camera)
    tm.col[3] += inst.getOrigin() - ctx->viewPosition;
}

static void iterate_instances(dynrend::ContextId dynrend_context_id, const DynamicRenderableSceneResource &res,
  const DynamicRenderableSceneInstance &inst, bool relative_to_camera, const dynrend::AddedPerInstanceRenderData &inst_render_data,
  const Tab<bool> &all_visibility, int visibility_index, float min_elem_radius, int node_offset_render_data,
  int instance_offset_render_data, const dynrend::InitialNodes *initial_nodes, int rigid_chunk_size, void *user_data)
{
  IterCtx *ctx = (IterCtx *)user_data;
  ContextId bvh_context_id = ctx->contextId;

  auto bvhId = inst.getLodsResource()->getBvhId();
  if (!bvhId)
  {
    // This might be a clone. Find the original resource.
    if (auto originalResource = inst.getLodsResource()->getFirstOriginal())
      bvhId = originalResource->getBvhId();
    if (!bvhId)
      return;
  }

  ++ctx->count;

  auto &instances = bvh_context_id->dynrendInstances[dynrend_context_id];

  auto nodeOffsetRenderData = node_offset_render_data;
  auto rigidCount = res.getRigidsConst().size();
  auto lodNo = inst.getCurrentLodNo();

  PerInstanceData camoData = PerInstanceData::ZERO;
  PerInstanceData skinData = PerInstanceData::ZERO;
  PerInstanceData seedData = PerInstanceData::ZERO;
  PerInstanceData colorModData = PerInstanceData::ZERO;

  auto getCamoData = [&](const ShaderMesh::RElem &elem) -> PerInstanceData * {
    auto camoTexture = elem.mat->get_texture(1);
    auto [camoValid, camoTexBindless] = hold_camo_tex(bvh_context_id, camoTexture, true);
    uint32_t camoSkinTexBindless = MeshMeta::INVALID_TEXTURE;
    if (camoValid)
    {
      static int camo_skin_texVarId = get_shader_variable_id("camo_skin_tex", true);
      TEXTUREID camoSkinTex;
      if (elem.mat->getTextureVariable(camo_skin_texVarId, camoSkinTex))
        camoSkinTexBindless = eastl::get<1>(hold_camo_tex(bvh_context_id, camoSkinTex, false));

      auto data = inst_render_data.params.data();

      float d1z = 0, d1w = 0, d0z = 0, d0w = 0;
      if (DAGOR_LIKELY(!inst_render_data.params.empty()))
      {
        // d1z and d1w are in the range of 0.000244 to 0.002441. That will just be lost in 8 bits.
        // So we normalize them values by a factor of 400, so we get a number in the range of 0.0 to 1.0.
        // This also make the TC calculation more preceise than it was on 16 bits.
        d1z = clamp(data[1].z * 400, -1.0f, 1.0f);
        d1w = clamp(data[1].w * 400, -1.0f, 1.0f);
        d0z = clamp(data[0].z, 0.0f, 1.0f);
        d0w = clamp(data[0].w, 0.0f, 1.0f);
      }

      if (bvh_decals)
      {
        static const ShaderVariableInfo planar_decal_countVarId("planar_decal_count", true);

        for (auto &interval : inst_render_data.intervals)
        {
          if (interval.varId == planar_decal_countVarId.get_var_id())
          {
            auto decalCount = interval.setValue;
            auto decalConstants = inst_render_data.constDataBuf;

            if (decalCount > 0)
            {
              // It has decals.
              // The buffer is copied into a buffer managed by the BVH, and the count packed with the index.

              uint32_t dataSlot = hold_decal_data(bvh_context_id, decalConstants);

              G_ASSERT(decalCount < (1 << 3)); // 3 bits
              G_ASSERT(dataSlot < (1 << 13));  // 13 bits

              camoData.z = (dataSlot << 3) | decalCount; // Upper 16 bits can be used!
            }

            break;
          }
        }
      }

      camoData.x = camoTexBindless << 16;
      camoData.x |= camoSkinTexBindless;
      camoData.y = uint32_t(d0z * 255) << 8 | uint32_t(d0w * 255);
      camoData.y |= uint32_t((d1z * 0.5f + 0.5f) * 0xFF) << 24 | uint32_t((d1w * 0.5f + 0.5f) * 0xFF) << 16;
      return &camoData;
    }

    return nullptr;
  };
  auto getSkinData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> PerInstanceData * {
    auto skinTexture = elem.mat->get_texture(0);
    if (skinTexture != BAD_TEXTUREID && skinTexture != mesh.albedoTextureId)
    {
      auto [skinValid, skinTexBindless] = hold_camo_tex(bvh_context_id, skinTexture, false);
      if (skinValid)
      {
        // Totally random numbers, indicating that this per instance data is replacing the albedo texture;
        skinData.x = 0xA1B3D0U; // ALBEDO ;)
        skinData.y = skinTexBindless;
        return &skinData;
      }
    }

    return nullptr;
  };
  auto getSeedData = [&](bool heightLocked) -> PerInstanceData * {
    if (inst_render_data.params.size() > 1)
    {
      uint32_t seed = bitwise_cast<uint32_t>(inst_render_data.params[1].w);
      if (seed)
        seedData = PerInstanceData{seed, 0, 0, 0};
      else
      {
        Point4 randomFloatVec;
        v_stu(&randomFloatVec, ri::random_float_from_pos_for_palette(v_ldu(&inst_render_data.params[1].x), heightLocked));
        seedData = PerInstanceData{bitwise_cast<uint32_t>(randomFloatVec.x), 1, 0, 0};
      }

      return &seedData;
    }

    return nullptr;
  };
  auto getColorModData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> PerInstanceData * {
    if (mesh.hasColorMod)
    {
      Color4 solid_fill_color;
      float color_multiplier;
      static int solid_fill_colorVarId = get_shader_variable_id("solid_fill_color");
      static int color_multiplierVarId = get_shader_variable_id("color_multiplier");
      if (elem.mat->getColor4Variable(solid_fill_colorVarId, solid_fill_color) &&
          elem.mat->getRealVariable(color_multiplierVarId, color_multiplier))
      {
        colorModData.x =
          E3DCOLOR(solid_fill_color.r * 255, solid_fill_color.g * 255, solid_fill_color.b * 255, solid_fill_color.a * 255);
        colorModData.y = color_multiplier * 255;
        return &colorModData;
      }
    }

    return nullptr;
  };

  PerInstanceData *camoDataPtr = nullptr;
  PerInstanceData *skinDataPtr = nullptr;
  PerInstanceData *seedDataPtr = nullptr;
  PerInstanceData *colorModDataPtr = nullptr;
  bool lastPaintHeightLocked = false;

  auto getpiDataPtr = [&](const Mesh &mesh, const ShaderMesh::RElem &elem, int node_id) -> PerInstanceData * {
    if (mesh.materialType & MeshMeta::bvhMaterialCamo)
    {
      if (!camoDataPtr)
        camoDataPtr = getCamoData(elem);
      if (camoDataPtr && bvh_decals)
      {
        camoDataPtr->z &= 0xFFFF;
        if (initial_nodes)
          camoDataPtr->z |= (hold_initial_node_tm(initial_nodes->nodesModelTm[node_id], bvh_context_id) + 1) << 16; // Index is one
                                                                                                                    // based. Subtract
                                                                                                                    // one in shader.
      }
      return camoDataPtr;
    }
    else if (mesh.materialType & MeshMeta::bvhMaterialPainted)
    {
      if (!seedDataPtr || (lastPaintHeightLocked != mesh.isPaintedHeightLocked))
      {
        seedDataPtr = getSeedData(mesh.isPaintedHeightLocked);
        lastPaintHeightLocked = mesh.isPaintedHeightLocked;
      }
      return seedDataPtr;
    }
    else if (mesh.hasColorMod)
    {
      if (!colorModDataPtr)
        colorModDataPtr = getColorModData(mesh, elem);
      return colorModDataPtr;
    }
    else
    {
      if (!skinDataPtr)
        skinDataPtr = getSkinData(mesh, elem);
      return skinDataPtr;
    }
  };

  dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> rigids = res.getRigidsConst();
  res.getMeshes(
    [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
      if (!mesh)
        return;

      if (!all_visibility[visibility_index + node_id] || radius < min_elem_radius)
        return;

      nodeOffsetRenderData += rigid_chunk_size;

      if (!bvh_context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull))
        return;

      if (rigids[rigid_no].uniqueRefId > 0)
        rigid_no = (rigids[rigid_no].uniqueRefId & 0xffff) - 1;

      auto elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      if (elems.empty())
        return;

      auto tm = inst.getNodeWtmRelToOrigin(node_id);

      pos_fix(relative_to_camera, tm, inst, ctx);

#if DAGOR_DBGLEVEL > 0
      if (bvh_dyn_range_enable)
      {
        auto distSq = lengthSq(tm.col[3]);
        if (sqr(bvh_dyn_range) < distSq)
          return;
      }
#endif

      mat44f tm44;
      mat43f tm43;
      v_mat44_make_from_43cu(tm44, tm.array);
      v_mat44_transpose_to_mat43(tm43, tm44);

      // The VN RT Validation layer emits a warning if the scale is zero.
      // It is enough to check one axis only. Having an instance with 0 scale on X but not on
      // YZ is unlikely.
      if (v_check_xyz_all_true(v_cmp_eq(tm43.row0, v_zero())))
        return;

      for (auto [elemIx, elem] : enumerate(elems))
      {
        bool hasIndices;
        if (!elem.vertexData->isRenderable(hasIndices))
          continue;

        auto meshId = make_relem_mesh_id(bvhId, lodNo, make_dyn_elem_id(rigid_no, elemIx));

        auto bvhObject = get_object(bvh_context_id, meshId);
        if (!bvhObject || bvhObject->meshes.size() != 1)
          return;

        auto &bvhMesh = bvhObject->meshes[0];

        auto piDataPtr = getpiDataPtr(bvhMesh, elem, node_id);

        if (DAGOR_UNLIKELY(bvhMesh.isHeliRotor))
        {
          auto &data = bvh_context_id->uniqueHeliRotorBuffers[inst.getUniqueId()][meshId];

          MeshHeliRotorInfo heliRotorInfo;
          heliRotorInfo.transformedBuffer = &data.buffer;
          heliRotorInfo.transformedBlas = &data.blas;
          heliRotorInfo.invWorldTm = TMatrix4(inverse(tm)).transpose();
          heliRotorInfo.getParamsFn = [=](Point4 &params, Point4 &sec_params) {
            auto allParams = inst_render_data.params.data();
            params = allParams[0];
            sec_params = allParams[1];
          };

          if (data.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
            data.metaAllocId = bvh_context_id->allocateMetaRegion(1);

          add_dynrend_instance(bvh_context_id, instances, meshId, tm43, piDataPtr, ctx->noShadow,
            Context::Instance::AnimationUpdateMode::DO_CULLING, heliRotorInfo, data.metaAllocId);
        }
        else if (DAGOR_UNLIKELY(is_deformed(elem)))
        {
          static ShaderVariableInfo maxHeightVar = ShaderVariableInfo("max_height", true);
          static ShaderVariableInfo springbackVar = ShaderVariableInfo("springback", true);

          auto &data = bvh_context_id->uniqueDeformedBuffers[inst.getUniqueId()][meshId];

          DeformedInfo deformedInfo;
          deformedInfo.transformedBuffer = &data.buffer;
          deformedInfo.transformedBlas = &data.blas;
          deformedInfo.invWorldTm = TMatrix4(inverse(tm)).transpose();
          deformedInfo.simParams = Point2::ZERO;

          if (!elem.mat->getRealVariable(maxHeightVar.get_var_id(), deformedInfo.simParams.x))
            deformedInfo.simParams.x = 0;

          if (!elem.mat->getRealVariable(springbackVar.get_var_id(), deformedInfo.simParams.y))
            deformedInfo.simParams.y = 0;

          deformedInfo.getParamsFn = [=](float &deform_id, Point2 &sim_params) {
            auto allParams = inst_render_data.params.data();
            deform_id = allParams[2].x;
            sim_params = deformedInfo.simParams;
          };

          if (data.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
            data.metaAllocId = bvh_context_id->allocateMetaRegion(1);

          add_dynrend_instance(bvh_context_id, instances, meshId, tm43, piDataPtr, ctx->noShadow,
            Context::Instance::AnimationUpdateMode::DO_CULLING, deformedInfo, data.metaAllocId);
        }
        else
          add_dynrend_instance(instances, meshId, tm43, piDataPtr, ctx->noShadow, Context::Instance::AnimationUpdateMode::DO_CULLING);
      }
    },
    [&](const ShaderSkinnedMesh *mesh, int node_id, int skin_no) {
      if (!all_visibility[visibility_index + node_id])
        return;

      auto elems = mesh->getShaderMesh().getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      if (!elems.empty() && bvh_context_id->has(Features::DynrendSkinnedFull))
      {
        auto tm = inst.getNodeWtmRelToOrigin(0);

        pos_fix(relative_to_camera, tm, inst, ctx);

#if DAGOR_DBGLEVEL > 0
        if (bvh_dyn_range_enable)
        {
          auto distSq = lengthSq(tm.col[3]);
          if (sqr(bvh_dyn_range) < distSq)
            return;
        }
#endif

        Context::Instance::AnimationUpdateMode animMode = Context::Instance::AnimationUpdateMode::DO_CULLING;
        auto distSq = lengthSq(tm.col[3]);
        if (distSq < sqr(bvh_force_anim_distance))
          animMode = Context::Instance::AnimationUpdateMode::FORCE_ON;

        mat44f tm44;
        mat43f tm43;
        v_mat44_make_from_43cu(tm44, tm.array);
        v_mat44_transpose_to_mat43(tm43, tm44);

        MeshSkinningInfo skinningInfo;
        skinningInfo.invWorldTm = TMatrix4(inverse(tm)).transpose();
        skinningInfo.setTransformsFn = [=]() {
          dynrend::set_instance_data_buffer(STAGE_CS, dynrend_context_id, nodeOffsetRenderData, instance_offset_render_data);
        };

        for (auto [elemIx, elem] : enumerate(elems))
        {
          bool hasIndices;
          if (!elem.vertexData->isRenderable(hasIndices))
            continue;

          auto meshId = make_relem_mesh_id(bvhId, lodNo, make_dyn_elem_id(skin_no + rigidCount, elemIx));

          auto bvhObject = get_object(bvh_context_id, meshId);
          if (!bvhObject || bvhObject->meshes.size() != 1)
            return;

          auto &bvhMesh = bvhObject->meshes[0];

          auto &data = bvh_context_id->uniqueSkinBuffers[inst.getUniqueId()];
          data.age = -1;
          auto &meshElem = data.elems[meshId];

          if (!meshElem.buffer)
          {
            if (auto iter = bvh_context_id->freeUniqueSkinBLASes.find(meshId); iter != bvh_context_id->freeUniqueSkinBLASes.end())
            {
              if (int index = iter->second.cursor.sub_fetch(1); index >= 0)
              {
                auto &blas = iter->second.blases[index];
                meshElem.blas.swap(blas);
              }
            }
          }

          skinningInfo.skinningBuffer = &meshElem.buffer;
          skinningInfo.skinningBlas = &meshElem.blas;

          if (meshElem.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
            meshElem.metaAllocId = bvh_context_id->allocateMetaRegion(1);

          // Tank track animation
          static int texcoord_animVarId = get_shader_variable_id("texcoord_anim", true);

          float texcoord_anim = 0;
          if (elem.mat->getRealVariable(texcoord_animVarId, texcoord_anim))
          {
            Point2 tcAnim = max(Point2(texcoord_anim, -texcoord_anim), Point2::ZERO);
            Point2 trackPos = Point2::xy(inst_render_data.params.data()[0]);
            auto &meta = bvh_context_id->meshMetaAllocator.get(meshElem.metaAllocId)[0];
            meta.texcoordAdd = uint32_t(float_to_half(dot(tcAnim, trackPos))) << 16;
            meta.materialType |= MeshMeta::bvhMaterialTexcoordAdd;
          }

          auto piDataPtr = getpiDataPtr(bvhMesh, elem, node_id);
          add_dynrend_instance(bvh_context_id, instances, meshId, tm43, piDataPtr, ctx->noShadow, animMode, skinningInfo,
            meshElem.metaAllocId);
        }
      }

      nodeOffsetRenderData += 9 + mesh->bonesCount() * 6;
    });
}

struct DynrendBVHJob : public cpujobs::IJob
{
  ContextId bvhContextId;
  dynrend::ContextId dynrendContextId;
  dynrend::ContextId dynrendNoShadowContextId;
  Point3 viewPosition;

  void start(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id, dynrend::ContextId dynrend_no_shadow_context_id,
    const Point3 &view_position)
  {
    bvhContextId = bvh_context_id;
    dynrendContextId = dynrend_context_id;
    dynrendNoShadowContextId = dynrend_no_shadow_context_id;
    viewPosition = view_position;
    threadpool::add(this, threadpool::PRIO_HIGH);
  }

  void wait()
  {
    TIME_PROFILE(Wait_DynrendBVHJob);
    threadpool::wait(this);
  }

  void doJob() override
  {
    {
      bvhContextId->decalDataHolderMap.clear();
      bvhContextId->decalDataHolderCursor = 0;
      bvhContextId->initialNodes.clear();

      TIME_PROFILE(bvh::update_dynrend_instances);
      IterCtx ctx = {bvhContextId, viewPosition, false, 0, dynrendContextId};

      dynrend::iterate_instances(dynrendContextId, iterate_instances, &ctx);
      DA_PROFILE_TAG(bvh::update_dynrend_instances, "count: %d", ctx.count);

      if (bvhContextId->decalDataHolder)
        d3d::resource_barrier(ResourceBarrierDesc(bvhContextId->decalDataHolder.get(), RB_RO_SRV | RB_STAGE_ALL_SHADERS));

      if (!bvhContextId->initialNodes.empty())
      {
        static constexpr int elemDwordCount = sizeof(mat43f) / 4;
        static constexpr int upsize = 32;

        int currentSize = bvhContextId->initialNodesHolder ? bvhContextId->initialNodesHolder->getNumElements() / elemDwordCount : 0;
        if (bvhContextId->initialNodes.size() >= currentSize)
        {
          int newSize = bvhContextId->initialNodes.size() + upsize;
          bvhContextId->initialNodesHolder.reset(d3d::buffers::create_persistent_sr_byte_address(newSize * elemDwordCount,
            "bvh_initial_nodes", d3d::buffers::Init::No, RESTAG_BVH));

          if (!bvhContextId->initialNodesHolderBindlessSlot)
            bvhContextId->initialNodesHolderBindlessSlot = d3d::allocate_bindless_resource_range(D3DResourceType::SBUF, 1);
          d3d::update_bindless_resource(D3DResourceType::SBUF, bvhContextId->initialNodesHolderBindlessSlot,
            bvhContextId->initialNodesHolder.get());
        }

        bvhContextId->initialNodesHolder->updateDataWithLock(0, bvhContextId->initialNodes.size() * sizeof(mat43f),
          bvhContextId->initialNodes.data(), VBLOCK_WRITEONLY);
        d3d::resource_barrier(ResourceBarrierDesc(bvhContextId->initialNodesHolder.get(), RB_RO_SRV | RB_STAGE_ALL_SHADERS));
      }
    }

    {
      TIME_PROFILE(bvh::update_dynrend_instances_shadow);
      IterCtx ctx = {bvhContextId, viewPosition, true, 0, dynrendNoShadowContextId};
      dynrend::iterate_instances(dynrendNoShadowContextId, iterate_instances, &ctx);
      DA_PROFILE_TAG(bvh::update_dynrend_instances_shadow, "count: %d", ctx.count);
    }
  }

  const char *getJobName(bool &) const override { return "DynrendBVHJob"; }
} dynrend_bvh_job;

void update_dynrend_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position)
{
  if (!bvh_context_id->has(Features::AnyDynrend))
    return;

  static int bvh_decalsVarId = get_shader_variable_id("bvh_decals", true);
  ShaderGlobal::set_int(bvh_decalsVarId, bvh_decals ? 1 : 0);

  dynrend_bvh_job.start(bvh_context_id, dynrend_context_id, dynrend_no_shadow_context_id, view_position);
}

void wait_dynrend_instances() { dynrend_bvh_job.wait(); }

static void iterate_instances_dng(const DynamicRenderableSceneInstance &inst, const DynamicRenderableSceneResource &res,
  const uint8_t *path_filter, uint32_t path_filter_size, uint8_t render_mask, dag::ConstSpan<int> offsets,
  dynrend::BVHSetInstanceData set_instance_data, bool animate, dynrend::BVHCamoData &camo_data, void *user_data)
{
  IterCtx *ctx = (IterCtx *)user_data;
  ContextId bvh_context_id = ctx->contextId;
  dynrend::ContextId dynrend_context_id = ctx->dynrendContextId;
  Context::Instance::AnimationUpdateMode animationUpdateMode =
    animate ? Context::Instance::AnimationUpdateMode::FORCE_ON : Context::Instance::AnimationUpdateMode::FORCE_OFF;

  auto bvhId = inst.getLodsResource()->getBvhId();
  if (!bvhId)
  {
    // This might be a clone. Find the original resource.
    if (auto originalResource = inst.getLodsResource()->getFirstOriginal())
      bvhId = originalResource->getBvhId();
    if (!bvhId)
      return;
  }

  auto &instances = bvh_context_id->dynrendInstances[dynrend_context_id];

  auto lodNo = inst.getCurrentLodNo();
  auto rigidCount = res.getRigidsConst().size();
  auto meshIndexCounter = 0;

  PerInstanceData camoData;
  PerInstanceData *camoDataPtr = nullptr;
  auto getCamoData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> const PerInstanceData * {
    if (mesh.materialType & MeshMeta::bvhMaterialCamo)
    {
      // Cache the result of the first calculation, it is going to be the same for all
      if (camoDataPtr)
        return camoDataPtr;

      static int burnt_tankVarId = get_shader_variable_id("burnt_tank", true);
      int burnt_tank = 0;
      bool isBurnt = elem.mat->getIntVariable(burnt_tankVarId, burnt_tank) && burnt_tank > 0;
      auto camoTexture = isBurnt ? camo_data.burntCamo : elem.mat->get_texture(1);
      auto [camoValid, camoTexBindless] = hold_camo_tex(bvh_context_id, camoTexture, true);
      camoData.x = (camoTexBindless & 0xFFFF0000U);
      if (camoValid)
      {
        float condition = clamp<float>(camo_data.condition, 1.0f / 255, 1.0f);
        float rotation = clamp<float>(camo_data.rotation, -M_PI, M_PI) / M_PI * 0.5 + 0.5;
        constexpr float MAX_SCALE = 2;
        float scale = clamp<float>(camo_data.scale, 0, MAX_SCALE) / MAX_SCALE;

        uint32_t camoSkinTexBindless = 0xFFFFFFFFU;
        if (!isBurnt)
        {
          static int camo_skin_texVarId = get_shader_variable_id("camo_skin_tex", true);
          TEXTUREID camo_skin_tex;
          if (elem.mat->getTextureVariable(camo_skin_texVarId, camo_skin_tex))
          {
            auto [_, bindless] = hold_camo_tex(bvh_context_id, camo_skin_tex, false);
            camoSkinTexBindless = bindless;
          }
        }

        constexpr uint32_t TWELVE_BIT_MASK = (1U << 12) - 1;
        camoData.x |= (camoSkinTexBindless >> 16) & 0xFFFFU;
        camoData.y = uint32_t(condition * 0xFF) << 24 | uint32_t(rotation * TWELVE_BIT_MASK) << 12 | uint32_t(scale * TWELVE_BIT_MASK);
      }

      camoDataPtr = &camoData;
      return camoDataPtr;
    }

    return nullptr;
  };

  PerInstanceData colorModData;
  auto getColorModData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> PerInstanceData * {
    if (mesh.hasColorMod)
    {
      Color4 solid_fill_color;
      float color_multiplier;
      static int solid_fill_colorVarId = get_shader_variable_id("solid_fill_color");
      static int color_multiplierVarId = get_shader_variable_id("color_multiplier");
      if (elem.mat->getColor4Variable(solid_fill_colorVarId, solid_fill_color) &&
          elem.mat->getRealVariable(color_multiplierVarId, color_multiplier))
      {
        colorModData.x =
          E3DCOLOR(solid_fill_color.r * 255, solid_fill_color.g * 255, solid_fill_color.b * 255, solid_fill_color.a * 255);
        colorModData.y = color_multiplier * 255;
        return &colorModData;
      }
    }

    return nullptr;
  };

  res.getMeshes(
    [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
      G_UNUSED(radius);
      bool isVisible = ((!path_filter && !inst.isNodeHidden(node_id)) ||
                        (path_filter && (node_id >= path_filter_size || (path_filter[node_id] & render_mask) == render_mask)));
      if (!isVisible)
        return;
      int currMeshIndex = meshIndexCounter++;
      if (!bvh_context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull))
        return;

      auto elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      if (elems.empty())
        return;

      auto tm = inst.getNodeWtm(node_id); // Camera relative already!


#if DAGOR_DBGLEVEL > 0
      if (bvh_dyn_range_enable)
      {
        auto distSq = lengthSq(tm.col[3]);
        if (sqr(bvh_dyn_range) < distSq)
          return;
      }
#endif

      mat44f tm44;
      mat43f tm43;
      v_mat44_make_from_43cu(tm44, tm.array);
      v_mat44_transpose_to_mat43(tm43, tm44);

      // The VN RT Validation layer emits a warning if the scale is zero.
      // It is enough to check one axis only. Having an instance with 0 scale on X but not on
      // YZ is unlikely.
      if (v_check_xyz_all_true(v_cmp_eq(tm43.row0, v_zero())))
        return;

      for (auto [elemIx, elem] : enumerate(elems))
      {
        bool hasIndices;
        if (!elem.vertexData->isRenderable(hasIndices))
          continue;

        auto meshId = make_relem_mesh_id(bvhId, lodNo, make_dyn_elem_id(rigid_no, elemIx));

        auto bvhObject = get_object(bvh_context_id, meshId);
        if (!bvhObject || bvhObject->meshes.size() != 1)
          return;
        auto &bvhMesh = bvhObject->meshes[0];

        PerInstanceData perInstanceDataWithIndex = PerInstanceData::ZERO;
        const PerInstanceData *perInstanceDataPtr = getCamoData(bvhMesh, elem);
        if (bvhMesh.materialType & MeshMeta::bvhMaterialAnimcharDecals)
        {
          if (perInstanceDataPtr)
            perInstanceDataWithIndex = *perInstanceDataPtr;
          perInstanceDataWithIndex.z = offsets[currMeshIndex];
          perInstanceDataPtr = &perInstanceDataWithIndex;
        }

        add_dynrend_instance(instances, meshId, tm43, perInstanceDataPtr, ctx->noShadow, animationUpdateMode);
      }
    },
    [&](const ShaderSkinnedMesh *mesh, int node_id, int skin_no) {
      bool isVisible = ((!path_filter && !inst.isNodeHidden(node_id)) ||
                        (path_filter && (node_id >= path_filter_size || (path_filter[node_id] & render_mask) == render_mask)));
      if (!isVisible)
        return;
      int currMeshIndex = meshIndexCounter++;
      if (!bvh_context_id->has(Features::DynrendSkinnedFull))
        return;

      auto elems = mesh->getShaderMesh().getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);
      if (elems.empty())
        return;

      auto tm = inst.getNodeWtmRelToOrigin(0); // Camera relative already!

#if DAGOR_DBGLEVEL > 0
      if (bvh_dyn_range_enable)
      {
        auto distSq = lengthSq(tm.col[3]);
        if (sqr(bvh_dyn_range) < distSq)
          return;
      }
#endif

      mat44f tm44;
      mat43f tm43;
      v_mat44_make_from_43cu(tm44, tm.array);
      v_mat44_transpose_to_mat43(tm43, tm44);

      MeshSkinningInfo skinningInfo;
      float det = tm.det();
      if (fabsf(det) <= 1e-12f)
        return;
      skinningInfo.invWorldTm = TMatrix4(inverse(tm, det)).transpose();
      skinningInfo.setTransformsFn = [instance_offset = offsets[currMeshIndex], set_instance_data]() {
        set_instance_data(instance_offset);
      };

      for (auto [elemIx, elem] : enumerate(elems))
      {
        bool hasIndices;
        if (!elem.vertexData->isRenderable(hasIndices))
          continue;

        auto meshId = make_relem_mesh_id(bvhId, lodNo, make_dyn_elem_id(skin_no + rigidCount, elemIx));

        auto bvhObject = get_object(bvh_context_id, meshId);
        if (!bvhObject || bvhObject->meshes.size() != 1)
          return;

        auto &bvhMesh = bvhObject->meshes[0];

        auto &data = bvh_context_id->uniqueSkinBuffers[inst.getUniqueId()];
        data.age = -1;
        auto &meshElem = data.elems[meshId];

        if (!meshElem.buffer)
        {
          if (auto iter = bvh_context_id->freeUniqueSkinBLASes.find(meshId); iter != bvh_context_id->freeUniqueSkinBLASes.end())
          {
            if (int index = iter->second.cursor.sub_fetch(1); index >= 0)
            {
              auto &blas = iter->second.blases[index];
              meshElem.blas.swap(blas);
            }
          }
        }

        skinningInfo.skinningBuffer = &meshElem.buffer;
        skinningInfo.skinningBlas = &meshElem.blas;

        if (meshElem.metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
        {
          meshElem.metaAllocId = bvh_context_id->allocateMetaRegion(1);
          auto &meta = bvh_context_id->meshMetaAllocator.get(meshElem.metaAllocId)[0];
          meta = bvh_context_id->meshMetaAllocator.get(bvhObject->metaAllocId)[0];
          auto albedoTex = elem.mat->get_texture(0);
          meta.holdAlbedoTex(bvh_context_id, albedoTex);
          auto alphaTex = elem.mat->get_texture(1);
          if (alphaTex == BAD_TEXTUREID)
            alphaTex = albedoTex;
          meta.holdAlphaTex(bvh_context_id, alphaTex);
          meta.holdNormalTex(bvh_context_id, elem.mat->get_texture(2));
          if (bvhMesh.hasColorMod)
          {
            static int camouflage_texVarId = get_shader_variable_id("camouflage_tex");
            TEXTUREID camouflage_tex;
            elem.mat->getTextureVariable(camouflage_texVarId, camouflage_tex);
            meta.holdExtraTex(bvh_context_id, camouflage_tex);
          }
          meta.materialType |= MeshMeta::bvhMaterialUseInstanceTextures;
          meta.markInitialized();
        }

        PerInstanceData perInstanceDataWithIndex = PerInstanceData::ZERO;
        const PerInstanceData *perInstanceDataPtr = getCamoData(bvhMesh, elem);
        if (!perInstanceDataPtr)
          perInstanceDataPtr = getColorModData(bvhMesh, elem);
        if (bvhMesh.materialType & MeshMeta::bvhMaterialAnimcharDecals)
        {
          if (perInstanceDataPtr)
            perInstanceDataWithIndex = *perInstanceDataPtr;
          perInstanceDataWithIndex.z = offsets[currMeshIndex];
          perInstanceDataPtr = &perInstanceDataWithIndex;
        }

        add_dynrend_instance(bvh_context_id, instances, meshId, tm43, perInstanceDataPtr, ctx->noShadow, animationUpdateMode,
          skinningInfo, meshElem.metaAllocId);
      }
    });
  G_VERIFY(offsets.size() == meshIndexCounter);
}

void update_animchar_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position, dynrend::BVHIterateCallback iterate_callback)
{
  G_UNUSED(dynrend_no_shadow_context_id); // Currently not supported
  if (!bvh_context_id->has(Features::AnyDynrend))
    return;

  TIME_D3D_PROFILE(bvh::update_animchar_instances);

  IterCtx ctx = {bvh_context_id, view_position, false, 0, dynrend_context_id};
  iterate_callback(iterate_instances_dng, view_position, &ctx);

  DA_PROFILE_TAG(bvh::update_animchar_instances, "count: %d", ctx.count);
}

void set_up_dynrend_context_for_processing(dynrend::ContextId dynrend_context_id)
{
  dynrend::update_reprojection_data(dynrend_context_id);
}

static struct TidyUpSkinsJob : public cpujobs::IJob
{
  ContextId contextId;

  const char *getJobName(bool &) const override { return "TidyUpSkinsJob"; }

  void doJob() override
  {
    WinAutoLock lock(contextId->tidyUpSkinsLock);

    // We make use of the fact that resize does not shrink the vector itself, only the elem count is changed
    for (auto &freeBLAS : contextId->freeUniqueSkinBLASes)
    {
      if (freeBLAS.second.cursor.load() < 0)
        freeBLAS.second.cursor.store(0);
      freeBLAS.second.blases.resize(freeBLAS.second.cursor.load());
    }

    for (auto iter = eastl::begin(contextId->uniqueSkinBuffers); iter != eastl::end(contextId->uniqueSkinBuffers);)
    {
      // Reset the used flags, so in the next frame, all start fresh
      for (auto &elem : iter->second.elems)
        elem.second.used = false;

      iter->second.age++;
      if (iter->second.age < 1)
      {
        ++iter;
        continue;
      }

      for (auto &elem : iter->second.elems)
      {
        auto &storage = contextId->freeUniqueSkinBLASes[elem.first].blases.push_back();
        storage.swap(elem.second.blas);

        if (elem.second.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
        {
          const auto &meta = contextId->meshMetaAllocator.get(elem.second.metaAllocId)[0];
          if (meta.materialType & MeshMeta::bvhMaterialUseInstanceTextures)
          {
            contextId->releaseTexture(meta.albedoTextureIndex);
            contextId->releaseTexture(meta.alphaTextureIndex);
            contextId->releaseTexture(meta.normalTextureIndex);
            contextId->releaseTexture(meta.extraTextureIndex);
          }
        }

        contextId->freeMetaRegion(elem.second.metaAllocId);

        if (elem.second.buffer)
        {
          WinAutoLock lock(contextId->processBufferAllocatorLock);
          contextId->processBufferAllocator[elem.second.buffer.allocator].first.free(elem.second.buffer.allocId);
        }
      }

      iter = contextId->uniqueSkinBuffers.erase(iter);
    }

    for (auto &freeSkins : contextId->freeUniqueSkinBLASes)
      freeSkins.second.cursor.store(freeSkins.second.blases.size());
  }
} tidy_up_skins_job;

void tidy_up_skins(ContextId context_id)
{
  tidy_up_skins_job.contextId = context_id;
  threadpool::add(&tidy_up_skins_job, threadpool::PRIO_HIGH);
}

void wait_tidy_up_skins() { threadpool::wait(&tidy_up_skins_job); }

} // namespace bvh::dyn
