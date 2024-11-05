#include "settingsOverrideES.cpp.inl"
ECS_DEF_PULL_VAR(settingsOverride);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc settings_override_quality_changed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("settings_override__manager"), ecs::ComponentTypeInfo<SettingsOverride>()},
//start of 10 ro components at [1]
  {ECS_HASH("settings_override__useCustomSettings"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("settings_override__graphicsSettings"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_pc"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_xboxone"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_scarlett"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_ps4"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_ps5"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_nswitch"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_android"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_ios"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void settings_override_quality_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    settings_override_quality_changed_es_event_handler(evt
        , ECS_RW_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__manager", SettingsOverride)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__useCustomSettings", bool)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_pc", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_xboxone", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_scarlett", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_ps4", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_ps5", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_nswitch", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_android", ecs::Object)
    , ECS_RO_COMP(settings_override_quality_changed_es_event_handler_comps, "settings_override__graphicsSettings_ios", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc settings_override_quality_changed_es_event_handler_es_desc
(
  "settings_override_quality_changed_es",
  "prog/daNetGame/main/settingsOverrideES.cpp.inl",
  ecs::EntitySystemOps(nullptr, settings_override_quality_changed_es_event_handler_all_events),
  make_span(settings_override_quality_changed_es_event_handler_comps+0, 1)/*rw*/,
  make_span(settings_override_quality_changed_es_event_handler_comps+1, 10)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"settings_override__useCustomSettings");
static constexpr ecs::ComponentDesc settings_override_logerr_es_event_handler_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("settings_override__graphicsSettings"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_pc"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_xboxone"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_scarlett"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_ps4"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_ps5"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_nswitch"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_android"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__graphicsSettings_ios"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("settings_override__useCustomSettings"), ecs::ComponentTypeInfo<bool>()}
};
static void settings_override_logerr_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__useCustomSettings", bool)) )
      continue;
    settings_override_logerr_es_event_handler(evt
          , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_pc", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_xboxone", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_scarlett", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_ps4", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_ps5", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_nswitch", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_android", ecs::Object)
      , ECS_RO_COMP(settings_override_logerr_es_event_handler_comps, "settings_override__graphicsSettings_ios", ecs::Object)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc settings_override_logerr_es_event_handler_es_desc
(
  "settings_override_logerr_es",
  "prog/daNetGame/main/settingsOverrideES.cpp.inl",
  ecs::EntitySystemOps(nullptr, settings_override_logerr_es_event_handler_all_events),
  empty_span(),
  make_span(settings_override_logerr_es_event_handler_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"settings_override__graphicsSettings,settings_override__graphicsSettings_android,settings_override__graphicsSettings_ios,settings_override__graphicsSettings_nswitch,settings_override__graphicsSettings_pc,settings_override__graphicsSettings_ps4,settings_override__graphicsSettings_ps5,settings_override__graphicsSettings_scarlett,settings_override__graphicsSettings_xboxone");
static constexpr ecs::ComponentDesc apply_settings_override_entity_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("settings_override__manager"), ecs::ComponentTypeInfo<SettingsOverride>()}
};
static ecs::CompileTimeQueryDesc apply_settings_override_entity_ecs_query_desc
(
  "apply_settings_override_entity_ecs_query",
  empty_span(),
  make_span(apply_settings_override_entity_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void apply_settings_override_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, apply_settings_override_entity_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(apply_settings_override_entity_ecs_query_comps, "settings_override__manager", SettingsOverride)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc is_setting_overriden_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("settings_override__manager"), ecs::ComponentTypeInfo<SettingsOverride>()}
};
static ecs::CompileTimeQueryDesc is_setting_overriden_ecs_query_desc
(
  "is_setting_overriden_ecs_query",
  empty_span(),
  make_span(is_setting_overriden_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_setting_overriden_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_setting_overriden_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(is_setting_overriden_ecs_query_comps, "settings_override__manager", SettingsOverride)
            );

        }while (++comp != compE);
    }
  );
}
