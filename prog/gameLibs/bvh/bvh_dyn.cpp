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

#if DAGOR_DBGLEVEL > 0
extern bool bvh_dyn_range_enable;
extern float bvh_dyn_range;
#endif

namespace bvh::dyn
{

static ShaderVariableInfo aircraft_damage_decals_setupVarId("aircraft_damage_decals_setup");

static eastl::unordered_set<ContextId> relem_changed_contexts;
static CallbackToken relem_changed_token;

static void on_relem_changed(ContextId context_id, const DynamicRenderableSceneLodsResource *resource, bool deleted)
{
  TIME_PROFILE_NAME(bvh::on_dyn_relem_changed, String(40, "bvh::on_dyn_relem_changed: %s", deleted ? "delete" : "modify"));

  if (resource->getBvhId() == 0)
    resource->setBvhId(++bvh_id_gen);

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

          auto elemId = make_dyn_elem_id(rigid_no, elemIx);

          if (deleted)
            remove_mesh(context_id, make_relem_mesh_id(bvhId, lodIxLocal, elemId));
          else
          {
            auto isPacked = elem.mat->isPositionPacked();
            auto posMulBatch = isPacked ? posMul : Point4(1.f, 1.f, 1.f, 0.0f);
            auto posAddBatch = isPacked ? posAdd : Point4(0.f, 0.f, 0.f, 0.0f);
            process_relem(context_id, elem, elemId, bvhId, lodIxLocal, nullptr, posMulBatch, posAddBatch, bounding, false);
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
          if (deleted)
            remove_mesh(context_id, make_relem_mesh_id(bvhId, lodIxLocal, elemId));
          else
            process_relem(context_id, elem, elemId, bvhId, lodIxLocal, &ProcessorInstances::getSkinnedVertexProcessor(), posMul,
              posAdd, bounding, false);
        }
      });
  }
}

static void on_relem_changed_all(const DynamicRenderableSceneLodsResource *resource, bool deleted)
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
    on_relem_changed(contextId, resource, deleted);
}

void init() { relem_changed_token = unitedvdata::dmUnitedVdata.on_mesh_relems_updated.subscribe(on_relem_changed_all); }

void teardown() { relem_changed_token = CallbackToken(); }

