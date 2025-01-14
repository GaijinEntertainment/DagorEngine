public:
// this defers events for loading entity. it doesn't have to be public, it is not used by anyone
void dispatchEventImmediate(EntityId eid, Event &evt); // if eid == INVALID_ENTITY_ID it is broadcast, otherwise it is unicast.

template_t instantiateTemplate(int id);
const Template *buildTemplateByName(const char *);
template_t templateByName(const char *templ_name, EntityId eid = {});
int buildTemplateIdByName(const char *);
TemplateDBInfo &getMutableTemplateDBInfo() { return templateDB.info(); }
uint32_t defragArchetypes();
uint32_t defragTemplates();
const component_index_t *replicatedComponentsList(template_t t, uint32_t &cnt) const
{
  return templates.replicatedComponentsList(t, cnt);
}
bool isReplicatedComponent(template_t t, component_index_t cidx) const { return templates.isReplicatedComponent(t, cidx); }
void forceServerEidGeneration(EntityId); // only for replication, create server eid on client
static inline entity_id_t make_eid(uint32_t index, uint32_t gen) { return index | (gen << ENTITY_INDEX_BITS); }

protected:
typedef uint16_t es_index_type;
static bool is_son_of(const Template &t, const int p, const TemplateDB &db, int rec_depth = 0);
TemplateDB &getMutableTemplateDB(); // not sure if we have to expose non-const version..
void updateEntitiesWithTemplate(template_t oldT, template_t newTemp, bool update_templ_values);
void removeTemplateInternal(const uint32_t *to_remove, const uint32_t cnt);
template_t createTemplate(ComponentsMap &&initializer, const char *debug_name = "(unknown)", const ComponentsFlags *flags = nullptr);
void initCompileTimeQueries();
bool validateInitializer(template_t templ, ComponentsInitializer &comp_init) const;
void createEntityInternal(EntityId eid, template_t templId, ComponentsInitializer &&initializer, ComponentsMap &&map,
  create_entity_async_cb_t &&cb);
bool collapseRecreate(ecs::EntityId eid, const char *templId, ComponentsInitializer &cinit, ComponentsMap &cmap,
  create_entity_async_cb_t &&cb);
__forceinline bool getEntityArchetype(EntityId eid, int &idx, uint32_t &archetype) const;
void *getEntityComponentDataInternal(EntityId eid, uint32_t cid, uint32_t &archetype) const;
void scheduleTrackChangedNoMutex(EntityId eid, component_index_t cidx);
void scheduleTrackChanged(EntityId eid, component_index_t cidx);
void scheduleTrackChangedCheck(EntityId eid, uint32_t archetypeId, component_index_t cidx);
bool archetypeTrackChangedCheck(uint32_t archetypeId, component_index_t cidx);
struct EntityDesc;
static constexpr int MAX_ONE_EID_QUERY_COMPONENTS = 96;
bool fillEidQueryView(ecs::EntityId eid, EntityDesc ent, QueryId h, QueryView &__restrict qv);
template <typename Fn>
bool performEidQuery(ecs::EntityId eid, QueryId h, Fn &&fun, void *user_data);

struct ESPayLoad;
typedef void (*ESFuncType)(const ESPayLoad &evt, const QueryView &__restrict components);
template <typename Cb>
__forceinline void performSTQuery(const Query &__restrict pQuery, void *user_data, const Cb &fun);
void performQueryES(QueryId h, ESFuncType fun, const ESPayLoad &, void *user_data, int min_quant = 0); // use min_quant of more than 0
                                                                                                       // for parallel for execution
                                                                                                       // (each job will take at least
                                                                                                       // min_quant of data)
void performQueryEmptyAllowed(QueryId h, ESFuncType fun, const ESPayLoad &, void *user_data, int min_quant = 0); // use min_quant of
                                                                                                                 // more than 0 for
                                                                                                                 // parallel for
                                                                                                                 // execution (each job
                                                                                                                 // will take at least
                                                                                                                 // min_quant of data)

void performQuery(QueryId query, const query_cb_t &fun, void *user_data = nullptr, int min_quant = 0); // use min_quant of more than 0
                                                                                                       // for parallel for execution
                                                                                                       // (each job will take ata least
                                                                                                       // min_quant of data)
void performQuery(const Query &__restrict query, const query_cb_t &__restrict fun, void *user_data, int min_quant = 1);
bool performMTQuery(const Query &__restrict query, const query_cb_t &__restrict fun, void *user_data, int min_quant);
QueryCbResult performQueryStoppable(const Query &__restrict pQuery, const stoppable_query_cb_t &fun, void *user_data);
QueryCbResult performQueryStoppable(QueryId query, const stoppable_query_cb_t &fun, void *__restrict user_data = nullptr); // use
                                                                                                                           // min_quant
                                                                                                                           // of more
                                                                                                                           // than 0
                                                                                                                           // for
                                                                                                                           // parallel
                                                                                                                           // for
                                                                                                                           // execution
                                                                                                                           // (each job
                                                                                                                           // will take
                                                                                                                           // ata least
                                                                                                                           // min_quant
                                                                                                                           // of data)

bool makeQuery(const char *name, const BaseQueryDesc &desc, ArchetypesQuery &arch_desc, Query &query, struct QueryContainer &);
// flags, not values
enum ResolvedStatus : uint8_t
{
  NOT_RESOLVED = 0,
  FULLY_RESOLVED = 1,
  RESOLVED = 2,
  RESOLVED_MASK = 3
};
ResolvedStatus resolveQuery(uint32_t index, ResolvedStatus current_status, ResolvedQueryDesc &resDesc);
bool makeArchetypesQuery(archetype_t first_archetype, uint32_t index, bool wasResolved);
uint32_t fillQuery(const ArchetypesQuery &archDesc, struct QueryContainer &);
struct CommittedQuery commitQuery(QueryId h, struct QueryContainer &ctx, const ArchetypesQuery &arch, uint32_t total);
void sheduleArchetypeTracking(const ArchetypesQuery &archDesc);
bool validateQueryDesc(const BaseQueryDesc &) const;

