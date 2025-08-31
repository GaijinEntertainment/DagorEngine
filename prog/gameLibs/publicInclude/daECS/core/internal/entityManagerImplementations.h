#include <daECS/core/internal/asserts.h>
#pragma once

// implementations
namespace ecs
{

__forceinline bool EntityManager::isConstrainedMTMode() const { return interlocked_acquire_load(constrainedMode) != 0; }

__forceinline const TemplateDB &EntityManager::getTemplateDB() const { return templateDB; }
__forceinline TemplateDB &EntityManager::getTemplateDB() { return templateDB; }
__forceinline TemplateDB &EntityManager::getMutableTemplateDB() { return templateDB; } // thread unsafe!
inline const Template *EntityManager::buildTemplateByName(const char *n)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  return templateDB.buildTemplateByName(n);
}
inline int EntityManager::buildTemplateIdByName(const char *n)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  return templateDB.buildTemplateIdByName(n);
}
inline TemplateDB::AddResult EntityManager::addTemplate(Template &&templ, dag::ConstSpan<const char *> *pnames)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  return templateDB.addTemplate(eastl::move(templ), pnames);
}
inline void EntityManager::addTemplates(TemplateRefs &trefs, uint32_t tag)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  templateDB.addTemplates(trefs, tag);
}

inline const BaseQueryDesc EntityManager::CopyQueryDesc::getDesc() const
{
  return BaseQueryDesc(dag::ConstSpan<ComponentDesc>(components.begin(), rwCnt),
    dag::ConstSpan<ComponentDesc>(components.begin() + rwEnd(), roCnt),
    dag::ConstSpan<ComponentDesc>(components.begin() + roEnd(), rqCnt),
    dag::ConstSpan<ComponentDesc>(components.begin() + rqEnd(), noCnt));
}

template <class T>
inline bool EntityManager::is(const HashedConstString name) const
{
  return dataComponents.findComponent(name.hash).componentTypeName == ComponentTypeInfo<T>::type;
}

__forceinline bool EntityManager::getEntityArchetype(EntityId eid, int &idx, uint32_t &archetype) const
{
  const bool ret = entDescs.getEntityArchetype(eid, idx, archetype);
  if (ret)
  {
    DAECS_VALIDATE_ARCHETYPE(archetype);
  }
  return ret;
}

inline archetype_component_id EntityManager::componentIndexInEntityArchetype(EntityId eid, const component_index_t index) const
{
  if (index == INVALID_COMPONENT_INDEX)
    return INVALID_ARCHETYPE_COMPONENT_ID;

  int idx;
  uint32_t archetype = INVALID_ARCHETYPE;
  if (!getEntityArchetype(eid, idx, archetype))
    return INVALID_ARCHETYPE_COMPONENT_ID;
  return archetypes.getArchetypeComponentId(archetype, index);
}

__forceinline archetype_component_id EntityManager::componentIndexInEntityArchetype(EntityId eid, const HashedConstString name) const
{
#if DAGOR_DBGLEVEL > 1
  const char *componentName = dataComponents.findComponentName(name.hash);
  G_ASSERTF(!name.str || !componentName || strcmp(componentName, name.str) == 0, "hash collision <%s> <%s>", name.str, componentName);
#endif
  return componentIndexInEntityArchetype(eid, dataComponents.findComponentId(name.hash));
}

inline bool EntityManager::has(EntityId eid, const HashedConstString name) const
{
  return componentIndexInEntityArchetype(eid, name) != INVALID_ARCHETYPE_COMPONENT_ID;
}

inline int EntityManager::getNumComponents(EntityId eid) const
{
  int idx;
  uint32_t archetype = INVALID_ARCHETYPE;
  if (!getEntityArchetype(eid, idx, archetype))
    return -1;
  return archetypes.getComponentsCount(archetype) - 1; // first is eid
}

inline void *EntityManager::getEntityComponentDataInternal(EntityId eid, uint32_t cid, uint32_t &archetype) const
{
  int idx;
  if (!getEntityArchetype(eid, idx, archetype) || cid >= archetypes.getComponentsCount(archetype))
    return nullptr;

  // this condition can and should be removed as soon as we have sepatrate callback for constructors
  return archetypes.getComponentDataUnsafe(archetype, cid, archetypes.getComponentSizeUnsafe(archetype, cid), entDescs[idx].chunkId,
    entDescs[idx].idInChunk);
}

