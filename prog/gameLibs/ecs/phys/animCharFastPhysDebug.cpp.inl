// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <animChar/dag_animCharacter2.h>
#include <debug/dag_assert.h>
#include <phys/dag_fastPhys.h>
#include <gameRes/dag_stdGameRes.h>
#include <ecs/render/updateStageRender.h>
#include <debug/dag_debug3d.h>
#include <util/dag_console.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <ecs/delayedAct/actInThread.h>
#include "fastPhysTag.h"

static dag::VectorSet<uint32_t> debugAnimCharsSet;
#define TEMPLATE_NAME "animchar_fast_phys_debug_render"
const char *template_name = TEMPLATE_NAME;
static void createTemplate()
{
  ecs::ComponentsMap map;
  map[ECS_HASH(TEMPLATE_NAME)] = ecs::Tag();
  eastl::string name = template_name;
  g_entity_mgr->addTemplate(ecs::Template(template_name, eastl::move(map), ecs::Template::component_set(),
    ecs::Template::component_set(), ecs::Template::component_set(), false));
  g_entity_mgr->instantiateTemplate(g_entity_mgr->buildTemplateIdByName(template_name));
}

static void removeSubTemplateAsync(ecs::EntityId eid)
{
  if (const char *fromTemplate = g_entity_mgr->getEntityTemplateName(eid))
  {
    if (!g_entity_mgr->getTemplateDB().getTemplateByName(template_name)) // -V595
      createTemplate();
    const auto newTemplate = remove_sub_template_name(fromTemplate, template_name);
    g_entity_mgr->reCreateEntityFromAsync(eid, newTemplate.c_str());
  }
}

static void addSubTemplateAsync(ecs::EntityId eid)
{
  if (const char *fromTemplate = g_entity_mgr->getEntityTemplateName(eid))
  {
    if (!g_entity_mgr->getTemplateDB().getTemplateByName(template_name)) // -V595
      createTemplate();
    const auto newTemplate = add_sub_template_name(fromTemplate, template_name);
    g_entity_mgr->reCreateEntityFromAsync(eid, newTemplate.c_str());
  }
}

template <typename Callable>
static void get_animchar_by_name_ecs_query(Callable c);


static void toggleDebugAnimChar(const eastl::string &str)
{
  auto it = debugAnimCharsSet.find(str_hash_fnv1(str.c_str()));
  if (it != debugAnimCharsSet.end())
  {
    get_animchar_by_name_ecs_query([&](ecs::EntityId eid, const ecs::string &animchar__res) {
      if (animchar__res == str)
        removeSubTemplateAsync(eid);
    });
    debugAnimCharsSet.erase(it);
  }
  else
  {
    get_animchar_by_name_ecs_query([&](ecs::EntityId eid, const ecs::string &animchar__res) {
      if (animchar__res == str)
        addSubTemplateAsync(eid);
    });
    debugAnimCharsSet.insert(str_hash_fnv1(str.c_str()));
  }
}

static void resetDebugAnimChars()
{
  for (auto &hash : debugAnimCharsSet)
  {
    get_animchar_by_name_ecs_query([&](ecs::EntityId eid, const ecs::string &animchar__res) {
      if (str_hash_fnv1(animchar__res.c_str()) == hash)
        removeSubTemplateAsync(eid);
    });
  }
  debugAnimCharsSet.clear();
}

ECS_NO_ORDER
ECS_REQUIRE(FastPhysTag animchar_fast_phys, ecs::Tag animchar_fast_phys_debug_render)
ECS_TAG(dev, render)
static void debug_draw_fast_phys_es(const UpdateStageInfoRenderDebug &, AnimV20::AnimcharBaseComponent &animchar)
{
  FastPhysSystem *fastPhys = animchar.getFastPhysSystem();
  if (!fastPhys)
    return;
  begin_draw_cached_debug_lines();

  TMatrix wtm = TMatrix::IDENT;
  vec3f ofs = animchar.getWtmOfs();
  wtm.setcol(3, as_point3(&ofs));
  ::set_cached_debug_lines_wtm(wtm);

  fastPhys->debugRender();

  for (auto action : fastPhys->updateActions)
    action->debugRender();

  end_draw_cached_debug_lines();
}

static bool fastphys_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("fastphys", "debug", 1, 2)
  {
    if (argc > 1)
    {
      const eastl::string resName(argv[1]);
      toggleDebugAnimChar(resName);
    }
    else
      resetDebugAnimChars();
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(fastphys_console_handler);