typedef HashedKeySet<uint64_t, uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>, framemem_allocator> TrackedChangesTemp;
typedef HashedKeySet<uint64_t, uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> TrackedChangeEid;
typedef HashedKeySet<uint32_t, uint32_t(0), oa_hashmap_util::MumStepHash<uint32_t>> TrackedChangeArchetype;

eastl::bitvector<> queryScheduled;
SmallTab<uint32_t> trackQueryIndices;
void convertArchetypeScheduledChanges(); // converts queryScheduled + trackQueryIndices -> archetypeTrackingQueue

TrackedChangeEid eidTrackingQueue;
OSSpinlock archetypeTrackingMutex, eidTrackingMutex;

eastl::bitvector<> canBeReplicated;
TrackedChangeArchetype archetypeTrackingQueue;
replication_cb_t replicationCb = NULL;

void onChangeEvents(const TrackedChangesTemp &process);
void preprocessTrackedChange(EntityId eid, archetype_t archetype, component_index_t cidx, TrackedChangesTemp &process);
bool trackChangedArchetype(uint32_t archetype, component_index_t cidx, component_index_t old_cidx, TrackedChangesTemp &process);
void trackChanged(EntityId eid, component_index_t cidx, TrackedChangesTemp &process);

template <class T, int const_csz = 1, bool use_ctm = false>
void compare_data(TrackedChangesTemp &process, uint32_t arch, const uint32_t compOffset, const uint32_t oldCompOffset,
  component_index_t cidx, size_t csz = 1, const ComponentTypeManager *ctm = nullptr);
unsigned trackScheduledChanges();

EntityId allocateOneEid();
EntityId allocateOneEidDelayed(bool delayed);

void removeDataFromArchetype(const uint32_t archetype, const chunk_type_t chunkId, const uint32_t idInChunk); // remove data, without
                                                                                                              // calling to destructors

template <typename FilterUsed>
void destroyComponents(const uint32_t archetype, const chunk_type_t chunkId, const uint32_t idInChunk, FilterUsed filter); // to get
                                                                                                                           // data
                                                                                                                           // (from
                                                                                                                           // loading
                                                                                                                           // entity as
                                                                                                                           // well)
template <typename FilterUsed>
void removeFromArchetype(const uint32_t archetype, const chunk_type_t chunkId, const uint32_t idInChunk, FilterUsed filter); // remove
                                                                                                                             // data
                                                                                                                             // and
                                                                                                                             // call
                                                                                                                             // destroy
friend class ChildComponent;

#if DAECS_EXTENSIVE_CHECKS
#define DAECS_EXTENSIVE_CHECK_FOR_WRITE_PASS , for_write
#define DAECS_EXTENSIVE_CHECK_FOR_WRITE_ARG  , bool for_write
#define DAECS_EXTENSIVE_CHECK_FOR_WRITE      , true
#define DAECS_EXTENSIVE_CHECK_FOR_READ       , false
#else
#define DAECS_EXTENSIVE_CHECK_FOR_WRITE_PASS
#define DAECS_EXTENSIVE_CHECK_FOR_WRITE_ARG
#define DAECS_EXTENSIVE_CHECK_FOR_WRITE
#define DAECS_EXTENSIVE_CHECK_FOR_READ
#endif

void *__restrict get(EntityId eid, const HashedConstString name, component_type_t type_name, uint32_t sz,
  component_index_t &__restrict cidx, uint32_t &__restrict archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE_ARG) const;
void *__restrict get(EntityId eid, const component_index_t cidx, uint32_t sz,
  uint32_t &__restrict archetype DAECS_EXTENSIVE_CHECK_FOR_WRITE_ARG) const;

archetype_component_id componentIndexInEntityArchetype(EntityId eid, const component_index_t index) const;
archetype_component_id componentIndexInEntityArchetype(EntityId eid, const HashedConstString name) const;

struct alignas(size_t) EntityDesc // 64 bit.
{
  archetype_t archetype = INVALID_ARCHETYPE;       // 16bit
  chunk_type_t chunkId = 0;                        // 8 bit
  uint8_t generation = 0;                          // 8 bit
  template_t template_id = INVALID_TEMPLATE_INDEX; // 16bit
  id_in_chunk_type_t idInChunk = 0;                // 16bit
  void reset()
  {
    template_id = INVALID_TEMPLATE_INDEX;
    archetype = INVALID_ARCHETYPE;
#if DAECS_EXTENSIVE_CHECKS
    idInChunk = eastl::numeric_limits<id_in_chunk_type_t>::max();
#endif
  }
};
G_STATIC_ASSERT(sizeof(EntityDesc) == 8);

struct EntitiesDescriptors
{
  SmallTab<EntityDesc> entDescs; // we rarely add entities, and often compare with size
  // Not zero counter in eid.index() slot means that entity was scheduled for [re]creation or destruction. SoA to entDescs
  SmallTab<uint8_t> currentlyCreatingEntitiesCnt;
  uint32_t totalSize = 0;
  uint32_t delayedAdded = 0;
  decltype(EntityDesc::generation) globalGen = 0;
  bool isCurrentlyCreating(uint32_t idx) const { return idx < entDescs.size() ? currentlyCreatingEntitiesCnt[idx] != 0 : true; }
  void decreaseCreating(uint32_t idx) { currentlyCreatingEntitiesCnt[idx]--; }
  void increaseCreating(uint32_t idx)
  {
    if (idx < entDescs.size())
      currentlyCreatingEntitiesCnt[idx]++;
  }
  void clear()
  {
    entDescs.clear();
    currentlyCreatingEntitiesCnt.clear();
    totalSize = delayedAdded = 0;
  }
  void addDelayed()
  {
    if (delayedAdded == 0)
      return;
    EntityDesc e;
    e.generation = globalGen;
    entDescs.resize(totalSize, e);
    currentlyCreatingEntitiesCnt.resize(totalSize, 1);
    DAECS_EXT_ASSERT(totalSize == entDescs.size());
    delayedAdded = 0;
  }
  uint32_t push_back_delayed()
  {
    delayedAdded++; // is done under mutex

    // while it is done under mutex, and simultaneuos add is not possible,
    // size() can be used in other thread, so it is important it is always sane numbers
    const uint32_t current = interlocked_relaxed_load(totalSize);
    interlocked_relaxed_store(totalSize, current + 1);
    return current;
  }
  uint32_t push_back()
  {
    addDelayed();
    const uint32_t idx = totalSize;
    entDescs.push_back();
    currentlyCreatingEntitiesCnt.push_back(0);
    totalSize++;
    DAECS_EXT_FAST_ASSERT(entDescs.size() == currentlyCreatingEntitiesCnt.size());
    return idx;
  }
  EntityId makeEid(uint32_t idx) { return EntityId(make_eid(idx, idx < allocated_size() ? entDescs[idx].generation : globalGen)); }
  __forceinline bool doesEntityExist(unsigned idx, decltype(EntityDesc::generation) generation) const
  {
    return idx < size() && generation == (idx < allocated_size() ? entDescs[idx].generation : globalGen);
  }
  bool doesEntityExist(EntityId e) const { return doesEntityExist(e.index(), e.generation()); }

