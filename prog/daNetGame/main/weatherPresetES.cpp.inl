// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <main/weatherPreset.h>
#include <main/level.h>

#include <ecs/core/entityManager.h>
#include <math/random/dag_random.h>
#include <util/dag_string.h>

const ecs::Object::value_type *get_preset_by_name(ecs::EntityManager &mgr, ecs::EntityId eid, const char *preset_name)
{
  const ecs::Object &weatherChoice = mgr.get<ecs::Object>(eid, ECS_HASH("level__weatherChoice"));
  for (auto &w : weatherChoice)
  {
    if (w.first == preset_name)
      return &w;
  }

  return nullptr;
}

const ecs::Object::value_type *get_preset_by_seed(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::Object &weatherChoice = mgr.get<ecs::Object>(eid, ECS_HASH("level__weatherChoice"));

  int weatherSeed = mgr.get<int>(eid, ECS_HASH("level__weatherSeed")); // seed to have reproducable time of day
  if (weatherSeed == -1)
  {
    weatherSeed = grnd();
    mgr.set(eid, ECS_HASH("level__weatherSeed"), weatherSeed);
    logdbg("Generated weather seed: %d", weatherSeed);
  }

  float totalWt = 0.f;
  for (auto &w : weatherChoice)
  {
    if (w.second.is<float>())
      totalWt += w.second.get<float>();
    else
      totalWt += w.second.get<ecs::Object>().getMemberOr<float>("weight", 0.0f);
  }

  float randomWt = _rnd_float(weatherSeed, 0.f, totalWt);
  float curWt = 0.f;
  for (auto &w : weatherChoice)
  {
    if (w.second.is<float>())
      curWt += w.second.get<float>();
    else
      curWt += w.second.get<ecs::Object>().getMemberOr<float>("weight", 0.0f);

    if (curWt > randomWt)
      return &w;
  }

  return nullptr;
}

void create_weather_choice_entity(ecs::EntityManager &mgr, const ecs::ChildComponent &effect, ecs::EntityId eid)
{
  const ecs::Object &f = effect.get<ecs::Object>();
  const ecs::string *templ = f.getNullable<ecs::string>(ECS_HASH("template"));
  const ecs::Object *comps = f.getNullable<ecs::Object>(ECS_HASH("components"));

  if (!templ)
  {
    logerr("No template was given for a weather effect <%s>", mgr.getEntityTemplateName(eid));
    return;
  }

  ecs::ComponentsInitializer init;
  if (comps)
    for (const auto &kv : *comps)
      init[ECS_HASH_SLOW(kv.first.c_str())] = kv.second;

  mgr.createEntityAsync(String(0, "%s+weather_choice_created", templ->c_str()), eastl::move(init));
}

ecs::string get_current_weather_preset_name()
{
  ecs::EntityId levelEid = get_current_level_eid();
  return (g_entity_mgr->get<ecs::string>(levelEid, ECS_HASH("level__weather")));
}

void get_weather_preset_list(ecs::StringList &list)
{
  list.clear();

  ecs::EntityId levelEid = get_current_level_eid();
  ecs::EntityManager &mgr = *g_entity_mgr;

  const ecs::Object &weatherChoice = mgr.get<ecs::Object>(levelEid, ECS_HASH("level__weatherChoice"));
  for (auto &w : weatherChoice)
  {
    list.push_back(w.first);
  }
}

template <typename Callable>
static void query_rain_entity_ecs_query(Callable c);
bool has_any_rain_entity()
{
  bool found = false;
  query_rain_entity_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag rain_tag)) {
    G_UNUSED(eid);
    found = true;
  });

  return found;
}

template <typename Callable>
static void query_snow_entity_ecs_query(Callable c);
bool has_any_snow_entity()
{
  bool found = false;
  query_snow_entity_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag snow_tag)) {
    G_UNUSED(eid);
    found = true;
  });

  return found;
}

template <typename Callable>
static void query_lightning_entity_ecs_query(Callable c);
bool has_any_lightning_entity()
{
  bool found = false;
  query_lightning_entity_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag lightning_tag)) {
    G_UNUSED(eid);
    found = true;
  });

  return found;
}

static void create_weather_entities_by_template_name_substring(const char *template_name_substring)
{
  ecs::EntityId levelEid = get_current_level_eid();
  ecs::EntityManager &mgr = *g_entity_mgr;
  const char *presetName = (mgr.get<ecs::string>(levelEid, ECS_HASH("level__weather"))).c_str();
  const ecs::Object::value_type *preset = get_preset_by_name(mgr, levelEid, presetName);
  G_ASSERT_RETURN(preset, );

  bool found = false;
  if (preset->second.is<ecs::Object>())
  {
    const ecs::Array *effectsArray = preset->second.get<ecs::Object>().getNullable<ecs::Array>(ECS_HASH("entities"));
    if (effectsArray)
    {
      for (auto &effect : *effectsArray)
      {
        const ecs::string *name = effect.get<ecs::Object>().getNullable<ecs::string>(ECS_HASH("template"));
        if (name && strstr(name->c_str(), template_name_substring))
        {
          found = true;
          create_weather_choice_entity(mgr, effect, levelEid);
        }
      }
    }
  }

  if (!found)
  {
    logerr("There is no %s for preset <%s>", template_name_substring, presetName);
  }
}

template <typename Callable>
static void delete_rain_entities_ecs_query(Callable c);
void delete_rain_entities()
{
  delete_rain_entities_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag rain_tag)) { g_entity_mgr->destroyEntity(eid); });
}

template <typename Callable>
static void delete_snow_entities_ecs_query(Callable c);
void delete_snow_entities()
{
  delete_snow_entities_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag snow_tag)) { g_entity_mgr->destroyEntity(eid); });
}

template <typename Callable>
static void delete_lightning_entities_ecs_query(Callable c);
void delete_lightning_entities()
{
  delete_lightning_entities_ecs_query(
    [&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag lightning_tag)) { g_entity_mgr->destroyEntity(eid); });
}

void create_lightning_entities()
{
  if (has_any_lightning_entity())
    return;

  create_weather_entities_by_template_name_substring("lightning");
}

void create_rain_entities()
{
  if (has_any_rain_entity())
    return;

  create_weather_entities_by_template_name_substring("rain");
}

void create_snow_entities()
{
  if (has_any_snow_entity())
    return;

  create_weather_entities_by_template_name_substring("snow");
}
