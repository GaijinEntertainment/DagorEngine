// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <main/settingsOverride.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <main/level.h>
#include <main/settingsOverrideUtil.h>
#include <main/settings.h>
#include <startup/dag_loadSettings.h>
#include <startup/dag_globalSettings.h>

ECS_REGISTER_RELOCATABLE_TYPE(SettingsOverride, nullptr);

template <typename Callable>
static void apply_settings_override_entity_ecs_query(Callable);

void apply_settings_override_entity()
{
  apply_settings_override_entity_ecs_query([&](const SettingsOverride &settings_override__manager) {
    dgs_apply_config_blk(*settings_override__manager.getSettings(), false, false);
  });
}

template <typename Callable>
static void is_setting_overriden_ecs_query(Callable);

bool is_setting_overriden(const char *setting_path)
{
  bool result = false;
  is_setting_overriden_ecs_query(
    [&](const SettingsOverride &settings_override__manager) { result = settings_override__manager.isSettingOverriden(setting_path); });
  return result;
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(settings_override__useCustomSettings)
static void settings_override_quality_changed_es_event_handler(const ecs::Event &,
  SettingsOverride &settings_override__manager,
  const bool settings_override__useCustomSettings,
  const ecs::Object &settings_override__graphicsSettings,
  const ecs::Object *settings_override__graphicsSettings_pc = nullptr,
  const ecs::Object *settings_override__graphicsSettings_xboxone = nullptr,
  const ecs::Object *settings_override__graphicsSettings_scarlett = nullptr,
  const ecs::Object *settings_override__graphicsSettings_ps4 = nullptr,
  const ecs::Object *settings_override__graphicsSettings_ps5 = nullptr,
  const ecs::Object *settings_override__graphicsSettings_nswitch = nullptr,
  const ecs::Object *settings_override__graphicsSettings_android = nullptr,
  const ecs::Object *settings_override__graphicsSettings_ios = nullptr)
{
  G_UNUSED(settings_override__graphicsSettings_pc);
  G_UNUSED(settings_override__graphicsSettings_xboxone);
  G_UNUSED(settings_override__graphicsSettings_scarlett);
  G_UNUSED(settings_override__graphicsSettings_ps4);
  G_UNUSED(settings_override__graphicsSettings_ps5);
  G_UNUSED(settings_override__graphicsSettings_nswitch);
  G_UNUSED(settings_override__graphicsSettings_android);
  G_UNUSED(settings_override__graphicsSettings_ios);

  const ecs::Object defaultObject;
#define DEREF_STG(X) X ? *X : defaultObject

#if _TARGET_PC
  const ecs::Object &platformObject = DEREF_STG(settings_override__graphicsSettings_pc);
#elif _TARGET_XBOXONE
  const ecs::Object &platformObject = DEREF_STG(settings_override__graphicsSettings_xboxone);
#elif _TARGET_SCARLETT
  const ecs::Object &platformObject = DEREF_STG(settings_override__graphicsSettings_scarlett);
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_C3

#elif _TARGET_ANDROID
  const ecs::Object &platformObject = DEREF_STG(settings_override__graphicsSettings_android);
#elif _TARGET_IOS
  const ecs::Object &platformObject = DEREF_STG(settings_override__graphicsSettings_ios);
#else
  const ecs::Object &platformObject = defaultObject;
  logerr("No platform specific override for settings_override entity!");
#endif
#undef DEREF_STG

  if (!settings_override__useCustomSettings)
  {
    settings_override__manager.useDefaultConfigBlk();
  }
  else
  {
    const DataBlock baseBlock = convert_settings_object_to_blk(settings_override__graphicsSettings);
    G_ASSERTF(
      is_level_loaded() || !do_settings_changes_need_videomode_change(find_difference_in_config_blk(*dgs_get_settings(), baseBlock)),
      "settings_override__graphicsSettings contains parameter which requires device reset");
    settings_override__manager.useCustomConfigBlk(convert_settings_object_to_blk(platformObject, &baseBlock));
  }
}

ECS_REQUIRE(eastl::true_type settings_override__useCustomSettings)
ECS_TRACK(settings_override__graphicsSettings)
ECS_TRACK(settings_override__graphicsSettings_pc)
ECS_TRACK(settings_override__graphicsSettings_xboxone)
ECS_TRACK(settings_override__graphicsSettings_scarlett)
ECS_TRACK(settings_override__graphicsSettings_ps4)
ECS_TRACK(settings_override__graphicsSettings_ps5)
ECS_TRACK(settings_override__graphicsSettings_nswitch)
ECS_TRACK(settings_override__graphicsSettings_android)
ECS_TRACK(settings_override__graphicsSettings_ios)
static void settings_override_logerr_es_event_handler(const ecs::Event &,
  const ecs::Object &settings_override__graphicsSettings,
  const ecs::Object *settings_override__graphicsSettings_pc = nullptr,
  const ecs::Object *settings_override__graphicsSettings_xboxone = nullptr,
  const ecs::Object *settings_override__graphicsSettings_scarlett = nullptr,
  const ecs::Object *settings_override__graphicsSettings_ps4 = nullptr,
  const ecs::Object *settings_override__graphicsSettings_ps5 = nullptr,
  const ecs::Object *settings_override__graphicsSettings_nswitch = nullptr,
  const ecs::Object *settings_override__graphicsSettings_android = nullptr,
  const ecs::Object *settings_override__graphicsSettings_ios = nullptr)
{
  G_UNUSED(settings_override__graphicsSettings);
  G_UNUSED(settings_override__graphicsSettings_pc);
  G_UNUSED(settings_override__graphicsSettings_xboxone);
  G_UNUSED(settings_override__graphicsSettings_scarlett);
  G_UNUSED(settings_override__graphicsSettings_ps4);
  G_UNUSED(settings_override__graphicsSettings_ps5);
  G_UNUSED(settings_override__graphicsSettings_nswitch);
  G_UNUSED(settings_override__graphicsSettings_android);
  G_UNUSED(settings_override__graphicsSettings_ios);
  logerr("Do not change settings_override__graphicsSettings.* while settings_override__useCustomSettings:b=yes !");
}