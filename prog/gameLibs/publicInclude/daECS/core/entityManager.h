//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <daECS/core/entityId.h>
#include <util/dag_oaHashNameMap.h>
#include <daECS/core/ecsQuery.h>
#include <EASTL/deque.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_map.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <generic/dag_span.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_initOnDemand.h>
#include <daECS/core/event.h>
#include "internal/templates.h"
#include "internal/stackAllocator.h"
#include "internal/eventsDb.h"
#include "internal/circularBuffer.h"
#include "internal/inplaceKeySet.h"
#include <daECS/core/template.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/ecsGameRes.h>
#include <osApiWrappers/dag_atomic.h> //for relaxed load in atomic

namespace ecs
{

static const char *nullstr = NULL; // legacy
// This function shall be implemented by external code (unsatisifed compile-time dependency)

struct ScheduledArchetypeComponentTrack;
struct ScheduledEidComponentTrack;
struct ArchetypesQuery;
struct ArchetypesEidQuery;
struct ResolvedQueryDesc;
struct SchemelessEvent;
struct Query;
struct UpdateStageInfo;
struct EntitySystemDesc;
struct ComponentsIterator;
struct NestedQueryRestorer;

typedef eastl::function<void(EntityId /*created_entity*/)> create_entity_async_cb_t;
typedef void (*replication_cb_t)(ecs::EntityManager &mgr, EntityId eid, component_index_t cidx);

class EntityManager
{
public:
  // sets replication callback. Will be called once per tick on changing of replicating (in template) component for entities that do
  // have reserved components(replication)
  void setReplicationCb(replication_cb_t cb);

  // sets ES execution order and which es to skip
  void setEsOrder(dag::ConstSpan<const char *> es_order, dag::ConstSpan<const char *> es_skip);

  // Essentially set filtering tags for further components creation
  void setFilterTags(dag::ConstSpan<const char *> tags);

  // Checks that EntityId is existing (it can be without archetype yet)
  bool doesEntityExist(EntityId e) const;

  // Call update functions of all registered systems for particular stage type
  void update(const UpdateStageInfo &);

  // This method shall be called once per frame or so.
  // It's might perform various delayed activities/bookkeeping like creating & destroying deferered entities,
  // sending delayed events, etc...
  void tick(bool flush_all = false);

  // This method performs delayed entityCreation/tracking, and if flush_all = true it ensures it all performed (i.e. queues are empty)
  void performDelayedCreation(bool flush_all = true);

  // This method flushes all tracked changes (i.e. might call EventComponentChanged ESes)
  void performTrackChanges(bool flush_all = false);

  // set Constrained multi-threading mode.
  //  Constrained MT mode is designed to allow several ES to work in parallel in threads
  // when ecs is in Constrained multi-threading mode:
  //  * async(Re)Create of entities can be called
  //  * delayed events will not be send
  //  * all sendEvent (except immediate) will be delayed/postponed
  //  * tracking of changes will be postponed
  //  * delayed creation of entities will be postponed
  //  * new components can not be added
  //  * resetESOrder can not be called
  //  The following requirements have to be met:
  //  * setConstrainedMTMode can not be called from other thread (validated)
  //  * tick can not be called (validated)
  //  * registerType, createComponent can not be called (partially validated)
  //  * mutable gets can not be called (not validated)
  //  * no data race (writing to same components as reading, and especially writing in other threads)
  //     this is currently NOT checked at all, but it can be
  // setConstrainedMTMode can be only called not within query or creation of Entities
  void setConstrainedMTMode(bool on);
  bool isConstrainedMTMode() const;

  // Enable mode where entities with reserved components assigned IDs within lowest 64K
  // Typically enabled on server in order to have identical eids for networking entities with client
  void setEidsReservationMode(bool on);

