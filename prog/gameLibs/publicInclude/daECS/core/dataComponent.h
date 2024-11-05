//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_span.h>
#include <generic/dag_smallTab.h>
#include <daECS/core/componentType.h>
#include "internal/ltComponentList.h"
#include <util/dag_stringTableAllocator.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#define ECS_AUTO_REGISTER_COMPONENT_BASE(type_name_, name, serializer, flags)                                                         \
  static ecs::CompileComponentRegister MAKE_UNIQUE(ccm_rec_, __LINE__)(ECS_HASH(name), ecs::ComponentTypeInfo<type_name_>::type_name, \
    ecs::ComponentTypeInfo<type_name_>::type, {}, serializer, flags);

#define ECS_AUTO_REGISTER_COMPONENT_DEPS_BASE(type_name_, name, serializer, flags, ...)                                               \
  static const char *MAKE_UNIQUE(ccm_deps_, __LINE__)[] = {__VA_ARGS__};                                                              \
  static ecs::CompileComponentRegister MAKE_UNIQUE(ccm_rec_, __LINE__)(ECS_HASH(name), ecs::ComponentTypeInfo<type_name_>::type_name, \
    ecs::ComponentTypeInfo<type_name_>::type, make_span(MAKE_UNIQUE(ccm_deps_, __LINE__), countof(MAKE_UNIQUE(ccm_deps_, __LINE__))), \
    serializer, flags);

#define ECS_AUTO_REGISTER_COMPONENT(type_name, name, serializer, flags) \
  ECS_AUTO_REGISTER_COMPONENT_BASE(type_name, name, serializer, flags)
#define ECS_AUTO_REGISTER_COMPONENT_DEPS(type_name, name, serializer, flags, ...) \
  ECS_AUTO_REGISTER_COMPONENT_DEPS_BASE(type_name, name, serializer, flags, __VA_ARGS__)

namespace ecs
{

class ComponentSerializer;
extern ComponentSerializer default_serializer;

struct CompileComponentRegister
{
  CompileComponentRegister(const HashedConstString name_, const char *type_name_, component_type_t type_,
    dag::ConstSpan<const char *> deps_, ComponentSerializer *io_, component_flags_t flags_) :
    name(name_), flags(flags_), type(type_), type_name(type_name_), deps(deps_), io(io_)
  {
    next = tail;
    tail = this;
  }

protected:
  CompileComponentRegister(const CompileComponentRegister &) = delete;
  CompileComponentRegister &operator=(const CompileComponentRegister &) = delete;
  HashedConstString name = {nullptr, 0};
  const char *type_name = 0;
  ComponentSerializer *io = nullptr;
  dag::ConstSpan<const char *> deps;
  CompileComponentRegister *next = NULL; // slist
  static CompileComponentRegister *tail;
  component_type_t type = 0;
  component_flags_t flags = 0;
  friend struct DataComponents;
};

struct DataComponent
{
  enum
  {
    DONT_REPLICATE = 0x01,
    IS_COPY = 0x02,
    HAS_SERIALIZER = 0x04,
    TYPE_HAS_CONSTRUCTOR = 0x08
  };
  type_index_t componentType;
  component_flags_t flags;
  component_type_t componentTypeName;
};

struct DataComponents
{
public:
  bool hasComponent(component_t name) const { return componentIndex.find(name) != componentIndex.end(); }
  component_index_t findComponentId(component_t name) const;
  component_t getComponentTpById(component_index_t i) const { return i < components.size() ? components.get<COMPONENT>()[i] : 0; }

  const char *findComponentName(component_t name) const;
  const char *getComponentNameById(component_index_t) const;

