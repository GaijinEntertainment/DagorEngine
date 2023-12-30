//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_oaHashNameMap.h>
#include <generic/dag_span.h>
#include <generic/dag_tab.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_set.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/bitvector.h>
#include <memory/dag_framemem.h>

#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/ecsHash.h>
#include "internal/inplaceKeySet.h"

class DataBlock;

namespace ecs
{
class TemplateDB;
struct TemplateRefs;
struct TemplatesData;

struct HashedLenConstString
{
  const char *str;
  hash_str_t hash;
  uint32_t len;
};
constexpr uint32_t constexpr_strlen(const char *s)
{
  uint32_t ret = 0;
  uint8_t c = 0;
  while ((c = *s++) != 0)
    ++ret;
  return ret;
}

#define ECS_HASHLEN(a)                                                                                 \
  (ecs::HashedLenConstString({a, eastl::integral_constant<::ecs::hash_str_t, str_hash_fnv1(a)>::value, \
    eastl::integral_constant<uint32_t, ::ecs::constexpr_strlen(a)>::value}))
#define ECS_HASHLEN_SLOW(a)        (::ecs::HashedLenConstString({a, str_hash_fnv1(a), (uint32_t)strlen(a)}))
#define ECS_HASHLEN_SLOW_LEN(a, l) (::ecs::HashedLenConstString({a, mem_hash_fnv1(a, (l)), (l)}))

typedef ska::flat_hash_set<hash_str_t, ska::power_of_two_std_hash<hash_str_t>> TagsSet;
enum
{
  FLAG_CHANGE_EVENT = 1, // Will generate EventComponentChanged on component change
  FLAG_REPLICATED = 2,   // Will be replicated to remote systems on change (and also generate EventComponentChanged)
  FLAG_CANT_INSTANTIATE = 4,

  FLAG_DEPENDENCIES_RESOLVED = 8, // not inherited, local flag

  FLAG_REPLICATED_OR_TRACKED = (FLAG_CHANGE_EVENT | FLAG_REPLICATED),
  FLAGS_ALL = FLAG_REPLICATED_OR_TRACKED | FLAG_CANT_INSTANTIATE
};

class Template
{
  Template();

public:
  struct ComponentsSet
  {
    const component_t *b = NULL;
    size_t cnt = 0;
    const component_t *begin() const { return b; }
    const component_t *end() const { return b + cnt; }
    const component_t *find() const { return b + cnt; }
    bool count(component_t c) const { return eastl::binary_search(begin(), end(), c); }
    const component_t *find(component_t c) const { return eastl::binary_search_i(begin(), end(), c); }
    const component_t *findNil(component_t c) const
    {
      const component_t *r = find(c);
      return r ? r : NULL;
    }
    bool empty() const { return cnt == 0; }
    size_t size() const { return cnt; }
    bool operator==(const ComponentsSet &a) { return size() == a.size() && memcmp(b, a.b, size() * sizeof(*b)) == 0; }
    bool operator!=(const ComponentsSet &a) { return !(*this == a); }
    template <class T>
    static ComponentsSet from(const T &t)
    {
      return ComponentsSet{t.begin(), t.end()};
    }
    static ComponentsSet from(const component_t *b, size_t cnt) { return ComponentsSet{b, cnt}; }
  };
  typedef eastl::vector_set<component_t, eastl::less<component_t>, EASTLAllocatorType, dag::Vector<component_t, EASTLAllocatorType>>
    component_set;
  static constexpr size_t PARENTS_INPLACE_SIZE = 5; // 98% templates in Enlisted have <=4 parents
  typedef InplaceKeyContainer<uint32_t, PARENTS_INPLACE_SIZE> ParentsList;

  Template(const char *tname, ComponentsMap &&amap, component_set &&tracked_set, component_set &&replicated_set,
    component_set &&ignored_initial_repl, bool singleton_, const char *path_ = "");
  ~Template();
  Template(const Template &t);
  Template(Template &&t) = default;
  Template &operator=(Template &&t) = default;

  const char *getName() const { return name; }
#if DAGOR_DBGLEVEL > 0
  const char *getPath() const { return path; }
#else
  const char *getPath() const { return ""; }
#endif
  dag::ConstSpan<uint32_t> getParents() const { return parents; }

  // Only for inspection
  bool hasComponent(const HashedConstString &hash_name, const TemplatesData &db) const;
  const ChildComponent &getComponent(const HashedConstString &hash_name, const TemplatesData &db) const;

