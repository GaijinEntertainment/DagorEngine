// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "CSMShadowsES.cpp.inl"
ECS_DEF_PULL_VAR(CSMShadows);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc csm_shadows_recreate_views_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__csm_shadows_manager"), ecs::ComponentTypeInfo<dagdp::CSMShadowsManager>()}
};
static void csm_shadows_recreate_views_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventRecreateViews>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::csm_shadows_recreate_views_es(static_cast<const dagdp::EventRecreateViews&>(evt)
        , components.manager()
    , ECS_RW_COMP(csm_shadows_recreate_views_es_comps, "dagdp__csm_shadows_manager", dagdp::CSMShadowsManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc csm_shadows_recreate_views_es_es_desc
(
  "csm_shadows_recreate_views_es",
  "prog/daNetGameLibs/daGdp/render/CSMShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, csm_shadows_recreate_views_es_all_events),
  make_span(csm_shadows_recreate_views_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventRecreateViews>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc recreate_views_on_shadow_settings_change_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("render_settings__shadowsQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void recreate_views_on_shadow_settings_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::recreate_views_on_shadow_settings_change_es(evt
        );
}
static ecs::EntitySystemDesc recreate_views_on_shadow_settings_change_es_es_desc
(
  "recreate_views_on_shadow_settings_change_es",
  "prog/daNetGameLibs/daGdp/render/CSMShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, recreate_views_on_shadow_settings_change_es_all_events),
  empty_span(),
  empty_span(),
  make_span(recreate_views_on_shadow_settings_change_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__shadowsQuality",nullptr,"init_shadows_es");
static constexpr ecs::ComponentDesc level_settings_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dagdp__csm_max_cascades"), ecs::ComponentTypeInfo<int>()},
//start of 1 rq components at [1]
  {ECS_HASH("dagdp_level_settings"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc level_settings_ecs_query_desc
(
  "dagdp::level_settings_ecs_query",
  empty_span(),
  make_span(level_settings_ecs_query_comps+0, 1)/*ro*/,
  make_span(level_settings_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::level_settings_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, level_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(level_settings_ecs_query_comps, "dagdp__csm_max_cascades", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc shadows_query_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc shadows_query_manager_ecs_query_desc
(
  "dagdp::shadows_query_manager_ecs_query",
  make_span(shadows_query_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::shadows_query_manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, shadows_query_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(shadows_query_manager_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