  __forceinline bool getEntityArchetype(EntityId eid, int &idx, uint32_t &archetype) const
  {
    idx = eid.index();
    if (idx >= size())
      return false;
    if (idx >= allocated_size())
    {
      archetype = INVALID_ARCHETYPE;
      return false;
    }
    const auto &entDesc = entDescs[idx];
    if (entDesc.generation != eid.generation())
      return false;
    archetype = entDesc.archetype;
    return archetype != INVALID_ARCHETYPE;
  }

  enum class EntityState
  {
    Alive,
    Dead,
    Loading
  };
  inline EntityState getEntityState(EntityId eid) const
  {
    const unsigned idx = eid.index();
    if (idx >= size()) // it is dead
      return EntityState::Dead;
    if (idx >= allocated_size()) // it is not spawned
      return eid.generation() == globalGen ? EntityState::Loading : EntityState::Dead;
    const EntityDesc &entDesc = entDescs[idx];
    if (entDesc.generation != eid.generation())
      return EntityState::Dead;
    if (entDesc.archetype != INVALID_ARCHETYPE) // dead or loaded
      return EntityState::Alive;
#if DAECS_EXTENSIVE_CHECKS
    if (currentlyCreatingEntitiesCnt[idx] == 0)
      logerr("Entity %d isn't scheduled for creation, but has no archetype.\n"
             "Most likely, reference to it in another entity was replicated from server, while entity itself was not.\n"
             "Consider changing order of creation on server, scope sorting, or add explicit poll-and-wait before access to Entity",
        eid);
#endif
    return EntityState::Loading;
  }

  uint32_t allocated_size() const { return entDescs.size(); }
  uint32_t size() const { return interlocked_relaxed_load(totalSize); }
  uint32_t capacity() const { return entDescs.capacity(); }
  void reserve(uint32_t a)
  {
    entDescs.reserve(a);
    currentlyCreatingEntitiesCnt.reserve(a);
  }
  void resize(uint32_t a)
  {
    EntityDesc e;
    e.generation = globalGen;
    entDescs.clear();
    entDescs.resize(a, e);

    currentlyCreatingEntitiesCnt.clear();
    currentlyCreatingEntitiesCnt.resize(a, 0);
    delayedAdded = 0;
    totalSize = a;
  }

  EntityDesc &operator[](uint32_t i) { return entDescs[i]; }
  const EntityDesc &operator[](uint32_t i) const { return entDescs[i]; }
} entDescs;

int constrainedMode = 0;
int nestedQuery = 0; // checked simultaneous with constrainedMode, so keep it close
bool eidsReservationMode = false;
bool exhaustedReservedIndices = false;
uint32_t nextResevedEidIndex = 0;
float lastTrackedCount = 0;
eastl::deque<ecs::entity_id_t> freeIndices, freeIndicesReserved;
Archetypes archetypes;
ComponentTypes componentTypes;
DataComponents dataComponents;
Templates templates;
eastl::unique_ptr<uint8_t[]> zeroMem; // size of biggest type registered, so we can always return something in case of getNullableRW
template <typename T>
static inline T &get_scratch_value_impl(void *mem, eastl::true_type)
{
  // Note: this will leak memory for types that hold memory (strings, vectors, objects...) if someone will write to it (since this
  // instance is never freed) but that's should be relatively ok since this is already emergency code-path
  return *new (mem, _NEW_INPLACE) T();
}
template <typename T>
static inline T &get_scratch_value_impl(void *mem, eastl::false_type)
{
  memset(mem, 0, sizeof(T));
  return *(T *)mem;
}

template <typename T>
DAGOR_NOINLINE T &getScratchValue() const
{
  typedef typename eastl::type_select<eastl::is_default_constructible<T>::value && !eastl::is_scalar<T>::value, eastl::true_type,
    eastl::false_type>::type TP;
  return get_scratch_value_impl<T>(zeroMem.get(), TP());
}

// during create get should work on entity currently being created.
// if create will cause another immediate create, we try to survive that gracefully using stack of creating entities, although it is
// potentially dangerous
StackAllocator<> creationAllocator;
#if DAECS_EXTENSIVE_CHECKS
struct CreatingEntity
{
  EntityId eid;
  component_index_t createdCindex;
};
CreatingEntity creatingEntityTop;
#endif

dag::VectorMap<ecs::EntityId, uint16_t> loadingEntities; // it is vector_map eid->loadCount, as there can be more than one
                                                         // recreation scheduled
enum class RequestResources
{
  AlreadyLoaded,
  Scheduled,
  Loaded,
  Error
};
enum class RequestResourcesType
{
  ASYNC,
  SYNC,
  CHECK_ONLY
};
RequestResources requestResources(EntityId eid, archetype_t old, template_t templId, const ComponentsInitializer &initializer,
  RequestResourcesType type);
bool validateResources(EntityId eid, archetype_t old, template_t templId, const ComponentsInitializer &initializer);
typename decltype(loadingEntities)::iterator find_loading_entity(EntityId eid) { return loadingEntities.find(eid); }
typename decltype(loadingEntities)::const_iterator find_loading_entity(EntityId eid) const { return loadingEntities.find(eid); }
void recreateOnSameArchetype(EntityId eid, template_t templId, ComponentsInitializer &&initializer,
  const create_entity_async_cb_t &cb);

