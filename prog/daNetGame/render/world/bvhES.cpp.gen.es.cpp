#include "bvhES.cpp.inl"
ECS_DEF_PULL_VAR(bvh);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc setup_bvh_scene_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bvh__heightProvider"), ecs::ComponentTypeInfo<BvhHeightProvider>()},
  {ECS_HASH("bvh__initialized"), ecs::ComponentTypeInfo<bool>()}
};
static void setup_bvh_scene_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__initialized", bool)) )
      continue;
    setup_bvh_scene_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__heightProvider", BvhHeightProvider)
      , ECS_RW_COMP(setup_bvh_scene_es_comps, "bvh__initialized", bool)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc setup_bvh_scene_es_es_desc
(
  "setup_bvh_scene_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, setup_bvh_scene_es_all_events),
  make_span(setup_bvh_scene_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc close_bvh_scene_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("bvh__heightProvider"), ecs::ComponentTypeInfo<BvhHeightProvider>()}
};
static void close_bvh_scene_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  close_bvh_scene_es(evt
        );
}
static ecs::EntitySystemDesc close_bvh_scene_es_es_desc
(
  "close_bvh_scene_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, close_bvh_scene_es_all_events),
  empty_span(),
  empty_span(),
  make_span(close_bvh_scene_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bvh_free_visibility_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static void bvh_free_visibility_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bvh_free_visibility_es(evt
        , ECS_RW_COMP(bvh_free_visibility_es_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_free_visibility_es_es_desc
(
  "bvh_free_visibility_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_free_visibility_es_all_events),
  make_span(bvh_free_visibility_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_bvh_scene_es_comps[] =
{
//start of 9 rw components at [0]
  {ECS_HASH("bvh__update_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("denoiser_prepare_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("rtsm_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("rtr_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("rtao_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("water_rt_early_before_envi_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("water_rt_early_after_envi_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("water_rt_late_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static void init_bvh_scene_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_bvh_scene_es(evt
        , ECS_RW_COMP(init_bvh_scene_es_comps, "bvh__update_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "denoiser_prepare_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "rtsm_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "rtr_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "rtao_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "water_rt_early_before_envi_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "water_rt_early_after_envi_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "water_rt_late_node", dabfg::NodeHandle)
    , ECS_RW_COMP(init_bvh_scene_es_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_bvh_scene_es_es_desc
(
  "init_bvh_scene_es",
  "prog/daNetGame/render/world/bvhES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_bvh_scene_es_all_events),
  make_span(init_bvh_scene_es_comps+0, 9)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_bvh_rigen_visibility_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("bvh__rendinst_visibility"), ecs::ComponentTypeInfo<RiGenVisibilityECS>()}
};
static ecs::CompileTimeQueryDesc get_bvh_rigen_visibility_ecs_query_desc
(
  "get_bvh_rigen_visibility_ecs_query",
  empty_span(),
  make_span(get_bvh_rigen_visibility_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_bvh_rigen_visibility_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_bvh_rigen_visibility_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_bvh_rigen_visibility_ecs_query_comps, "bvh__rendinst_visibility", RiGenVisibilityECS)
            );

        }while (++comp != compE);
    }
  );
}
