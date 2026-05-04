#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t burnt_ground_decal_placer__decal_size_get_type();
static ecs::LTComponentList burnt_ground_decal_placer__decal_size_component(ECS_HASH("burnt_ground_decal_placer__decal_size"), burnt_ground_decal_placer__decal_size_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl", "burnt_ground__on_fire_event_es", 0);
static constexpr ecs::component_t burnt_ground_decal_placer__fade_in_time_get_type();
static ecs::LTComponentList burnt_ground_decal_placer__fade_in_time_component(ECS_HASH("burnt_ground_decal_placer__fade_in_time"), burnt_ground_decal_placer__fade_in_time_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl", "burnt_ground__on_fire_event_es", 0);
static constexpr ecs::component_t burnt_ground_decal_placer__fade_out_time_get_type();
static ecs::LTComponentList burnt_ground_decal_placer__fade_out_time_component(ECS_HASH("burnt_ground_decal_placer__fade_out_time"), burnt_ground_decal_placer__fade_out_time_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl", "burnt_ground__on_fire_event_es", 0);
static constexpr ecs::component_t burnt_ground_decal_placer__radius_get_type();
static ecs::LTComponentList burnt_ground_decal_placer__radius_component(ECS_HASH("burnt_ground_decal_placer__radius"), burnt_ground_decal_placer__radius_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl", "burnt_ground__on_fire_event_es", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl", "burnt_ground__on_fire_event_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "burntGroundES.cpp.inl"
ECS_DEF_PULL_VAR(burntGround);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc burnt_ground_renderer_on_appear_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("burnt_ground_renderer__prepare_decals_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("burnt_ground_renderer__decals_staging_buf"), ecs::ComponentTypeInfo<UniqueBuf>()},
  {ECS_HASH("burnt_ground_renderer__decals_indices_staging_buf"), ecs::ComponentTypeInfo<UniqueBuf>()},
  {ECS_HASH("burnt_ground_renderer__clipmap_decal_type"), ecs::ComponentTypeInfo<int>()},
//start of 7 ro components at [4]
  {ECS_HASH("burnt_ground_renderer__tex_d"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("burnt_ground_renderer__tex_n"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("burnt_ground_renderer__tex_m"), ecs::ComponentTypeInfo<SharedTexHolder>()},
  {ECS_HASH("burnt_ground_renderer__edge_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__edge_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__max_clipmap_decal_count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("burnt_ground_renderer__max_animating_decal_count"), ecs::ComponentTypeInfo<int>()}
};
static void burnt_ground_renderer_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground_renderer_on_appear_es(evt
        , components.manager()
    , ECS_RW_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__prepare_decals_node", dafg::NodeHandle)
    , ECS_RW_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__decals_staging_buf", UniqueBuf)
    , ECS_RW_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__decals_indices_staging_buf", UniqueBuf)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__tex_d", SharedTexHolder)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__tex_n", SharedTexHolder)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__tex_m", SharedTexHolder)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__edge_noise_scale", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__edge_width", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__max_clipmap_decal_count", int)
    , ECS_RO_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__max_animating_decal_count", int)
    , ECS_RW_COMP(burnt_ground_renderer_on_appear_es_comps, "burnt_ground_renderer__clipmap_decal_type", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground_renderer_on_appear_es_es_desc
(
  "burnt_ground_renderer_on_appear_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground_renderer_on_appear_es_all_events),
  make_span(burnt_ground_renderer_on_appear_es_comps+0, 4)/*rw*/,
  make_span(burnt_ground_renderer_on_appear_es_comps+4, 7)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc burnt_ground_renderer_on_change_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("burnt_ground_renderer__allowed_biome_groups"), ecs::ComponentTypeInfo<ecs::List<int>>()},
//start of 8 ro components at [1]
  {ECS_HASH("burnt_ground_renderer__selfillum_color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("burnt_ground_renderer__selfillum_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__selfillum_worldscale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__edge_noise_frequency"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__edge_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__edge_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__tile_size"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("burnt_ground_renderer__biomeNames"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void burnt_ground_renderer_on_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground_renderer_on_change_es(evt
        , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__selfillum_color", Point4)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__selfillum_strength", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__selfillum_worldscale", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__edge_noise_frequency", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__edge_noise_scale", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__edge_width", float)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__tile_size", Point2)
    , ECS_RO_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__biomeNames", ecs::StringList)
    , ECS_RW_COMP(burnt_ground_renderer_on_change_es_comps, "burnt_ground_renderer__allowed_biome_groups", ecs::List<int>)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground_renderer_on_change_es_es_desc
(
  "burnt_ground_renderer_on_change_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground_renderer_on_change_es_all_events),
  make_span(burnt_ground_renderer_on_change_es_comps+0, 1)/*rw*/,
  make_span(burnt_ground_renderer_on_change_es_comps+1, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc static_burnt_ground_decal_on_appear_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("static_burnt_ground_decal__radius"), ecs::ComponentTypeInfo<float>()}
};
static void static_burnt_ground_decal_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    static_burnt_ground_decal_on_appear_es(evt
        , ECS_RO_COMP(static_burnt_ground_decal_on_appear_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(static_burnt_ground_decal_on_appear_es_comps, "static_burnt_ground_decal__radius", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc static_burnt_ground_decal_on_appear_es_es_desc
(
  "static_burnt_ground_decal_on_appear_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, static_burnt_ground_decal_on_appear_es_all_events),
  empty_span(),
  make_span(static_burnt_ground_decal_on_appear_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc burnt_ground__on_fire_event_es_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("burnt_ground_renderer__edge_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__edge_width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__fadein_duration_sec"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__fadeout_duration_sec"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__allowed_biome_groups"), ecs::ComponentTypeInfo<ecs::List<int>>()}
};
static void burnt_ground__on_fire_event_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<FireOnGroundEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground__on_fire_event_es(static_cast<const FireOnGroundEvent&>(evt)
        , components.manager()
    , ECS_RO_COMP(burnt_ground__on_fire_event_es_comps, "burnt_ground_renderer__edge_noise_scale", float)
    , ECS_RO_COMP(burnt_ground__on_fire_event_es_comps, "burnt_ground_renderer__edge_width", float)
    , ECS_RO_COMP(burnt_ground__on_fire_event_es_comps, "burnt_ground_renderer__fadein_duration_sec", float)
    , ECS_RO_COMP(burnt_ground__on_fire_event_es_comps, "burnt_ground_renderer__fadeout_duration_sec", float)
    , ECS_RO_COMP(burnt_ground__on_fire_event_es_comps, "burnt_ground_renderer__allowed_biome_groups", ecs::List<int>)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground__on_fire_event_es_es_desc
(
  "burnt_ground__on_fire_event_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground__on_fire_event_es_all_events),
  empty_span(),
  make_span(burnt_ground__on_fire_event_es_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<FireOnGroundEvent>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc burnt_ground_decal_placer_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("burnt_ground_decal_placer__time_passed"), ecs::ComponentTypeInfo<float>()},
//start of 5 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("burnt_ground_decal_placer__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_decal_placer__fade_in_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_decal_placer__fade_out_time"), ecs::ComponentTypeInfo<float>()}
};
static void burnt_ground_decal_placer_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground_decal_placer_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RO_COMP(burnt_ground_decal_placer_update_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(burnt_ground_decal_placer_update_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(burnt_ground_decal_placer_update_es_comps, "burnt_ground_decal_placer__radius", float)
    , ECS_RW_COMP(burnt_ground_decal_placer_update_es_comps, "burnt_ground_decal_placer__time_passed", float)
    , ECS_RO_COMP(burnt_ground_decal_placer_update_es_comps, "burnt_ground_decal_placer__fade_in_time", float)
    , ECS_RO_COMP(burnt_ground_decal_placer_update_es_comps, "burnt_ground_decal_placer__fade_out_time", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground_decal_placer_update_es_es_desc
(
  "burnt_ground_decal_placer_update_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground_decal_placer_update_es_all_events),
  make_span(burnt_ground_decal_placer_update_es_comps+0, 1)/*rw*/,
  make_span(burnt_ground_decal_placer_update_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_static_burnt_ground_decals_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("static_burnt_ground_decal__radius"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc init_static_burnt_ground_decals_ecs_query_desc
(
  "init_static_burnt_ground_decals_ecs_query",
  empty_span(),
  make_span(init_static_burnt_ground_decals_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_static_burnt_ground_decals_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, init_static_burnt_ground_decals_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(init_static_burnt_ground_decals_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(init_static_burnt_ground_decals_ecs_query_comps, "static_burnt_ground_decal__radius", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_burnt_ground_renderer_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("burnt_ground_renderer__clipmap_decal_type"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("burnt_ground_renderer__edge_noise_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_renderer__edge_width"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_burnt_ground_renderer_ecs_query_desc
(
  "get_burnt_ground_renderer_ecs_query",
  empty_span(),
  make_span(get_burnt_ground_renderer_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_burnt_ground_renderer_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_burnt_ground_renderer_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_burnt_ground_renderer_ecs_query_comps, "burnt_ground_renderer__clipmap_decal_type", int)
            , ECS_RO_COMP(get_burnt_ground_renderer_ecs_query_comps, "burnt_ground_renderer__edge_noise_scale", float)
            , ECS_RO_COMP(get_burnt_ground_renderer_ecs_query_comps, "burnt_ground_renderer__edge_width", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_burnt_ground_decals_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("burnt_ground_decal_placer__time_passed"), ecs::ComponentTypeInfo<float>()},
//start of 4 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("burnt_ground_decal_placer__decal_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_decal_placer__fade_in_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_decal_placer__fade_out_time"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc gather_burnt_ground_decals_ecs_query_desc
(
  "gather_burnt_ground_decals_ecs_query",
  make_span(gather_burnt_ground_decals_ecs_query_comps+0, 1)/*rw*/,
  make_span(gather_burnt_ground_decals_ecs_query_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_burnt_ground_decals_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, gather_burnt_ground_decals_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_burnt_ground_decals_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(gather_burnt_ground_decals_ecs_query_comps, "burnt_ground_decal_placer__decal_size", float)
            , ECS_RW_COMP(gather_burnt_ground_decals_ecs_query_comps, "burnt_ground_decal_placer__time_passed", float)
            , ECS_RO_COMP(gather_burnt_ground_decals_ecs_query_comps, "burnt_ground_decal_placer__fade_in_time", float)
            , ECS_RO_COMP(gather_burnt_ground_decals_ecs_query_comps, "burnt_ground_decal_placer__fade_out_time", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t burnt_ground_decal_placer__decal_size_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t burnt_ground_decal_placer__fade_in_time_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t burnt_ground_decal_placer__fade_out_time_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t burnt_ground_decal_placer__radius_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