uint32_t lastEsGen = 0;
ska::flat_hash_set<eastl::string> esTags;
dag::VectorMap<eastl::string, uint32_t> esOrder;
dag::VectorSet<eastl::string> disableEntitySystems, esSkip;
dag::VectorSet<ecs::hash_str_t> ignoredES;
SmallTab<const EntitySystemDesc *, MidmemAlloc> esList; // sorted
eastl::bitvector<> esForAllEntities;

eastl::vector<ecs::EntityId> resourceEntities;
gameres_list_t requestedResources;

friend void singleton_creation_event(const Event &evt, const QueryView &components);
ska::flat_hash_map<hash_str_t, ecs::EntityId, ska::power_of_two_std_hash<hash_str_t>> singletonEntities;
EntityId hasSingletoneEntity(hash_str_t hash) const;
EntityId hasSingletoneEntity(const char *n) const;

void flushGameResRequests();
struct EsIndexFixedSet
{
  const es_index_type *begin() const { return list.cbegin(); }
  const es_index_type *end() const { return list.cend(); }
  bool empty() const { return list.empty(); }
  size_t size() const { return list.size(); }
  size_t count(es_index_type value) const { return list.count(value); }
  void insert(es_index_type value) { list.insert(value); }

  typedef InplaceKeySet<es_index_type, 7, es_index_type> set_type_t;
  typedef typename set_type_t::shallow_copy_t shallow_copy_t;
  shallow_copy_t getShallowCopy() const { return list.getShallowCopy(); }
  EsIndexFixedSet() = default;
  EsIndexFixedSet(const EsIndexFixedSet &) = delete;
  EsIndexFixedSet &operator=(const EsIndexFixedSet &) = delete;
#if DAECS_EXTENSIVE_CHECKS
  ~EsIndexFixedSet() { checkUnlocked("destructor", 0); }
  EsIndexFixedSet(EsIndexFixedSet &&a) : list(eastl::move(a.list)), lockedEid(a.lockedEid) { a.unlock(); }
  EsIndexFixedSet &operator=(EsIndexFixedSet &&a)
  {
    list = eastl::move(a.list);
    eastl::swap(lockedEid, a.lockedEid);
    return *this;
  }
  void lock(EntityId eid) const
  {
    checkUnlocked("while locking entity", entity_id_t(eid));
    lockedEid = eid;
  }
  void unlock() const { lockedEid = EntityId{}; }
  void checkUnlocked(const char *s, uint32_t value) const
  {
    if (lockedEid)
      logerr("can't update es list %s %d, locked for processing eid=%d", value, s, lockedEid);
  }

protected:
  mutable EntityId lockedEid;
#else
  EsIndexFixedSet(EsIndexFixedSet &&a) = default;
  EsIndexFixedSet &operator=(EsIndexFixedSet &&) = default;
  static __forceinline void lock(EntityId) {}
  static __forceinline void unlock() {}
  static __forceinline void checkUnlocked(const char *, uint32_t) {}
#endif
protected:
  set_type_t list;
};
typedef EsIndexFixedSet es_index_set;
dag::Vector<es_index_set> esUpdates; // sorted by update priority ES functions (for each update stage)

// probably use ska::flat_hash_map<event_type_t, event_index_t> esEventsMap; eastl::vector<dag::VectorSet<es_index_type>>
// esEventsList; check performance
ska::flat_hash_map<event_type_t, es_index_set, ska::power_of_two_std_hash<event_type_t>> esEvents;
ska::flat_hash_map<component_t, es_index_set, ska::power_of_two_std_hash<event_type_t>> esOnChangeEvents;

enum ArchEsList
{
  ENTITY_CREATION_ES,
  ENTITY_RECREATION_ES,
  ENTITY_DESTROY_ES,
  ARCHETYPES_ES_LIST_COUNT
};

struct InternalEvent final : public Event
{
  InternalEvent(event_type_t tp) : Event(tp, sizeof(Event), EVCAST_UNICAST | EVFLG_CORE) {}
};

InternalEvent archListEvents[ARCHETYPES_ES_LIST_COUNT] = {
  ECS_HASH("EventEntityCreated").hash, ECS_HASH("EventEntityRecreated").hash, ECS_HASH("EventEntityDestroyed").hash};

enum ArchRecreateEsList
{
  DISAPPEAR_ES,
  APPEAR_ES,
  RECREATE_ES_LIST_COUNT
};
InternalEvent recreateEvents[RECREATE_ES_LIST_COUNT] = {
  ECS_HASH("EventComponentsDisappear").hash, ECS_HASH("EventComponentsAppear").hash};
typedef dag::Vector<es_index_set> archetype_es_list;
archetype_es_list archetypesES[ARCHETYPES_ES_LIST_COUNT];

struct RecreateEsSet
{
  es_index_set disappear, appear;
};
typedef dag::VectorMap<archetype_t, RecreateEsSet> archetype_es_map; // this is pointer, and not value, so we can add archetypes
                                                                     // within recreate ES call

dag::Vector<archetype_es_map> archetypesRecreateES; // same size as archetypesES. Make tuple vector?
const RecreateEsSet *updateRecreatePair(archetype_t old_archetype, archetype_t new_archetype);

void updateArchetypeESLists(archetype_t arch);
void rebuildArchetypeESLists();
void validateArchetypeES(archetype_t archetype);
bool doesEsApplyToArch(es_index_type esIndex, archetype_t archetype);
bool doesQueryIdApplyToArch(QueryId queryId, archetype_t archetype);

const EntitySystemDesc &getESDescForEvent(es_index_type esIndex, const Event &evt) const;
void registerEsEvents();
void registerEsStage(int esId);
void registerEsEvent(int esId);
void registerEs(int esId);
void validateEventRegistrationInternal(const Event &evt, const char *name);
#if DAECS_EXTENSIVE_CHECKS
void validateEventRegistration(const Event &evt, const char *name) { validateEventRegistrationInternal(evt, name); }
#else
void validateEventRegistration(const Event &, const char *) {}
#endif

void callESEvent(es_index_type esIndex, const Event &evt, QueryView &qv);
void notifyESEventHandlers(EntityId eid, const Event &evt);
void notifyESEventHandlersInternal(EntityId eid, const Event &evt, const es_index_type *__restrict es_start,
  const es_index_type *__restrict es_end);
