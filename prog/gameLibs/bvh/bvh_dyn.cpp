// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <render/dynmodelRenderer.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <generic/dag_enumerate.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <drv/3d/dag_lock.h>

#include "bvh_context.h"
#include "bvh_tools.h"
#include "bvh_add_instance.h"
#include "bvh_color_from_pos.h"

#if DAGOR_DBGLEVEL > 0
extern bool bvh_dyn_range_enable;
extern float bvh_dyn_range;
#endif

namespace bvh::dyn
{

static eastl::unordered_set<ContextId> relem_changed_contexts;
static CallbackToken relem_changed_token;

void wait_purge_skin_buffers();

static void on_relem_changed(ContextId context_id, const DynamicRenderableSceneLodsResource *resource, bool deleted, int upper_lod)
{
  TIME_PROFILE_NAME(bvh::on_dyn_relem_changed, String(40, "bvh::on_dyn_relem_changed: %s", deleted ? "delete" : "modify"));

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
    auto lodIxLocal = lodIx;

    lod.scene->getMeshes(
      [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
        if (!context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull))
          return;

        if (context_id->has(Features::DynrendRigidBaked) && lodIxLocal < (resource->lods.size() - 1))
          return;

        TIME_PROFILE(rigid);

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
            if (auto meshInfo = process_relem(context_id, elem, nullptr, posMulBatch, posAddBatch, bounding, false))
            {
              bool isAnimated = meshInfo->vertexProcessor && !meshInfo->vertexProcessor->isOneTimeOnly();
              add_object(context_id, objectId, {{eastl::move(meshInfo.value())}, isAnimated});
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
            if (auto meshInfo =
                  process_relem(context_id, elem, &ProcessorInstances::getSkinnedVertexProcessor(), posMul, posAdd, bounding, false))
            {
              bool isAnimated = meshInfo->vertexProcessor && !meshInfo->vertexProcessor->isOneTimeOnly();
              add_object(context_id, objectId, {{eastl::move(meshInfo.value())}, isAnimated});
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

void init() { relem_changed_token = unitedvdata::dmUnitedVdata.on_mesh_relems_updated.subscribe(on_relem_changed_all); }

void teardown() { relem_changed_token = CallbackToken(); }

void init(ContextId context_id)
{
  if (context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull | Features::DynrendSkinnedFull))
  {
    relem_changed_contexts.insert(context_id);
    unitedvdata::dmUnitedVdata.availableRElemsAccessor([](dag::Span<DynamicRenderableSceneLodsResource *> resources) {
      for (DynamicRenderableSceneLodsResource *resource : resources)
        on_relem_changed_all(resource, false, 0);
    });
  }
}

void teardown(ContextId context_id)
{
  on_unload_scene(context_id);
  relem_changed_contexts.erase(context_id);
}

void on_unload_scene(ContextId context_id)
{
  if (!context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull | Features::DynrendSkinnedFull))
    return;

  wait_dynrend_instances();
  wait_purge_skin_buffers();

  for (auto &buffers : context_id->uniqueSkinBuffers)
    for (auto &buffer : buffers.second.elems)
    {
      if (buffer.second.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
      {
        const auto &meta = context_id->meshMetaAllocator.get(buffer.second.metaAllocId)[0];
        if (meta.materialType & MeshMeta::bvhMaterialUseInstanceTextures)
        {
          context_id->releaseTextureFromPackedIndices(meta.albedoTextureIndex);
          context_id->releaseTextureFromPackedIndices(meta.alphaTextureIndex);
          context_id->releaseTextureFromPackedIndices(meta.normalTextureIndex);
          context_id->releaseTextureFromPackedIndices(meta.extraTextureIndex);
        }
      }
      context_id->releaseBuffer(buffer.second.buffer.get());
      context_id->freeMetaRegion(buffer.second.metaAllocId);
    }
  context_id->uniqueSkinBuffers.clear();

  for (auto &buffers : context_id->uniqueHeliRotorBuffers)
    for (auto &buffer : buffers.second)
    {
      context_id->releaseBuffer(buffer.second.buffer.get());
      context_id->freeMetaRegion(buffer.second.metaAllocId);
    }
  context_id->uniqueHeliRotorBuffers.clear();

  for (auto &buffers : context_id->uniqueDeformedBuffers)
    for (auto &buffer : buffers.second)
    {
      context_id->releaseBuffer(buffer.second.buffer.get());
      context_id->freeMetaRegion(buffer.second.metaAllocId);
    }
  context_id->uniqueDeformedBuffers.clear();
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
    uint32_t camoTexBindless = 0xFFFFFFFFU;
    if (camoTexture != BAD_TEXTUREID)
    {
      auto iter = bvh_context_id->camoTextures.find((uint32_t)camoTexture);
      if (iter == bvh_context_id->camoTextures.end())
      {
        uint32_t texIndex = MeshMeta::INVALID_TEXTURE;
        uint32_t smpIndex = MeshMeta::INVALID_SAMPLER;
        bvh_context_id->holdTexture(camoTexture, texIndex, smpIndex, d3d::SamplerHandle::Invalid, forceRefreshSrvsWhenLoaded);
        camoTexBindless = (texIndex << 16) | smpIndex;
        bvh_context_id->camoTextures.emplace((uint32_t)camoTexture, camoTexBindless);
      }
      else
        camoTexBindless = iter->second;

      return {true, camoTexBindless};
    }
  }
  return {false, 0xFFFFFFFFU};
}

static void iterate_instances(dynrend::ContextId dynrend_context_id, const DynamicRenderableSceneResource &res,
  const DynamicRenderableSceneInstance &inst, const dynrend::AddedPerInstanceRenderData &inst_render_data,
  const Tab<bool> &all_visibility, int visibility_index, float min_elem_radius, int node_offset_render_data,
  int instance_offset_render_data, int rigid_chunk_size, void *user_data)
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

  UPoint2 camoData;
  UPoint2 skinData;
  UPoint2 seedData;

  auto getCamoData = [&](const ShaderMesh::RElem &elem) -> const UPoint2 * {
    auto camoTexture = elem.mat->get_texture(1);
    auto [camoValid, camoTexBindless] = hold_camo_tex(bvh_context_id, camoTexture, true);
    uint32_t camoSkinTexBindless = 0xFFFFFFFFU;
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

      camoData.x = camoTexBindless;
      camoData.x &= 0xFFFF0000U;
      camoData.x |= camoSkinTexBindless >> 16;
      camoData.y = uint32_t(d0z * 255) << 8 | uint32_t(d0w * 255);
      camoData.y |= uint32_t((d1z * 0.5f + 0.5f) * 0xFF) << 24 | uint32_t((d1w * 0.5f + 0.5f) * 0xFF) << 16;
      return &camoData;
    }

    return nullptr;
  };
  auto getSkinData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> const UPoint2 * {
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

  auto getSeedData = [&](bool heightLocked) -> const UPoint2 * {
    if (inst_render_data.params.size() > 1)
    {
      uint32_t seed = bitwise_cast<uint32_t>(inst_render_data.params[1].w);
      if (seed)
        seedData = UPoint2{seed, 0};
      else
      {
        Point4 randomFloatVec;
        v_stu(&randomFloatVec, ri::random_float_from_pos_for_palette(v_ldu(&inst_render_data.params[1].x), heightLocked));
        seedData = UPoint2{bitwise_cast<uint32_t>(randomFloatVec.x), 1};
      }

      return &seedData;
    }

    return nullptr;
  };

  const UPoint2 *camoDataPtr = nullptr;
  const UPoint2 *skinDataPtr = nullptr;
  const UPoint2 *seedDataPtr = nullptr;
  bool lastPaintHeightLocked = false;

  auto getpiDataPtr = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> const UPoint2 * {
    if (mesh.materialType & MeshMeta::bvhMaterialCamo)
    {
      if (!camoDataPtr)
        camoDataPtr = getCamoData(elem);
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
    else
    {
      if (!skinDataPtr)
        skinDataPtr = getSkinData(mesh, elem);
      return skinDataPtr;
    }
  };

  res.getMeshes(
    [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
      if (!all_visibility[visibility_index + node_id] || radius < min_elem_radius)
        return;

      nodeOffsetRenderData += rigid_chunk_size;

      if (!bvh_context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull))
        return;

      auto elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      if (elems.empty())
        return;

      auto tm = inst.getNodeWtmRelToOrigin(node_id);

      tm.col[3] += inst.getOrigin() - ctx->viewPosition;

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

        auto piDataPtr = getpiDataPtr(bvhMesh, elem);

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

        tm.col[3] += inst.getOrigin() - ctx->viewPosition;

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

          auto piDataPtr = getpiDataPtr(bvhMesh, elem);
          add_dynrend_instance(bvh_context_id, instances, meshId, tm43, piDataPtr, ctx->noShadow,
            Context::Instance::AnimationUpdateMode::DO_CULLING, skinningInfo, meshElem.metaAllocId);
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
      TIME_PROFILE(bvh::update_dynrend_instances);
      IterCtx ctx = {bvhContextId, viewPosition, false, 0, dynrendContextId};
      dynrend::iterate_instances(dynrendContextId, iterate_instances, &ctx);
      DA_PROFILE_TAG(bvh::update_dynrend_instances, "count: %d", ctx.count);
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
  if (!bvh_context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull | Features::DynrendSkinnedFull))
    return;

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

  UPoint2 camoData;
  UPoint2 *camoDataPtr = nullptr;
  auto getCamoData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> const UPoint2 * {
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
        camoData.x = (camoTexBindless & 0xFFFF0000U) | ((camoSkinTexBindless >> 16) & 0xFFFFU);
        camoData.y = uint32_t(condition * 0xFF) << 24 | uint32_t(rotation * TWELVE_BIT_MASK) << 12 | uint32_t(scale * TWELVE_BIT_MASK);

        camoDataPtr = &camoData;
        return camoDataPtr;
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
      meshIndexCounter++;
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

        auto camoDataPtr = getCamoData(bvhMesh, elem);

        add_dynrend_instance(instances, meshId, tm43, camoDataPtr, ctx->noShadow, animationUpdateMode);
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
          bool isSoldierCamo = strncmp(elem.mat->getShaderClassName(), "dynamic_sheen_camo", 18) == 0;
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
          meta.holdExtraTex(bvh_context_id, isSoldierCamo ? elem.mat->get_texture(4) : BAD_TEXTUREID);
          meta.materialType |= MeshMeta::bvhMaterialUseInstanceTextures;
          meta.markInitialized();
        }

        auto camoDataPtr = getCamoData(bvhMesh, elem);
        add_dynrend_instance(bvh_context_id, instances, meshId, tm43, camoDataPtr, ctx->noShadow, animationUpdateMode, skinningInfo,
          meshElem.metaAllocId);
      }
    });
  G_VERIFY(offsets.size() == meshIndexCounter);
}

void update_animchar_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position, dynrend::BVHIterateCallback iterate_callback)
{
  G_UNUSED(dynrend_no_shadow_context_id); // Currently not supported
  if (!bvh_context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull | Features::DynrendSkinnedFull))
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

static struct PurgeSkinBuffersJob : public cpujobs::IJob
{
  ContextId contextId;

  const char *getJobName(bool &) const override { return "PurgeSkinBuffersJob"; }

  void doJob() override
  {
    WinAutoLock lock(contextId->purgeSkinBuffersLock);

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

        contextId->freeMetaRegion(elem.second.metaAllocId);
        contextId->releaseBuffer(elem.second.buffer.get());
      }

      iter = contextId->uniqueSkinBuffers.erase(iter);
    }

    for (auto &freeSkins : contextId->freeUniqueSkinBLASes)
      freeSkins.second.cursor.store(freeSkins.second.blases.size());
  }
} purge_skin_buffers_job;

void purge_skin_buffers(ContextId context_id)
{
  purge_skin_buffers_job.contextId = context_id;
  threadpool::add(&purge_skin_buffers_job, threadpool::PRIO_HIGH);
}

void wait_purge_skin_buffers() { threadpool::wait(&purge_skin_buffers_job); }

} // namespace bvh::dyn
