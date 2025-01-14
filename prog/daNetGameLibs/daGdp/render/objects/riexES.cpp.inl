// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_enumerate.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entitySystem.h>
#include <ecs/rendInst/riExtra.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <shaders/dag_rendInstRes.h>
#include <rendInst/riShaderConstBuffers.h>
#include <riGen/riGenExtra.h>
#include "../globalManager.h"
#include "riex.h"

namespace material_var
{
static ShaderVariableInfo use_cross_dissolve("use_cross_dissolve");
} // namespace material_var

ECS_REGISTER_BOXED_TYPE(dagdp::RiexManager, nullptr);

namespace dagdp
{

template <typename Callable>
static inline void riex_object_group_ecs_query(Callable);

ECS_NO_ORDER
static inline void riex_object_group_process_es(const dagdp::EventObjectGroupProcess &evt, dagdp::RiexManager &dagdp__riex_manager)
{
  auto &rulesBuilder = *evt.get<0>();
  auto &builder = dagdp__riex_manager.currentBuilder;

  builder = {}; // Reset the state.

  riex_object_group_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_object_group_riex) ecs::EntityId eid, const ecs::Object &dagdp__assets,
                                const ecs::Object &dagdp__params) {
    auto iter = rulesBuilder.objectGroups.find(eid);
    if (iter == rulesBuilder.objectGroups.end())
      return;

    ObjectGroupInfo &objectGroupInfo = iter->second;

    PlaceableParams commonParams;
    if (!set_common_params(dagdp__params, eid, commonParams))
      return;

    float totalWeight = 0.0f;
    for (auto [i, pair] : enumerate(dagdp__assets))
    {
      const auto &[assetName, child] = pair;

      ObjectGroupPlaceable placeable;
      placeable.params = commonParams;
      if (!set_object_params(child, eid, assetName, placeable.params))
        continue;

      RiexResource *resource = nullptr;
      int numMeshLods = 0;

      uint32_t assetNameHash = NameMap::string_hash(assetName.data(), assetName.size());
      if (const auto nameId = builder.resourceAssetNameMap.getNameId(assetName.data(), assetName.size(), assetNameHash); nameId >= 0)
      {
        resource = &builder.resources[nameId];
        numMeshLods = resource->lods_rId.size();
      }
      else
      {
        // Expected to be preloaded by RiexPreload -> GameresPreLoaded flag is applicable
        const auto riAddFlag = rendinst::AddRIFlag::UseShadow | rendinst::AddRIFlag::GameresPreLoaded;
        const int riExId = get_or_add_ri_extra_res_id(assetName.c_str(), riAddFlag);
        const GameResHandle handle = GAMERES_HANDLE_FROM_STRING(assetName.c_str());
        eastl::shared_ptr<GameResource> gameRes(get_one_game_resource_ex(handle, RendInstGameResClassId), GameResDeleter());

        if (!gameRes.get() || riExId < 0)
        {
          logerr("daGdp: object group with EID %u has invalid RendInstExtra asset with name %s.", static_cast<unsigned int>(eid),
            assetName.c_str());
          continue;
        }

        const auto lodsRes = reinterpret_cast<RenderableInstanceLodsResource *>(gameRes.get());
        bool hasCrossDissolve = false;
        bool stagesOk = true;
        numMeshLods = lodsRes->lods.size();

        if (lodsRes->hasImpostor())
        {
          // The last LOD is the impostor LOD.
          numMeshLods--;
          G_ASSERT(numMeshLods > 0);
        }

        for (int lodIndex = 0; lodIndex < numMeshLods; lodIndex++)
        {
          const RenderableInstanceLodsResource::Lod &lod = lodsRes->lods[lodIndex];
          G_ASSERT(lod.scene != nullptr);
          const ShaderMesh *shaderMesh = lod.scene->getMesh()->getMesh()->getMesh();

          for (const auto &rElem : shaderMesh->getAllElems())
          {
            int crossDissolve = 0;
            rElem.mat->getIntVariable(material_var::use_cross_dissolve.get_var_id(), crossDissolve);
            if (crossDissolve > 0)
            {
              G_ASSERT(lodIndex == numMeshLods - 1);
              hasCrossDissolve = true;
              break;
            }
          }

          // TODO: Distortions https://youtrack.gaijin.team/issue/RE-741/daGDP-distortions-support
          // TODO: Transparency https://youtrack.gaijin.team/issue/RE-742/daGDP-transparency-support
          if (shaderMesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_decal).size() != shaderMesh->getAllElems().size())
          {
            stagesOk = false;
            break;
          }
        }

        if (hasCrossDissolve)
          --numMeshLods;

        if (!stagesOk)
        {
          logerr("daGdp: RendInstExtra asset with name %s is not yet supported because it uses distortions or transparency.",
            assetName.c_str());
          continue;
        }

        builder.resourceAssetNameMap.addNameId(assetName.data(), assetName.size(), assetNameHash);
        resource = &builder.resources.push_back();
        resource->gameRes = eastl::move(gameRes);
        resource->riExId = riExId;

        // PoolOffset is calculated by: poolIdx * (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(vec4f)) + 1;
        // Where RiShaderConstBuffers is a struct with all extra data for a single RI (type).
        // PER_DRAW_VECS_COUNT should be equal to (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(vec4f)).
        resource->riPoolOffset = riExId * rendinst::render::PER_DRAW_VECS_COUNT + 1;
        for (int lodIndex = 0; lodIndex < numMeshLods; lodIndex++)
          resource->lods_rId.push_back(rulesBuilder.nextRenderableId++);

        update_per_draw_gathered_data(riExId);
      }

      G_ASSERT(resource);
      const auto lodsRes = reinterpret_cast<RenderableInstanceLodsResource *>(resource->gameRes.get());

      G_ASSERT(numMeshLods > 0);
      G_ASSERT(numMeshLods <= lodsRes->lods.size());
      G_ASSERT(numMeshLods == resource->lods_rId.size());

      for (int lodIndex = 0; lodIndex < numMeshLods; lodIndex++)
      {
        RiexRenderableInfo rInfo;
        rInfo.lodsRes = lodsRes;
        rInfo.lodIndex = lodIndex;
        rInfo.isTree = rendinst::riExtra[resource->riExId].isTree;

        const RenderableId rId = resource->lods_rId[lodIndex];
        builder.renderablesInfo.emplace(rId, rInfo);

        PlaceableRange range;
        range.rId = rId;

        int rangeLodIndex = lodIndex;
        if (lodIndex == numMeshLods - 1)
        {
          // If we have impostor and transition LODs, we ignore them, but would like the last mesh LOD to have extended range.
          rangeLodIndex = lodsRes->lods.size() - 1;
        }

        range.baseDrawDistance = lodsRes->lods[rangeLodIndex].range;

        placeable.ranges.push_back(range);
      }

      placeable.params.riPoolOffset = resource->riPoolOffset;

      const float maxScale = md_max(placeable.params.scaleMidDev);
      const float maxDrawDistance = placeable.ranges.back().baseDrawDistance * maxScale;

      if (maxDrawDistance > GLOBAL_MAX_DRAW_DISTANCE)
      {
        // Split huge logerr into several lines...
        logerr("daGdp: object with RI name %s has max. scale %f. It means the effective draw distance for it is %f, " //
               "greater than the hard limit %f.",
          assetName.c_str(), maxScale, maxDrawDistance, GLOBAL_MAX_DRAW_DISTANCE);
        logerr("The asset LOD range might be incorrectly set too high. Alternatively, objects being placed are too large, " //
               "and should not be implemented as GPU objects.");

        // Don't try to clamp values here; views should implement GLOBAL_MAX_DRAW_DISTANCE limit instead and everything will just work.
      }

      totalWeight += placeable.params.weight;
      objectGroupInfo.placeables.push_back(placeable);
      objectGroupInfo.maxPlaceableBoundingRadius = max(objectGroupInfo.maxPlaceableBoundingRadius, lodsRes->bound0rad * maxScale);
      objectGroupInfo.maxDrawDistance = max(objectGroupInfo.maxDrawDistance, maxDrawDistance);
    }

    // TODO: this is common logic and could be shared.
    for (auto &placeable : objectGroupInfo.placeables)
      placeable.params.weight /= totalWeight; // Normalize group weights.
  });
}

