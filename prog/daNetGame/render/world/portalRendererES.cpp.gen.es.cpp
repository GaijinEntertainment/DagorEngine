// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "portalRendererES.cpp.inl"
ECS_DEF_PULL_VAR(portalRenderer);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_portal_data_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 1 ro components at [1]
  {ECS_HASH("dynamic_portal__index"), ecs::ComponentTypeInfo<int>()}
};
static void update_portal_data_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_portal_data_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(update_portal_data_es_comps, "dynamic_portal__index", int)
    , ECS_RW_COMP(update_portal_data_es_comps, "additional_data", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_portal_data_es_es_desc
(
  "update_portal_data_es",
  "prog/daNetGame/render/world/portalRendererES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_portal_data_es_all_events),
  make_span(update_portal_data_es_comps+0, 1)/*rw*/,
  make_span(update_portal_data_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_portal_renderer_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("portal_renderer"), ecs::ComponentTypeInfo<PortalRenderer>()}
};
static ecs::CompileTimeQueryDesc get_portal_renderer_manager_ecs_query_desc
(
  "get_portal_renderer_manager_ecs_query",
  make_span(get_portal_renderer_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_portal_renderer_manager_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_portal_renderer_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_portal_renderer_manager_ecs_query_comps, "portal_renderer", PortalRenderer)
            );

        }
    }
  );
}