  // Send event to particular entity / all entities.
  // Actual time of delivery for this events is not determined (i.e. it might be spread over several frames in case of congestion)
  // Order of delivery is guaranteed to be FIFO (Queue)
  template <class EventType>
  void sendEvent(EntityId eid, EventType &&evt);
  void sendEvent(EntityId eid, Event &evt); // untyped, Event will be copied from with memcpy. it's actual sizeof has to be getLength()
  void sendEvent(EntityId eid, Event &&evt);
  void sendEvent(EntityId eid, SchemelessEvent &&evt);

  template <class EventType>
  void broadcastEvent(EventType &&evt);
  void broadcastEvent(Event &evt); // untyped, Event will be copied from with memcpy. it's actual sizeof has to be getLength()
  void broadcastEvent(Event &&evt);
  void broadcastEvent(SchemelessEvent &&evt);

  //
  template <class EventType>
  void dispatchEvent(EntityId eid, EventType &&evt); // if eid == INVALID_ENTITY_ID it is broadcast, otherwise it is unicast
  void dispatchEvent(EntityId eid, Event &evt);      // if eid == INVALID_ENTITY_ID it is broadcast, otherwise it is unicast.
  void dispatchEvent(EntityId eid, Event &&evt);     // if eid == INVALID_ENTITY_ID it is broadcast, otherwise it is unicast.

  // Same as above but events are guaranteed to be delivered at the time of call.
  // Usage of this methods is discouraged as it can't be optimized (e.g. spread over several frames in case of congestion)
  // Also ordering of this events are not determined.
  void sendEventImmediate(EntityId eid, Event &evt);
  void broadcastEventImmediate(Event &evt);
  void sendEventImmediate(EntityId eid, Event &&evt);
  void broadcastEventImmediate(Event &&evt);

  // if return not AR_OK, nothing was added
  TemplateDB::AddResult addTemplate(Template &&templ, dag::ConstSpan<const char *> *pnames = NULL);

  // AR_DUP here is not an error, it means it was updated
  TemplateDB::AddResult updateTemplate(Template &&templ, dag::ConstSpan<const char *> *pnames = NULL,
    bool update_template_values = false // update_template_values == true, will update entities component values which are same as in
                                        // old template, to values from new template
  );
  enum class RemoveTemplateResult
  {
    NotFound,
    HasEntities,
    Removed
  };
  RemoveTemplateResult removeTemplate(const char *name);
  void addTemplates(TemplateRefs &trefs, uint32_t tag = 0); // merely TemplateDB::resolveTemplateParents(trefs); for (t:trefs)
                                                            // addTemplate(move(t))

  enum class UpdateTemplateResult
  {
    Added,
    Updated,
    Same,
    InvalidName,
    RemoveHasEntities,
    DifferentTag,
    InvalidParents,
    Removed,
    Unknown
  };
  bool updateTemplates(TemplateRefs &trefs, bool update_templ_values, uint32_t tag,
    eastl::function<void(const char *, EntityManager::UpdateTemplateResult)> cb);

  const TemplateDB &getTemplateDB() const;
  TemplateDB &getTemplateDB();
  EntityId getSingletonEntity(const HashedConstString hashed_name);         // todo: check if it can be replaced with template name
  EntityId getOrCreateSingletonEntity(const HashedConstString hashed_name); // todo: check if it can be replaced with template name

  // Schedule entity creation with given components
  // Actual creation will happen in some unspecified (usually pretty close) moment in the future
  // Sends EventEntityCreated to entity systems that registered for it
  // 'templ' - template name to build entity from
  // 'initializer' - optional instance-specific components
  // 'cb' - callback that executed when actual entity created
  // Return INVALID_ENTITY_ID if entity creation failed
  // Returned eid is valid but can't be used for getting components or executing ESes until actual entity creation happens.
  EntityId createEntityAsync(const char *templ_name, ComponentsInitializer &&initializer = ComponentsInitializer(),
    ComponentsMap &&map = ComponentsMap(), create_entity_async_cb_t &&cb = create_entity_async_cb_t());
  // Same as above but entity created synchronously. It's generally discouraged to use this,
  // but it still might usefull for creation of logical entities that doesn't need game resources
  EntityId createEntitySync(const char *templ_name, ComponentsInitializer &&initializer = ComponentsInitializer(),
    ComponentsMap &&map = ComponentsMap()); // creates entity base on template