void notifyESEventHandlers(EntityId eid, archetype_t archetype, ArchEsList list);

__forceinline const RecreateEsSet *__restrict getRecreatePair(archetype_t old_archetype, archetype_t new_archetype)
{
  const auto &archRecreateList = archetypesRecreateES[old_archetype];
  auto recreateListIt = archRecreateList.find(new_archetype);
  // update cache on the fly. If we hadn't ever recreated from old_archetype to new_archetype
  // ofc, we can make that N*(N-1) matrices, and remove updating cache on the fly, but there way less actual recreates.
  if (DAGOR_UNLIKELY(recreateListIt == archetypesRecreateES[old_archetype].end()))
    return updateRecreatePair(old_archetype, new_archetype);
  else
    return &recreateListIt->second;
}

__forceinline void notifyESEventHandlersAppear(EntityId eid, archetype_t old_archetype, archetype_t new_archetype)
{
  // we use shallow copy instead of just const reference, because unfortunately we still have createSync/instantiateTemplate inside ES
  // calls as long as it is removed - should not be needed anymore.
  auto &appearRef = getRecreatePair(old_archetype, new_archetype)->appear;
  if (!appearRef.empty())
  {
#if DAECS_EXTENSIVE_CHECKS
    appearRef.lock(eid);
#endif
    // we use shallow copy instead of just const reference, because unfortunately we still have createSync/instantiateTemplate inside
    // ES calls as long as it is removed - should not be needed anymore.
    auto appear = appearRef.getShallowCopy();
    notifyESEventHandlersInternal(eid, recreateEvents[APPEAR_ES], appear.cbegin(), appear.cend());
#if DAECS_EXTENSIVE_CHECKS
    getRecreatePair(old_archetype, new_archetype)->appear.unlock(); // we have to reget it, as it could be reallocated inside
#endif
  }
}

__forceinline void notifyESEventHandlersDisappear(EntityId eid, archetype_t old_archetype, archetype_t new_archetype)
{
  auto &disappearRef = getRecreatePair(old_archetype, new_archetype)->disappear;
  if (!disappearRef.empty())
  {
#if DAECS_EXTENSIVE_CHECKS
    disappearRef.lock(eid);
#endif
    // we use shallow copy instead of just const reference, because unfortunately we still have createSync/instantiateTemplate inside
    // ES calls as long as it is removed - should not be needed anymore.
    auto disappear = disappearRef.getShallowCopy();
    notifyESEventHandlersInternal(eid, recreateEvents[DISAPPEAR_ES], disappear.cbegin(), disappear.cend());
#if DAECS_EXTENSIVE_CHECKS
    getRecreatePair(old_archetype, new_archetype)->disappear.unlock(); // we have to reget it, as it could be reallocated inside
#endif
  }
}

// eastl::hash_map<, ecs_query_handle> resolvedQueriesIndices;//combined NameQuery. list of all component_t + flags. has to be updated
// if ecs_query_handle is completely destroyed
bool isResolved(uint32_t idx) const; // completely or partially
bool isCompletelyResolved(uint32_t idx) const;
bool updatePersistentQuery(archetype_t first_arch, QueryId id, uint32_t &index, bool should_re_resolve);
bool updatePersistentQuery(archetype_t first_arch, QueryId id, bool should_re_resolve)
{
  uint32_t index;
  return updatePersistentQuery(first_arch, id, index, should_re_resolve);
}
bool updatePersistentQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve);
bool resolvePersistentQueryInternal(uint32_t index);
bool makeArchetypesQueryInternal(archetype_t first_arch, uint32_t index, bool should_re_resolve);

struct CopyQueryDesc
{
#if DAGOR_DBGLEVEL != 0
  eastl::string name;
#endif
  dag::Vector<ComponentDesc> components; // replace with FixedVector?
  uint8_t requiredSetCount = 0, rwCnt = 0, roCnt = 0, rqCnt = 0, noCnt = 0;
  // 3 bytes of padding. add is_eid_query?
  uint16_t rwEnd() const { return rwCnt; }
  uint16_t roEnd() const { return rwEnd() + roCnt; }
  uint16_t rqEnd() const { return roEnd() + rqCnt; }
  uint16_t noEnd() const { return rqEnd() + noCnt; }
  CopyQueryDesc() = default;
#if DAGOR_DBGLEVEL != 0
  const char *getName() const { return name.c_str(); }
#else
  const char *getName() const { return ""; }
#endif
  void setDebugName(const char *name_)
  {
    G_UNUSED(name_);
#if DAGOR_DBGLEVEL != 0
    name = name_;
#endif
  }
  void clear() {}
  void init(const EntityManager &mgr, const char *name_, const BaseQueryDesc &d);
  const BaseQueryDesc getDesc() const;
};

// SoA for QueryId
uint8_t currentQueryGen = 0;
void clearQueries();
void invalidatePersistentQueries();
struct QueryHasher
{
  size_t operator()(const QueryId &h) { return wyhash64(uint32_t(h), 1); }
};
ska::flat_hash_map<QueryId, dag::Vector<es_index_type>, QueryHasher> queryToEsMap;
QueryId createUnresolvedQuery(const NamedQueryDesc &desc);
#if DAGOR_DBGLEVEL > 0
OAHashNameMap<false> queriesComponentsNames;
#endif
dag::Vector<ArchetypesQuery> archetypeQueries; // SoA, we need to update ArchetypesQuery from ResolvedQueryDesc again, if we add new
                                               // archetype
dag::Vector<ArchetypesEidQuery> archetypeEidQueries; // SoA, we need to update ArchetypesQuery from ResolvedQueryDesc again, if we add
                                                     // new archetype
dag::Vector<uint16_t, EASTLAllocatorType, false> archComponentsSizeContainers;
dag::Vector<ResolvedQueryDesc> resolvedQueries; // SoA, we never need resolvedQuery again, if it was successfully resolved, but we need
                                                // it to resolve ArchetypesQuery
