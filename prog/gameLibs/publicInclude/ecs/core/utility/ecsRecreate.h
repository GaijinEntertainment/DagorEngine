//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

template <typename T1, typename T2, typename Callback>
inline void for_each_sub_template_name_iter(T1 full_template_begin, T2 full_template_end, Callback callback)
{
  if (full_template_begin == full_template_end)
    return;

  auto curSubTemplateBegin = full_template_begin;
  auto curSubTemplateEnd = eastl::find(full_template_begin, full_template_end, '+');

  for (;; curSubTemplateBegin = eastl::next(curSubTemplateEnd),
          curSubTemplateEnd = eastl::find(curSubTemplateBegin, full_template_end, '+'))
  {
    callback(curSubTemplateBegin, curSubTemplateEnd);

    if (curSubTemplateEnd == full_template_end)
    {
      break;
    }
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

template <typename T1, typename T2, typename U1, typename U2>
inline T1 find_sub_template_name_iter(T1 full_name_begin, T2 full_name_end, U1 sub_name_begin, U2 sub_name_end)
{
  auto entryEnd = full_name_begin;
  for (auto entryStart = eastl::search(full_name_begin, full_name_end, sub_name_begin, sub_name_end); entryStart != full_name_end;
       entryStart = eastl::search(entryEnd, full_name_end, sub_name_begin, sub_name_end))
  {
    entryEnd = eastl::next(entryStart, eastl::distance(sub_name_begin, sub_name_end));

    // We need to ensure, that the entry is a full match and not a substring of a longer subtemplate
    bool goodStart = entryStart == full_name_begin || *eastl::prev(entryStart) == '+';
    bool goodEnd = entryEnd == full_name_end || *entryEnd == '+';
    if (goodStart && goodEnd)
      return entryStart;
  }
  return full_name_end;
}

template <typename S = ECS_DEFAULT_RECREATE_STR_T>
inline S add_sub_template_name(const char *from_templ_name, const char *add_templ_name, const char *def = nullptr)
{
  S newTemplateName = from_templ_name;
  auto originalTemplateSize = newTemplateName.size();
  eastl::string_view fullAddTemplate(add_templ_name);

  for_each_sub_template_name_iter(fullAddTemplate.begin(), fullAddTemplate.end(),
    [&newTemplateName](auto subTemplateBegin, auto subTemplateEnd) {
      auto subTemplateEntry =
        find_sub_template_name_iter(newTemplateName.begin(), newTemplateName.end(), subTemplateBegin, subTemplateEnd);
      if (subTemplateEntry != newTemplateName.end())
        return;

      // ECS_DEFAULT_RECREATE_STR_T aka eastl::fixed_string doesn't have operator '+' with eastl::string_view
      auto currentSize = newTemplateName.size();
      newTemplateName.resize(newTemplateName.size() + 1 + eastl::distance(subTemplateBegin, subTemplateEnd));
      newTemplateName[currentSize] = '+';
      eastl::copy(subTemplateBegin, subTemplateEnd, eastl::next(newTemplateName.begin(), currentSize + 1));
    });

  bool templateChanged = newTemplateName.size() != originalTemplateSize;
  return (def == nullptr || templateChanged) ? newTemplateName : def;
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
  S newTemplateName = from_templ_name;
  auto originalTemplateSize = newTemplateName.size();
  eastl::string_view fullRemoveTemplate(remove_name_str);

  for_each_sub_template_name_iter(fullRemoveTemplate.begin(), fullRemoveTemplate.end(),
    [&newTemplateName](auto subTemplateBegin, auto subTemplateEnd) {
      auto subTemplateEntry =
        find_sub_template_name_iter(newTemplateName.begin(), newTemplateName.end(), subTemplateBegin, subTemplateEnd);
      if (subTemplateEntry == newTemplateName.end())
        return;

      // We can't remove the first subtemplate
      if (subTemplateEntry == newTemplateName.begin())
        return;

      // remove subtemplate and preceding '+'
      auto subTemplateEntryEnd = eastl::next(subTemplateEntry, eastl::distance(subTemplateBegin, subTemplateEnd));
      newTemplateName.erase(eastl::prev(subTemplateEntry), subTemplateEntryEnd);
    });

  bool templateChanged = newTemplateName.size() != originalTemplateSize;
  return (def == nullptr || templateChanged) ? newTemplateName : def;
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