inline int EntityManager::getArchetypeNumComponents(archetype_t archetype) const
{
  if (archetype >= archetypes.size())
    return -1;
  return archetypes.getComponentsCount(archetype) - 1; // first is eid
}

inline ecs::component_index_t EntityManager::getArchetypeComponentIndex(archetype_t archetype, uint32_t cid) const
{
  if (archetype >= archetypes.size())
    return INVALID_COMPONENT_INDEX;

  cid++; // first is eid
  return archetypes.getComponentUnsafe(archetype, cid);
}

inline EntityComponentRef EntityManager::getEntityComponentRef(EntityId eid, uint32_t cid) const
{
  cid++; // first is eid
  uint32_t archetype;
  void *data = getEntityComponentDataInternal(eid, cid, archetype);
  if (!data)
    return EntityComponentRef();

  const component_index_t cIndex = archetypes.getComponentUnsafe(archetype, cid);
  trackComponentIndex(cIndex, TrackComponentOp::READ, "getRef", TrackAccessStack::Yes, eid);
  DataComponent dataComponentInfo = dataComponents.getComponentById(cIndex);
  return EntityComponentRef(data, dataComponentInfo.componentTypeName, dataComponentInfo.componentType, cIndex);
}

inline const EntityManager::ComponentInfo EntityManager::getEntityComponentInfo(EntityId eid, uint32_t cid) const
{
  EntityComponentRef ref = getEntityComponentRef(eid, cid);
  if (ref.isNull())
    return ComponentInfo("<invalid>", eastl::move(ref));
  return ComponentInfo(dataComponents.getComponentNameById(ref.getComponentId()), eastl::move(ref));
}

inline const EntityComponentRef EntityManager::getComponentRef(EntityId eid, const HashedConstString name) const
{
  return getEntityComponentRef(eid, componentIndexInEntityArchetype(eid, name) - 1); // first is eid, so we don't ask for eid
}
inline const EntityComponentRef EntityManager::getComponentRef(EntityId eid, component_index_t cidx) const
{
  return getEntityComponentRef(eid, componentIndexInEntityArchetype(eid, cidx) - 1); // first is eid, so we don't ask for eid
}

inline EntityComponentRef EntityManager::getComponentRefRW(EntityId eid, component_index_t cidx)
{
  scheduleTrackChanged(eid, cidx);
  return getEntityComponentRef(eid, componentIndexInEntityArchetype(eid, cidx) - 1); // first is eid, so we don't ask for eid
}

inline bool EntityManager::isEntityComponentSameAsTemplate(ecs::EntityId eid, const EntityComponentRef ref, uint32_t cid) const
{
  DAECS_EXT_ASSERT(ref.getRawData() != nullptr);
  DAECS_EXT_ASSERT(doesEntityExist(eid));
  cid++; // first is eid
  EntityDesc entDesc = entDescs[eid.index()];
  const uint32_t ofs = archetypes.getComponentInitialOfs(entDesc.archetype, cid);
  const void *templateData = templates.getTemplateData(entDesc.template_id, ofs, cid);
  if (!templateData)
    return false;
  ComponentType typeInfo = componentTypes.getTypeInfo(ref.getTypeId());
  if (is_pod(typeInfo.flags))
    return memcmp(ref.getRawData(), templateData, typeInfo.size) == 0;
  ComponentTypeManager *ctm = getComponentTypes().getTypeManager(ref.getTypeId());
  return ctm ? ctm->is_equal(ref.getRawData(), templateData) : true;
}

inline bool EntityManager::isEntityComponentSameAsTemplate(EntityId eid, uint32_t cid) const
{
  EntityComponentRef cref = getEntityComponentRef(eid, cid);
  if (!cref.getRawData())
    return false; // actually error
  return isEntityComponentSameAsTemplate(eid, cref, cid);
}