  // ECS2.0 Compatibility
  EntityId createEntityAsync(const char *templ_name, ComponentsInitializer &&initializer, create_entity_async_cb_t &&cb)
  {
    return createEntityAsync(templ_name, eastl::move(initializer), ComponentsMap(), eastl::move(cb));
  }

  // Schedule compare & remove difference in components/components of already existing entity & new template.
  // (i.e. remove extra/add missing components/components).
  // Parameters:
  //  'from_eid' - already existing entity that need to be modified
  //  'templ_name' - name of new template to calculate difference with. Supports special syntax
  //                 "templ1"+"templ2" for automatic creation new templates (sum of "templ1" & "templ2")
  //  'cb' - callback that will be executed on actual entity re-created
  //  Returns INVALID_ENTITY_ID if something gone wrong (e.g. passed template not found) or 'from_eid' otherwise.
  //  Returned eid is valid but can't be used for getting components or executing ESes until actual creation happens.
  //  Warning: 'from_eid' Entity should not have components that exist in instance only (i.e. not in template),
  //  otherwise it will be removed.
  EntityId reCreateEntityFromAsync(EntityId from_eid, const char *templ_name,
    ComponentsInitializer &&initializer = ComponentsInitializer(), ComponentsMap &&map = ComponentsMap(),
    create_entity_async_cb_t &&cb = create_entity_async_cb_t());

  // Deferred Destroy entity by id.
  // Returns true if entity exist and is added to destroy list, and false otherwise
  // Will send EventEntityDesroyed to entity systems that registered for it, only when destroyed
  bool destroyEntity(const EntityId &eid);
  bool destroyEntity(EntityId &eid);

  // Get Template's name for given entity (or nullptr if entity not exist/just allocated)
  const char *getEntityTemplateName(ecs::EntityId) const;

  // this is slower then getEntityTemplateId, but it checks queued entities
  // it is also NOT guaranteed to return correct results, if recreation requries adding components that need resources
  // it also SHOULD NOT be needed, and we add it is we currently have flawed design of recreation
  // instead of add_list, remove_list we provide the 'destination'
  // because of that add_sub_template(eid, "B"), add_sub_template(eid, "C") will result in +C, not +B+C, as expected!
  // in order to fix that, entitiy templates should be redesigned to be set of sub_templates
  //  getFutureTemplate allows simple workaround
  const char *getEntityFutureTemplateName(ecs::EntityId) const;

  // inspection
  //  How many components given entity has? Return negative if entity doesn't exist
  int getNumComponents(EntityId eid) const;
  int getArchetypeNumComponents(archetype_t archetype) const;

  // Remove all entities and all entity systems
  void clear();

  // Get entity components iterator (with or without template ones)
  // Warning: DO NOT recreateEntity from within this iterator - it might get invalidated!
  ComponentsIterator getComponentsIterator(EntityId eid, bool including_templates = true) const;

  typedef eastl::pair<const char *, const EntityComponentRef> ComponentInfo;
  // cid is 0.. till getArchetypeNumComponents
  ecs::component_index_t getArchetypeComponentIndex(archetype_t archetype, uint32_t cid) const;
  EntityComponentRef getEntityComponentRef(EntityId eid, uint32_t cid) const;   // cid is 0.. till getNumComponents
  const ComponentInfo getEntityComponentInfo(EntityId eid, uint32_t cid) const; // cid is 0.. till getNumComponents
  bool isEntityComponentSameAsTemplate(ecs::EntityId eid, const EntityComponentRef cref, uint32_t cid) const; // for inspection.
                                                                                                              // returns true, if cid
                                                                                                              // is same as in template
  bool isEntityComponentSameAsTemplate(EntityId eid, uint32_t cid) const; // for inspection. returns true, if cid is same as in
                                                                          // template

