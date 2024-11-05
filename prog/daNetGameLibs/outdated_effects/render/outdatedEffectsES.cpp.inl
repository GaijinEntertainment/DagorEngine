// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/fx.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <hash/mum_hash.h>
#include <startup/dag_globalSettings.h>
#include <main/appProfile.h>


static constexpr uint64_t HASH_SEED = 0;
static ska::flat_hash_set<uint64_t> outdated_effects_set;
static ska::flat_hash_set<uint64_t> checked_outdated_effects;
static bool check_once = true;
static bool check_all_effects = false;


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void register_outdated_effects_set_es_event_handler(const ecs::Event &, ecs::StringList &outdated_effects)
{
  outdated_effects_set.clear();
  outdated_effects_set.reserve(outdated_effects.size());

  for (const auto &effect : outdated_effects)
    outdated_effects_set.insert(mum_hash(effect.data(), effect.size(), HASH_SEED));

  outdated_effects.clear();
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(outdated_effects__check_once, outdated_effects__check_all_effects)
static inline void register_outdated_effects_params_es_event_handler(
  const ecs::Event &, bool outdated_effects__check_once, bool outdated_effects__check_all_effects)
{
  check_once = outdated_effects__check_once;
  check_all_effects = outdated_effects__check_all_effects;
}


static inline bool is_outdated_effect(const eastl::string &effect, bool skip_checked)
{
  if (outdated_effects_set.empty())
    return false;

  uint64_t hash = mum_hash(effect.data(), effect.size(), HASH_SEED);
  bool is_outdated = outdated_effects_set.find(hash) != outdated_effects_set.end();

  if (is_outdated && skip_checked)
    return checked_outdated_effects.insert(hash).second;

  return is_outdated;
}


static inline void validate_outdated_effect(const eastl::string &effect_name, const TMatrix &transform, ecs::EntityId eid)
{
#if DAGOR_DBGLEVEL == 0
  if (!app_profile::get().devMode)
    return;
#endif

  if (is_outdated_effect(effect_name, check_once))
    logerr("Using outdated effect \"%s\" _template:t=\"%s\" at pos=%@, eid=%d\n"
           "This effect will be removed soon.\n"
           "Please, replace it with some not outdated effect or remove.",
      effect_name, g_entity_mgr->getEntityTemplateName(eid), transform.getcol(3), eid);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void validate_outdated_one_effect_es_event_handler(
  const ecs::Event &, ecs::EntityId eid, const ecs::string &effect__name, const TMatrix &transform = TMatrix::IDENT)
{
  validate_outdated_effect(effect__name, transform, eid);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void validate_outdated_many_effects_es_event_handler(
  const ecs::Event &, ecs::EntityId eid, const ecs::Object &effects, const TMatrix &transform = TMatrix::IDENT)
{
  for (const auto &effect : effects)
    validate_outdated_effect(effect.first, transform, eid);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void validate_outdated_ground_effects_es_event_handler(
  const ecs::Event &, ecs::EntityId eid, const ecs::string &ground_effect__fx_name)
{
  validate_outdated_effect(ground_effect__fx_name, TMatrix::IDENT, eid);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void validate_outdated_effect_area_es_event_handler(
  const ecs::Event &, ecs::EntityId eid, const ecs::string &effect_area__effectName, const TMatrix &transform = TMatrix::IDENT)
{
  validate_outdated_effect(effect_area__effectName, transform, eid);
}


ECS_TAG(render, dev)
static inline void validate_outdated_start_effect_es_event_handler(const acesfx::StartEffectEvent &event)
{
  const char *fxName = acesfx::get_name_by_type(event.fxType);

  if (check_all_effects && is_outdated_effect(fxName, check_once))
    logerr("Using outdated effect '%s' at pos=%@\n"
           "This effect will be removed soon.\n"
           "Please, replace it with some not outdated effect",
      fxName, event.pos);
}