DAECS_RELEASE_INLINE void *__restrict EntityManager::get(EntityId eid, component_index_t index, uint32_t sz,
  uint32_t &__restrict archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE_ARG) const
{
#if DAECS_EXTENSIVE_CHECKS
  if (eid && eid == destroyingEntity && !for_write) // we don't check if we get invalid eid or for write. 'setOptional' to destroying
                                                    // entity is ok, set non optional will cause assert
    logerr("attempt to get (0x%X|%s) from eid = %d|%s during it's destroy", dataComponents.getComponentNameById(index),
      dataComponents.getComponentTpById(index), entity_id_t(eid), getEntityTemplateName(eid));
#endif
  int idx = eid.index();
  archetype = INVALID_ARCHETYPE;
  if (idx >= entDescs.allocated_size())
    return nullptr;
  const auto entDesc = entDescs[idx];
  if (entDesc.generation != eid.generation())
    return nullptr;
  archetype = entDesc.archetype;
  DAECS_VALIDATE_ARCHETYPE(archetype);
  if (DAGOR_UNLIKELY(archetype == INVALID_ARCHETYPE))
    return nullptr;
  archetype_component_id componentInArchetypeIndex = archetypes.getArchetypeComponentIdUnsafe(archetype, index);
  if (componentInArchetypeIndex == INVALID_ARCHETYPE_COMPONENT_ID)
    return nullptr;
  DAECS_EXT_ASSERT(archetypes.getComponent(entDesc.archetype, componentInArchetypeIndex) == index);
#if DAECS_EXTENSIVE_CHECKS
  trackComponentIndex(index, for_write ? TrackComponentOp::WRITE : TrackComponentOp::READ, for_write ? "getRW/set" : "get",
    TrackAccessStack::Yes, eid);
  if (DAGOR_UNLIKELY(
        for_write && creatingEntityTop.eid == eid && creatingEntityTop.createdCindex < index &&
        (componentTypes.getTypeInfo(dataComponents.getComponentById(index).componentType).flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)))
    logerr("attempt to write to component <%s> of type <%s> during creation of <%s>.\n"
           "Consider move writing to ES or put direct dependency (check levelES for example)",
      dataComponents.getComponentNameById(index), componentTypes.getTypeNameById(dataComponents.getComponentById(index).componentType),
      dataComponents.getComponentNameById(creatingEntityTop.createdCindex + 1));
#endif
  return archetypes.getComponentDataUnsafe(archetype, componentInArchetypeIndex, sz, entDesc.chunkId, entDesc.idInChunk);
}

inline void *__restrict EntityManager::get(EntityId eid, const HashedConstString name, component_type_t type_name, uint32_t sz,
  component_index_t &__restrict index, uint32_t &__restrict archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE_ARG) const
{
  index = dataComponents.findComponentId(name.hash);
  if (index == INVALID_COMPONENT_INDEX)
    return nullptr;
  //-- this check is sanity check. We actually don't do that at all, however, it can happen during development and even in release
  // can be potentially remove from release build, to get extra performance
  DataComponent componentInfo = dataComponents.getComponentById(index);
  if (componentInfo.componentTypeName != type_name)
  {
    logwarn("type mismatch on get <%s> <0x%X != requested 0x%X>", dataComponents.getComponentNameById(index),
      dataComponents.getComponentById(index).componentTypeName, type_name);
    return nullptr;
  }
//-- end of sanity check
#if DAGOR_DBGLEVEL > 1
  G_ASSERTF(!name.str || strcmp(dataComponents.getComponentNameById(index), name.str) == 0, "hash collision <%s> <%s>", name.str,
    dataComponents.getComponentNameById(index));
#endif
  return get(eid, index, sz, archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE_PASS);
}


template <class T>
DAECS_RELEASE_INLINE T &__restrict EntityManager::getRW(EntityId eid, const HashedConstString name)
{
  component_index_t cidx;
  uint32_t archetype;
  void *__restrict val =
    get(eid, name, ComponentTypeInfo<T>::type, ComponentTypeInfo<T>::size, cidx, archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE);
  if (DAGOR_LIKELY(val != nullptr))
  {
    if (ComponentTypeInfo<T>::can_be_tracked)
      scheduleTrackChangedCheck(eid, archetype, cidx);
    return PtrComponentType<T>::ref(val);
  }
  accessError(eid, name);
  return getScratchValue<T>();
}

template <class T, typename U>
DAECS_RELEASE_INLINE U EntityManager::get(EntityId eid, const HashedConstString name) const
{
  component_index_t cidx;
  uint32_t archetype;
  const void *__restrict val =
    get(eid, name, ComponentTypeInfo<T>::type, ComponentTypeInfo<T>::size, cidx, archetype DAECS_EXTENSIVE_CHECK_FOR_READ);
  if (DAGOR_LIKELY(val != nullptr))
    return PtrComponentType<T>::cref(val);
  accessError(eid, name);
  return getScratchValue<T>();
}