  const EntityComponentRef getComponentRef(EntityId eid, const HashedConstString name) const; // named for ecs20 compatibility
  //! not checked for non-pod types. named for ecs20 compatibility
  void set(EntityId eid, const HashedConstString name, ChildComponent &&);
  //! not checked for non-pod types. Won't assert if missing. named for ecs20 compatibility
  bool setOptional(EntityId eid, const HashedConstString name, ChildComponent &&);

  const EntityComponentRef getComponentRef(EntityId eid, component_index_t) const;
  EntityComponentRef getComponentRefRW(EntityId eid, component_index_t);

  template <class T>
  bool is(EntityId eid, const HashedConstString name) const
  {
    return is<T>(name) && has(eid, name);
  } // legacy! to be removed.

  template <class T>
  const T *__restrict getNullable(EntityId eid, const HashedConstString name) const;
  template <class T>
  T *__restrict getNullableRW(EntityId eid, const HashedConstString name);

  template <class T, typename U = ECS_MAYBE_VALUE_T(T)>
  U get(EntityId eid, const HashedConstString name) const;

  // Note: we intentonally return value (instead of const reference) here, because 'def' might be bound to temp variable
  template <class T>
  T getOr(EntityId eid, const HashedConstString name, const T &def) const;

  const char *getOr(EntityId eid, const HashedConstString name, const char *def) const;

  template <class T>
  T &__restrict getRW(EntityId eid, const HashedConstString name); // dangerous, as it has to return something!

  template <class T>
  void set(EntityId eid, const HashedConstString name, const T &__restrict v);
  void set(EntityId eid, const HashedConstString name, const char *v);
  template <class T>
  bool setOptional(EntityId eid, const HashedConstString name, const T &__restrict v); // won't assert if missing

  // fast versions (about 40% faster in release)
  // they skip hasmap lookup version, and type validation. One have to validate type when resolve component_index_t
  template <class T, typename U = ECS_MAYBE_VALUE_T(T)>
  U getFast(EntityId eid, const component_index_t cidx, const LTComponentList *__restrict list) const;
  template <class T, typename U = ECS_MAYBE_VALUE_T(T)>
  U getOrFast(EntityId eid, const component_index_t cidx, const T &__restrict def) const;
  template <class T>
  const T *__restrict getNullableFast(EntityId eid, const component_index_t name) const;

  template <class T>
  T &__restrict getRWFast(EntityId eid, const FastGetInfo name, const LTComponentList *__restrict list);
  template <class T>
  T *__restrict getNullableRWFast(EntityId eid, const FastGetInfo name);
  template <class T>
  bool setOptionalFast(EntityId eid, const FastGetInfo name, const T &__restrict v); // won't assert if missing. return true on success
  template <class T>
  void setFast(EntityId eid, const FastGetInfo name, const T &__restrict v, const LTComponentList *__restrict list);

  // template <typename T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<T&&>::value, void>>

  template <typename T>
  typename eastl::enable_if<eastl::is_rvalue_reference<T &&>::value>::type set(EntityId eid, const HashedConstString name, T &&v);
  template <typename T>
  typename eastl::enable_if<eastl::is_rvalue_reference<T &&>::value, bool>::type setOptional(EntityId eid,
    const HashedConstString name,
    T &&v); // won't assert if missing
  template <class T>
  bool is(const HashedConstString name) const;
  bool has(EntityId eid, const HashedConstString name) const;

  QueryId createQuery(const NamedQueryDesc &desc);
  void destroyQuery(QueryId);
  const component_index_t *queryComponents(QueryId) const; // invalid index means unresolved

