#include "dynShadowsES.cpp.inl"
ECS_DEF_PULL_VAR(dynShadows);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dyn_shadows_recreate_views_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__dyn_shadows_manager"), ecs::ComponentTypeInfo<dagdp::DynShadowsManager>()}
};
static void dyn_shadows_recreate_views_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventRecreateViews>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dyn_shadows_recreate_views_es(static_cast<const dagdp::EventRecreateViews&>(evt)
        , ECS_RW_COMP(dyn_shadows_recreate_views_es_comps, "dagdp__dyn_shadows_manager", dagdp::DynShadowsManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dyn_shadows_recreate_views_es_es_desc
(
  "dyn_shadows_recreate_views_es",
  "prog/daNetGameLibs/daGdp/render/dynShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dyn_shadows_recreate_views_es_all_events),
  make_span(dyn_shadows_recreate_views_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventRecreateViews>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc spot_lights_changed_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadows"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>()}
};
static void spot_lights_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(spot_lights_changed_es_comps, "light__render_gpu_objects", bool) && ECS_RO_COMP(spot_lights_changed_es_comps, "spot_light__shadows", bool)) )
      continue;
    dagdp::spot_lights_changed_es(evt
          );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc spot_lights_changed_es_es_desc
(
  "spot_lights_changed_es",
  "prog/daNetGameLibs/daGdp/render/dynShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spot_lights_changed_es_all_events),
  empty_span(),
  make_span(spot_lights_changed_es_comps+0, 2)/*ro*/,
  make_span(spot_lights_changed_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","light__max_radius,light__render_gpu_objects,spot_light__shadows");
static constexpr ecs::ComponentDesc omni_lights_changed_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__shadows"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>()}
};
static void omni_lights_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(omni_lights_changed_es_comps, "light__render_gpu_objects", bool) && ECS_RO_COMP(omni_lights_changed_es_comps, "omni_light__shadows", bool)) )
      continue;
    dagdp::omni_lights_changed_es(evt
          );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc omni_lights_changed_es_es_desc
(
  "omni_lights_changed_es",
  "prog/daNetGameLibs/daGdp/render/dynShadowsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, omni_lights_changed_es_all_events),
  empty_span(),
  make_span(omni_lights_changed_es_comps+0, 2)/*ro*/,
  make_span(omni_lights_changed_es_comps+2, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","light__max_radius,light__render_gpu_objects,omni_light__shadows");
static constexpr ecs::ComponentDesc spot_lights_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("spot_light__shadows"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc spot_lights_ecs_query_desc
(
  "dagdp::spot_lights_ecs_query",
  empty_span(),
  make_span(spot_lights_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::spot_lights_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, spot_lights_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(spot_lights_ecs_query_comps, "light__render_gpu_objects", bool) && ECS_RO_COMP(spot_lights_ecs_query_comps, "spot_light__shadows", bool)) )
            continue;
          function(
              ECS_RO_COMP(spot_lights_ecs_query_comps, "light__max_radius", float)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc omni_lights_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("light__max_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("light__render_gpu_objects"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("omni_light__shadows"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc omni_lights_ecs_query_desc
(
  "dagdp::omni_lights_ecs_query",
  empty_span(),
  make_span(omni_lights_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::omni_lights_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, omni_lights_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(omni_lights_ecs_query_comps, "light__render_gpu_objects", bool) && ECS_RO_COMP(omni_lights_ecs_query_comps, "omni_light__shadows", bool)) )
            continue;
          function(
              ECS_RO_COMP(omni_lights_ecs_query_comps, "light__max_radius", float)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc manager_ecs_query_desc
(
  "dagdp::manager_ecs_query",
  make_span(manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(manager_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