template <class T>
DAECS_RELEASE_INLINE const T *__restrict EntityManager::getNullable(EntityId eid, const HashedConstString name) const
{
  component_index_t cidx;
  uint32_t archetype;
  const void *__restrict val =
    get(eid, name, ComponentTypeInfo<T>::type, ComponentTypeInfo<T>::size, cidx, archetype DAECS_EXTENSIVE_CHECK_FOR_READ);
  return (!PtrComponentType<T>::is_boxed || val) ? &PtrComponentType<T>::cref(val) : nullptr;
}

template <class T>
DAECS_RELEASE_INLINE T EntityManager::getOr(EntityId eid, const HashedConstString name, const T &__restrict def) const
{
  static_assert(!eastl::is_same<T, ecs::string>::value,
    "Generic \"getOr\" is not for strings, use \"const char* getOr(...)\" method instead");
  component_index_t cidx;
  uint32_t archetype;
  const void *val =
    get(eid, name, ComponentTypeInfo<T>::type, ComponentTypeInfo<T>::size, cidx, archetype DAECS_EXTENSIVE_CHECK_FOR_READ);
  return val ? PtrComponentType<T>::cref(val) : def;
}

// fast functions
template <class T, typename U>
DAECS_RELEASE_INLINE U EntityManager::getFast(EntityId eid, const component_index_t cidx, const LTComponentList *list) const
{
  uint32_t archetype;
  const void *__restrict val = get(eid, cidx, ComponentTypeInfo<T>::size, archetype DAECS_EXTENSIVE_CHECK_FOR_READ);
  if (DAGOR_LIKELY(val != nullptr))
    return PtrComponentType<T>::cref(val);
  accessError(eid, cidx, list);
  return getScratchValue<T>();
}

template <class T>
DAECS_RELEASE_INLINE const T *__restrict EntityManager::getNullableFast(EntityId eid, const component_index_t cidx) const
{
  uint32_t archetype;
  const void *__restrict val = get(eid, cidx, ComponentTypeInfo<T>::size, archetype DAECS_EXTENSIVE_CHECK_FOR_READ);
  return (!PtrComponentType<T>::is_boxed || val) ? &PtrComponentType<T>::cref(val) : nullptr;
}
template <class T, typename U>
__forceinline U EntityManager::getOrFast(EntityId eid, const component_index_t cidx, const T &__restrict def) const
{
  static_assert(!eastl::is_same<T, ecs::string>::value, "Generic \"getOrFast\" is not strings, use non-generic method for that");
  uint32_t archetype;
  const void *val = get(eid, cidx, ComponentTypeInfo<T>::size, archetype DAECS_EXTENSIVE_CHECK_FOR_READ);
  return val ? PtrComponentType<T>::cref(val) : def;
}

template <class T>
DAECS_RELEASE_INLINE T &__restrict EntityManager::getRWFast(EntityId eid, const FastGetInfo name,
  const LTComponentList *__restrict list)
{
  uint32_t archetype;
  void *__restrict val = get(eid, name.cidx, ComponentTypeInfo<T>::size, archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE);
  if (DAGOR_LIKELY(val != nullptr))
  {
    if (ComponentTypeInfo<T>::can_be_tracked && name.canBeTracked())
      scheduleTrackChangedCheck(eid, archetype, name.cidx);
    return PtrComponentType<T>::ref(val);
  }
  accessError(eid, name.cidx, list);
  return getScratchValue<T>();
}

template <class T>
DAECS_RELEASE_INLINE T *__restrict EntityManager::getNullableRWFast(EntityId eid, const FastGetInfo name)
{
  uint32_t archetype;
  void *__restrict val = get(eid, name.cidx, ComponentTypeInfo<T>::size, archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE);
  if (!val)
    return nullptr;
  if (ComponentTypeInfo<T>::can_be_tracked && name.canBeTracked())
    scheduleTrackChangedCheck(eid, archetype, name.cidx);
  return &PtrComponentType<T>::ref(val);
}

template <class T>
DAECS_RELEASE_INLINE bool EntityManager::setOptionalFast(EntityId eid, const FastGetInfo name, const T &__restrict v)
{
  T *__restrict to = getNullableRWFast<T>(eid, name);
  if (!to)
    return false;
  *to = v;
  return true;
}

template <class T>
DAECS_RELEASE_INLINE void EntityManager::setFast(EntityId eid, const FastGetInfo name, const T &__restrict v,
  const LTComponentList *__restrict list)
{
  T *__restrict to = getNullableRWFast<T>(eid, name);
  if (to)
    *to = v;
  else
    accessError(eid, name.cidx, list);
}


