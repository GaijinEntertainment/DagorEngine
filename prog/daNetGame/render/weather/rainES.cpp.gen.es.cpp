// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "rainES.cpp.inl"
ECS_DEF_PULL_VAR(rain);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_rain_effects_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("rain"), ecs::ComponentTypeInfo<Rain>()},
//start of 2 ro components at [1]
  {ECS_HASH("puddles__growthRate"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("puddles__growthLimit"), ecs::ComponentTypeInfo<float>()}
};
static void update_rain_effects_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    update_rain_effects_es(*info.cast<ecs::UpdateStageInfoBeforeRender>()
    , ECS_RW_COMP(update_rain_effects_es_comps, "rain", Rain)
    , ECS_RO_COMP(update_rain_effects_es_comps, "puddles__growthRate", float)
    , ECS_RO_COMP(update_rain_effects_es_comps, "puddles__growthLimit", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_rain_effects_es_es_desc
(
  "update_rain_effects_es",
  "prog/daNetGame/render/weather/rainES.cpp.inl",
  ecs::EntitySystemOps(update_rain_effects_es_all),
  make_span(update_rain_effects_es_comps+0, 1)/*rw*/,
  make_span(update_rain_effects_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoBeforeRender::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc update_drop_effects_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
//start of 1 ro components at [1]
  {ECS_HASH("drop_splash_fx__spawnRate"), ecs::ComponentTypeInfo<float>()}
};
static void update_drop_effects_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_drop_effects_es(evt
        , ECS_RO_COMP(update_drop_effects_es_comps, "drop_splash_fx__spawnRate", float)
    , ECS_RW_COMP(update_drop_effects_es_comps, "effect", TheEffect)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_drop_effects_es_es_desc
(
  "update_drop_effects_es",
  "prog/daNetGame/render/weather/rainES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_drop_effects_es_all_events),
  make_span(update_drop_effects_es_comps+0, 1)/*rw*/,
  make_span(update_drop_effects_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc rain_es_event_handler_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("rain"), ecs::ComponentTypeInfo<Rain>()},
  {ECS_HASH("effect"), ecs::ComponentTypeInfo<TheEffect>()},
  {ECS_HASH("drop_splashes__splashesFxId"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 9 ro components at [4]
  {ECS_HASH("far_rain__length"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("far_rain__speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("far_rain__width"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("far_rain__density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("far_rain__alpha"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("wetness__strength"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("drop_splashes__spriteYPos"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("drop_fx_template"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("far_rain__maxDensity"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void rain_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    rain_es_event_handler(evt
        , ECS_RW_COMP(rain_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP(rain_es_event_handler_comps, "rain", Rain)
    , ECS_RO_COMP(rain_es_event_handler_comps, "far_rain__length", float)
    , ECS_RO_COMP(rain_es_event_handler_comps, "far_rain__speed", float)
    , ECS_RO_COMP(rain_es_event_handler_comps, "far_rain__width", float)
    , ECS_RO_COMP(rain_es_event_handler_comps, "far_rain__density", float)
    , ECS_RO_COMP(rain_es_event_handler_comps, "far_rain__alpha", float)
    , ECS_RO_COMP(rain_es_event_handler_comps, "wetness__strength", float)
    , ECS_RO_COMP(rain_es_event_handler_comps, "drop_splashes__spriteYPos", float)
    , ECS_RW_COMP(rain_es_event_handler_comps, "effect", TheEffect)
    , ECS_RO_COMP(rain_es_event_handler_comps, "drop_fx_template", ecs::string)
    , ECS_RW_COMP(rain_es_event_handler_comps, "drop_splashes__splashesFxId", ecs::EntityId)
    , ECS_RO_COMP_OR(rain_es_event_handler_comps, "far_rain__maxDensity", float(10))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc rain_es_event_handler_es_desc
(
  "rain_es",
  "prog/daNetGame/render/weather/rainES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rain_es_event_handler_all_events),
  make_span(rain_es_event_handler_comps+0, 4)/*rw*/,
  make_span(rain_es_event_handler_comps+4, 9)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"*");
static constexpr ecs::ComponentDesc detach_rain_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("drop_splashes__splashesFxId"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("rain"), ecs::ComponentTypeInfo<Rain>()}
};
static void detach_rain_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    detach_rain_es_event_handler(evt
        , ECS_RW_COMP(detach_rain_es_event_handler_comps, "drop_splashes__splashesFxId", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc detach_rain_es_event_handler_es_desc
(
  "detach_rain_es",
  "prog/daNetGame/render/weather/rainES.cpp.inl",
  ecs::EntitySystemOps(nullptr, detach_rain_es_event_handler_all_events),
  make_span(detach_rain_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(detach_rain_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc set_params_for_splashes_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("drop_splash_fx__spawnRate"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc set_params_for_splashes_ecs_query_desc
(
  "set_params_for_splashes_ecs_query",
  make_span(set_params_for_splashes_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_params_for_splashes_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, set_params_for_splashes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(set_params_for_splashes_ecs_query_comps, "drop_splash_fx__spawnRate", float)
            );

        }
    }
  );
}