  void setMaxUpdateJobs(uint32_t max_num_jobs);
  uint32_t getMaxWorkerId() const; // this is inclusive! i.e. it can return setMaxUpdateJobs()/threadpool::MAX_WORKER_COUNT + 1!
  void setQueryUpdateQuant(const char *es, uint16_t min_quant);

  // this function to be explicitly called only when new systems or tags appeared/disappeared
  void resetEsOrder();

  EntityManager();
  EntityManager(const EntityManager &from);
  ~EntityManager();
  void copyFrom(const EntityManager &from);
  // bool performQuery(EntityId eid, const NamedQueryDesc &desc, const query_cb_t &fun);//not yet implemented

  // for inspection
  const DataComponents &getDataComponents() const { return dataComponents; }
  const ComponentTypes &getComponentTypes() const { return componentTypes; }
  dag::ConstSpan<const EntitySystemDesc *> getSystems() const { return esList; } // active systems
  int getEntitySystemSize(uint32_t es); // just return total entities size for es in getSystems()

  uint32_t getQueriesCount() const { return (uint32_t)queryDescs.size(); }
  QueryId getQuery(uint32_t id) const;
  const char *getQueryName(QueryId h) const;
  int getQuerySize(QueryId query); // just return total entities size
  const BaseQueryDesc getQueryDesc(QueryId h) const;

  // Get total number of created entities
  int getNumEntities() const;

  void onEntitiesLoaded(dag::ConstSpan<EntityId> ents, bool all_or); // this can only be called by resource system
  bool isLoadingEntity(EntityId eid) const;                          // returns true, only if entity is alive and not yet fully loaded

  component_index_t createComponent(const HashedConstString name, type_index_t component_type,
    dag::Span<component_t> non_optional_deps, ComponentSerializer *io, component_flags_t flags);
  type_index_t registerType(const HashedConstString name, uint16_t data_size, ComponentSerializer *io, ComponentTypeFlags flags,
    create_ctm_t ctm, destroy_ctm_t dtm, void *user = nullptr); // manual registration. will not overwrite existing
  // this is to be made protected members
  EntityId createEntitySync(template_t, ComponentsInitializer &&initializer = ComponentsInitializer(),
    ComponentsMap &&map = ComponentsMap()); // creates entity base on template

  void flushDeferredEvents(); // do not casually call this! do not call from thread!

  // end of protected members
  void dumpArchetypes(int max_a);
  bool dumpArchetype(uint32_t arch);
  size_t dumpMemoryUsage();                             // dumps to debug
  void setEsTags(dag::ConstSpan<const char *> es_tags); // sets acceptable tags. If not set, _all_ are acceptable

  // Get Template's name for given entity (or nullptr if entity not exist/just allocated)
  ecs::template_t getEntityTemplateId(ecs::EntityId) const;
  archetype_t getArchetypeByTemplateId(ecs::template_t template_id) const;
  const char *getTemplateName(ecs::template_t) const;

  EventsDB &getEventsDbMutable() { return eventDb; }
  const EventsDB &getEventsDb() const { return eventDb; }
  void enableES(const char *es_name, bool on);

  // track ecs components access - only for development mode
  enum class TrackComponentOp : uint8_t
  {
    READ,
    WRITE
  };
  void startTrackComponent(component_t comp);
  void stopTrackComponentsAndDump();

  enum class TrackAccessStack
  {
    No,
    Yes
  };
  void trackComponent(const BaseQueryDesc &desc, const char *details, TrackAccessStack need_stack = TrackAccessStack::No,
    EntityId eid = ecs::INVALID_ENTITY_ID) const;

  void trackComponent(component_t comp, TrackComponentOp op, const char *details, TrackAccessStack need_stack = TrackAccessStack::No,
    EntityId eid = ecs::INVALID_ENTITY_ID) const;

  void trackComponentIndex(component_index_t cidx, TrackComponentOp op, const char *details,
    TrackAccessStack need_stack = TrackAccessStack::No, EntityId eid = INVALID_ENTITY_ID) const;