typedef uint32_t status_word_type_t;
dag::Vector<status_word_type_t> resolvedQueryStatus; // SoA, two-bit vector
static constexpr status_word_type_t status_words_shift = get_const_log2(sizeof(status_word_type_t) * 4),
                                    status_words_mask = (1 << status_words_shift) - 1;
static bool isFullyResolved(ResolvedStatus s) { return s & FULLY_RESOLVED; }
static bool isResolved(ResolvedStatus s) { return s != NOT_RESOLVED; }
ResolvedStatus getQueryStatus(uint32_t idx) const
{
  const status_word_type_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
  return (ResolvedStatus)((resolvedQueryStatus[wordIdx] >> wordShift) & RESOLVED_MASK);
}
void orQueryStatus(uint32_t idx, ResolvedStatus status)
{
  const uint32_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
  resolvedQueryStatus[wordIdx] |= (status << wordShift);
}
void resetQueryStatus(uint32_t idx)
{
  const status_word_type_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
  resolvedQueryStatus[wordIdx] &= ~(status_word_type_t(RESOLVED_MASK) << wordShift);
}
void addOneResolvedQueryStatus()
{
  uint32_t sz = (resolvedQueries.size() + status_words_mask) >> status_words_shift;
  if (sz != resolvedQueryStatus.size())
    resolvedQueryStatus.push_back(status_word_type_t(0));
}
dag::Vector<CopyQueryDesc> queryDescs;   // SoA, not empty ONLY if resolvedQueries is not resolved fully
dag::Vector<uint16_t> queriesReferences; // SoA, reference count of ecs_query_handles
dag::Vector<uint8_t> queriesGenerations; // SoA, generation in ecs_query_handles. Sanity check.
uint32_t freeQueriesCount = 0;

typedef uint64_t query_components_hash;
static query_components_hash query_components_hash_calc(const BaseQueryDesc &desc);
ska::flat_hash_map<query_components_hash, QueryId, ska::power_of_two_std_hash<query_components_hash>> queryMap;

uint32_t addOneQuery();
bool isQueryValidGen(QueryId id) const
{
  if (!id)
    return false;
  auto idx = id.index();
  return idx < queriesGenerations.size() && queriesGenerations[idx] == id.generation();
}
bool isQueryValid(QueryId id) const
{
  bool ret = isQueryValidGen(id);
  G_FAST_ASSERT(!ret || queriesReferences[id.index()]);
  return ret;
}
void queriesShrink();
//--SoA for QueryId
dag::Vector<QueryId> esListQueries; // parallel to esList, index in ecs_query_handle

#if DAECS_EXTENSIVE_CHECKS
EntityId destroyingEntity;
bool isQueryingArchetype(uint32_t) const;
void changeQueryingArchetype(uint32_t, int);
struct ScopedQueryingArchetypesCheck
{
  ScopedQueryingArchetypesCheck(uint32_t index_, EntityManager &mgr);
  ~ScopedQueryingArchetypesCheck() { changeQueryingArchetypes(-1); }
  EA_NON_COPYABLE(ScopedQueryingArchetypesCheck)
private:
  EntityManager &mgr;
  uint32_t index, validateCount;
  void changeQueryingArchetypes(int add);
};
#else
bool isQueryingArchetype(uint32_t) const { return false; }
void changeQueryingArchetype(uint32_t, int) {}
struct ScopedQueryingArchetypesCheck
{
  ScopedQueryingArchetypesCheck(uint32_t, EntityManager &) {}
};
#endif
bool isEventSendingPossible() const { return nestedQuery == 0 && !isConstrainedMTMode(); }
bool isDeferredCreationPossible() const { return nestedQuery == 0 && !isConstrainedMTMode(); }
uint32_t maxNumJobs = 0, maxNumJobsSet = 0;
void updateCurrentUpdateMaxJobs();

void destroyEntityImmediate(EntityId e);

DeferredEventsStorage<> eventsStorage;
uint32_t deferredEventsCount = 0;
WinCritSec deferredEventsMutex;
template <class CircularBuffer, typename ProcessEvent>
uint32_t processEventInternal(CircularBuffer &buffer, ProcessEvent &&cb);
template <class EventStorage>
uint32_t processEventsActive(uint32_t count, EventStorage &);
template <class EventStorage>
uint32_t processEventsAnyway(uint32_t count, EventStorage &);
template <class EventStorage>
void emplaceUntypedEvent(EventStorage &storage, EntityId eid, Event &evt);
template <class T>
void destroyEvents(T &storage);
// should be out-of-line
template <class EventStorage>
uint32_t processEventsExhausted(uint32_t count, EventStorage &);

struct ScopeSetMtConstrained
{
  EntityManager &mgr;
  const bool wasConstrained;
  ScopeSetMtConstrained(EntityManager &m) : mgr(m), wasConstrained(m.isConstrainedMTMode())
  {
    if (!wasConstrained)
      mgr.setConstrainedMTMode(true);
  }
  ~ScopeSetMtConstrained()
  {
    if (!wasConstrained)
      mgr.setConstrainedMTMode(false);
  }
  EA_NON_COPYABLE(ScopeSetMtConstrained)
};

void performDeferredEvents(bool flush_all); // manually call perform deferred events
void debugInvalidateComponentes();

struct LoadingEntityEvents
{
  EntityId eid;
  DeferredEventsStorage<10> events; // it is bigger_log2i(Event::max_event_size)
  bool operator<(const LoadingEntityEvents &rhs) const { return entity_id_t(eid) < entity_id_t(rhs.eid); }
  struct Compare
  {
    bool operator()(EntityId eid, const LoadingEntityEvents &rhs) const { return entity_id_t(eid) < entity_id_t(rhs.eid); }
    bool operator()(const LoadingEntityEvents &rhs, EntityId eid) const { return entity_id_t(rhs.eid) < entity_id_t(eid); }
  };
};
dag::VectorSet<LoadingEntityEvents> eventsForLoadingEntities;

dag::VectorSet<LoadingEntityEvents>::iterator findLoadingEntityEvents(EntityId eid)
{
  return eventsForLoadingEntities.find_as(eid, LoadingEntityEvents::Compare());
}

void addEventForLoadingEntity(EntityId eid, Event &evt);