ECS_NO_ORDER
static inline void riex_view_finalize_es(const dagdp::EventViewFinalize &evt, dagdp::RiexManager &dagdp__riex_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();
  riex_finalize_view(viewInfo, viewBuilder, dagdp__riex_manager, nodes);
}

ECS_NO_ORDER
static inline void riex_finalize_es(const dagdp::EventFinalize &, dagdp::RiexManager &dagdp__riex_manager)
{
  riex_finalize(dagdp__riex_manager);
}

ECS_NO_ORDER static inline void riex_invalidate_views_es(const dagdp::EventInvalidateViews &, dagdp::RiexManager &dagdp__riex_manager)
{
  dagdp__riex_manager.shadowExtensionHandle = {};
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name, dagdp__assets, dagdp__params)
ECS_REQUIRE(
  ecs::Tag dagdp_object_group_riex, const ecs::string &dagdp__name, const ecs::Object &dagdp__assets, const ecs::Object &dagdp__params)
static void dagdp_object_group_riex_changed_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

struct RiexPreload
{
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    ecs::Object emptyObject;
    const ecs::Object &assets = res_cb.getOr<ecs::Object>(ECS_HASH("dagdp__assets"), emptyObject);
    for (const auto &[assetName, _] : assets)
    {
      request_ri_resources(res_cb, assetName.c_str());
    }
  }
};

} // namespace dagdp

ECS_DECLARE_RELOCATABLE_TYPE(dagdp::RiexPreload);
ECS_REGISTER_RELOCATABLE_TYPE(dagdp::RiexPreload, nullptr);
