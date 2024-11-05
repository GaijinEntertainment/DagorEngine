// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsUtils.h"
#include "templateFilters.h"
#include "game/gameEvents.h"
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/io/blk.h>
#include <ecs/scene/scene.h>
#include <daECS/core/template.h>
#include <debug/dag_assert.h>
#include "app.h"
#include "net/net.h"
#include "net/dedicated.h"
#include <soundSystem/soundSystem.h>
#include "sound_net/soundNet.h"
#include <memory/dag_framemem.h>
#include <EASTL/algorithm.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_findFiles.h>
#include "render/renderer.h"
#include "input/globInput.h"
#include <ioSys/dag_dataBlock.h>
#include "main/appProfile.h"
#include <rapidJsonUtils/rapidJsonUtils.h>
#include <jsonUtils/decodeJwt.h>
#include "ui/uiShared.h"
#include <util/dag_console.h>

static bool load_templates_blk(
  ecs::TemplateRefs &trefs, const char *path, const char *ugm, ecs::service_datablock_cb cb = ecs::service_datablock_cb())
{
  DataBlock templatesBlk;
  if (!dblk::load(templatesBlk, path, dblk::ReadFlag::ROBUST_IN_REL))
    return false;
  if (ugm && ugm[0])
  {
    debug("load_ecs_templates: appending %s", ugm);
    templatesBlk.addStr("import", ugm);
  }
  ecs::load_templates_blk_file(*g_entity_mgr, path, templatesBlk, trefs, &g_entity_mgr->getMutableTemplateDBInfo(), cb);
  return true;
}

static eastl::string last_user_game_mode_import_fn, last_loaded_templates;
static constexpr uint32_t default_templates_tag = 0x1; // why not 1, but can be hash of path if we load several

void load_ecs_templates(const char *user_game_mode_import_fn)
{
  G_ASSERT(g_entity_mgr.get());
  ComponentToFilterAndPath netFilters, modifyBlackList;
  const char *tpath = dgs_get_settings()->getStr("entitiesPath", "content/common/gamedata/templates/entities.blk");
  ecs::TemplateRefs trefs(*g_entity_mgr);
  if (load_templates_blk(trefs, tpath, user_game_mode_import_fn, [&](const DataBlock &blk) {
        if (!has_network())
          return;
        const char *name = blk.getBlockName();
        if (strcmp(name, "_componentFilters") == 0)
        {
          if (is_true_net_server())
            read_component_filters(blk, netFilters);
        }
#if DAGOR_DBGLEVEL > 0
        else if (strcmp(name, "_replicatedComponentClientModifyBlacklist") == 0)
        {
          if (!is_server() && g_entity_mgr->getTemplateDB().info().filterTags.count(ECS_HASH("dev").hash))
            read_replicated_component_client_modify_blacklist(blk, modifyBlackList);
        }
#endif
      }))
  {
    last_loaded_templates = tpath;
    last_user_game_mode_import_fn = user_game_mode_import_fn ? user_game_mode_import_fn : "";
    g_entity_mgr->addTemplates(trefs, default_templates_tag);
    apply_component_filters(netFilters);
    apply_replicated_component_client_modify_blacklist(modifyBlackList);
  }
}


bool reload_ecs_templates(eastl::function<void(const char *, ecs::EntityManager::UpdateTemplateResult)> cb)
{
  const bool update_template_values_bool = true;

  ecs::TemplateRefs trefs(*g_entity_mgr);
  g_entity_mgr->getMutableTemplateDBInfo().resetMetaInfo();
  if (!load_templates_blk(trefs, last_loaded_templates.c_str(), last_user_game_mode_import_fn.c_str()))
  {
    logerr("Can't load %s", last_loaded_templates.c_str());
    return false;
  }

  return g_entity_mgr->updateTemplates(trefs, update_template_values_bool, default_templates_tag, cb);
}


