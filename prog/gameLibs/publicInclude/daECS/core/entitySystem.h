//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "entityId.h"
#include "event.h"
#include "ecsQuery.h"
#include "updateStage.h"
#include "ecsHash.h"
#include <osApiWrappers/dag_atomic.h> //for relaxed load in atomic

namespace ecs
{

typedef void (*UpdateFuncType)(const UpdateStageInfo &info, const QueryView &components);
typedef void (*EventFuncType)(const Event &evt, const QueryView &components);
typedef void (*RWEventFuncType)(Event &evt, const QueryView &components);

struct EntitySystemOps
{
  UpdateFuncType onUpdate;
  union
  {
    EventFuncType onEvent;
    RWEventFuncType onRWEvent;
  };

  EntitySystemOps(UpdateFuncType upf, EventFuncType evf = NULL) : onUpdate(upf), onEvent(evf) {}
  EntitySystemOps(UpdateFuncType upf, RWEventFuncType evf) : onUpdate(upf), onRWEvent(evf) {}

  bool empty() const { return !onUpdate && !onEvent; }
};

struct EntitySystemDesc;

extern void remove_system_from_list(EntitySystemDesc *desc);

struct EntitySystemDesc : public NamedQueryDesc
{
  typedef void (*DeleteHandler)(EntitySystemDesc *desc);
  EntitySystemDesc(const char *n, const char *module, EntitySystemOps ops_, dag::ConstSpan<ComponentDesc> comps_rw,
    dag::ConstSpan<ComponentDesc> comps_ro, dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no,
    EventSet &&evm, int stm, const char *tag_set = nullptr, const char *comp_set = nullptr, const char *before_set = nullptr,
    const char *after_set = nullptr, void *user_data = nullptr, uint32_t def_quant = 0, bool dyn = false,
    DeleteHandler on_delete = nullptr) :
    NamedQueryDesc(n, comps_rw, comps_ro, comps_rq, comps_no),
    ops(ops_),
    evSet(eastl::move(evm)),
    stageMask(stm),
    userData(user_data),
    quant(def_quant),
    onDelete(on_delete),
    tagSet(tag_set),
    beforeSet(before_set),
    afterSet(after_set),
    compChangeSet(comp_set),
    dynamic(dyn),
    moduleName(module)
  {
    // check on intialization in entityManager
    emptyES = (comps_rw.size() == 0 && comps_ro.size() == 0 && comps_rq.size() == 0 && comps_no.size() == 0);
    next = tail;
    tail = this;
    generation++;
  }
  EntitySystemDesc(const char *n, EntitySystemOps ops_, dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro,
    dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no, EventSet &&evm, int stm,
    const char *tag_set = nullptr, const char *comp_set = nullptr, const char *before_set = nullptr, const char *after_set = nullptr,
    void *user_data = nullptr, uint32_t def_quant = 0, bool dyn = false, DeleteHandler on_delete = nullptr) :
    EntitySystemDesc(n, nullptr, ops_, comps_rw, comps_ro, comps_rq, comps_no, eastl::move(evm), stm, tag_set, comp_set, before_set,
      after_set, user_data, def_quant, dyn, on_delete)
  {}

  ~EntitySystemDesc();
  EntitySystemDesc &operator=(EntitySystemDesc &&) = default;

  void freeIfDynamic()
  {
    if (dynamic)
      delete this;
  }
  uint32_t getQuant() const { return quant; }
  void *getUserData() const { return userData; }
  void setUserData(void *p) { userData = p; }
  bool isDynamic() const { return dynamic; }
  bool isEmpty() const { return emptyES; }
  static EntitySystemDesc *getTail() { return tail; }
  const EntitySystemOps getOps() const { return ops; }
  const char *getBefore() const { return beforeSet; }
  const char *getAfter() const { return afterSet; }
  const char *getTags() const { return tagSet; }
  const char *getCompSet() const { return compChangeSet; }
  const char *getModuleName() const { return moduleName; }
  void setEvSet(EventSet &&evs);
  const EventSet &getEvSet() const { return evSet; }

protected:
  friend class EntityManager;
  template <class T>
  friend struct SortDescByPrio;
  friend void remove_system_from_list(EntitySystemDesc *desc);
  friend void clear_entity_systems_registry();

#if DAGOR_DBGLEVEL > 0
  mutable uint32_t dapToken = 0; // To consider: move to NamedQueryDesc ?
  void cacheProfileTokensOnceOOL() const;
  void cacheProfileTokensOnce() const
  {
    if (DAGOR_UNLIKELY(!interlocked_relaxed_load(dapToken)))
      cacheProfileTokensOnceOOL();
  }
#else
  void cacheProfileTokensOnce() const {}
#endif
  EntitySystemOps ops;      // operations that this components perform (func-table)
  EventSet evSet;           // set of events types that this ES handles
  void *userData = nullptr; // for extensions such as scripted system
  uint32_t stageMask = 0;   // mask of update stages for onUpdate operation
  uint16_t quant = 0;
  bool dynamic = false, emptyES = false; // emptyES will be always called but with empty Query