  const ComponentsMap &getComponentsMap() const { return components; }
  uint32_t getRegExpInheritedFlags(component_t aname, const TemplatesData &db) const; // get flags from sets
  uint32_t getRegExpInheritedFlags(const HashedConstString aname, const TemplatesData &db) const
  {
    return getRegExpInheritedFlags(aname.hash, db);
  }
  bool isSingletonDB(const TemplatesData &db) const { return hasComponent(ECS_HASH("_singleton"), db); }

  ComponentsSet ignoredSet() const { return ComponentsSet::from(sets.get() + ignoredOfs(), ignoredCount); }
  ComponentsSet getIgnoredInitialReplicationSet() const { return ignoredSet(); }
  ComponentsSet trackedSet() const { return ComponentsSet::from(sets.get() + trackedOfs(), trackedCount); }
  ComponentsSet replicatedSet() const { return ComponentsSet::from(sets.get() + replicatedOfs(), replicatedCount); }
  bool isEqual(const Template &t) const;

  bool isSingleton() const;
  const ChildComponent &getComponent(const HashedConstString &hash_name) const;
  bool hasComponent(const HashedConstString &hash_name) const;
  bool needCopy(component_t comp, const TemplatesData &db) const;     // get flags from sets
  bool isReplicated(component_t comp, const TemplatesData &db) const; // get flags from sets
  bool isValid() const { return name != NULL; }                       // we can use flags for that, there are plenty bits
  bool reportInvalidDependencies(const TemplatesData &db) const;
  bool canInstantiate() const { return !(inheritedFlags & FLAG_CANT_INSTANTIATE); }
  typedef uint32_t template_hash_t;
  template_hash_t buildHash() const; // not including values of components
  size_t componentsCount() const { return components.size(); }

private:
  uint32_t trackedOfs() const { return 0; }
  uint32_t replicatedOfs() const { return trackedCount; }
  uint32_t ignoredOfs() const { return replicatedOfs() + replicatedCount; }
  void buildSets(const component_set &tracked, const component_set &repl, const component_set &ignored);
  uint32_t resolveComponentsFlagsRecursive(const TemplatesData &db) const;
  void resolveFlags(const TemplatesData &db)
  {
    setInheritedFlags(resolveComponentsFlagsRecursive(db) | FLAG_CANT_INSTANTIATE);
    setInstantiatable(calcValidDependencies(db));
  }
  bool calcValidDependencies(const TemplatesData &db) const;
  void setParentsInternal(const uint32_t *pb, const uint32_t *pe);

  friend class TemplateDB;
  friend class EntityManager;
  friend struct TemplateRefs;
  friend struct TemplatesData;
  eastl::unique_ptr<component_t[]> sets;
  ParentsList parents;   // Root if empty. Reversed relatively to data description
  const char *name = ""; // todo: remove me, we have nameId by index of template
#if DAGOR_DBGLEVEL > 0
  const char *path = "";
#endif
  ComponentsMap components;
  mutable dag::RelocatableFixedVector<component_index_t, 7, true, MidmemAlloc, uint16_t> dependencies; // cache for dependencies
  uint16_t trackedCount = 0, replicatedCount = 0, ignoredCount = 0;
  mutable uint8_t localFlags = FLAGS_ALL;
  uint8_t inheritedFlags = FLAGS_ALL; // including parents

  uint32_t tag = 0;
  uint8_t templAllFlags() const { return localFlags; }
  uint8_t totalAllFlags() const { return inheritedFlags; }
  uint8_t templComponentFlags() const { return localFlags & ~FLAG_CANT_INSTANTIATE; }
  uint8_t totalComponentFlags() const { return inheritedFlags & ~FLAG_CANT_INSTANTIATE; }
  void setDependenciesResolved() const { localFlags |= FLAG_DEPENDENCIES_RESOLVED; }
  void setInheritedFlags(uint8_t flags) { inheritedFlags |= flags & FLAGS_ALL; }
  void setNonInstantiatable()
  {
    inheritedFlags |= FLAG_CANT_INSTANTIATE;
    localFlags |= FLAG_CANT_INSTANTIATE;
  }
  void setInstantiatable()
  {
    inheritedFlags &= ~FLAG_CANT_INSTANTIATE;
    localFlags &= ~FLAG_CANT_INSTANTIATE;
  }
  void setInstantiatable(bool on) { on ? setInstantiatable() : setNonInstantiatable(); }
};

typedef ska::flat_hash_map<component_t, component_flags_t, ska::power_of_two_std_hash<component_t>> ComponentsFlags;

struct TemplateDBInfo
{
#if DAGOR_DBGLEVEL > 0
  ska::flat_hash_map<hash_str_t, eastl::string, ska::power_of_two_std_hash<hash_str_t>> componentTemplate; // this is for debug only
  ska::flat_hash_map<hash_str_t, eastl::string, ska::power_of_two_std_hash<hash_str_t>> componentTags;     // bit mask.
#endif
  TagsSet filterTags;
  ComponentsFlags componentFlags;
  bool hasServerTag = false;
  void updateComponentTags(HashedConstString comp_name, const char *filter_tags, const char *templ_name, const char *tpath);

