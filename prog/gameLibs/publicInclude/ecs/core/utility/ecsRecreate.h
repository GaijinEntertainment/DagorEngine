//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityManager.h>
#include <EASTL/fixed_string.h>
#include <daECS/core/componentsMap.h>
#include <debug/dag_assert.h>

#define ECS_DEFAULT_RECREATE_STR_T eastl::fixed_string<char, 256>

template <typename Lambda>
inline void for_each_sub_template_name(const char *template_name, Lambda fn)
{
  if (!template_name)
    return;
  const char *p = template_name;
  eastl::string tplName;
  for (;;)
  {
    const char ch = *p++;
    if (ch == '+' || ch == '\0')
    {
      fn(tplName.c_str());
      if (!ch)
        break;
      tplName.clear();
    }
    else
      tplName += ch;
  }
}

inline const char *find_sub_template_name(const char *templ_name, const char *sub_templ_name)
{
  G_ASSERT_RETURN(templ_name && sub_templ_name, "");
  if (!*templ_name || !*sub_templ_name)
    return nullptr;
  const char *at = templ_name;
  const int len = strlen(sub_templ_name);
  for (;;)
  {
    at = strstr(at, sub_templ_name);
    if (!at)
      break;
    const char ch = at[len];
    if ((at == templ_name || at[-1] == '+') && (ch == '\0' || ch == '+'))
      return at;
    ++at;
  }
  return nullptr;
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline S add_sub_template_name(const char *from_templ_name, const char *add_templ_name, const char *def = nullptr)
{
  if (const char *at = find_sub_template_name(from_templ_name, add_templ_name)) // part already exist?
  {
    return S(def ? def : from_templ_name);
  }
  S newTemplName;
  newTemplName = from_templ_name;
  newTemplName += '+';
  newTemplName += add_templ_name;
  return newTemplName;
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline S add_sub_template_name(ecs::EntityId eid, const char *add_templ_name, const char *def = nullptr)
{
  G_ASSERT_RETURN(add_templ_name && *add_templ_name, "");
  const char *fromTemplName = g_entity_mgr->getEntityFutureTemplateName(eid);
  if (!fromTemplName)
  {
    if (!g_entity_mgr->getEntityTemplateName(eid))
      logwarn("Entity %d does not exist (+%s)", (ecs::entity_id_t)eid, add_templ_name);
    return "";
  }
  return add_sub_template_name<S>(fromTemplName, add_templ_name, def);
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline S remove_sub_template_name(const char *from_templ_name, const char *remove_name_str, const char *def = nullptr)
{
  const char *removePos = find_sub_template_name(from_templ_name, remove_name_str);
  if (!removePos || removePos == from_templ_name || removePos[-1] != '+') // part not exist or not subtemplate?
    return S(def ? def : from_templ_name);
  S newTemplName = from_templ_name;
  eastl_size_t subTemplNameEndPos = newTemplName.find('+', removePos - from_templ_name);
  eastl_size_t startPos = (removePos - from_templ_name) - 1;
  newTemplName.erase(startPos, subTemplNameEndPos - startPos); // erase from '+' until to the next '+' (or string end)
  return newTemplName;
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline S remove_sub_template_name(ecs::EntityId eid, const char *remove_name_str, const char *def = nullptr)
{
  G_ASSERT_RETURN(remove_name_str && *remove_name_str, "");
  const char *fromTemplName = g_entity_mgr->getEntityFutureTemplateName(eid);
  if (!fromTemplName)
  {
    logwarn("Entity %d does not exist (-%s)", (ecs::entity_id_t)eid, remove_name_str);
    return "";
  }
  return remove_sub_template_name<S>(fromTemplName, remove_name_str, def);
}

template <class ChangeFunc, class ReCreateFunc>
inline ecs::EntityId change_sub_template(ecs::EntityId eid, ChangeFunc &&change_templ, ReCreateFunc &&recreate, bool ignore_destroyed)
{
  G_UNUSED(eid);
  auto newTemplName = change_templ();
  if (newTemplName.empty())
    return ignore_destroyed ? eid : ecs::INVALID_ENTITY_ID;
  ecs::EntityId reid = recreate(newTemplName.c_str());
#if DAGOR_DBGLEVEL > 0
  if (!reid)
    logerr("Failed to recreate entity %d<%s> to <%s>", (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid),
      newTemplName.c_str());
#endif
  return reid;
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline ecs::EntityId add_sub_template_async(ecs::EntityId eid, const char *add_name_str, ecs::ComponentsInitializer &&init = {},
  ecs::create_entity_async_cb_t &&cb = {}, bool ignore_destroyed = false)
{
  bool initEmpty = init.empty();
  return change_sub_template(
    eid, [&]() { return add_sub_template_name<S>(eid, add_name_str, initEmpty ? "" : nullptr); },
    [&](const char *templ) {
      return g_entity_mgr->reCreateEntityFromAsync(eid, templ, eastl::move(init), ecs::ComponentsMap(), eastl::move(cb));
    },
    ignore_destroyed);
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline ecs::EntityId remove_sub_template_async(ecs::EntityId eid, const char *remove_name_str, ecs::ComponentsInitializer &&init = {},
  ecs::create_entity_async_cb_t &&cb = {}, bool ignore_destroyed = false)
{
  bool initEmpty = init.empty();
  return change_sub_template(
    eid, [&]() { return remove_sub_template_name<S>(eid, remove_name_str, initEmpty ? "" : nullptr); },
    [&](const char *templ) {
      return g_entity_mgr->reCreateEntityFromAsync(eid, templ, eastl::move(init), ecs::ComponentsMap(), eastl::move(cb));
    },
    ignore_destroyed);
}


template <typename S = ECS_DEFAULT_RECREATE_STR_T, typename Func = void>
inline S change_template_name(ecs::EntityId eid, Func &&func, const char *def)
{
  const char *fromTemplName = g_entity_mgr->getEntityFutureTemplateName(eid);
  if (!fromTemplName)
  {
    logwarn("Entity %d does not exist", ecs::entity_id_t(eid));
    return "";
  }
  S s(fromTemplName);
  func(s, def);
  return s;
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T, typename Func = void>
inline ecs::EntityId change_template_name_async(ecs::EntityId eid, Func &&func, ecs::ComponentsInitializer &&init = {},
  ecs::create_entity_async_cb_t &&cb = {}, bool ignore_destroyed = false)
{
  bool initEmpty = init.empty();
  return change_sub_template(
    eid, [&]() { return change_template_name<S>(eid, eastl::forward<Func>(func), initEmpty ? "" : nullptr); },
    [&](const char *templ) {
      return g_entity_mgr->reCreateEntityFromAsync(eid, templ, eastl::move(init), ecs::ComponentsMap(), eastl::move(cb));
    },
    ignore_destroyed);
}