DAECS_RELEASE_INLINE bool EntityManager::archetypeTrackChangedCheck(uint32_t archetypeId, component_index_t cidx)
{
  if (archetypeId == INVALID_ARCHETYPE) // do not use archetypes.size(), to avoid memory read
    return false;
  DAECS_VALIDATE_ARCHETYPE(archetypeId);
  const component_index_t old_cidx = dataComponents.getTrackedPair(cidx);
  if (old_cidx == INVALID_COMPONENT_INDEX)
    return false;
  // this is even existence check, we don't care about actual offset.
  // todo: can be optimized with bitvector (we have up to ~2048 total components, so less than 256 bytes)
  // todo: we can check if archetype has tracked at all, even faster
  if (archetypes.getArchetypeComponentIdUnsafe(archetypeId, old_cidx) == INVALID_ARCHETYPE_COMPONENT_ID)
    return false;
  return true;
}

inline void EntityManager::scheduleTrackChangedCheck(EntityId eid, uint32_t archetypeId, component_index_t cidx)
{
  if (archetypeTrackChangedCheck(archetypeId, cidx))
    scheduleTrackChanged(eid, cidx);
}

template <class T>
DAECS_RELEASE_INLINE T *__restrict EntityManager::getNullableRW(EntityId eid, const HashedConstString name)
{
  component_index_t cidx;
  uint32_t archetype;
  void *__restrict val =
    get(eid, name, ComponentTypeInfo<T>::type, ComponentTypeInfo<T>::size, cidx, archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE);
  if (!val)
    return NULL;
  if (ComponentTypeInfo<T>::can_be_tracked)
    scheduleTrackChangedCheck(eid, archetype, cidx);
  return &PtrComponentType<T>::ref(val);
}


template <class T, bool optional>
DAECS_RELEASE_INLINE bool EntityManager::setComponentInternal(EntityId eid, const HashedConstString name, const T &__restrict v)
{
  T *__restrict attr = getNullableRW<T>(eid, name);
  if (attr)
    *attr = v;
  else if (!optional)
  {
    accessError(eid, name);
    return false;
  }
  return true;
}

template <typename T, bool optional>
typename eastl::enable_if<eastl::is_rvalue_reference<T &&>::value, bool>::type DAECS_RELEASE_INLINE
EntityManager::setComponentInternal(EntityId eid, const HashedConstString name, T &&v)
{
  T *__restrict attr = getNullableRW<T>(eid, name);
  if (attr)
    *attr = eastl::move(v);
  else if (!optional)
  {
    accessError(eid, name);
    return false;
  }
  return true;
}

inline entity_id_t make_eid(uint32_t index, uint32_t gen) { return EntityManager::make_eid(index, gen); }
inline unsigned get_generation(const EntityId e) { return e.generation(); }

inline bool EntityManager::doesEntityExist(EntityId e) const { return entDescs.doesEntityExist(e); }

__forceinline void EntityManager::sendEventImmediate(EntityId eid, Event &&evt) { sendEventImmediate(eid, evt); }
__forceinline void EntityManager::broadcastEventImmediate(Event &&evt) { broadcastEventImmediate(evt); }

template <class EventType>
__forceinline void EntityManager::dispatchEvent(EntityId eid, EventType &&evt) // ecs::INVALID_ENTITY_ID means broadcast
{
  const bool isMtMode = isConstrainedMTMode();
  DAECS_EXT_ASSERT(isMtMode || get_current_thread_id() == ownerThreadId);
  DAECS_EXT_ASSERTF(bool(eid) == bool(evt.getFlags() & EVCAST_UNICAST), "event %s has %s flags but sent as %s", evt.getName(),
    (evt.getFlags() & EVCAST_UNICAST) ? "unicast" : "broadcast", bool(eid) ? "unicast" : "broadcast");
  ScopedMTMutexT<decltype(deferredEventsMutex)> evtMutex(isMtMode, ownerThreadId, deferredEventsMutex);
  validateEventRegistration(evt, eastl::remove_reference<EventType>::type::staticName());

  deferredEventsCount++;
  eventsStorage.emplaceEvent(eid, eastl::move(evt));
}

