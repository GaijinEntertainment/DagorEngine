// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bloodPuddlesInitES.cpp.inl"
ECS_DEF_PULL_VAR(bloodPuddlesInit);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc reset_blood_es_comps[] ={};
static void reset_blood_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  reset_blood_es(static_cast<const AfterDeviceReset&>(evt)
        );
}
static ecs::EntitySystemDesc reset_blood_es_es_desc
(
  "reset_blood_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesInitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_blood_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
);
static constexpr ecs::ComponentDesc update_blood_puddles_group_settings_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("blood_puddles_settings__groupAttributes"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void update_blood_puddles_group_settings_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_blood_puddles_group_settings_es(evt
        , ECS_RO_COMP(update_blood_puddles_group_settings_es_comps, "blood_puddles_settings__groupAttributes", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_blood_puddles_group_settings_es_es_desc
(
  "update_blood_puddles_group_settings_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesInitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_blood_puddles_group_settings_es_all_events),
  empty_span(),
  make_span(update_blood_puddles_group_settings_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc update_blood_puddles_shader_params_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("blood_puddles_settings__visualAttributes"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void update_blood_puddles_shader_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_blood_puddles_shader_params_es(evt
        , ECS_RO_COMP(update_blood_puddles_shader_params_es_comps, "blood_puddles_settings__visualAttributes", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_blood_puddles_shader_params_es_es_desc
(
  "update_blood_puddles_shader_params_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodPuddlesInitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_blood_puddles_shader_params_es_all_events),
  empty_span(),
  make_span(update_blood_puddles_shader_params_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc get_blood_settings_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("blood_puddles_settings__atlas"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("blood_puddles_settings__groupAttributes"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc get_blood_settings_ecs_query_desc
(
  "get_blood_settings_ecs_query",
  empty_span(),
  make_span(get_blood_settings_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_blood_settings_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_blood_settings_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_blood_settings_ecs_query_comps, "blood_puddles_settings__atlas", ecs::Object)
            , ECS_RO_COMP(get_blood_settings_ecs_query_comps, "blood_puddles_settings__groupAttributes", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