  const DataBlock *getTemplateMetaInfo(const char *templ_name) const;
  const DataBlock *getComponentMetaInfo(const char *comp_name) const;
  bool hasComponentMetaInfo(const char *templ_name, const char *comp_name) const;
  enum class MetaInfoType
  {
    COMMON_COMPONENT,
    COMMON_TEMPLATE,
    COMPONENT,
    TEMPLATE
  };
  bool addMetaData(MetaInfoType type, const char *name, const DataBlock &info);
  bool addMetaLink(MetaInfoType type, const char *name, const char *link);
  void addMetaComp(const char *templ_name, const char *comp_name);
  struct MetaInfoData;
  struct MetaInfoDataDeleter
  {
    void operator()(MetaInfoData *);
  };
  eastl::unique_ptr<MetaInfoData, MetaInfoDataDeleter> metaInfoData;
  void resetMetaInfo() { metaInfoData.reset(); }
};
extern bool filter_needed(const char *filter_tags, const ska::flat_hash_set<eastl::string> &valid_tags);
extern bool filter_needed(const char *filter_tags, const TagsSet &valid_tags);

struct TemplateComponentsDB
{
  typedef uint16_t template_component_index_t;
  HashedKeyMap<component_t, template_component_index_t> componentToName;
  enum
  {
    COMPONENT_TYPE,
    COMPONENT_NAME
  };
  eastl::tuple_vector<component_type_t, uint32_t> componentTypes; // type, name in componentNames
  StringTableAllocator componentNames = {13, 13};

  component_type_t getComponentTypeById(template_component_index_t id) const
  {
    G_FAST_ASSERT(id < componentTypes.size());
    return componentTypes.get<COMPONENT_TYPE>()[id];
  }
  const char *getComponentNameById(template_component_index_t id) const
  {
    G_FAST_ASSERT(id < componentTypes.size());
    uint32_t nameAddr = componentTypes.get<COMPONENT_NAME>()[id];
    return nameAddr == ~0 ? NULL : componentNames.getDataRawUnsafe(nameAddr);
  }
  component_type_t getComponentType(component_t c) const
  {
    if (auto ci = componentToName.findVal(c))
      return getComponentTypeById(*ci);
    return 0;
  }
  const char *getComponentName(component_t c) const
  {
    if (auto ci = componentToName.findVal(c))
      return getComponentNameById(*ci);
    return NULL;
  }
  bool addComponent(component_t c, component_type_t type, const char *n)
  {
    if (auto ci = componentToName.findVal(c))
    {
      G_FAST_ASSERT(*ci < componentTypes.size());
      component_type_t &oldType = *(componentTypes.get<COMPONENT_TYPE>() + *ci);
      uint32_t &oldNameAddr = *(componentTypes.get<COMPONENT_NAME>() + *ci); // same as [], but PVS is OK
      if (n && oldNameAddr == ~0)
        oldNameAddr = componentNames.addDataRaw(n, strlen(n) + 1);
      else if (n && strcmp(n, componentNames.getDataRawUnsafe(oldNameAddr)) != 0)
      {
        logerr("hash collision for 0x%X on name %s and %s", c, n, componentNames.getDataRawUnsafe(oldNameAddr));
        return false;
      }
      if (type && oldType == 0)
        oldType = type;
      else if (type && oldType != type)
      {
        logerr("different types 0x%X (was 0x%X) for component <%s|0x%X>", type, oldType, componentNames.getDataRawUnsafe(oldNameAddr),
          c);
        return false;
      }
      return true;
    }
    else
    {
      const template_component_index_t index = componentTypes.size();
      if (index == template_component_index_t(~template_component_index_t(0)))
      {
        logerr("too much components added. replace template_component_index_t with uint32_t");
        return false;
      }
      uint32_t nameAddr = n ? componentNames.addDataRaw(n, strlen(n) + 1) : ~0;
      componentTypes.emplace_back(eastl::move(type), eastl::move(nameAddr));
      componentToName.emplace_new(c, index);
      return true;
    }
  }
  void addComponentNames(const TemplateComponentsDB &d)
  {
    for (auto &i : d.componentToName)
      addComponent(i.key(), d.getComponentTypeById(i.val()), d.getComponentNameById(i.val()));
  }
  bool addComponentName(component_t c, const char *n) { return addComponent(c, 0, n); }
};

struct TemplatesData : public TemplateComponentsDB
{
  typedef SmallTab<Template> TemplateList;
  typedef OAHashNameMap<false, FNV1OAHasher<false>> NameMap;
  typedef typename TemplateList::iterator iterator;
  typedef typename TemplateList::const_iterator const_iterator;
  TemplateList templates;
  NameMap templatesIds; // probably consider use compile time hashes for creation
#if DAGOR_DBGLEVEL > 0
  OAHashNameMap<false> pathNames;
#endif
  size_t size() const { return templates.size(); }
  const_iterator begin() const { return templates.begin(); }
  const_iterator end() const { return templates.end(); }
  bool empty() const { return templates.empty(); }
  const_iterator find(const char *s) const
  {
    int i = templatesIds.getNameId(s);
    return i < 0 ? end() : templates.begin() + i;
  }
  const Template *getTemplateById(uint32_t id) const { return id < templates.size() ? &templates[id] : nullptr; }
  const Template &getTemplateRefById(uint32_t id) const { return templates[id]; }
  inline int getTemplateIdByName(const HashedLenConstString &n) const { return templatesIds.getNameId(n.str, (size_t)n.len, n.hash); }
  inline int getTemplateIdByName(const char *n) const { return getTemplateIdByName(ECS_HASHLEN_SLOW(n)); }