static bool reload_templates_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  if (!g_entity_mgr)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("ecs", "reload_templates", 1, 2)
  {
    ecs::TemplateRefs trefs(*g_entity_mgr);
    const bool update_template_values_bool = argc > 1 ? console::to_bool(argv[1]) : true;
    g_entity_mgr->getMutableTemplateDBInfo().resetMetaInfo();
    if (!load_templates_blk(trefs, last_loaded_templates.c_str(), last_user_game_mode_import_fn.c_str()))
      console::print_d("Can't load %s. ecs.reload_templates [update_template_values_bool=on].", last_loaded_templates.c_str());
    else
    {
      uint32_t same = 0, added = 0, updated = 0, removed = 0, invalidNames = 0, wrongParents = 0, cantRemove = 0, diffTag = 0;
      const bool ret = g_entity_mgr->updateTemplates(trefs, update_template_values_bool, default_templates_tag,
        [&](const char *n, ecs::EntityManager::UpdateTemplateResult r) {
          switch (r)
          {
            case ecs::EntityManager::UpdateTemplateResult::Same: ++same; break;
            case ecs::EntityManager::UpdateTemplateResult::Added:
              ++added;
              console::print_d("%s added", n);
              break;
            case ecs::EntityManager::UpdateTemplateResult::Updated:
              ++updated;
              console::print_d("%s updated", n);
              break;
            case ecs::EntityManager::UpdateTemplateResult::Removed:
              ++removed;
              console::print_d("%s removed", n);
              break;
            case ecs::EntityManager::UpdateTemplateResult::InvalidName:
              ++invalidNames;
              console::print_d("%s wrong name", n);
              break;
            case ecs::EntityManager::UpdateTemplateResult::RemoveHasEntities:
              ++cantRemove;
              console::print_d("%s has entities and cant be removed", n);
              break;
            case ecs::EntityManager::UpdateTemplateResult::DifferentTag:
              ++diffTag;
              console::print_d("%s is registered with different tag and can't be updated", n);
              break;
            default: console::print_d("unknown err=%d, %s", (int)r, n); break;
          }
        });
      if (ret)
      {
        console::print_d("updated with update_template_values_bool=%s", update_template_values_bool ? "on" : "off");
        console::print_d("added=%d, removed=%d, updated=%d same=%d", added, removed, updated, same);
      }
      else
      {
        console::print_d("can't update: invalidNames=%d, wrongParents=%d, cantRemove=%d diffTag=%d", invalidNames, wrongParents,
          cantRemove, diffTag);
      }
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(reload_templates_console_handler);

ecs::EntityId create_simple_entity(const char *templ_name, ecs::ComponentsInitializer &&amap)
{
  ecs::EntityId eid = g_entity_mgr->createEntitySync(templ_name, eastl::move(amap));
  G_ASSERTF(eid != ecs::INVALID_ENTITY_ID, "Can't create entity for template '%s'. Perhaps you need update your data files.",
    templ_name);
  return eid;
}

Tab<const char *> ecs_get_global_tags_context()
{
  Tab<const char *> globalTags(framemem_ptr());
  if (has_network())
    globalTags.push_back("net");
  globalTags.push_back(is_server() ? "server" : "netClient"); // Note: currently is_server() might return false only within network
  if (have_glob_input()) // Warning: don't mind this condition, this can be implemented differently (e.g. with stubs) with same outcome
    globalTags.push_back("gameClient"); // i.e. client or local server
  if (sndsys::is_inited())
    globalTags.push_back("sound");
  if (have_sound_net())
    globalTags.push_back("soundNet");
  if (app_profile::get().haveGraphicsWindow)
    globalTags.push_back("render");
  if (app_profile::get().replay.record)
    globalTags.push_back("recordingReplay");
  if (auto pModeInfo = jsonutils::get_ptr<rapidjson::Value::ConstObject>(app_profile::get().matchingInviteData, "mode_info"))
    if (jsonutils::get_or(*pModeInfo, "writeReplay", false))
      globalTags.push_back("recordingReplay");
  if (uishared::is_ui_available_in_build())
    globalTags.push_back("ui");
  if (!app_profile::get().replay.playFile.empty())
    globalTags.push_back("playingReplay");
  if (have_glob_input())
    globalTags.push_back("input");
#if DAECS_EXTENSIVE_CHECKS
  if (!dgs_get_argv("noecsDebug"))
    globalTags.push_back("ecsDebug");
  if (!dgs_get_argv("nodev"))
    globalTags.push_back("dev");
#else
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC // Note: ECS 'dev' tag has perfomance implications hence don't enable it by default on consoles
  if (!dedicated::is_dedicated() && !dgs_get_argv("nodev"))
    globalTags.push_back("dev");
#endif
#endif

  // allow custom tags from command line only in debug builds for security
  bool allowCustomTags = app_profile::get().devMode;
#if DAGOR_DBGLEVEL > 0
  allowCustomTags = true;
#endif
  if (allowCustomTags)
  {
    int iterator = 0;
    while (const char *config_tag = ::dgs_get_argv("es_tag", iterator))
      globalTags.push_back(config_tag);
  }
  return globalTags;
}

static void load_es_order(
  Tab<SimpleString> &out_es_order, Tab<SimpleString> &out_es_skip, dag::ConstSpan<const char *> tags, const char *game_name)
{
  DataBlock blk(framemem_ptr());
  char path[DAGOR_MAX_PATH];
  SNPRINTF(path, sizeof(path), "content/%s/gamedata/es_order.blk", game_name);
  if (!dblk::load(blk, path, dblk::ReadFlag::ROBUST))
    if (!dblk::load(blk, "content/common/gamedata/es_order.blk", dblk::ReadFlag::ROBUST))
      debug("no es_order specified");
  ecs::load_es_order(*g_entity_mgr, blk, out_es_order, out_es_skip, tags);
}

void ecs_set_global_tags_context(const char *game_name, const char *user_game_mode_es_order_fn)
{
  Tab<const char *> globalTags = ecs_get_global_tags_context();
  g_entity_mgr->setFilterTags(globalTags);
  Tab<SimpleString> esOrder(framemem_ptr()), esSkip(framemem_ptr());
  load_es_order(esOrder, esSkip, globalTags, game_name);
  if (user_game_mode_es_order_fn)
  {
    debug("ecs_set_global_tags_context: appending %s", user_game_mode_es_order_fn);
    DataBlock blk(framemem_ptr());
    if (dblk::load(blk, user_game_mode_es_order_fn, dblk::ReadFlag::ROBUST))
      ecs::load_es_order(*g_entity_mgr, blk, esOrder, esSkip, globalTags);
    else
      logerr("ecs_set_global_tags_context: failed to load  %s", user_game_mode_es_order_fn);
  }
  g_entity_mgr->setEsOrder(dag::Span<const char *>((const char **)esOrder.data(), esOrder.size()),
    dag::Span<const char *>((const char **)esSkip.data(), esSkip.size()));
}