  DataComponent getComponentById(component_index_t id) const;
  DataComponent findComponent(component_t name) const;
  component_index_t createComponent(const HashedConstString name, type_index_t component_type,
    dag::Span<component_t> non_optional_deps, ComponentSerializer *io, component_flags_t flags, const ComponentTypes &);
  uint32_t size() const { return (uint32_t)components.size(); }
  bool getDependeciesById(component_index_t id, uint32_t &begin, uint32_t &end) const;
  component_t getDependence(uint32_t dep) const { return dep < dependencies.size() ? dependencies[dep] : 0; }
  inline void setFilterOutComponents(dag::ConstSpan<const char *> tags);
  inline bool isFilteredOutComponent(component_t name) const;
  inline void addFilteredOutComponent(component_t name);
  component_index_t getTrackedPair(component_index_t id) const;
  component_index_t findTrackedPair(component_t name) const;
  inline bool isTracked(component_index_t id) const { return getTrackedPair(id) != INVALID_COMPONENT_INDEX; }
  inline bool findIsTracked(component_t name) const { return findTrackedPair(name) != INVALID_COMPONENT_INDEX; }
  ComponentSerializer *getComponentIO(component_index_t id) const;
  ComponentSerializer *findComponentIO(component_t id) const;
  ComponentSerializer *getComponentIo(component_index_t id, component_flags_t flags, type_index_t type_id,
    const ComponentTypes &types);

protected:
  const char *getHashName(component_index_t id) const;
  DataComponent getComponentByIdUnsafe(component_index_t id) const;
  component_index_t getTrackedPairUnsafe(component_index_t id) const;
  void initialize(const ComponentTypes &);
  void clear();
  friend class EntityManager;
  // eastl::hash_map<component_t, component_index_t> componentIndex;
  ska::flat_hash_map<component_t, component_index_t, ska::power_of_two_std_hash<component_t>> componentIndex; //
  struct DependencyRange
  {
    uint32_t start;
  };
  enum
  {
    DCOMP,
    SERIALIZER,
    COMPONENT,
    CIDX,
    DEPRANGE,
    NAME
  };
  eastl::tuple_vector<DataComponent, ComponentSerializer *,
    component_t,       // hash
    component_index_t, // trackedPair
    DependencyRange, uint32_t>
    components;
  dag::Vector<component_t> dependencies;
  StringTableAllocator names = {13, 13};
  G_STATIC_ASSERT(sizeof(DataComponent) == 8);
  ska::flat_hash_set<component_t, ska::power_of_two_std_hash<component_t>> filterTags;
  HashedKeyMap<component_t, LTComponentList *> componentToLT;
  void updateComponentToLTInternal();
  void updateComponentToLT()
  {
    if (DAGOR_LIKELY(NULL == LTComponentList::tail))
      return;
    updateComponentToLTInternal();
  }
};

inline bool DataComponents::getDependeciesById(component_index_t id, uint32_t &begin, uint32_t &end) const
{
  begin = end = 0;
  if (id >= components.size())
    return false;
  begin = components.get<DependencyRange>()[id].start;
  end = id + 1 < components.size() ? components.get<DependencyRange>()[id + 1].start : (uint32_t)dependencies.size();
  return true;
}

inline component_index_t DataComponents::findComponentId(component_t name) const
{
  auto it = componentIndex.find(name);
  if (it == componentIndex.end())
    return INVALID_COMPONENT_INDEX;
  return it->second;
}

__forceinline DataComponent DataComponents::getComponentByIdUnsafe(component_index_t id) const
{
#if DAECS_EXTENSIVE_CHECKS
  G_ASSERT(id < components.size());
#endif
  return components.get<DataComponent>()[id];
}
inline DataComponent DataComponents::getComponentById(component_index_t id) const
{
  return id < components.size() ? components.get<DataComponent>()[id] : DataComponent{INVALID_COMPONENT_TYPE_INDEX, 0, 0};
}
inline ComponentSerializer *DataComponents::getComponentIO(component_index_t id) const
{
  return id < components.size() ? components.get<ComponentSerializer *>()[id] : nullptr;
}

inline ComponentSerializer *DataComponents::findComponentIO(component_t nm) const { return getComponentIO(findComponentId(nm)); }

inline DataComponent DataComponents::findComponent(component_t name) const { return getComponentById(findComponentId(name)); }

inline const char *DataComponents::getComponentNameById(component_index_t id) const
{
  if (DAGOR_UNLIKELY(id >= components.size()))
    return nullptr;
  uint32_t addr = components.get<NAME>()[id];
  if (DAGOR_LIKELY(addr))
    return names.getDataRawUnsafe(addr);
  return getHashName(id);
}

inline const char *DataComponents::findComponentName(component_t name) const { return getComponentNameById(findComponentId(name)); }

inline void DataComponents::addFilteredOutComponent(component_t name) { filterTags.insert(name); }

inline void DataComponents::setFilterOutComponents(dag::ConstSpan<const char *> tags)
{
  filterTags.clear();
  for (const char *tag : tags)
    addFilteredOutComponent(ECS_HASH_SLOW(tag).hash);
}
inline bool DataComponents::isFilteredOutComponent(component_t name) const { return filterTags.find(name) != filterTags.end(); }

inline component_index_t DataComponents::getTrackedPair(component_index_t id) const
{
  return id < components.size() ? components.get<component_index_t>()[id] : INVALID_COMPONENT_INDEX;
}

inline component_index_t DataComponents::getTrackedPairUnsafe(component_index_t id) const
{
  return components.get<component_index_t>()[id];
}

inline component_index_t DataComponents::findTrackedPair(component_t name) const { return getTrackedPair(findComponentId(name)); }

}; // namespace ecs
