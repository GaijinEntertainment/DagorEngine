// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <ecs/render/updateStageRender.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entitySystem.h>
#include <drv/3d/dag_info.h>
#include <render/renderer.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/shadowsManager.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include "globalManager.h"

ECS_REGISTER_BOXED_TYPE(dagdp::GlobalManager, nullptr);

namespace dagdp
{

static bool g_forceDisable = false;

ECS_TAG(render)
static void dagdp_update_es(const UpdateStageInfoBeforeRender &, dagdp::GlobalManager &dagdp__global_manager)
{
  dagdp__global_manager.update();
}

ECS_TAG(render)
static void dagdp_after_device_reset_es(const AfterDeviceReset &, dagdp::GlobalManager &dagdp__global_manager)
{
  // Force recreation of all GPU buffers.
  dagdp__global_manager.invalidateViews();
}

static GlobalConfig get_current_config()
{
  auto *wr = get_world_renderer();
  if (!wr)
    return {};

  FeatureRenderFlagMask renderFeatures = get_current_render_features();
  DeviceDriverCapabilities caps = d3d::get_driver_desc().caps;
  bool isForward = renderFeatures.test(FeatureRenderFlags::FORWARD_RENDERING);

  GlobalConfig config;
  config.enabled = !g_forceDisable && caps.hasWellSupportedIndirect && !isForward;
  // Not supporting forward rendering, which may be used on mobile.
  // TODO: more fine-grained configuration based on graphics settings.

  config.enableDynamicShadows = wr->getSettingsShadowsQuality() >= ShadowsQuality::SHADOWS_HIGH;
  config.dynamicShadowQualityParams = wr->getShadowRenderQualityParams();

  return config;
}

ECS_TAG(render)
static void dagdp_on_render_settings_change_es(const OnRenderSettingsReady &, dagdp::GlobalManager &dagdp__global_manager)
{
  // Note: relying on this event to be broadcast at level load.

  dagdp__global_manager.reconfigure(get_current_config());
}

ECS_TAG(render)
static void dagdp_on_render_features_change_es(const ChangeRenderFeatures &, dagdp::GlobalManager &dagdp__global_manager)
{
  // Always reconfigure (which implies invalidation) when render features have changed,
  // because shaders might be reloaded, and so cached data in the nodes may need to be reset.
  // We could do more fine-grained cache invalidation, but nuking the caches is simple and effective.
  dagdp__global_manager.reconfigure(get_current_config());
}

ECS_TAG(render)
static void dagdp_on_shader_reload_es(const AfterShaderReload &, dagdp::GlobalManager &dagdp__global_manager)
{
  dagdp__global_manager.reconfigure(get_current_config());
}

ECS_TAG(render)
static void dagdp_on_level_unload_es(const UnloadLevel &, dagdp::GlobalManager &dagdp__global_manager)
{
  GlobalConfig emptyConfig; // Everything disabled by default.
  dagdp__global_manager.reconfigure(emptyConfig);
}

template <typename Callable>
static inline void level_settings_ecs_query(Callable);

void GlobalManager::queryLevelSettings(RulesBuilder &rules_builder)
{
  level_settings_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag dagdp_level_settings) int dagdp__max_objects, bool dagdp__use_heightmap_dynamic_objects) {
      rules_builder.maxObjects = dagdp__max_objects;
      rules_builder.useHeightmapDynamicObjects = dagdp__use_heightmap_dynamic_objects;
    });
}

template <typename Callable>
static inline void object_groups_named_ecs_query(Callable);

template <typename Callable>
static inline void object_groups_nameless_ecs_query(Callable);

