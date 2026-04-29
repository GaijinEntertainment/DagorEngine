// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "nbsHandlerES.cpp.inl"
ECS_DEF_PULL_VAR(nbsHandler);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_nbs_params_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("nbs_params"), ecs::ComponentTypeInfo<Point4>()}
};
static void update_nbs_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_nbs_params_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(update_nbs_params_es_comps, "nbs_params", Point4)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_nbs_params_es_es_desc
(
  "update_nbs_params_es",
  "prog/daNetGame/render/world/nbsHandlerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_nbs_params_es_all_events),
  empty_span(),
  make_span(update_nbs_params_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
);
static constexpr ecs::ComponentDesc add_volfog_optional_graph_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("volfog_optional_graph"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void add_volfog_optional_graph_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    add_volfog_optional_graph_es_event_handler(evt
        , ECS_RO_COMP(add_volfog_optional_graph_es_event_handler_comps, "volfog_optional_graph", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc add_volfog_optional_graph_es_event_handler_es_desc
(
  "add_volfog_optional_graph_es",
  "prog/daNetGame/render/world/nbsHandlerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, add_volfog_optional_graph_es_event_handler_all_events),
  empty_span(),
  make_span(add_volfog_optional_graph_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc add_envi_cover_optional_graph_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("envi_cover_optional_graph"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void add_envi_cover_optional_graph_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    add_envi_cover_optional_graph_es_event_handler(evt
        , ECS_RO_COMP(add_envi_cover_optional_graph_es_event_handler_comps, "envi_cover_optional_graph", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc add_envi_cover_optional_graph_es_event_handler_es_desc
(
  "add_envi_cover_optional_graph_es",
  "prog/daNetGame/render/world/nbsHandlerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, add_envi_cover_optional_graph_es_event_handler_all_events),
  empty_span(),
  make_span(add_envi_cover_optional_graph_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc nbs_volfog_init_es_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("volfog_nbs__rootGraph"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("volfog_nbs__low_range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("volfog_nbs__high_range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("volfog_nbs__low_height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("volfog_nbs__high_height"), ecs::ComponentTypeInfo<float>()}
};
static void nbs_volfog_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnLevelLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    nbs_volfog_init_es(static_cast<const OnLevelLoaded&>(evt)
        , ECS_RO_COMP(nbs_volfog_init_es_comps, "volfog_nbs__rootGraph", ecs::string)
    , ECS_RO_COMP(nbs_volfog_init_es_comps, "volfog_nbs__low_range", float)
    , ECS_RO_COMP(nbs_volfog_init_es_comps, "volfog_nbs__high_range", float)
    , ECS_RO_COMP(nbs_volfog_init_es_comps, "volfog_nbs__low_height", float)
    , ECS_RO_COMP(nbs_volfog_init_es_comps, "volfog_nbs__high_height", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc nbs_volfog_init_es_es_desc
(
  "nbs_volfog_init_es",
  "prog/daNetGame/render/world/nbsHandlerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, nbs_volfog_init_es_all_events),
  empty_span(),
  make_span(nbs_volfog_init_es_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc nbs_envi_cover_init_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("envi_cover_nbs__rootGraph"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void nbs_envi_cover_init_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnLevelLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    nbs_envi_cover_init_es(static_cast<const OnLevelLoaded&>(evt)
        , ECS_RO_COMP(nbs_envi_cover_init_es_comps, "envi_cover_nbs__rootGraph", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc nbs_envi_cover_init_es_es_desc
(
  "nbs_envi_cover_init_es",
  "prog/daNetGame/render/world/nbsHandlerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, nbs_envi_cover_init_es_all_events),
  empty_span(),
  make_span(nbs_envi_cover_init_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc volfog_optional_graphs_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("volfog_optional_graph"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc volfog_optional_graphs_ecs_query_desc
(
  "volfog_optional_graphs_ecs_query",
  empty_span(),
  make_span(volfog_optional_graphs_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void volfog_optional_graphs_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, volfog_optional_graphs_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(volfog_optional_graphs_ecs_query_comps, "volfog_optional_graph", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc envi_cover_optional_graphs_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("envi_cover_optional_graph"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc envi_cover_optional_graphs_ecs_query_desc
(
  "envi_cover_optional_graphs_ecs_query",
  empty_span(),
  make_span(envi_cover_optional_graphs_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void envi_cover_optional_graphs_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, envi_cover_optional_graphs_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(envi_cover_optional_graphs_ecs_query_comps, "envi_cover_optional_graph", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