  void setUserData(void *user_data) { userData = user_data; }
  void *getUserData() const { return userData; }

  int64_t getOwnerThreadId() const { return ownerThreadId; }
  void setOwnerThreadId(int64_t value) { ownerThreadId = value; }

protected:
#include "internal/entityManagerProtected.h"
};

}; // namespace ecs

template <class T, bool ptr>
class InitOnDemandValidateThread : public InitOnDemand<T, ptr>
{
  typedef InitOnDemand<T, ptr> base_class;

public:
  using base_class::demandDestroy;
  using base_class::demandInit;
#if DAECS_EXTENSIVE_CHECKS
  int64_t threadId = 0;
  bool acquiredThread = false;
  void validateThread() const
  {
    DAECS_EXT_ASSERTF(!acquiredThread || threadId == get_current_thread_id(), "acquired thread %d != current %d", threadId,
      get_current_thread_id());
  }
  void freeThread() { acquiredThread = false; }
  void acquireThread()
  {
    threadId = get_current_thread_id();
    acquiredThread = true;
  }
#else
  void validateThread() const {}
  void freeThread() {}
  void acquireThread() {}
#endif
  inline operator T *() const
  {
    validateThread();
    return base_class::operator T *();
  }
  inline T &operator*() const
  {
    validateThread();
    return base_class::operator*();
  }
  inline T *operator->() const
  {
    validateThread();
    return base_class::operator->();
  }
  inline T *get() const
  {
    validateThread();
    return base_class::get();
  }
  explicit operator bool() const
  {
    validateThread();
    return base_class::operator bool();
  }
};

extern InitOnDemandValidateThread<ecs::EntityManager, false> g_entity_mgr;

#define ECS_DECLARE_GET_FAST_BASE(type_, aname, aname_str) \
  static ecs::LTComponentList aname##_component(ECS_HASH(aname_str), ecs::ComponentTypeInfo<type_>::type, __FILE__, "", 0)
#define ECS_DECLARE_GET_FAST(type_, aname) ECS_DECLARE_GET_FAST_BASE(type_, aname, #aname)

