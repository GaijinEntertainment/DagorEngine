//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <dasModules/aotEcs.h>

namespace bind_dascript
{
struct CreatingTemplate
{
  ecs::TemplateRefs &db;
  ecs::TagsSet filterTags;
  ecs::ComponentsMap map;
  ska::flat_hash_map<ecs::component_t, ecs::TagsSet, ska::power_of_two_std_hash<ecs::component_t>> compTags;
  ecs::Template::component_set replicated, tracked, ignored;
  ecs::Template::ParentsList parents;
  SimpleString name;
  void process_flags(das::Bitfield flags, ecs::component_t hash);
  bool addInfo(das::Bitfield flags, ecs::component_type_t tp, const ecs::HashedConstString comp_name);
  template <class T>
  inline bool addInfo(das::Bitfield flags, const ecs::HashedConstString comp_name)
  {
    return addInfo(flags, ecs::ComponentTypeInfo<typename eastl::remove_const<T>::type>::type, comp_name);
  }
  template <class T>
  inline void addInfo(das::Bitfield flags, const ecs::HashedConstString comp_name, const T &v)
  {
    if (addInfo<T>(flags, comp_name))
      map[comp_name] = v;
  }
  inline void addTag(ecs::hash_str_t tag) { filterTags.emplace(tag); }
  inline void addCompTag(uint32_t comp_tag, ecs::hash_str_t tag) { compTags[comp_tag].emplace(tag); }
};


#define TYPE(type)                                                                                   \
  inline void setCreatingTemplate##type(CreatingTemplate &map, das::Bitfield flags, const char *key, \
    typename DasTypeParamAlias<type>::cparam_alias to)                                               \
  {                                                                                                  \
    map.addInfo<type>(flags, ECS_HASH_SLOW(key), *(const type *)&to);                                \
  }
ECS_BASE_TYPE_LIST
#undef TYPE
inline void setCreatingTemplateStr(CreatingTemplate &map, das::Bitfield flags, const char *key, const char *to)
{
  if (!key)
    return;
  ecs::HashedConstString name = ECS_HASH_SLOW(key);
  if (map.addInfo<ecs::string>(flags, name))
    map.map[name] = ecs::string(to ? to : "");
}

inline bool setCreatingTemplateUndef(CreatingTemplate &map, das::Bitfield flags, const char *key, const char *type)
{
  extern bool setCreatingTemplateTyped(CreatingTemplate & map, das::Bitfield flags, const char *key, const char *type);
  if (type && *type)
    return setCreatingTemplateTyped(map, flags, key, type);
  if (!key)
    return false;
  ecs::HashedConstString name = ECS_HASH_SLOW(key);
  if (!map.addInfo(flags, 0, name))
    return false;
  map.map[name];
  return true;
}

bool createTemplate2(das::ModuleGroup &libGroup, uint32_t tag, const char *tname, const das::TArray<char *> &parents,
  const das::TBlock<void, CreatingTemplate> &block, das::Context *context, das::LineInfoArg *at);

} // namespace bind_dascript