template <class Storage>
__forceinline void EntityManager::emplaceUntypedEvent(Storage &storage, EntityId eid, Event &evt)
{
  const uint32_t len = evt.getLength();
  void *__restrict at = storage.allocateUntypedEvent(eid, len);
  if (DAGOR_LIKELY(!(evt.getFlags() & EVFLG_DESTROY)))
  {
    memcpy(at, (void *)&evt, len); // we can memcpy such event
  }
  else
    eventDb.moveOut(*this, at, eastl::move(evt)); // hash lookup
}

__forceinline void EntityManager::dispatchEvent(EntityId eid, Event &&evt) { dispatchEvent(eid, (Event &)evt); }

template <class EventType>
inline void EntityManager::sendEvent(EntityId eid, EventType &&evt)
{
  G_STATIC_ASSERT(eastl::remove_reference<EventType>::type::staticFlags() & EVCAST_UNICAST);
  if (DAGOR_LIKELY(eid)) // attempt to send event to invalid entity is no-op
    dispatchEvent(eid, eastl::move(evt));
}

inline void EntityManager::sendEvent(EntityId eid, Event &evt)
{
  if (DAGOR_LIKELY(eid)) // attempt to send event to invalid entity is no-op
    dispatchEvent(eid, evt);
}

inline void EntityManager::sendEvent(EntityId eid, Event &&evt) { sendEvent(eid, (Event &)evt); }

template <class EventType>
inline void EntityManager::broadcastEvent(EventType &&evt)
{
  G_STATIC_ASSERT(eastl::remove_reference<EventType>::type::staticFlags() & EVCAST_BROADCAST);
  dispatchEvent(INVALID_ENTITY_ID, eastl::move(evt));
}

inline void EntityManager::broadcastEvent(Event &evt) { dispatchEvent(INVALID_ENTITY_ID, evt); }
inline void EntityManager::broadcastEvent(Event &&evt) { broadcastEvent((Event &)evt); }

inline bool EntityManager::destroyEntity(const EntityId &eid)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex); // todo: we can make free threadsafety
  if (!doesEntityExist(eid))
    return false;
  // always postpone destruction
  emplaceDestroy(eid);
  return true;
}

inline bool EntityManager::destroyEntity(EntityId &eid)
{
  bool existed = destroyEntity(static_cast<const EntityId &>(eid));
  eid = ecs::INVALID_ENTITY_ID;
  return existed;
}

inline void EntityManager::setFilterTags(dag::ConstSpan<const char *> tags) { templateDB.setFilterTags(tags); }

inline ecs::template_t EntityManager::getEntityTemplateId(ecs::EntityId eid) const
{
  const unsigned idx = eid.index();
  if (!entDescs.doesEntityExist(idx, eid.generation()) || idx >= entDescs.allocated_size())
    return INVALID_TEMPLATE_INDEX;
  return entDescs[idx].template_id;
}

inline const char *EntityManager::getEntityTemplateName(ecs::EntityId eid) const { return getTemplateName(getEntityTemplateId(eid)); }

template <bool optional>
__forceinline bool EntityManager::setComponentInternal(EntityId eid, const HashedConstString name, ChildComponent &&a)
{
  component_index_t cidx;
  uint32_t archetype;
  void *data = get(eid, name, a.getUserType(), a.getSize(), cidx, archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE);
  if (!data)
  {
    if (!optional)
      accessError(eid, name);
    return false;
  }
  scheduleTrackChangedCheck(eid, archetype, cidx);
  type_index_t componentType = a.componentTypeIndex;
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(componentType);
  if (is_pod(componentTypeInfo.flags))
    memcpy(data, a.getRawData(), a.getSize());
  else
  {
    // G_ASSERTF(0, "move is not checked");
    ComponentTypeManager *ctm = componentTypes.getTypeManager(componentType);
    if (ctm)
    {
      ctm->assign(data, a.getRawData());
      // ctm->move(data, a.getRawData());a.reset();//should be move, but we don't support move yet
    }
  }
  return true;
}

inline bool EntityManager::isLoadingEntity(EntityId eid) const
{
  return entDescs.getEntityState(eid) == EntitiesDescriptors::EntityState::Loading;
}

inline void EntityManager::set(EntityId eid, const HashedConstString name, ChildComponent &&a)
{
  setComponentInternal<false>(eid, name, eastl::move(a));
}

template <typename T>
typename eastl::enable_if<eastl::is_rvalue_reference<T &&>::value>::type inline EntityManager::set(EntityId eid,
  const HashedConstString name, T &&v)
{
  setComponentInternal<T, false>(eid, name, eastl::move(v));
}
template <class T>
inline void EntityManager::set(EntityId eid, const HashedConstString name, const T &v)
{
  setComponentInternal<T, false>(eid, name, v);
}