  template <typename PreCallable, typename PostCallable>
  inline void iterate_parents(const uint32_t t, PreCallable pre, PostCallable post) const;
  template <typename Callable>
  inline void iterate_template_parents(const Template &, Callable fn) const;
};

struct TemplateRefs : public TemplatesData
{
  typedef typename TemplatesData::TemplateList TemplateList;
  typedef typename TemplatesData::NameMap NameMap;
  typedef typename TemplatesData::iterator iterator;
  typedef typename TemplatesData::const_iterator const_iterator;
  using TemplatesData::begin;
  using TemplatesData::empty;
  using TemplatesData::end;
  using TemplatesData::find;
  using TemplatesData::size;
  iterator begin() { return templates.begin(); }
  iterator end() { return templates.end(); }
  const_iterator find(uint32_t nid) const { return nid < templates.size() ? templates.begin() + nid : end(); }
  iterator find(const char *s)
  {
    int i = templatesIds.getNameId(s);
    return i < 0 ? end() : templates.begin() + i;
  }
  iterator find(uint32_t nid) { return nid < templates.size() ? templates.begin() + nid : end(); }
  template <typename CB>                 // const char *child, const char *parent
  void reportBrokenParents(CB cb) const; // only works before finalize
  template <typename CB>                 // const char *child, const char *parent
  void reportLoops(CB cb) const;         // only works before finalize
  uint32_t getEmptyCount() const { return emptyCount; }
  void reserve(uint32_t c) { templates.reserve(c); }
  uint32_t ensureTemplate(const char *p)
  {
    int n = templatesIds.getNameId(p);
    if (n >= 0)
      return n;
    return pushEmpty(p);
  }
  void emplace(Template &&t, Template::ParentsList &&parents);
  void amend(Template &&t, Template::ParentsList &&parents);
  void amend(TemplateRefs &&overrides);
  void finalize(uint32_t tag);
  bool isTopoSorted() const;

private:
  void topoSort();
  void resolveBroken();
  uint32_t pushEmpty(const char *n);
  uint32_t emptyCount = 0;
  bool finalized = false;
};

template <typename CB>
inline void TemplateRefs::reportBrokenParents(CB cb) const
{
  if (finalized || getEmptyCount() == 0)
    return;
  for (const Template &t : templates)
  {
    if (t.isValid())
      for (auto p : t.parents)
      {
        if (p >= templates.size())
          cb(t.getName(), "invalid index!");
        else if (!templates[p].isValid())
          cb(t.getName(), templatesIds.getName(p));
      }
  }
}

// https://en.wikipedia.org/wiki/Topological_sorting, dfs
template <typename CB>
inline void TemplateRefs::reportLoops(CB cb) const
{
  if (finalized)
  {
    G_ASSERT(isTopoSorted());
    return;
  }
  eastl::bitvector<framemem_allocator> temp(templates.size()), perm(templates.size());
  for (uint32_t i = 0, sz = templates.size(); i < sz; ++i)
    iterate_parents(
      i,
      [&](uint32_t p) {
        if (perm.test(p, true))
          return false;
        if (!temp.test(p, false)) // loop not detected
        {
          temp.set(p, true);
          return true;
        }
        cb(templatesIds.getName(i), templatesIds.getName(p)); // loop detected
        return false;
      },
      [&](uint32_t p) {
        temp.set(p, false);
        perm.set(p, true);
      });
}

// Append-only Template Database
class TemplateDB : protected TemplatesData
{
public:
  typedef typename TemplatesData::TemplateList TemplateList;
  typedef typename TemplatesData::const_iterator const_iterator;
  using TemplatesData::begin;
  using TemplatesData::empty;
  using TemplatesData::end;
  using TemplatesData::getComponentName;
  using TemplatesData::getComponentType;
  using TemplatesData::getTemplateById;
  using TemplatesData::getTemplateIdByName;
  using TemplatesData::size;

