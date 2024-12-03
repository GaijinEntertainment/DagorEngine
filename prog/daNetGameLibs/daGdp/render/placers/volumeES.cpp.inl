// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_enumerate.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <daECS/core/coreEvents.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <shaders/dag_rendInstRes.h>
#include "../riexProcessor.h"
#include "../globalManager.h"
#include "volume.h"

constexpr int RI_FIXED_LOD = 2;
constexpr float MIN_GEOMETRY_SIZE = 1.0f;

ECS_REGISTER_BOXED_TYPE(dagdp::VolumeManager, nullptr);

namespace dagdp
{

template <typename Callable>
static inline void volume_placers_ecs_query(Callable);

template <typename Callable>
static inline void volumes_ecs_query(Callable);

ECS_NO_ORDER
static inline void volume_view_process_es(const EventViewProcess &evt, VolumeManager &dagdp__volume_manager)
{
  const auto &rulesBuilder = evt.get<0>();
  auto &builder = dagdp__volume_manager.currentBuilder;
  auto &viewBuilder = *evt.get<2>();

  volume_placers_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_placer_volume) ecs::EntityId eid, float dagdp__density) {
    auto iter = rulesBuilder.placers.find(eid);
    if (iter == rulesBuilder.placers.end())
      return;

    if (rulesBuilder.maxObjects == 0)
    {
      logerr("daGdp: placer with EID %u disabled, as the max. objects setting is 0", static_cast<unsigned int>(eid));
      return;
    }

    auto &placer = iter->second;

    if (dagdp__density <= 0.0f)
    {
      logerr("daGdp: placer with EID %u has invalid density", static_cast<unsigned int>(eid));
      return;
    }

    builder.mapping.variantIds.insert({eid, builder.variants.size()});
    auto &variant = builder.variants.push_back();
    variant.density = dagdp__density;

    for (const auto objectGroupEid : placer.objectGroupEids)
    {
      auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
      if (iter == rulesBuilder.objectGroups.end())
        continue;

      auto &objectGroup = variant.objectGroups.push_back();
      objectGroup.effectiveDensity = dagdp__density;
      objectGroup.info = &iter->second;
    }

    viewBuilder.hasDynamicPlacers = true;
  });
}

ECS_NO_ORDER static inline void volume_view_finalize_es(const EventViewFinalize &evt, VolumeManager &dagdp__volume_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();

  create_volume_nodes(viewInfo, viewBuilder, dagdp__volume_manager, nodes);

  dagdp__volume_manager.currentBuilder = {};
}

void gather_meshes(const VolumeMapping &volume_mapping,
  const ViewInfo &viewInfo,
  const Viewport &viewport,
  float max_bounding_radius,
  RiexProcessor &riex_processor,
  RelevantMeshes &out_result)
{
  volumes_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      const auto iter = volume_mapping.variantIds.find(dagdp_internal__volume_placer_eid);
      if (iter == volume_mapping.variantIds.end())
        return;

      const uint32_t variantIndex = iter->second;

      // Reference: volumePlacerES.cpp.inl, VolumePlacer::updateVisibility
      mat44f bboxTm44;
      v_mat44_make_from_43cu_unsafe(bboxTm44, transform.array);

      mat44f bboxItm44;
      // v_mat44_orthonormal_inverse43(bboxItm44, bboxTm44);
      // v_mat44_inverse(bboxItm44, bboxTm44);
      v_mat44_inverse43(bboxItm44, bboxTm44);

      mat43f bboxItm43;
      v_mat44_transpose_to_mat43(bboxItm43, bboxItm44);

      const vec4f extent2 =
        v_add(v_add(v_abs(bboxTm44.col0), v_add(v_abs(bboxTm44.col1), v_abs(bboxTm44.col2))), v_splats(max_bounding_radius));
      const vec4f center2 = v_add(bboxTm44.col3, bboxTm44.col3);

      Frustum frustum = viewport.frustum;
      Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
      shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), v_splats(min(viewInfo.maxDrawDistance, viewport.maxDrawDistance)));
      const bool isIntersecting = frustum.testBoxExtentB(center2, extent2);

      if (!isIntersecting)
        return;

      // Reference: volumePlacerES.cpp.inl, VolumePlacer::gatherGeometryInBox
      bbox3f bbox;
      v_bbox3_init(bbox, bboxTm44, {v_neg(V_C_HALF), V_C_HALF});
      rendinst::riex_collidable_t out_handles;
      rendinst::gatherRIGenExtraCollidableMin(out_handles, bbox, MIN_GEOMETRY_SIZE);

      for (const auto handle : out_handles)
      {
        const uint32_t resIndex = rendinst::handle_to_ri_type(handle);
        const RenderableInstanceLodsResource *riLodsRes = rendinst::getRIGenExtraRes(resIndex);
        const int bestLod = riLodsRes->getQlBestLod();
        const int actualLod = min<int>(RI_FIXED_LOD, riLodsRes->lods.size() - 1);
        if (bestLod > actualLod)
          continue;

        const mat43f &instTm = rendinst::getRIGenExtra43(handle);
        RenderableInstanceResource *riRes = riLodsRes->lods[actualLod].scene;
        const auto *state = riex_processor.ask(riRes);

        if (state)
        {
          const dag::ConstSpan<ShaderMesh::RElem> elems = riRes->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);

          G_ASSERT(elems.size() == state->areasIndices.size());
          for (auto [i, elem] : enumerate(elems))
          {
            auto &item = out_result.push_back();
            v_stu(&item.bboxItmRow0, bboxItm43.row0);
            v_stu(&item.bboxItmRow1, bboxItm43.row1);
            v_stu(&item.bboxItmRow2, bboxItm43.row2);
            item.startIndex = elem.si;
            item.numFaces = elem.numf;
            item.baseVertex = elem.baseVertex;
            item.stride = elem.vertexData->getStride();
            v_stu(&item.tmRow0.x, instTm.row0);
            v_stu(&item.tmRow1.x, instTm.row1);
            v_stu(&item.tmRow2.x, instTm.row2);
            item.areasIndex = state->areasIndices[i];
            item.vbIndex = elem.vertexData->getVbIdx();
            item.variantIndex = variantIndex;
            item.uniqueId = rendinst::handle_to_ri_inst(handle);
          }
        }
      }
    });
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name, dagdp__density)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume, const ecs::string &dagdp__name, float dagdp__density)
static void dagdp_placer_volume_changed_es(const ecs::Event &)
{
  manager_ecs_query([](GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

template <typename Callable>
static inline void volumes_link_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__name)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume)
static void dagdp_placer_volume_link_es(const ecs::Event &, ecs::EntityId eid, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query([&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume)
static void dagdp_placer_volume_unlink_es(const ecs::Event &, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query([&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = ecs::INVALID_ENTITY_ID;
  });
}

// TODO: when dagdp__name changes, all volumes that were linked to the old name should ideally be unlinked.
// However, this is a dev-only scenario, and probably a rare one, so it's not implemented for now.

template <typename Callable>
static inline void volume_placers_link_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__volume_placer_name)
ECS_REQUIRE(ecs::Tag dagdp_volume)
static void dagdp_volume_link_es(
  const ecs::Event &, const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  volume_placers_link_ecs_query([&](ecs::EntityId eid, const ecs::string &dagdp__name) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

} // namespace dagdp