void init(ContextId context_id)
{
  if (context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull | Features::DynrendSkinnedFull))
  {
    relem_changed_contexts.insert(context_id);
    unitedvdata::dmUnitedVdata.enumRElems(
      [](const DynamicRenderableSceneLodsResource *resource) { on_relem_changed_all(resource, false); });
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

  for (auto &buffers : context_id->uniqueSkinBuffers)
    for (auto &buffer : buffers.second)
    {
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
  bool isPlane;
};

bool has_camo(ShaderMesh::RElem &elem)
{
  return strcmp(elem.mat->getShaderClassName(), "dynamic_masked_tank") == 0 ||
         strcmp(elem.mat->getShaderClassName(), "dynamic_masked_ship") == 0 ||
         strcmp(elem.mat->getShaderClassName(), "dynamic_masked_track") == 0 ||
         strcmp(elem.mat->getShaderClassName(), "dynamic_tank_net_atest") == 0;
}

inline bool is_deformed(ShaderMesh::RElem &elem) { return strcmp(elem.mat->getShaderClassName(), "dynamic_deformed") == 0; }

static void iterate_instances(dynrend::ContextId dynrend_context_id, const DynamicRenderableSceneResource &res,
  const DynamicRenderableSceneInstance &inst, const dynrend::PerInstanceRenderData &inst_render_data, const Tab<bool> &all_visibility,
  int visibility_index, float min_elem_radius, int base_offset_render_data, int rigid_chunk_size, void *user_data)
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

  auto baseOffsetRenderData = base_offset_render_data;
  auto rigidCount = res.getRigidsConst().size();
  auto lodNo = inst.getCurrentLodNo();

  bool isDamaged = false;
  for (auto &interval : inst_render_data.intervals)
    if (interval.varId == aircraft_damage_decals_setupVarId.get_var_id())
      isDamaged = true;

  Point4 camoData;
  auto getCamoData = [&](const Mesh &mesh, const ShaderMesh::RElem &elem) -> const Point4 * {
    if (mesh.materialType & MeshMeta::bvhMaterialCamo)
    {
      auto camoTexture = elem.mat->get_texture(1);
      auto tql = get_managed_res_cur_tql(camoTexture);
      if (tql != TQL_stub)
      {
        uint32_t camoTexBindless = 0xFFFFFFFFU;
        if (camoTexture != BAD_TEXTUREID)
        {
          auto iter = bvh_context_id->camoTextures.find((uint32_t)camoTexture);
          if (iter == bvh_context_id->camoTextures.end())
          {
            bvh_context_id->holdTexture(camoTexture, camoTexBindless, d3d::SamplerHandle::Invalid, true);
            bvh_context_id->camoTextures.emplace((uint32_t)camoTexture, camoTexBindless);
          }
          else
            camoTexBindless = iter->second;
        }

        auto data = inst_render_data.params.data();
        auto pack = (uint32_t)float_to_half(data[1].z) | (float_to_half(data[1].w) << 16);
        camoData.x = data[0].z;
        camoData.y = data[0].w;
        memcpy(&camoData.z, &pack, sizeof(pack));
        memcpy(&camoData.w, &camoTexBindless, sizeof(camoTexBindless));
        return &camoData;
      }
    }
    else
    {
      auto skinTexture = elem.mat->get_texture(0);
      if (skinTexture != BAD_TEXTUREID && skinTexture != mesh.albedoTextureId)
      {
        auto tql = get_managed_res_cur_tql(skinTexture);
        if (tql != TQL_stub)
        {
          uint32_t skinTexBindless = 0xFFFFFFFFU;
          auto iter = bvh_context_id->camoTextures.find((uint32_t)skinTexture);
          if (iter == bvh_context_id->camoTextures.end())
          {
            bvh_context_id->holdTexture(skinTexture, skinTexBindless);
            bvh_context_id->camoTextures.emplace((uint32_t)skinTexture, skinTexBindless);
          }
          else
            skinTexBindless = iter->second;

          memcpy(&camoData.x, &skinTexBindless, sizeof(skinTexBindless));
          // Totally random numbers, indicating that this per instance data is replacing the albedo texture;
          camoData.y = 80;
          camoData.z = 7;
          camoData.w = 14;
          return &camoData;
        }
      }
    }

    return nullptr;
  };

  res.getMeshes(
    [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
      if (!all_visibility[visibility_index + node_id] || radius < min_elem_radius)
        return;

      baseOffsetRenderData += rigid_chunk_size;

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
        auto distSq = lengthSq(tm.col[3] - ctx->viewPosition);
        if (sqr(bvh_dyn_range) < distSq)
          return;
      }
#endif

      mat44f tm44;
      mat43f tm43;
      v_mat44_make_from_43cu(tm44, tm.array);
      v_mat44_transpose_to_mat43(tm43, tm44);

      for (auto [elemIx, elem] : enumerate(elems))
      {
        bool hasIndices;
        if (!elem.vertexData->isRenderable(hasIndices))
          continue;

        auto meshId = make_relem_mesh_id(bvhId, lodNo, make_dyn_elem_id(rigid_no, elemIx));

        auto bvhMesh = get_mesh(bvh_context_id, meshId);
        if (!bvhMesh)
          return;

        auto camoDataPtr = getCamoData(*bvhMesh, elem);

        if (bvhMesh->isHeliRotor)
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

          if (data.metaAllocId < 0)
            data.metaAllocId = bvh_context_id->allocateMetaRegion();

          add_dynrend_instance(bvh_context_id, dynrend_context_id, meshId, tm43, camoDataPtr, isDamaged, heliRotorInfo,
            data.metaAllocId);
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

          if (data.metaAllocId < 0)
            data.metaAllocId = bvh_context_id->allocateMetaRegion();

          add_dynrend_instance(bvh_context_id, dynrend_context_id, meshId, tm43, camoDataPtr, isDamaged, deformedInfo,
            data.metaAllocId);
        }
        else
          add_dynrend_instance(bvh_context_id, dynrend_context_id, meshId, tm43, camoDataPtr, isDamaged);
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

        mat44f tm44;
        mat43f tm43;
        v_mat44_make_from_43cu(tm44, tm.array);
        v_mat44_transpose_to_mat43(tm43, tm44);

        MeshSkinningInfo skinningInfo;
        skinningInfo.invWorldTm = TMatrix4(inverse(tm)).transpose();
        skinningInfo.setTransformsFn = [=]() {
          dynrend::set_instance_data_buffer(STAGE_CS, dynrend_context_id, baseOffsetRenderData);
        };

        for (auto [elemIx, elem] : enumerate(elems))
        {
          bool hasIndices;
          if (!elem.vertexData->isRenderable(hasIndices))
            continue;

          auto meshId = make_relem_mesh_id(bvhId, lodNo, make_dyn_elem_id(skin_no + rigidCount, elemIx));

          auto bvhMesh = get_mesh(bvh_context_id, meshId);
          if (!bvhMesh)
            return;

          auto &data = bvh_context_id->uniqueSkinBuffers[inst.getUniqueId()][meshId];
          skinningInfo.skinningBuffer = &data.buffer;
          skinningInfo.skinningBlas = &data.blas;

          if (data.metaAllocId < 0)
            data.metaAllocId = bvh_context_id->allocateMetaRegion();

          auto camoDataPtr = getCamoData(*bvhMesh, elem);
          add_dynrend_instance(bvh_context_id, dynrend_context_id, meshId, tm43, camoDataPtr, isDamaged, skinningInfo,
            data.metaAllocId);
        }
      }

      baseOffsetRenderData += 9 + mesh->bonesCount() * 6;
    });
}

void update_dynrend_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_plane_context_id, const Point3 &view_position)
{
  if (!bvh_context_id->has(Features::DynrendRigidBaked | Features::DynrendRigidFull | Features::DynrendSkinnedFull))
    return;

  TIME_D3D_PROFILE(bvh::update_dynrend_instances);

  IterCtx ctx = {bvh_context_id, view_position, false};
  dynrend::iterate_instances(dynrend_context_id, iterate_instances, &ctx);
  ctx.isPlane = true;
  dynrend::iterate_instances(dynrend_plane_context_id, iterate_instances, &ctx);
}

void set_up_dynrend_context_for_processing(dynrend::ContextId dynrend_context_id)
{
  dynrend::update_reprojection_data(dynrend_context_id);
}

} // namespace bvh::dyn