  enum AddResult
  {
    AR_OK,
    AR_SAME, // returned by updateTemplate only, means template was exactly same as existing one. addTemplate will return just AR_OK
    AR_DUP,  // name exists AND template is different!
    AR_UNKNOWN_PARENT,
    AR_BAD_NAME,
  };

  // 'templ' shall be allocated on heap (ownership change)
  AddResult addTemplate(Template &&templ, dag::ConstSpan<const char *> *pnames = NULL, uint32_t tag = 0);
  void addTemplates(TemplateRefs &trefs, uint32_t tag = 0); // tag is probably hash from path loaded. allows to reload from path. tag
                                                            // == 0 means it can't be reloaded
  __forceinline const Template *getTemplateByName(const HashedLenConstString &n) const
  {
    const int id = getTemplateIdByName(n);
    return id < 0 ? nullptr : &templates[id];
  }
  const TemplatesData &data() const { return (const TemplatesData &)*this; }

  const Template *getTemplateByName(const eastl::string_view &name) const
  {
    return getTemplateByName(ECS_HASHLEN_SLOW_LEN(name.data(), (uint32_t)name.length()));
  }
  const Template *getTemplateByName(const char *name) const { return getTemplateByName(ECS_HASHLEN_SLOW(name)); }

  template_t getInstantiatedTemplateByName(const char *name) const;
  int buildTemplateIdByName(const char *n);
  const Template *buildTemplateByName(const char *n)
  {
    const int id = buildTemplateIdByName(n);
    return id >= 0 ? &templates[id] : nullptr;
  }
  void clear();

  const TemplateDBInfo &info() const { return db; }
  TemplateDBInfo &info() { return db; }
  void setFilterTags(dag::ConstSpan<const char *> tags);

  static bool is_valid_template_name(const char *n, size_t nlen);
  static bool is_valid_template_name(const char *n) { return n && is_valid_template_name(n, strlen(n)); }

private:
  TemplateDB::AddResult addTemplateWithParents(Template &&templ, uint32_t tag);
  using TemplatesData::getTemplateRefById;
  friend class EntityManager;
  friend class Template;
  void push_back(Template &&);

  TemplateDBInfo db;
  struct InstantiatedTemplateInfo
  {
    template_t t = INVALID_TEMPLATE_INDEX;
    bool singleton = false; /*uint8_t pad=0;*/
  };
  SmallTab<InstantiatedTemplateInfo> instantiatedTemplates; // parallel to templates
  // tuple - we need instantiated_id+singleton array. that's the only thing that is needed for typical creation
};

template <typename Callable>
inline void TemplatesData::iterate_template_parents(const Template &parent, Callable fn) const
{
  fn(parent);
  for (auto p : parent.parents)
    iterate_template_parents(getTemplateRefById(p), fn);
}

template <typename PreCallable, typename PostCallable>
inline void TemplatesData::iterate_parents(const uint32_t t, PreCallable pre, PostCallable post) const
{
  if (pre(t))
  {
    for (auto p : templates[t].parents)
      iterate_parents(p, pre, post);
    post(t);
  }
}

}; // namespace ecs
