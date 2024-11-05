#include "assetManagerES.cpp.inl"
ECS_DEF_PULL_VAR(assetManager);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc asset_manager_track_changes_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("asset__manager"), ecs::ComponentTypeInfo<DagorAssetMgr>()}
};
static void asset_manager_track_changes_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    asset_manager_track_changes_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(asset_manager_track_changes_es_comps, "asset__manager", DagorAssetMgr)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc asset_manager_track_changes_es_es_desc
(
  "asset_manager_track_changes_es",
  "prog/daNetGameLibs/assets_import/main/assetManagerES.cpp.inl",
  ecs::EntitySystemOps(asset_manager_track_changes_es_all),
  make_span(asset_manager_track_changes_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc init_asset_manager_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("asset__manager"), ecs::ComponentTypeInfo<DagorAssetMgr>()},
  {ECS_HASH("asset__baseFolderAbsPath"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("asset__rendInstType"), ecs::ComponentTypeInfo<int>()},
//start of 3 ro components at [3]
  {ECS_HASH("asset__applicationBlkPath"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("asset__baseFolder"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("asset__pluginsFolder"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void init_asset_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_asset_manager_es(evt
        , ECS_RW_COMP(init_asset_manager_es_comps, "asset__manager", DagorAssetMgr)
    , ECS_RO_COMP(init_asset_manager_es_comps, "asset__applicationBlkPath", ecs::string)
    , ECS_RO_COMP(init_asset_manager_es_comps, "asset__baseFolder", ecs::string)
    , ECS_RO_COMP(init_asset_manager_es_comps, "asset__pluginsFolder", ecs::string)
    , ECS_RW_COMP(init_asset_manager_es_comps, "asset__baseFolderAbsPath", ecs::string)
    , ECS_RW_COMP(init_asset_manager_es_comps, "asset__rendInstType", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_asset_manager_es_es_desc
(
  "init_asset_manager_es",
  "prog/daNetGameLibs/assets_import/main/assetManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_asset_manager_es_all_events),
  make_span(init_asset_manager_es_comps+0, 3)/*rw*/,
  make_span(init_asset_manager_es_comps+3, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc destoy_asset_manager_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("asset__manager"), ecs::ComponentTypeInfo<DagorAssetMgr>()}
};
static void destoy_asset_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  destoy_asset_manager_es(evt
        );
}
static ecs::EntitySystemDesc destoy_asset_manager_es_es_desc
(
  "destoy_asset_manager_es",
  "prog/daNetGameLibs/assets_import/main/assetManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destoy_asset_manager_es_all_events),
  empty_span(),
  empty_span(),
  make_span(destoy_asset_manager_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
//static constexpr ecs::ComponentDesc track_a2d_es_comps[] ={};
static void track_a2d_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<AssetChangedEvent>());
  track_a2d_es(static_cast<const AssetChangedEvent&>(evt)
        );
}
static ecs::EntitySystemDesc track_a2d_es_es_desc
(
  "track_a2d_es",
  "prog/daNetGameLibs/assets_import/main/assetManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_a2d_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AssetChangedEvent>::build(),
  0
);
static constexpr ecs::ComponentDesc track_animchar_assets_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar__animStateDirty"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [3]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("animchar__res"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void track_animchar_assets_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AssetChangedEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    track_animchar_assets_es(static_cast<const AssetChangedEvent&>(evt)
        , ECS_RO_COMP(track_animchar_assets_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(track_animchar_assets_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(track_animchar_assets_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(track_animchar_assets_es_comps, "animchar__res", ecs::string)
    , ECS_RW_COMP_PTR(track_animchar_assets_es_comps, "animchar__animStateDirty", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc track_animchar_assets_es_es_desc
(
  "track_animchar_assets_es",
  "prog/daNetGameLibs/assets_import/main/assetManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_animchar_assets_es_all_events),
  make_span(track_animchar_assets_es_comps+0, 3)/*rw*/,
  make_span(track_animchar_assets_es_comps+3, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AssetChangedEvent>::build(),
  0
);