inline bool EntityManager::setOptional(EntityId eid, const HashedConstString name, ChildComponent &&a)
{
  return setComponentInternal<true>(eid, name, eastl::move(a));
}
template <typename T>
typename eastl::enable_if<eastl::is_rvalue_reference<T &&>::value, bool>::type inline EntityManager::setOptional(EntityId eid,
  const HashedConstString name, T &&v)
{
  return setComponentInternal<T, true>(eid, name, eastl::move(v));
}
template <class T>
inline bool EntityManager::setOptional(EntityId eid, const HashedConstString name, const T &v)
{
  return setComponentInternal<T, true>(eid, name, v);
}


inline component_index_t EntityManager::createComponent(const HashedConstString name, type_index_t component_type,
  dag::Span<component_t> non_optional_deps, ComponentSerializer *io, component_flags_t flags)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  return dataComponents.createComponent(name, component_type, non_optional_deps, io, flags, componentTypes);
}

inline type_index_t EntityManager::registerType(const HashedConstString name, uint16_t data_size, ComponentSerializer *io,
  ComponentTypeFlags flags, create_ctm_t ctm, destroy_ctm_t dtm, void *user)
{
  DAECS_EXT_ASSERT(!isConstrainedMTMode());
  return componentTypes.registerType(name.str, name.hash, data_size, io, flags, ctm, dtm, user);
}

inline void EntityManager::flushDeferredEvents() { sendQueuedEvents(-1); }

inline void EntityManager::setEidsReservationMode(bool on)
{
  G_ASSERTF(nextResevedEidIndex <= 1, "%s shall be called before creation of entities with reserved components", __FUNCTION__);
  eidsReservationMode = on;
}

struct ComponentsIterator
{
  typedef EntityManager::ComponentInfo Ret;
  void operator++();
  operator bool() const { return currentAttr < attrCount; }
  Ret operator*() const;

protected:
  ComponentsIterator() = delete;
  ComponentsIterator(const EntityManager &manager_, EntityId eid_, bool include_, int attr_count);
  friend class EntityManager;
  const EntityManager &manager;
  EntityId eid = ecs::INVALID_ENTITY_ID;
  int currentAttr = 0, attrCount = 0;
  bool including_templates = false;
};

inline ComponentsIterator EntityManager::getComponentsIterator(EntityId eid, bool including_templates) const
{
  return ComponentsIterator(*this, eid, including_templates, getNumComponents(eid));
}

inline ComponentsIterator::ComponentsIterator(const EntityManager &manager_, EntityId eid_, bool include_, int attr_count) :
  manager(manager_), eid(eid_), including_templates(include_), attrCount(attr_count)
{}

inline EntityManager::ComponentInfo ComponentsIterator::operator*() const { return manager.getEntityComponentInfo(eid, currentAttr); }

inline void ComponentsIterator::operator++()
{
  currentAttr++;
  if (!including_templates)
    for (; currentAttr < attrCount && manager.isEntityComponentSameAsTemplate(eid, currentAttr);)
      ++currentAttr;
}

inline uint32_t EntityManager::getMaxWorkerId() const { return maxNumJobs; }

template <typename Fn>
bool EntityManager::performEidQuery(ecs::EntityId eid, QueryId h, Fn &&fun, void *user_data)
{
  uint32_t eidIdx = eid.index();
  if (eidIdx >= entDescs.allocated_size())
    return false;
  const auto &entDesc = entDescs[eidIdx];
  archetype_t archetype = entDesc.archetype;
  if (entDesc.generation != eid.generation() || archetype == INVALID_ARCHETYPE)
    return false;

  QueryView qv(*this, user_data);
  QueryView::ComponentsData componentData[MAX_ONE_EID_QUERY_COMPONENTS];
  qv.componentData = componentData;
  if (!fillEidQueryView(eid, entDesc, h, qv))
    return false;

  trackComponent(queryDescs[h.index()].getDesc(), queryDescs[h.index()].getName(), TrackAccessStack::No, eid);

  const bool isConstrained = isConstrainedMTMode();
  if (!isConstrained)
    nestedQuery++;
  fun(qv);
  if (!isConstrained)
    nestedQuery--;
  return true;
}

