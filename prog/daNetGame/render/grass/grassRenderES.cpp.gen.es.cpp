// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "grassRenderES.cpp.inl"
ECS_DEF_PULL_VAR(grassRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc grass_render_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static void grass_render_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grass_render_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(grass_render_update_es_comps, "grass_render", GrassRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grass_render_update_es_es_desc
(
  "grass_render_update_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_render_update_es_all_events),
  make_span(grass_render_update_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc track_fast_grass_settings_es_comps[] =
{
//start of 15 rq components at [0]
  {ECS_HASH("fast_grass__ao_curve"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__ao_max_strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__fade_end"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__fade_start"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__height_variance_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__normal_fade_end"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__normal_fade_start"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__placed_fade_end"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__placed_fade_start"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__slice_step"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__smoothness_fade_end"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__smoothness_fade_start"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__step_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__max_samples"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("fast_grass__num_samples"), ecs::ComponentTypeInfo<int>()}
};
static void track_fast_grass_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  track_fast_grass_settings_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc track_fast_grass_settings_es_es_desc
(
  "track_fast_grass_settings_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_fast_grass_settings_es_all_events),
  empty_span(),
  empty_span(),
  make_span(track_fast_grass_settings_es_comps+0, 15)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","fast_grass__ao_curve,fast_grass__ao_max_strength,fast_grass__fade_end,fast_grass__fade_start,fast_grass__height_variance_scale,fast_grass__max_samples,fast_grass__normal_fade_end,fast_grass__normal_fade_start,fast_grass__num_samples,fast_grass__placed_fade_end,fast_grass__placed_fade_start,fast_grass__slice_step,fast_grass__smoothness_fade_end,fast_grass__smoothness_fade_start,fast_grass__step_scale");
static constexpr ecs::ComponentDesc track_fast_grass_settings2_es_comps[] =
{
//start of 4 rq components at [0]
  {ECS_HASH("fast_grass__hmap_cell_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__hmap_range"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__clipmap_cascades"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("fast_grass__clipmap_resolution"), ecs::ComponentTypeInfo<int>()}
};
static void track_fast_grass_settings2_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  track_fast_grass_settings2_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc track_fast_grass_settings2_es_es_desc
(
  "track_fast_grass_settings2_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_fast_grass_settings2_es_all_events),
  empty_span(),
  empty_span(),
  make_span(track_fast_grass_settings2_es_comps+0, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","fast_grass__clipmap_cascades,fast_grass__clipmap_resolution,fast_grass__hmap_cell_size,fast_grass__hmap_range");
static constexpr ecs::ComponentDesc track_fast_grass_types_es_comps[] =
{
//start of 13 rq components at [0]
  {ECS_HASH("fast_grass__color_mask_b_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_b_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_g_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_g_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_r_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_r_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("dagdp__biomes"), ecs::ComponentTypeInfo<ecs::List<int>>()},
  {ECS_HASH("fast_grass__impostor"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("fast_grass_type"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("fast_grass__height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__height_variance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__stiffness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__weight_to_height_mul"), ecs::ComponentTypeInfo<float>()}
};
static void track_fast_grass_types_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  track_fast_grass_types_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc track_fast_grass_types_es_es_desc
(
  "track_fast_grass_types_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, track_fast_grass_types_es_all_events),
  empty_span(),
  empty_span(),
  make_span(track_fast_grass_types_es_comps+0, 13)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__biomes,fast_grass__color_mask_b_from,fast_grass__color_mask_b_to,fast_grass__color_mask_g_from,fast_grass__color_mask_g_to,fast_grass__color_mask_r_from,fast_grass__color_mask_r_to,fast_grass__height,fast_grass__height_variance,fast_grass__impostor,fast_grass__stiffness,fast_grass__weight_to_height_mul");
static constexpr ecs::ComponentDesc fast_grass_on_render_settings_change_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("render_settings__grassRendInstRangeMul"), ecs::ComponentTypeInfo<float>()}
};
static void fast_grass_on_render_settings_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  fast_grass_on_render_settings_change_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc fast_grass_on_render_settings_change_es_es_desc
(
  "fast_grass_on_render_settings_change_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, fast_grass_on_render_settings_change_es_all_events),
  empty_span(),
  empty_span(),
  make_span(fast_grass_on_render_settings_change_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__grassRendInstRangeMul","*");
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
        , components.manager()
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
static constexpr ecs::ComponentDesc grass_change_set_ereaser_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static void grass_change_set_ereaser_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetGrassSdfEareaser>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    grass_change_set_ereaser_es(static_cast<const SetGrassSdfEareaser&>(evt)
        , ECS_RW_COMP(grass_change_set_ereaser_es_comps, "grass_render", GrassRenderer)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc grass_change_set_ereaser_es_es_desc
(
  "grass_change_set_ereaser_es",
  "prog/daNetGame/render/grass/grassRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, grass_change_set_ereaser_es_all_events),
  make_span(grass_change_set_ereaser_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetGrassSdfEareaser>::build(),
  0
);
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
inline void get_grass_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_grass_render_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc grass_range_mul_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("render_settings__grassRendInstRangeMul"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc grass_range_mul_ecs_query_desc
(
  "grass_range_mul_ecs_query",
  make_span(grass_range_mul_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void grass_range_mul_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, grass_range_mul_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(grass_range_mul_ecs_query_comps, "render_settings__grassRendInstRangeMul", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc fast_grass_types_ecs_query_comps[] =
{
//start of 13 ro components at [0]
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp__biomes"), ecs::ComponentTypeInfo<ecs::List<int>>()},
  {ECS_HASH("fast_grass__impostor"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("fast_grass__height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__weight_to_height_mul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__height_variance"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__stiffness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("fast_grass__color_mask_r_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_r_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_g_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_g_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_b_from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("fast_grass__color_mask_b_to"), ecs::ComponentTypeInfo<E3DCOLOR>()},
//start of 1 rq components at [13]
  {ECS_HASH("fast_grass_type"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc fast_grass_types_ecs_query_desc
(
  "fast_grass_types_ecs_query",
  empty_span(),
  make_span(fast_grass_types_ecs_query_comps+0, 13)/*ro*/,
  make_span(fast_grass_types_ecs_query_comps+13, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void fast_grass_types_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, fast_grass_types_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(fast_grass_types_ecs_query_comps, "dagdp__name", ecs::string)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "dagdp__biomes", ecs::List<int>)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__impostor", ecs::string)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__height", float)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__weight_to_height_mul", float)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__height_variance", float)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__stiffness", float)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__color_mask_r_from", E3DCOLOR)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__color_mask_r_to", E3DCOLOR)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__color_mask_g_from", E3DCOLOR)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__color_mask_g_to", E3DCOLOR)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__color_mask_b_from", E3DCOLOR)
            , ECS_RO_COMP(fast_grass_types_ecs_query_comps, "fast_grass__color_mask_b_to", E3DCOLOR)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc grass_max_range_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__grass_max_range"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc grass_max_range_ecs_query_desc
(
  "grass_max_range_ecs_query",
  make_span(grass_max_range_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void grass_max_range_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, grass_max_range_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(grass_max_range_ecs_query_comps, "dagdp__grass_max_range", float)
            );

        }while (++comp != compE);
    }
  );
}
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
inline void grass_erasers_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, grass_erasers_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc init_grass_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static ecs::CompileTimeQueryDesc init_grass_render_ecs_query_desc
(
  "init_grass_render_ecs_query",
  make_span(init_grass_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_grass_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, init_grass_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(init_grass_render_ecs_query_comps, "grass_render", GrassRenderer)
            );

        }while (++comp != compE);
    }
  );
}
