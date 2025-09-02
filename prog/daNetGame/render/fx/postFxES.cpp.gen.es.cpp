// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "postFxES.cpp.inl"
ECS_DEF_PULL_VAR(postFx);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc post_fx_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("damage_indicator__phase"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__stage"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("damage_indicator__startTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__prevLife"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__pulseState"), ecs::ComponentTypeInfo<Point3>()},
//start of 12 ro components at [5]
  {ECS_HASH("damage_indicator__thresholds"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("damage_indicator__blockDuration"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__lightPulsationFreq"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__lightIntensities"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__lightIntensitySaturations"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__mediumPulsationFreq"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__mediumIntensities"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__mediumIntensitySaturations"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__severePulsationFreq"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__severeIntensities"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__severeIntensitySaturations"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("damage_indicator__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void post_fx_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    post_fx_es(*info.cast<ecs::UpdateStageInfoAct>()
    , components.manager()
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__thresholds", Point3)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__blockDuration", float)
    , ECS_RW_COMP(post_fx_es_comps, "damage_indicator__phase", float)
    , ECS_RW_COMP(post_fx_es_comps, "damage_indicator__stage", int)
    , ECS_RW_COMP(post_fx_es_comps, "damage_indicator__startTime", float)
    , ECS_RW_COMP(post_fx_es_comps, "damage_indicator__prevLife", float)
    , ECS_RW_COMP(post_fx_es_comps, "damage_indicator__pulseState", Point3)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__lightPulsationFreq", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__lightIntensities", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__lightIntensitySaturations", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__mediumPulsationFreq", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__mediumIntensities", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__mediumIntensitySaturations", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__severePulsationFreq", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__severeIntensities", Point4)
    , ECS_RO_COMP(post_fx_es_comps, "damage_indicator__severeIntensitySaturations", Point4)
    , ECS_RO_COMP_OR(post_fx_es_comps, "damage_indicator__enabled", bool(true))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc post_fx_es_es_desc
(
  "post_fx_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(post_fx_es_all),
  make_span(post_fx_es_comps+0, 5)/*rw*/,
  make_span(post_fx_es_comps+5, 12)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc tonemap_update_es_comps[] =
{
//start of 4 rq components at [0]
  {ECS_HASH("color_grading"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("tonemap"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("white_balance"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemap_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  tonemap_update_es(evt
        );
}
static ecs::EntitySystemDesc tonemap_update_es_es_desc
(
  "tonemap_update_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemap_update_es_all_events),
  empty_span(),
  empty_span(),
  make_span(tonemap_update_es_comps+0, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,"color_grading,tonemap,white_balance");
static constexpr ecs::ComponentDesc post_fx_appear_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("generic_post_fx"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("post_fx"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void post_fx_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    post_fx_appear_es(evt
        , ECS_RW_COMP(post_fx_appear_es_comps, "generic_post_fx", ecs::Object)
    , ECS_RW_COMP(post_fx_appear_es_comps, "post_fx", ecs::Object)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc post_fx_appear_es_es_desc
(
  "post_fx_appear_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, post_fx_appear_es_all_events),
  make_span(post_fx_appear_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc post_fx_reload_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("post_fx"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void post_fx_reload_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    post_fx_reload_es(evt
        , ECS_RO_COMP(post_fx_reload_es_comps, "post_fx", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc post_fx_reload_es_es_desc
(
  "post_fx_reload_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, post_fx_reload_es_all_events),
  empty_span(),
  make_span(post_fx_reload_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel,
                       ReloadPostFx>::build(),
  0
,"render","post_fx");
static constexpr ecs::ComponentDesc override_post_fx_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("override_post_fx__params"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("override_post_fx__priority"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("override_post_fx__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void override_post_fx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    override_post_fx_es(evt
        , components.manager()
    , ECS_RO_COMP(override_post_fx_es_comps, "override_post_fx__params", ecs::Object)
    , ECS_RO_COMP(override_post_fx_es_comps, "override_post_fx__priority", int)
    , ECS_RO_COMP(override_post_fx_es_comps, "override_post_fx__enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc override_post_fx_es_es_desc
(
  "override_post_fx_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, override_post_fx_es_all_events),
  empty_span(),
  make_span(override_post_fx_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc hero_dof_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dof_target_id"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 3 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("fStop"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sensorHeight_mm"), ecs::ComponentTypeInfo<float>()}
};
static void hero_dof_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<UpdateStageInfoBeforeRender>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      hero_dof_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
            , ECS_RO_COMP(hero_dof_es_comps, "fStop", float)
      , ECS_RO_COMP(hero_dof_es_comps, "sensorHeight_mm", float)
      , ECS_RW_COMP(hero_dof_es_comps, "dof_target_id", ecs::EntityId)
      , ECS_RO_COMP(hero_dof_es_comps, "transform", TMatrix)
      , components.manager()
      );
    while (++comp != compE);
  } else {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      hero_dof_es(evt
            , ECS_RW_COMP(hero_dof_es_comps, "dof_target_id", ecs::EntityId)
      , ECS_RO_COMP(hero_dof_es_comps, "transform", TMatrix)
      );
    while (++comp != compE);
  }
}
static ecs::EntitySystemDesc hero_dof_es_es_desc
(
  "hero_dof_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hero_dof_es_all_events),
  make_span(hero_dof_es_comps+0, 1)/*rw*/,
  make_span(hero_dof_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc post_fx_blood_es_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("damage_indicator__phase"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__stage"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("damage_indicator__startTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__prevLife"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("damage_indicator__pulseState"), ecs::ComponentTypeInfo<Point3>()},
//start of 1 ro components at [5]
  {ECS_HASH("damage_indicator__thresholds"), ecs::ComponentTypeInfo<Point3>()}
};
static void post_fx_blood_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    post_fx_blood_es(evt
        , ECS_RO_COMP(post_fx_blood_es_comps, "damage_indicator__thresholds", Point3)
    , ECS_RW_COMP(post_fx_blood_es_comps, "damage_indicator__phase", float)
    , ECS_RW_COMP(post_fx_blood_es_comps, "damage_indicator__stage", int)
    , ECS_RW_COMP(post_fx_blood_es_comps, "damage_indicator__startTime", float)
    , ECS_RW_COMP(post_fx_blood_es_comps, "damage_indicator__prevLife", float)
    , ECS_RW_COMP(post_fx_blood_es_comps, "damage_indicator__pulseState", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc post_fx_blood_es_es_desc
(
  "post_fx_blood_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, post_fx_blood_es_all_events),
  make_span(post_fx_blood_es_comps+0, 5)/*rw*/,
  make_span(post_fx_blood_es_comps+5, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventHeroChanged,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc post_fx_disappear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("damage_indicator__phase"), ecs::ComponentTypeInfo<float>()}
};
static void post_fx_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  post_fx_disappear_es(evt
        );
}
static ecs::EntitySystemDesc post_fx_disappear_es_es_desc
(
  "post_fx_disappear_es",
  "prog/daNetGame/render/fx/postFxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, post_fx_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(post_fx_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gather_override_post_fx_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("override_post_fx__params"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("override_post_fx__priority"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("override_post_fx__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc gather_override_post_fx_ecs_query_desc
(
  "gather_override_post_fx_ecs_query",
  empty_span(),
  make_span(gather_override_post_fx_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_override_post_fx_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_override_post_fx_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(gather_override_post_fx_ecs_query_comps, "override_post_fx__enabled", bool)) )
            continue;
          function(
              ECS_RO_COMP(gather_override_post_fx_ecs_query_comps, "eid", ecs::EntityId)
            , components.manager()
            , ECS_RO_COMP(gather_override_post_fx_ecs_query_comps, "override_post_fx__params", ecs::Object)
            , ECS_RO_COMP(gather_override_post_fx_ecs_query_comps, "override_post_fx__priority", int)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc animchar_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc animchar_ecs_query_desc
(
  "animchar_ecs_query",
  empty_span(),
  make_span(animchar_ecs_query_comps+0, 2)/*ro*/,
  make_span(animchar_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void animchar_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(animchar_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(animchar_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_hero_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("hitpoints__maxHp"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("hitpoints__hp"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_hero_ecs_query_desc
(
  "get_hero_ecs_query",
  empty_span(),
  make_span(get_hero_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_hero_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_hero_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP_OR(get_hero_ecs_query_comps, "hitpoints__maxHp", float(1.f))
            , ECS_RO_COMP_OR(get_hero_ecs_query_comps, "hitpoints__hp", float(1.f))
            );

        }
    }
  );
}