  EntitySystemDesc *next = NULL; // slist
  static EntitySystemDesc *tail;
  static uint32_t generation;

  DeleteHandler onDelete = nullptr;
  const char *beforeSet = nullptr;     // CSV entity systems names
  const char *afterSet = nullptr;      // CSV entity systems names
  const char *tagSet = nullptr;        // CSV entity system tags
  const char *compChangeSet = nullptr; // CSV list of component change event submission
  const char *moduleName = nullptr;

  template <typename Lambda>
  friend void iterate_systems(Lambda fn);
  template <typename Lambda>
  friend void remove_if_systems(Lambda fn);
  template <typename Lambda>
  friend EntitySystemDesc *find_if_systems(Lambda fn);
  friend void clear_component_manager_registry();
};

inline void EntitySystemDesc::setEvSet(EventSet &&evs)
{
  evSet = eastl::move(evs);
  ++generation;
}

template <typename Lambda>
inline void iterate_systems(Lambda fn)
{
  for (EntitySystemDesc *system = EntitySystemDesc::tail; system;)
  {
    EntitySystemDesc *nextSys = system->next;
    fn(system);
    system = nextSys;
  }
}

template <typename Lambda> // Lambda return true
inline EntitySystemDesc *find_if_systems(Lambda fn)
{
  for (EntitySystemDesc *system = EntitySystemDesc::tail; system;)
  {
    EntitySystemDesc *nextSys = system->next;
    if (fn(system))
      return system;
    system = nextSys;
  }
  return nullptr;
}

template <typename Lambda> // return bools if should be removed
inline void remove_if_systems(Lambda fn)
{
  for (EntitySystemDesc *system = EntitySystemDesc::tail, *prevSys = nullptr; system;)
  {
    EntitySystemDesc *nextSys = system->next;
    if (fn(system))
    {
      ++EntitySystemDesc::generation;
      if (prevSys)
        prevSys->next = nextSys;
      else
        EntitySystemDesc::tail = nextSys;
    }
    else
      prevSys = system;
    system = nextSys;
  }
}

inline EntitySystemDesc::~EntitySystemDesc()
{
  for (EntitySystemDesc *system = EntitySystemDesc::tail, *prevSys = nullptr; system;)
  {
    EntitySystemDesc *nextSys = system->next;
    if (system == this)
    {
      ++EntitySystemDesc::generation;
      if (prevSys)
        prevSys->next = nextSys;
      else
        EntitySystemDesc::tail = nextSys;
      break;
    }
    else
      prevSys = system;
    system = nextSys;
  }
  if (onDelete)
    onDelete(this);
}

inline void clear_component_manager_registry()
{
  iterate_systems([](EntitySystemDesc *esd) { esd->freeIfDynamic(); });
  EntitySystemDesc::tail = nullptr;
}

}; // namespace ecs

#if _ECS_CODEGEN
#define ECS_BEFORE_ONE(a) __attribute__((annotate("@before:" #a)))
#define ECS_BEFORE(...)   ECS_FOR_EACH(ECS_BEFORE_ONE, __VA_ARGS__)
#define ECS_AFTER_ONE(a)  __attribute__((annotate("@after:" #a)))
#define ECS_AFTER(...)    ECS_FOR_EACH(ECS_AFTER_ONE, __VA_ARGS__)
#define ECS_TAG_ONE(a)    __attribute__((annotate("@tag:" #a)))
#define ECS_TAG(...)      ECS_FOR_EACH(ECS_TAG_ONE, __VA_ARGS__)
#define ECS_TRACK_ONE(a)  __attribute__((annotate("@track:" #a)))
#define ECS_TRACK(...)    ECS_FOR_EACH(ECS_TRACK_ONE, __VA_ARGS__)
#define ECS_NO_ORDER      __attribute__((annotate("@before:*")))
#else
#define ECS_BEFORE_ONE(a)
#define ECS_BEFORE(...)
#define ECS_AFTER_ONE(a)
#define ECS_AFTER(...)
#define ECS_TAG_ONE(a)
#define ECS_TAG(...)
#define ECS_TRACK_ONE(a)
#define ECS_TRACK(...)
#define ECS_NO_ORDER
#endif