void GlobalManager::accumulateObjectGroups(RulesBuilder &rules_builder)
{
  object_groups_named_ecs_query([&](ECS_REQUIRE_NOT(ecs::Tag dagdp_placer) ECS_REQUIRE(ecs::Tag dagdp_object_group) ecs::EntityId eid,
                                  const ecs::string &dagdp__name) {
    uint32_t hash = NameMap::string_hash(dagdp__name.data(), dagdp__name.size());
    if (rules_builder.objectGroupNameMap.getNameId(dagdp__name.data(), dagdp__name.size(), hash) >= 0)
    {
      logerr("daGdp: duplicate object group name %s. Skipping it.", dagdp__name.c_str());
      return;
    }
    rules_builder.objectGroupNameMap.addNameId(dagdp__name.data(), dagdp__name.size(), hash);
    rules_builder.objectGroupsWithNames.push_back(eid);
    rules_builder.objectGroups.insert({eid});
  });

  object_groups_nameless_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_placer) ECS_REQUIRE(ecs::Tag dagdp_object_group) ecs::EntityId eid) {
    if (rules_builder.objectGroups.find(eid) != rules_builder.objectGroups.end())
    {
      logerr("daGdp: placer with EID %u can have only one anonymous object group. Skipping it.", static_cast<unsigned int>(eid));
      return;
    }
    rules_builder.objectGroups.insert({eid});
  });
}

template <typename Callable>
static inline void placers_ecs_query(Callable);

void GlobalManager::accumulatePlacers(RulesBuilder &rules_builder)
{
  placers_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_placer) ecs::EntityId eid, const ecs::StringList &dagdp__object_groups,
                      const ecs::string &dagdp__name) {
    bool ignoreMissingObjectGroups = false;
    PlacerInfo placerInfo;

    if (dagdp__name.empty())
    {
      logerr("daGdp: placer with EID %u has no name. Skipping it.", static_cast<unsigned int>(eid));
      return;
    }

    const uint32_t placerNameHash = NameMap::string_hash(dagdp__name.data(), dagdp__name.size());
    if (rules_builder.placerNameMap.getNameId(dagdp__name.data(), dagdp__name.size(), placerNameHash) >= 0)
    {
      logerr("daGdp: duplicate placer name '%s'. Skipping it.", dagdp__name.c_str());
      return;
    }

    rules_builder.placerNameMap.addNameId(dagdp__name.data(), dagdp__name.size(), placerNameHash);

    for (const auto &name : dagdp__object_groups)
    {
      const uint32_t nameHash = NameMap::string_hash(name.data(), name.size());
      if (const auto nameId = rules_builder.objectGroupNameMap.getNameId(name.data(), name.size(), nameHash); nameId >= 0)
        placerInfo.objectGroupEids.push_back(rules_builder.objectGroupsWithNames[nameId]);
      else
      {
        if (is_editor_activated())
        {
          // If not in the in-game editor, we don't know if the entity has not been created yet, so we can't show an error.
          logerr("daGdp: %s has unknown object group name %s.", dagdp__name, name.c_str());
        }
        else
          ignoreMissingObjectGroups = true;
      }
    }

    // If the current rule entity also has an object group template, we consider this a "shorthand notation":
    // instead of referencing the object group by name, the rule simply contains the (anonymous) object group.
    if (rules_builder.objectGroups.find(eid) != rules_builder.objectGroups.end())
      placerInfo.objectGroupEids.push_back(eid);

    if (!ignoreMissingObjectGroups && placerInfo.objectGroupEids.empty())
    {
      logerr("daGdp: %s has no object groups. Skipping it.", dagdp__name);
      return;
    }

    rules_builder.placers.insert({eid, eastl::move(placerInfo)});
  });
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name, dagdp__object_groups)
ECS_REQUIRE(ecs::Tag dagdp_placer, const ecs::string &dagdp__name, const ecs::StringList &dagdp__object_groups)
static void dagdp_placer_changed_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__max_objects)
ECS_TRACK(dagdp__use_heightmap_dynamic_objects)
ECS_REQUIRE(ecs::Tag dagdp_level_settings, int dagdp__max_objects, bool dagdp__use_heightmap_dynamic_objects)
static void dagdp_level_settings_changed_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

} // namespace dagdp

static bool dagdp_console_handler(const char *argv[], int argc)
{
  using namespace dagdp;
  int found = 0;
  CONSOLE_CHECK_NAME("dagdp", "toggle", 1, 1)
  {
    g_forceDisable = !g_forceDisable;
    if (g_forceDisable)
      console::print_d("daGDP forced to be fully disabled.");
    else
      console::print_d("daGDP re-enabled.");

    manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.reconfigure(get_current_config()); });
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dagdp_console_handler);