#define ECS_GET_COMPONENT(type_, eid, aname) \
  g_entity_mgr->getNullableRW<typename eastl::remove_const<type_>::type>(eid, ECS_HASH(#aname))
#define ECS_GET_COMPONENT_RO(type_, eid, aname) \
  g_entity_mgr->getNullable<typename eastl::remove_const<type_>::type>(eid, ECS_HASH(#aname))

#if _ECS_CODEGEN


template <class T>
T &ecs_codegen_stub_type();
template <class T>
T &a_ecs_codegen_get_call(const char *, const char *);
template <class T>
T &or_a_ecs_codegen_set_call(const char *, T &&);
template <class T>
T *nullable_a_ecs_codegen_get_call(const char *, const char *);
template <class T>
void a_ecs_codegen_set_call(const char *, T &&);

#define ECS_GET(type_, eid, aname)          a_ecs_codegen_get_call<type_>(#aname, #type_)
#define ECS_GET_OR(eid, aname, def)         or_a_ecs_codegen_set_call(#aname, def)
#define ECS_GET_NULLABLE(type_, eid, aname) nullable_a_ecs_codegen_get_call<type_>(#aname, #type_)

#define ECS_GET_RW(type_, eid, aname)          a_ecs_codegen_get_call<type_>(#aname, #type_)
#define ECS_GET_NULLABLE_RW(type_, eid, aname) nullable_a_ecs_codegen_get_call<type_>(#aname, #type_)
#define ECS_SET(eid, aname, v)                 a_ecs_codegen_set_call(#aname, v)
#define ECS_SET_OPTIONAL(eid, aname, v)        a_ecs_codegen_set_call(#aname, v)

#define ECS_GET_MGR(mgr, type_, eid, aname)          a_ecs_codegen_get_call<type_>(#aname, #type_)
#define ECS_GET_OR_MGR(mgr, eid, aname, def)         or_a_ecs_codegen_set_call(#aname, def)
#define ECS_GET_NULLABLE_MGR(mgr, type_, eid, aname) nullable_a_ecs_codegen_get_call<type_>(#aname, #type_)

#define ECS_GET_RW_MGR(mgr, type_, eid, aname)          a_ecs_codegen_get_call<type_>(#aname, #type_)
#define ECS_GET_NULLABLE_RW_MGR(mgr, type_, eid, aname) nullable_a_ecs_codegen_get_call<type_>(#aname, #type_)
#define ECS_SET_MGR(mgr, eid, aname, v)                 a_ecs_codegen_set_call(#aname, v)
#define ECS_SET_OPTIONAL_MGR(mgr, eid, aname, v)        (a_ecs_codegen_set_call(#aname, v), true)

#define ECS_INIT(to, aname, v) a_ecs_codegen_set_call(#aname, v)

#else

#define ECS_GET(type_, eid, aname)          g_entity_mgr->getFast<type_>(eid, aname##_component.getCidx(), &aname##_component)
#define ECS_GET_OR(eid, aname, def)         g_entity_mgr->getOrFast(eid, aname##_component.getCidx(), def)
#define ECS_GET_NULLABLE(type_, eid, aname) g_entity_mgr->getNullableFast<type_>(eid, aname##_component.getCidx())

#define ECS_GET_RW(type_, eid, aname)          g_entity_mgr->getRWFast<type_>(eid, aname##_component.getInfo(), &aname##_component)
#define ECS_GET_NULLABLE_RW(type_, eid, aname) g_entity_mgr->getNullableRWFast<type_>(eid, aname##_component.getInfo())
#define ECS_SET(eid, aname, v)                 g_entity_mgr->setFast(eid, aname##_component.getInfo(), v, &aname##_component)
#define ECS_SET_OPTIONAL(eid, aname, v)        g_entity_mgr->setOptionalFast(eid, aname##_component.getInfo(), v)

#define ECS_GET_MGR(mgr, type_, eid, aname)          (mgr).getFast<type_>(eid, aname##_component.getCidx(), &aname##_component)
#define ECS_GET_OR_MGR(mgr, eid, aname, def)         (mgr).getOrFast(eid, aname##_component.getCidx(), def)
#define ECS_GET_NULLABLE_MGR(mgr, type_, eid, aname) (mgr).getNullableFast<type_>(eid, aname##_component.getCidx())

#define ECS_GET_RW_MGR(mgr, type_, eid, aname)          (mgr).getRWFast<type_>(eid, aname##_component.getInfo(), &aname##_component)
#define ECS_GET_NULLABLE_RW_MGR(mgr, type_, eid, aname) (mgr).getNullableRWFast<type_>(eid, aname##_component.getInfo())
#define ECS_SET_MGR(mgr, eid, aname, v)                 (mgr).setFast(eid, aname##_component.getInfo(), v, &aname##_component)
#define ECS_SET_OPTIONAL_MGR(mgr, eid, aname, v)        (mgr).setOptionalFast(eid, aname##_component.getInfo(), v)

#define ECS_INIT(to, aname, v) to.insert(aname##_component.getName(), aname##_component.getInfo(), v, aname##_component.getNameStr())

#endif

// ecs20 preprocessor macro, to be removed
#define ECS_GET_ENTITY_COMPONENT(type, var, eid, aname) const type *var = ECS_GET_NULLABLE(type, eid, aname)

#define ECS_GET_ENTITY_COMPONENT_RW(type, var, eid, aname) type *var = ECS_GET_NULLABLE_RW(type, eid, aname) //-V1003

#define ECS_GET_SINGLETON_COMPONENT(type_, aname) \
  g_entity_mgr->getNullableRW<type_>(g_entity_mgr->getSingletonEntity(ECS_HASH(#aname)), ECS_HASH(#aname))

#include "internal/entityManagerImplementations.h"