void sendQueuedEvents(uint32_t top_count_to);
uint32_t current_tick_events = 0, average_tick_events_uint = 16;
float average_tick_events = 16.f;

mutable WinCritSec creationMutex;
template <typename T>
struct ScopedMTMutexT
{
  ScopedMTMutexT(bool is_mt, int64_t owner_thread, T &mutex_) : mutex(is_mt ? &mutex_ : nullptr)
  {
    G_UNUSED(owner_thread);
    if (mutex)
      mutex->lock();
    else
      DAECS_EXT_ASSERT(get_current_thread_id() == owner_thread);
  }
  ~ScopedMTMutexT()
  {
    if (mutex)
      mutex->unlock();
  }
  operator bool() const { return mutex != nullptr; }
  EA_NON_COPYABLE(ScopedMTMutexT);

protected:
  T *mutex;
};
using ScopedMTMutex = ScopedMTMutexT<WinCritSec>;

struct DelayedEntityCreation
{
  ComponentsInitializer compInit;
  ComponentsMap compMap; // this one should probably be replaced with unique_ptr or something. todo: optimize it
  // We almost never have ComponentsMap, and it's size is 24 byte (pointer is just 8 bytes). But we have one(!) such component in each
  // network replicated object.
  create_entity_async_cb_t cb;
  eastl::string templateName;
  EntityId eid;
  template_t templ = INVALID_TEMPLATE_INDEX;
  enum class Op : uint8_t
  {
    Destroy,
    Create,
    Add,
    Sub
  } op = Op::Destroy; // Add, Sub are unused currently but actually should be the only used!
  bool currentlyCreating = false;
  bool isToDestroy() const { return op == Op::Destroy; }
  DelayedEntityCreation(EntityId eid_, Op op_, const char *templ_name, ComponentsInitializer &&ai, ComponentsMap &&am,
    create_entity_async_cb_t &&cb_) :
    eid(eid_), op(op_), templateName(templ_name), compInit(eastl::move(ai)), compMap(eastl::move(am)), cb(eastl::move(cb_))
  {
    G_FAST_ASSERT(!isToDestroy());
  }
  DelayedEntityCreation(EntityId eid_, Op op_, template_t t, ComponentsInitializer &&ai, ComponentsMap &&am,
    create_entity_async_cb_t &&cb_) :
    eid(eid_), op(op_), templ(t), compInit(eastl::move(ai)), compMap(eastl::move(am)), cb(eastl::move(cb_))
  {
    G_FAST_ASSERT(!isToDestroy());
  }
  DelayedEntityCreation(EntityId eid_) : eid(eid_) {}
  void clear();
};
struct DelayedEntityCreationChunk
{
  static constexpr uint16_t minChunkCapacity = 64, // no reason to create chunk with less than this number of entities
    maxChunkCapacity = SHRT_MAX + 1;
  DelayedEntityCreation *queue = nullptr;
  uint16_t readFrom = 0, writeTo = 0, capacity;
  DelayedEntityCreationChunk(uint16_t cap) : capacity(cap)
  {
    queue = (DelayedEntityCreation *)memalloc(capacity * sizeof(DelayedEntityCreation), defaultmem);
  }
  ~DelayedEntityCreationChunk()
  {
    for (auto i = begin(), e = end(); i != e; ++i)
      i->~DelayedEntityCreation();
    memfree(queue, defaultmem);
  }
  DelayedEntityCreationChunk(const DelayedEntityCreationChunk &) = delete;
  DelayedEntityCreationChunk &operator=(const DelayedEntityCreationChunk &) = delete;
  DelayedEntityCreationChunk(DelayedEntityCreationChunk &&a)
  {
    memcpy(this, &a, sizeof(DelayedEntityCreationChunk));
    memset(&a, 0, sizeof(DelayedEntityCreationChunk));
  }
  DelayedEntityCreationChunk &operator=(DelayedEntityCreationChunk &&a)
  {
    alignas(DelayedEntityCreationChunk) char buf[sizeof(DelayedEntityCreationChunk)]; // swap
    memcpy(buf, this, sizeof(DelayedEntityCreationChunk));
    memcpy(this, &a, sizeof(DelayedEntityCreationChunk));
    memcpy(&a, buf, sizeof(DelayedEntityCreationChunk));
    return *this;
  }
  DelayedEntityCreation *erase(DelayedEntityCreation *pos)
  {
    DelayedEntityCreation *end_ = end();
    G_ASSERTF(pos >= begin() && pos < end_, "%p %p %d %d", queue, pos, readFrom, writeTo);
    if ((pos + 1) < end_)
      eastl::move(pos + 1, end_, pos);
    --writeTo;
    (end_ - 1)->~DelayedEntityCreation();
    return pos;
  }
  bool empty() const { return readFrom == writeTo; }
  bool full() const { return writeTo == capacity; }
  uint16_t size() const { return writeTo - readFrom; }
  uint16_t next_capacity() const { return min(capacity * 2, (int)maxChunkCapacity); }
  template <typename... Args>
  bool emplace_back(EntityId eid, Args &&...args)
  {
    DAECS_EXT_ASSERT(!full());
    new (queue + (writeTo++), _NEW_INPLACE) DelayedEntityCreation(eid, eastl::forward<Args>(args)...);
    return full();
  }
  void reset() { readFrom = writeTo = 0; }
  const DelayedEntityCreation *end() const { return queue + writeTo; }
  const DelayedEntityCreation *begin() const { return queue + readFrom; }
  DelayedEntityCreation *end() { return queue + writeTo; }
  DelayedEntityCreation *begin() { return queue + readFrom; }
};
// we will always insert in top chunk, which is just always delayedCreationQueue.back();
// which is never empty (i.e. topChunk != nullptr)
dag::Vector<DelayedEntityCreationChunk> delayedCreationQueue;
enum
{
  INVALID_CREATION_QUEUE_GEN = 0,
  INITIAL_CREATION_QUEUE_GEN = 1
};
uint32_t createOrDestroyGen = INITIAL_CREATION_QUEUE_GEN + 1;
uint32_t lastUpdatedCreationQueueGen = INITIAL_CREATION_QUEUE_GEN;
bool someLoadedEntitiesHasErrors = false;

