#include "grassRenderES.cpp.inl"
ECS_DEF_PULL_VAR(grassRender);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_grass_ri_clipmap_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static void init_grass_ri_clipmap_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_grass_ri_clipmap_es(evt
        , ECS_RW_COMP(init_grass_ri_clipmap_es_comps, "grass_render", GrassRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_grass_ri_clipmap_es_es_desc
(
  "init_grass_ri_clipmap_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_grass_ri_clipmap_es_all_events),
  make_span(init_grass_ri_clipmap_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRendinstsLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc grass_quiality_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__grassQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void grass_quiality_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grass_quiality_es(evt
        , ECS_RO_COMP(grass_quiality_es_comps, "render_settings__bare_minimum", bool)
    , ECS_RO_COMP(grass_quiality_es_comps, "render_settings__grassQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grass_quiality_es_es_desc
(
  "grass_quiality_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_quiality_es_all_events),
  empty_span(),
  make_span(grass_quiality_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,nullptr,"render_settings__bare_minimum,render_settings__grassQuality");
static constexpr ecs::ComponentDesc track_anisotropy_change_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()},
//start of 1 rq components at [1]
  {ECS_HASH("render_settings__anisotropy"), ecs::ComponentTypeInfo<int>()}
};
static void track_anisotropy_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    track_anisotropy_change_es(evt
        , ECS_RW_COMP(track_anisotropy_change_es_comps, "grass_render", GrassRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc track_anisotropy_change_es_es_desc
(
  "track_anisotropy_change_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_anisotropy_change_es_all_events),
  make_span(track_anisotropy_change_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(track_anisotropy_change_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"render_settings__anisotropy");
//static constexpr ecs::ComponentDesc reset_grass_render_es_comps[] ={};
static void reset_grass_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  reset_grass_render_es(static_cast<const AfterDeviceReset&>(evt)
        );
}
static ecs::EntitySystemDesc reset_grass_render_es_es_desc
(
  "reset_grass_render_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_grass_render_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
);
static constexpr ecs::ComponentDesc grass_erasers_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("grass_erasers__spots"), ecs::ComponentTypeInfo<ecs::Point4List>()}
};
static ecs::CompileTimeQueryDesc grass_erasers_ecs_query_desc
(
  "grass_erasers_ecs_query",
  empty_span(),
  make_span(grass_erasers_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void grass_erasers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, grass_erasers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(grass_erasers_ecs_query_comps, "grass_erasers__spots", ecs::Point4List)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_grass_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static ecs::CompileTimeQueryDesc get_grass_render_ecs_query_desc
(
  "get_grass_render_ecs_query",
  make_span(get_grass_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_grass_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_grass_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_grass_render_ecs_query_comps, "grass_render", GrassRenderer)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc init_grass_render_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()},
  {ECS_HASH("grass_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc init_grass_render_ecs_query_desc
(
  "init_grass_render_ecs_query",
  make_span(init_grass_render_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_grass_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, init_grass_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(init_grass_render_ecs_query_comps, "grass_render", GrassRenderer)
            , ECS_RW_COMP(init_grass_render_ecs_query_comps, "grass_node", dabfg::NodeHandle)
            );

        }while (++comp != compE);
    }
  );
}