template <class T>
inline const T *EntityManager::getRequestingBase(const HashedConstString name) const
{
  const CurrentlyRequesting &creating = *requestingTop;

  if constexpr (eastl::is_same_v<T, EntityId>)
    if (name.hash == ECS_HASH("eid").hash)
      return &creating.eid;

  // Iterate in reverse order, so later values overwrite earlier ones
  for (auto it = creating.initializer.end() - 1, ite = creating.initializer.begin() - 1; it != ite; --it) // linear search!
    if (it->name == name.hash)
      return it->second.getNullable<T>();

  // check current entity data (from archetype eventsStorage)
  if (DAGOR_UNLIKELY(creating.oldArchetype != INVALID_ARCHETYPE))
  {
    const T *old = getNullable<T>(creating.eid, name);
    if (old)
      return old;
  }

  // new template
  const archetype_t newArchetype = creating.newArchetype;
  const template_t newTemplate = creating.newTemplate;
  const component_index_t index = dataComponents.findComponentId(name.hash);
  if (index == INVALID_COMPONENT_INDEX)
    return nullptr;

  //-- this check is sanity check. We actually don't do that at all, however, it can happen during development and even in release
  // can be potentially remove from release build, to get extra performance
  DataComponent componentInfo = dataComponents.getComponentById(index);
  const component_type_t type_name = ComponentTypeInfo<T>::type;
  if (componentInfo.componentTypeName != type_name)
  {
    logwarn("type mismatch on get <%s> <0x%X != requested 0x%X>", dataComponents.getComponentNameById(index),
      componentInfo.componentTypeName, type_name);
    return nullptr;
  }

  archetype_component_id componentInArchetypeIndex = archetypes.getArchetypeComponentId(newArchetype, index);
  if (componentInArchetypeIndex == INVALID_ARCHETYPE_COMPONENT_ID)
    return nullptr;

  const uint8_t *__restrict templateData = templates.getTemplate(newTemplate).initialData.get();
  return &PtrComponentType<T>::cref(templateData + archetypes.initialComponentDataOffset(newArchetype)[componentInArchetypeIndex]);
}

template <class T>
inline const T &EntityManager::getRequesting(const HashedConstString name) const
{
  const T *comp = getRequestingBase<T>(name);
  if (DAGOR_LIKELY(comp != nullptr))
    return *comp;
  accessError(requestingTop->eid, name);
  return getScratchValue<T>();
}

template <class T>
inline const T &EntityManager::getRequesting(const HashedConstString name, const T &def) const
{
  const T *comp = getRequestingBase<T>(name);
  return comp == nullptr ? def : *comp;
};

class ResourceRequestCb
{
public:
  template <class T>
  const T &get(const HashedConstString hashed_name) const
  {
    return mgr.getRequesting<T>(hashed_name);
  }
  template <class T>
  const T &getOr(const HashedConstString hashed_name, const T &def) const
  {
    return mgr.getRequesting<T>(hashed_name, def);
  }
  template <class T>
  const T *getNullable(const HashedConstString hashed_name) const
  {
    return mgr.getRequestingBase<T>(hashed_name);
  }
  void operator()(const char *n, unsigned type_id) const
  {
    G_ASSERT_RETURN(n && *n, );
    requestedResources.emplace(n, type_id);
  }
  EA_NON_COPYABLE(ResourceRequestCb)
protected:
  ResourceRequestCb(const EntityManager &m) : mgr(m) {}
  const EntityManager &mgr;
  mutable gameres_list_t requestedResources;
  friend class EntityManager;
};

inline void EntityManager::setReplicationCb(replication_cb_t cb) { replicationCb = cb; }

#if !DAECS_EXTENSIVE_CHECKS
inline void EntityManager::startTrackComponent(component_t) {}
inline void EntityManager::stopTrackComponentsAndDump() {}

inline void EntityManager::trackComponent(const BaseQueryDesc &, const char *, TrackAccessStack, EntityId) const {}

inline void EntityManager::trackComponent(component_t, TrackComponentOp, const char *, TrackAccessStack, EntityId) const {}

inline void EntityManager::trackComponentIndex(component_index_t, TrackComponentOp, const char *, TrackAccessStack, EntityId) const {}
#else
inline void EntityManager::trackComponentIndex(component_index_t cidx, TrackComponentOp op, const char *details,
  TrackAccessStack need_stack, EntityId eid) const
{
  if (cidx < getDataComponents().size())
    trackComponent(getDataComponents().getComponentTpById(cidx), op, details, need_stack, eid);
}
#endif

}; // namespace ecs