template <typename... Args>
void emplaceCreateInt(EntityId eid, Args &&...args)
{
  DAECS_EXT_FAST_ASSERT(eid);
  DAECS_EXT_FAST_ASSERT(!delayedCreationQueue.empty());

#if DAGOR_THREAD_SANITIZER
  // why it is OK to shut tsan like that (but not in release mode)?
  //  the reason is simple. regardless of race in creation, if we are in constrainedMode,
  //   createOrDestroyGen is not ever READ, or WRITTEN, as isDeferredCreationPossible() == false
  //   so, this line is the only line that actually changes value of memory
  //  now, in any memory model, regardless of how many threads are simultaneously perform value++, value will CHANGE at least ONCE
  //  since we only use it to compare to lastUpdatedCreationQueueGen,
  //  which is only upated when isDeferredCreationPossible() == true to latest value of createOrDestroyGen
  //   we don't care if counter is actually representing amount of emplaceCreateInt called.
  //  it is good enough that createOrDestroyGen != lastUpdatedCreationQueueGen
  interlocked_increment(createOrDestroyGen);
#else
  createOrDestroyGen++;
#endif
  if (delayedCreationQueue.back().emplace_back(eid, eastl::forward<Args>(args)...))
    delayedCreationQueue.emplace_back(delayedCreationQueue.back().next_capacity());
}

template <typename... Args>
void emplaceCreate(EntityId eid, Args &&...args)
{
  emplaceCreateInt(eid, eastl::forward<Args>(args)...);
  entDescs.increaseCreating(eid.index());
}
void emplaceDestroy(ecs::EntityId eid) { emplaceCreateInt(eid); }
void initializeCreationQueue()
{
  DAECS_EXT_FAST_ASSERT(delayedCreationQueue.empty());
  delayedCreationQueue.emplace_back(+DelayedEntityCreationChunk::minChunkCapacity);
}
void clearCreationQueue()
{
  delayedCreationQueue.clear();
  initializeCreationQueue();
  lastUpdatedCreationQueueGen = INITIAL_CREATION_QUEUE_GEN;
  createOrDestroyGen = INITIAL_CREATION_QUEUE_GEN;
}
bool createQueuedEntitiesOOL(); // return if new entites were added during creation
inline bool hasQueuedEntitiesCreation()
{
  // first comparison is 10x time faster than interlocked_acquire_load, so this order gives us performance
  // there is no actual race here!
  return lastUpdatedCreationQueueGen != interlocked_relaxed_load(createOrDestroyGen) && isDeferredCreationPossible();
}
inline bool createQueuedEntities()
{
  return hasQueuedEntitiesCreation() ? createQueuedEntitiesOOL() : false;
} // return if new entites were added during creation

TemplateDB templateDB;

template <class T, bool optional>
void setComponentInternal(EntityId eid, const HashedConstString name, const T &v);
template <typename T, bool optional>
typename eastl::enable_if<eastl::is_rvalue_reference<T &&>::value>::type setComponentInternal(EntityId eid,
  const HashedConstString name, T &&v);
template <bool optional>
void setComponentInternal(EntityId eid, const HashedConstString name, ChildComponent &&); // not checked for non-pod types. Won't
                                                                                          // assert if missing. named for ecs20
                                                                                          // compatibility

void allocateInvalid();
uint32_t defragmentArchetypeId = 0;
void validateQuery(uint32_t q);

uint32_t allQueriesUpdatedToArch = 0;
uint32_t lastQueriesResolvedComponents = 0;
void updateAllQueriesInternal();
bool updateAllQueriesAnyMT()
{
  if (DAGOR_LIKELY(allQueriesUpdatedToArch == archetypes.size()))
    return false;
  updateAllQueriesInternal();
  return true;
}
void updateAllQueries()
{
  if (DAGOR_UNLIKELY(!isConstrainedMTMode()))
    updateAllQueriesAnyMT();
}

void maintainQuery(uint32_t q);
uint32_t queryToCheck = 0;
void maintainQueries() { maintainQuery(queryToCheck++); }
void defragmentArchetypes();
template <typename Fn>
friend bool perform_query(EntityManager *, ecs::EntityId eid, QueryId, Fn &&, void *a);
template <typename Fn>
friend bool perform_query(EntityManager *, ecs::EntityId eid, QueryId, Fn &&);
friend QueryCbResult perform_query(EntityManager *, const NamedQueryDesc &, const stoppable_query_cb_t &, void *);
friend QueryCbResult perform_query(EntityManager *, QueryId, const stoppable_query_cb_t &, void *);
friend void perform_query(EntityManager *, const NamedQueryDesc &, const query_cb_t &, void *, int);
friend void perform_query(EntityManager *, QueryId, const query_cb_t &, void *, int);

void accessError(EntityId eid, const HashedConstString name) const;
void accessError(EntityId eid, component_index_t cidx, const LTComponentList *list) const;
mutable int errorCount = 0;
EventsDB eventDb;

friend class ResourceRequestCb;
struct CurrentlyRequesting
{
  EntityId eid;
  template_t newTemplate;
  archetype_t oldArchetype;
  archetype_t newArchetype;
  const ComponentsInitializer &initializer;
  CurrentlyRequesting(EntityId eid_, template_t newTemplate_, archetype_t oldArch, archetype_t newArch,
    const ComponentsInitializer &i) :
    initializer(i), eid(eid_), newTemplate(newTemplate_), newArchetype(newArch), oldArchetype(oldArch)
  {}
};
CurrentlyRequesting *requestingTop = nullptr; // pointer to stack variable
template <class T>
const T *getRequestingBase(const HashedConstString name) const;
template <class T>
const T &getRequesting(const HashedConstString hashed_name) const;
template <class T>
const T &getRequesting(const HashedConstString hashed_name, const T &def) const;

int getNestedQuery() const { return nestedQuery; }
void setNestedQuery(int value) { nestedQuery = value; }
friend struct ecs::NestedQueryRestorer;
void schedule_tracked_changes(const ScheduledArchetypeComponentTrack *__restrict trackedI, uint32_t trackedChangesCount, EntityId eid,
  uint32_t archetype);

void *userData = nullptr;
int64_t ownerThreadId = 0;

MTLinkedList<QueryStackData> queryStack;