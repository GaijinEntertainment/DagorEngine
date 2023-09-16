#include <EASTL/utility.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <atomic>
#include <util/dag_threadPool.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <daECS/core/ecsQuery.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <daECS/core/internal/trackComponentAccess.h>
#include <math/dag_bits.h>
#include "ecsPerformQueryInline.h"

// todo: codegen should actually sort components by names. That would increase aliasing. Same for scripts.
static constexpr int max_query_components_count = 256;

#define VALIDATE_QUERY_THOROUGHLY (DAGOR_DBGLEVEL > 1) // very expensive check
namespace ecs
{
template <>
MTLinkedList<QueryStackData>::Node *MTLinkedList<QueryStackData>::head = nullptr;
template <>
thread_local MTLinkedList<QueryStackData>::Node *MTLinkedList<QueryStackData>::thread_data = nullptr;

G_STATIC_ASSERT(sizeof(ArchetypesQuery) <= 64);
static constexpr int MAX_ES_JOBS = MAX_POSSIBLE_WORKERS_COUNT;

// all JobInfo fits in one cache line, if there is only one chunk
struct JobInfo //-V730
{
  std::atomic<int> work_started = {0};
  uint32_t totalSize = 0;
  EntityManager &mgr;
  const Query &__restrict query;
  const query_cb_t &__restrict fun;
  void *__restrict userData;
  int quant;
  eastl::fixed_vector<uint32_t, 32, true, framemem_allocator> starts;
  JobInfo(EntityManager &mgr, const Query &__restrict query, const query_cb_t &__restrict fun, void *__restrict userData, int quant) :
    mgr(mgr), query(query), fun(fun), userData(userData), quant(quant)
  {}
};

#if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101400
class ESJob final : public cpujobs::IJob // workaround for 'error: aligned deallocation function of type 'void (void *,
                                         // std::align_val_t) noexcept' is only available on macOS 10.14 or newer'
#else
class alignas(128) ESJob final : public cpujobs::IJob // align on 2 cache lines to avoid false sharing on setting 'done' within
                                                      // threadpool
#endif
{
  JobInfo *__restrict parent = 0; // same lifetime as thread
  uint32_t workerId = 0;

public:
  ESJob(JobInfo *parent_, uint32_t id) : parent(parent_), workerId(id) {}

  static inline void perform_fun( // ESJob *job,
    JobInfo *__restrict info, const uint32_t *__restrict startsPtr, uint32_t worker_id)
  {
    // debug("start job %d us", get_time_usec(baset));
    const uint32_t totalSize = info->totalSize;
    if (info->work_started.load(std::memory_order_relaxed) >= totalSize)
      return;
    TIME_PROFILE_DEV(perform_fun);
    // QueryView jobView = query.getView(user_data, chunk);//still can happen that query is already dead, if all work is done!!

    int currentChunk = 0;
    for (;;)
    {
      const int minQuant = info->quant;
      int totalStart = info->work_started.fetch_add(minQuant);
      int totalCount = min<int>(totalSize - totalStart, minQuant);
      // we can use query Only after this line, so we won't start working on already dead pointer.
      //  if info->work_finished became zero, work is finished, and job won't be 'waited' upon, code will leave, &query will become
      //  dead, but thread can just 'sleep' that can happen only before this line (or after info->work_finished.fetch_sub)
      // thread will be 'finalized' sometime.
      //'info', unlike 'query', have SAME lifetime as thread
      if (totalCount <= 0)
        return;
      // if we are here, query is valid pointer
      // debug("%d:we start at %d do %d out of %d",threadId, totalStart, totalCount, query.totalSize);
      const uint32_t *__restrict starts = startsPtr + currentChunk;
      for (; totalStart >= starts[1]; currentChunk++, starts++) // we first need to find chunk where unfinished work starts
        ;
      // debug("%d:current chunk is %d, which starts at %d has size of %d",threadId, currentChunk, *starts, starts[1]-starts[0]);

      const Query &__restrict query = info->query;
      G_FAST_ASSERT(currentChunk < query.chunksCount());
      QueryView jobView = query.getView(info->mgr, info->userData, 0, 0, 0, worker_id);
      jobView.componentData = query.getChunkData(currentChunk);
      for (int workLeft = totalCount; workLeft > 0;)
      {
        const int chunkStartsAt = *starts;
        const int chunkStart = totalStart - chunkStartsAt;
        const int chunkWork = min((int)(starts[1]) - totalStart, workLeft);
        // debug("%d:perform on chunk %d c_i %d c_size %d",threadId, currentChunk, chunkStart, chunkWork);

        G_FAST_ASSERT(currentChunk < query.chunksCount() &&
                      uint32_t(chunkStart) + uint32_t(chunkWork) <= info->query.chunkEntitiesCnt[currentChunk]);
        jobView.chunkEntitiesStart = chunkStart;
        jobView.chunkEntitiesEnd = chunkStart + chunkWork;
        info->fun(jobView);
        if (chunkWork != workLeft) // work goes to next chunk, overlap
        {
          ++starts;
          ++currentChunk; // will be done anyway on next loop, only needed for G_FAST_ASSERT to be correct
          jobView.componentData = info->query.getChunkData(currentChunk);
        }
        workLeft -= chunkWork;
        totalStart += chunkWork;
      }
    }
  }
  static uint32_t getChunkSize(const Query &q, int ci) { return q.chunkEntitiesCnt[ci]; }

  virtual void doJob() override { perform_fun(parent, parent->starts.data(), workerId); }
};

static constexpr int num_jobs_to_wake_up_all = 2; // it is faster to wake up 2 thread workers one-by-one, than wake up all

static __forceinline void parallel_for(int num_jobs, EntityManager &mgr, const query_cb_t &fun, const Query &pQuery, void *user_data,
  int min_quant, threadpool::JobPriority tpprio)
{
  // debug("start working on %p[%d] == %d(%d)", &pQuery, chunk, cnt);
  alignas(ESJob) char jobStorage[MAX_ES_JOBS * sizeof(ESJob)];
  ESJob *jobs = (ESJob *)jobStorage;
  JobInfo info(mgr, pQuery, fun, user_data, min_quant);
  TIME_PROFILE(ecs_parallel_for_query);
  const uint32_t chunksCount = pQuery.chunksCount();
  info.starts.resize(chunksCount + 1);
  uint32_t querySize = 0;
  for (uint32_t ci = 0; ci < chunksCount; ++ci)
  {
    info.starts[ci] = querySize;
    querySize += ESJob::getChunkSize(pQuery, ci);
  }
  info.starts[chunksCount] = info.totalSize = querySize; // we have to copy querySize to info.totalSize. &query will die before thread
                                                         // exits

  num_jobs = min(num_jobs, (int)MAX_ES_JOBS);
  // debug("num_jobs = %d", num_jobs);
  uint32_t queue_pos = 0;
  auto wakeOnAdd = num_jobs <= num_jobs_to_wake_up_all ? threadpool::AddFlags::WakeOnAdd : threadpool::AddFlags::None;
  {
    // TIME_PROFILE(add);
    for (int i = 0; i < num_jobs; ++i)
    {
      ESJob *j = new (jobs + i, _NEW_INPLACE) ESJob(&info, i + 1);
      threadpool::add(j, tpprio, queue_pos, wakeOnAdd | threadpool::AddFlags::IgnoreNotDone); // job->done == 1 for sure
    }
  }
  if (wakeOnAdd == threadpool::AddFlags::None)
  {
    // TIME_PROFILE(wake);
    threadpool::wake_up_all();
  }
  // debug("jobs added start is %d (%d)",
  //   info.work_started.load(std::memory_order_acquire), cnt);
  {
    // TIME_PROFILE(active_wait);
    ESJob::perform_fun(&info, info.starts.data(), 0);
  }
  // todo: we can use just active wait, instead of barrier active wait, if we are running from non main thread
  // it reflects the following assumption, when we run from main thread our primary goal is to reduce latency, if we run from other
  // threads - our goal is to increase occupancy
  threadpool::barrier_active_wait_for_job(&jobs[num_jobs - 1], tpprio, queue_pos); // make sure we have captured all jobs
  // debug("all work scheduled %d done = (%d)",
  //   info.work_started.load(std::memory_order_acquire), cnt);
  TIME_PROFILE_WAIT_DEV("ecs_pfor_wait");
  for (int i = 0; i < num_jobs; ++i) // ensure all other jobs are also finished
    threadpool::wait(&jobs[i]);
}

void EntityManager::setMaxUpdateJobs(uint32_t num_jobs) { maxNumJobsSet = min(num_jobs, (uint32_t)MAX_ES_JOBS); }

void EntityManager::updateCurrentUpdateMaxJobs()
{
  maxNumJobs = min((uint32_t)max(int(threadpool::get_num_workers()), int(0)), maxNumJobsSet);
}

bool EntityManager::performMTQuery(const Query &pQuery, const query_cb_t &fun, void *user_data, int min_quant)
{
  const bool isMainThread = is_main_thread();
  const int numJobs = min<int>(maxNumJobs - (isMainThread ? 0 : 1), // actually we want to check if we are running from thread pool
                                                                    // worker thread!
                                                                    // but if we are running from thread, than it is likely we are in
                                                                    // worker thread
    (min_quant > 1 ? int(pQuery.totalSize + min_quant - 1) / min_quant : int(pQuery.totalSize)) - 1); // there is no point in starting
                                                                                                      // more jobs than work for them,
                                                                                                      // plus we also have main thread
  if (numJobs <= 0)
    return false;
  setConstrainedMTMode(true);
  // debug("start for %d jobs %d quant do: %d", numJobs, min_quant, pQuery.totalSize);
  threadpool::JobPriority tpprio = isMainThread ? threadpool::PRIO_HIGH : threadpool::PRIO_NORMAL; // Note: this probably need to be
                                                                                                   // configured per ES
  parallel_for(numJobs, *this, fun, pQuery, user_data, min_quant, tpprio);
  // debug("done");
  setConstrainedMTMode(false);
  return true;
}
}; // namespace ecs

namespace ecs
{

bool EntityManager::resolvePersistentQueryInternal(uint32_t index)
{
  const ResolvedStatus ret = resolveQuery(index, getQueryStatus(index), resolvedQueries[index]);
  orQueryStatus(index, ret);
  DEBUG_VERY_VERBOSE_QUERY("set resolved query <%s> to %d", queryDescs[index].getName(), isResolved(index));
  return ret != NOT_RESOLVED;
}

EntityManager::ResolvedStatus EntityManager::resolveQuery(uint32_t index, ResolvedStatus currentStatus, ResolvedQueryDesc &resDesc)
{
  const CopyQueryDesc &copyDesc = queryDescs[index];
  const char *name = copyDesc.getName();
  const BaseQueryDesc desc = copyDesc.getDesc();
  // TIME_PROFILE_DEV(resolveQuery);
  FRAMEMEM_REGION;
  DEBUG_VERY_VERBOSE_QUERY("try resolve query <%s> %d, resolve = %d", name,
    &resDesc >= resolvedQueries.begin() && &resDesc < resolvedQueries.end() ? eastl::distance(resolvedQueries.begin(), &resDesc) : -1,
    currentStatus);
  G_UNUSED(name);
  DAECS_EXT_ASSERT(desc.componentsRQ.size() + desc.componentsNO.size() + desc.componentsRW.size() + desc.componentsRO.size() > 0);
  uint8_t ret = RESOLVED_MASK;
  int componentsIterated = 0;

  enum ReqType
  {
    DATA,
    REQUIRED,
    REQUIRED_NOT
  };
  auto makeRange = [&](dag::ConstSpan<ComponentDesc> components, ReqType req) -> bool {
    auto &dest = resDesc.getComponents();
    for (auto cdI = components.begin(), cdE = components.end(); cdI != cdE; ++cdI, ++componentsIterated)
    {
      DAECS_EXT_ASSERTF(componentsIterated < dest.size(), "%d: %d < %d", req, componentsIterated, dest.size());
      if (dest[componentsIterated] != INVALID_COMPONENT_INDEX)
      {
        DAECS_EXT_ASSERT(bool(cdI->flags & CDF_OPTIONAL) == resDesc.getOptionalMask()[componentsIterated]);
        continue;
      }
      auto &cd = *cdI;
      component_index_t id = dataComponents.findComponentId(cd.name);
      DEBUG_VERY_VERBOSE_QUERY("query <%s>, component 0x%X, type 0x%x", name, cd.name, cd.type);

      if (id == INVALID_COMPONENT_INDEX)
      {
        if ((req == REQUIRED_NOT) || (cd.flags & CDF_OPTIONAL)) // optional component can be even not registered at all
        {
          ret &= ~FULLY_RESOLVED;
          continue;
        }
        ret = NOT_RESOLVED;
        return false;
      }
      // verify type
      if (cd.type != ComponentTypeInfo<auto_type>::type && ((DAGOR_DBGLEVEL > 0) || req == DATA)) //-V560
      {
        DataComponent comp = dataComponents.getComponentById(id);
        if (comp.componentTypeName != cd.type) // cd.type == auto_type is special case, basically it is 'auto' (generic). It is legit
                                               // at least in require/require_not
        {
          logerr("component<%s> type mismatch in query <%s>, type is %s(%0xX), required in query <%s>(0x%X)",
            dataComponents.getComponentNameById(id), name, componentTypes.getTypeNameById(comp.componentType), comp.componentTypeName,
            componentTypes.findTypeName(cd.type), cd.type);
          resDesc.reset();
          // wrong query can not become correct one ever. no need to check it, just assume it is empty query
          ret = RESOLVED_MASK;
          return false;
        }
      }
#if DAECS_EXTENSIVE_CHECKS
      if (req == DATA && !(cd.flags & CDF_OPTIONAL) &&
          componentTypes.getTypeInfo(dataComponents.getComponentById(id).componentType).size == 0)
      {
        logerr("component<%s> in query <%s>, is requested as data (ro/rw), but it has zero size",
          dataComponents.getComponentNameById(id), name);
      }
#endif
      dest[componentsIterated] = id;
    }
    return true;
  };
  const bool wasResolved = isResolved(currentStatus);
  if (!wasResolved)
  {
    const uint32_t dataComponentsCount = desc.componentsRW.size() + desc.componentsRO.size();
    const uint32_t totalComponents = desc.componentsRQ.size() + desc.componentsNO.size() + dataComponentsCount;
    DAECS_EXT_ASSERT(resDesc.getComponents().size() == 0 || resDesc.getComponents().size() == totalComponents);
    resDesc.getComponents().reserve(totalComponents);
    resDesc.getComponents().assign(totalComponents, INVALID_COMPONENT_INDEX);
    resDesc.getOptionalMask().reset();
    for (uint32_t i = 0, e = dataComponentsCount; i < e; ++i)
      if (copyDesc.components[i].flags & CDF_OPTIONAL)
        resDesc.getOptionalMask().set(i);

    const uint32_t checkComponents = DAGOR_DBGLEVEL > 0 ? totalComponents : dataComponentsCount;

    // reduce finds in later stages
    for (uint32_t i = 0, e = checkComponents; i != e; ++i)
    {
      auto &cd = copyDesc.components[i];
      auto cidx = dataComponents.findComponentId(cd.name);
      if (cidx != INVALID_COMPONENT_INDEX)
      {
        DataComponent comp = dataComponents.getComponentById(cidx);

        if (comp.componentTypeName != cd.type)
        {
#if DAGOR_DBGLEVEL != 0
          if (i < dataComponentsCount || cd.type != ComponentTypeInfo<auto_type>::type)
#endif
          {
            logerr("component<%s> type mismatch in query <%s>, type is %s(%0xX), required in query <%s>(0x%X)",
              dataComponents.getComponentNameById(cidx), name, componentTypes.getTypeNameById(comp.componentType),
              comp.componentTypeName, componentTypes.findTypeName(cd.type), cd.type);
            resDesc.reset();
            // wrong query can not become correct one ever. no need to check it, just assume it is empty query
            return RESOLVED_MASK;
          }
        }
      }
      resDesc.getComponents()[i] = cidx;
    }

    for (uint32_t i = checkComponents, e = totalComponents; i < e; ++i)
      resDesc.getComponents()[i] = dataComponents.findComponentId(copyDesc.components[i].name);
    resDesc.getRwCnt() = uint8_t(desc.componentsRW.size());
    resDesc.getRoCnt() = uint8_t(desc.componentsRO.size());
    resDesc.getRqCnt() = uint8_t(desc.componentsRQ.size());
    resDesc.setRequiredComponentsCount(copyDesc.requiredSetCount);
  }
  else
  {
#if DAECS_EXTENSIVE_CHECKS
    DAECS_EXT_ASSERT(resDesc.getComponents().size() ==
                     desc.componentsRQ.size() + desc.componentsRW.size() + desc.componentsRO.size() + desc.componentsNO.size());
    DAECS_EXT_ASSERTF(resDesc.getRwCnt() == desc.componentsRW.size() && resDesc.getRoCnt() == desc.componentsRO.size() &&
                        resDesc.getRqCnt() == desc.componentsRQ.size() && resDesc.getNoCnt() == desc.componentsNO.size() &&
                        resDesc.requiredComponentsCount() == copyDesc.requiredSetCount,
      "rw %d==%d ro %d==%d rq %d==%d, no %d==%d, req_set %d==%d", resDesc.getRwCnt(), desc.componentsRW.size(), resDesc.getRoCnt(),
      desc.componentsRO.size(), resDesc.getRqCnt(), desc.componentsRQ.size(), resDesc.getNoCnt(), desc.componentsNO.size(),
      resDesc.requiredComponentsCount(), copyDesc.requiredSetCount);
#endif
  }
  // Note: order of range fill is important
  if (!makeRange(dag::ConstSpan<ComponentDesc>(copyDesc.components.data(), resDesc.getRwCnt() + resDesc.getRoCnt()), DATA))
    return (ResolvedStatus)ret;
  DAECS_EXT_ASSERTF(resDesc.getRwCnt() + resDesc.getRoCnt() == componentsIterated,
    "we rely on optionalmask to be parallel with data components");
  if (wasResolved) // if query was partially resolved, it for sure has resolved required components
    componentsIterated += desc.componentsRQ.size();
  else
  {
    if (!makeRange(desc.componentsRQ, REQUIRED))
      return (ResolvedStatus)ret;
  }
  if (!makeRange(desc.componentsNO, REQUIRED_NOT))
    return (ResolvedStatus)ret;
  DEBUG_VERY_VERBOSE_QUERY("resolved query <%s>, resolve=%d", name, currentStatus);
  return (ResolvedStatus)ret;
}

template <typename T, typename Cnt>
static T *add_to_fixed_container(T *ft, Cnt &__restrict cnt, const T *__restrict add, size_t size)
{
  if (size == 0)
    return ft;
  const size_t nextSize = cnt + size;
  DAECS_EXT_ASSERT(nextSize <= eastl::numeric_limits<Cnt>::max());
  if (!ft || !memresizeinplace_default(ft, nextSize * sizeof(T)))
  {
    T *next = (T *)memrealloc_default(ft, nextSize * sizeof(T));
    if (DAGOR_UNLIKELY(next == nullptr))
    {
      next = (T *)memalloc_default(nextSize * sizeof(T)); // so no mem setting
      memcpy(next, ft, cnt * sizeof(T));                  //-V575
      memfree_default(ft);
    }
    eastl::swap(ft, next);
  }
  memcpy(ft + cnt, add, size * sizeof(T));
  cnt = nextSize;
  return ft;
}

template <typename T, typename Cnt>
static void add_to_FixedTab(eastl::unique_ptr<T[], memfree_deleter> &ft, Cnt &cnt, const T *add, size_t size)
{
  if (size != 0)
    ft.reset(add_to_fixed_container(ft.release(), cnt, add, size));
}

bool EntityManager::makeArchetypesQuery(archetype_t first_archetype, uint32_t index, bool wasFullyResolved) //, bool
                                                                                                            // componentsCountChanged
{
  const ResolvedQueryDesc &resDesc = resolvedQueries[index];
  if (resDesc.isEmpty())
  {
    DEBUG_VERY_VERBOSE_QUERY("skipping empty query %d", index);
    return false;
  }
  ArchetypesQuery &query = archetypeQueries[index];
  ArchetypesEidQuery &queryEid = archetypeEidQueries[index];
  if (DAGOR_UNLIKELY(!wasFullyResolved))
  {
    const uint32_t totalDataComponentsCount = resDesc.getRwCnt() + resDesc.getRoCnt();
    DEBUG_VERBOSE_QUERY("copying types and sizes to archetype query %d rw = %d ro =%d", index, resDesc.rw.cnt, resDesc.ro.cnt);

    const bool validCsz = totalDataComponentsCount == query.getComponentsCount();
    // todo: lastQueriesResolvedComponents != dataComponents.size() should be passed as argument
    const bool reResolve = !validCsz || lastQueriesResolvedComponents != dataComponents.size();
    if (!validCsz) // todo : only needed for eid queries. Can be deferred explicitly marked, if eid query is needed at all
    {
      if (totalDataComponentsCount)
      {
        queryEid.componentsSizesAt = archComponentsSizeContainers.size();
        archComponentsSizeContainers.append_default(totalDataComponentsCount);
      }
    }
    if (reResolve)
    {
      query.rwCount = resDesc.getRW().cnt;
      query.roCount = resDesc.getRO().cnt;
      DAECS_EXT_ASSERT(totalDataComponentsCount <= resDesc.getComponents().size());
      DAECS_EXT_ASSERT(
        totalDataComponentsCount == 0 || queryEid.componentsSizesAt + totalDataComponentsCount <= archComponentsSizeContainers.size());
      // todo: this is only needed for eid queries
      auto componentCsz = archComponentsSizeContainers.data() + queryEid.componentsSizesAt;
      for (uint32_t compI = 0; compI < totalDataComponentsCount; ++compI, ++componentCsz)
      {
        if (validCsz && *componentCsz) // if it is already inited, why bother
          continue;
        component_index_t cidx = resDesc.getComponents()[compI];
        DataComponent dt = dataComponents.getComponentById(cidx);
        *componentCsz = componentTypes.getTypeInfo(dt.componentType).size;
      }
    }
  }
  else
  {
#if DAECS_EXTENSIVE_CHECKS
    const uint32_t totalDataComponentsCount = query.rwCount + query.roCount;
    DAECS_EXT_ASSERT(totalDataComponentsCount <= resDesc.getComponents().size());
    DAECS_EXT_ASSERTF(resDesc.getRW().start == 0 && query.rwCount == resDesc.getRW().cnt && query.roCount == resDesc.getRO().cnt,
      "q %d rw %d == %d ro %d == %d, tp = %p", index, query.rwCount, resDesc.getRW().cnt, query.roCount, resDesc.getRO().cnt);
    DAECS_EXT_ASSERT(
      totalDataComponentsCount == 0 || queryEid.componentsSizesAt + totalDataComponentsCount <= archComponentsSizeContainers.size());
    auto componentCsz = archComponentsSizeContainers.data() + queryEid.componentsSizesAt;
    for (uint32_t compI = 0; compI < totalDataComponentsCount; ++compI)
    {
      component_index_t cidx = resDesc.getComponents()[compI];
      DataComponent dt = dataComponents.getComponentById(cidx);
      DAECS_EXT_ASSERT(componentCsz[compI] == componentTypes.getTypeInfo(dt.componentType).size);
    }
#endif
  }
  const uint32_t archetypesCount = archetypes.size();
  const uint32_t minRequired = resDesc.requiredComponentsCount();
  const bool updateOneArchetype = first_archetype == archetypesCount - 1;

  if (updateOneArchetype) // saves 5% of framemem_region
  {
    if (archetypes.getArchetype(first_archetype).getComponentsCount() < minRequired) // not enough components to even match
    {
      return archetypeQueries[index].getArchetypesRelated() != 0;
    }
  }
  // TIME_PROFILE_DEV(makeArchetypesQuery);
  FRAMEMEM_REGION;
  // G_ASSERT_RETURN(resDesc.components.size() == 0, false);//not sure if it is needed
  DAECS_EXT_ASSERTF_RETURN(resDesc.getRO().start == resDesc.getRW().start + resDesc.getRW().cnt, false, "%d %d %d",
    resDesc.getRO().start, resDesc.getRW().start, resDesc.getRW().cnt);
  // DAECS_EXT_ASSERT(query.lastArchetypesGeneration != archetypes.generation() || archetypes.generation() == 0);
  const uint32_t totalDataComponentsCount = query.getComponentsCount();
  DAECS_EXT_ASSERT(resDesc.getRwCnt() + resDesc.getRoCnt() + resDesc.getRqCnt() <= resDesc.getComponents().size());

  // this is potentially slow, as it depends on amount of archetypes
  // for persistent queries doesn't matter (won't be re-calculated until new archetype added)
  // but we can speed that up, if we will have list/range of archetype for each components, and iterate over smallest list/range from
  // all archetype components there are much less components, than archetypes, todo: make this optimization if ever slow

  DAECS_EXT_ASSERT(archetypes.generation() == archetypes.size()); // todo: otherwise we can't incrementally update queries

  eastl::fixed_vector<archetype_t, 4, true, framemem_allocator> queries;
  eastl::fixed_vector<ArchetypesQuery::offset_type_t, 16, true, framemem_allocator> allComponentsArchOffsets;

  DEBUG_VERBOSE_QUERY("adding %d archetypes to query %d", archetypesCount - first_archetype,
    &query >= archetypeQueries.begin() && &query < archetypeQueries.end() ? &query - archetypeQueries.begin() : -1);

  // we don't want to add same archetypes to query list
  // chose max between next after last already found (query.queriesEnd()[-1] + 1) and first_archetype to add
  archetype_t firstArchetypeToAdd =
    !updateOneArchetype && query.queriesCount > 0 ? max(archetype_t(query.queriesEnd()[-1] + 1), first_archetype) : first_archetype;
  for (int ai = firstArchetypeToAdd, ae = archetypesCount; ai < ae; ++ai)
  {
    const auto &archetype = archetypes.getArchetype(ai);
    const auto componentCount = archetype.getComponentsCount();
    if (componentCount < minRequired) // not enough components to even match
      continue;
    const auto &archInfo = archetypes.getArchetypeInfoUnsafe(ai);
    archetype_component_id tempIds[max_query_components_count];

    if (resDesc.getRqCnt())
    {
      for (uint32_t i = resDesc.getRQ().start, ei = i + resDesc.getRqCnt(); i < ei; ++i)
        if (archInfo.getComponentId(resDesc.getComponents()[i]) == INVALID_ARCHETYPE_COMPONENT_ID)
          goto loop_continue; // legal continue outer loop
    }
    if (resDesc.getNoCnt())
    {
      for (uint32_t i = resDesc.getNO().start, ei = resDesc.getComponents().size(); i < ei; ++i)
        if (archInfo.getComponentId(resDesc.getComponents()[i]) != INVALID_ARCHETYPE_COMPONENT_ID)
          goto loop_continue; // legal continue outer loop
    }
    for (uint32_t i = 0; i != totalDataComponentsCount; ++i)
    {
      archetype_component_id id = archInfo.getComponentId(resDesc.getComponents()[i]);
      if (id == INVALID_ARCHETYPE_COMPONENT_ID)
      {
        if (!resDesc.getOptionalMask()[i]) // required data is not optional
          goto loop_continue;              // legal continue outer loop
      }
      tempIds[i] = id;
    }
    goto loop_normal;
  loop_continue:
    continue;
  loop_normal:;
    // we have found acceptable archetype
    const uint32_t oldOffsets = allComponentsArchOffsets.size();

    allComponentsArchOffsets.resize(oldOffsets + totalDataComponentsCount);
    archetype_component_id *__restrict id = tempIds;
    auto archOffsetsOfs = archetypes.getArchetypeComponentOfsUnsafe(ai);
    for (auto oi = allComponentsArchOffsets.data() + oldOffsets, oe = oi + totalDataComponentsCount; oi != oe; ++oi, ++id)
      *oi = *id == INVALID_ARCHETYPE_COMPONENT_ID ? ArchetypesQuery::INVALID_OFFSET
                                                  : archetypes.getComponentOfsFromOfs(*id, archOffsetsOfs);
    queries.push_back(ai);
  }

  DAECS_EXT_ASSERT(resDesc.getRO().start == resDesc.getRW().start + resDesc.getRW().cnt);
  // G_ASSERT(query.componentsCount == totalDataComponentsCount || !query.componentsType);

  if (!queries.size())
    return query.getArchetypesRelated() != 0;
  DAECS_EXT_ASSERTF_RETURN(totalDataComponentsCount == resDesc.getRO().cnt + resDesc.getRW().cnt, false, "%d",
    totalDataComponentsCount);

  const size_t oldQueriesCount = query.getQueriesCount(), newQueriesCount = oldQueriesCount + queries.size();
  size_t oldOfsCount = oldQueriesCount * totalDataComponentsCount, newOfsCount = newQueriesCount * totalDataComponentsCount;

  if (query.isInplaceOffsets(newOfsCount))
  {
    // we still fit inside inplace array
    memcpy(query.getArchetypeOffsetsPtrInplace() + oldOfsCount, allComponentsArchOffsets.data(),
      sizeof(ArchetypesQuery::offset_type_t) * allComponentsArchOffsets.size());
  }
  else
  {
    if (query.isInplaceOffsets(oldOfsCount))
    {
      // convert offsets into allocated array, we are not fitting anymore inside inplace array
      int cnt = 0;
      query.allComponentsArchOffsets =
        add_to_fixed_container((ArchetypesQuery::offset_type_t *)nullptr, cnt, query.getArchetypeOffsetsPtrInplace(), oldOfsCount);
    }

    query.allComponentsArchOffsets = add_to_fixed_container(query.allComponentsArchOffsets, oldOfsCount,
      allComponentsArchOffsets.data(), allComponentsArchOffsets.size());
  }

  if (query.isInplaceQueries(newQueriesCount))
  {
    // we still fit inside inplace array
    memcpy(query.queriesInplace() + oldQueriesCount, queries.data(), sizeof(archetype_t) * queries.size());
    query.queriesCount = uint16_t(query.queriesCount + queries.size());
  }
  else
  {

    if (query.isInplaceQueries())
    {
      // convert queries into allocated array, we are not fitting anymore inside inplace array
      int cnt = 0;
      query.queries = add_to_fixed_container((archetype_t *)nullptr, cnt, query.queriesInplace(), oldQueriesCount);
    }
    query.queries = add_to_fixed_container(query.queries, query.queriesCount, queries.data(), queries.size());
  }

  if (oldQueriesCount == 0)
    query.firstArch = *query.queriesBegin();

  G_ASSERT(query.getQueriesCount() < eastl::numeric_limits<archetype_t>::max());

  // todo: only needed for eid queries. Not all queries are used in eid queries!
  const uint32_t newSubQueriesCount = uint32_t(query.queriesBegin()[newQueriesCount - 1] - query.firstArch) + 1;
  const uint32_t oldSubQueriesCount = oldQueriesCount ? uint32_t(query.queriesBegin()[oldQueriesCount - 1] - query.firstArch) + 1 : 0;
  query.archSubQueriesCount = newSubQueriesCount;

  archSubQueriesWasted += oldSubQueriesCount;
  const uint32_t archSubQueriesAtNew = archSubQueriesContainer.size();
  archSubQueriesContainer.append_default(newSubQueriesCount);
  ;
  auto archSubQueriesNew = archSubQueriesContainer.data() + archSubQueriesAtNew;
  memcpy(archSubQueriesNew, archSubQueriesContainer.data() + queryEid.archSubQueriesAt, oldSubQueriesCount * sizeof(archetype_t));
  memset(archSubQueriesNew + oldSubQueriesCount, 0xFF, (newSubQueriesCount - oldSubQueriesCount) * sizeof(archetype_t));
  for (size_t i = oldQueriesCount, e = query.getQueriesCount(); i != e; ++i)
    archSubQueriesNew[query.queriesBegin()[i] - query.firstArch] = i;
  queryEid.archSubQueriesAt = archSubQueriesAtNew;

  if (query.rwCount && query.getArchetypesRelated()) // query write something
  {
    eastl::fixed_vector<eastl::pair<component_index_t, component_index_t>, 8, true, framemem_allocator> trackedRw;
    for (int i = 0, rwE = resDesc.getRW().cnt; i < rwE; ++i)
    {
      component_index_t cidx = resDesc.getComponents()[resDesc.getRW().start + i];
      component_index_t oldCidx = dataComponents.getTrackedPair(cidx);
      if (oldCidx == INVALID_COMPONENT_INDEX) // optimization, not all dataComponents are ever tracked in any archetype
        continue;
      trackedRw.emplace_back(cidx, oldCidx);
    }
    if (trackedRw.size())
    {
      eastl::fixed_vector<ScheduledArchetypeComponentTrack, 8, true, framemem_allocator> trackedChanges;
      // everything earlier than that, is already scheduled in previous updates
      for (auto aqi = query.queriesBegin() + oldQueriesCount, aqe = query.queriesBegin() + query.getQueriesCount(); aqi != aqe; ++aqi)
      {
        const auto aq = *aqi;
        for (auto trackedPair : trackedRw)
        {
          component_index_t cidx = trackedPair.first;
          component_index_t oldCidx = trackedPair.second;
          DAECS_EXT_ASSERT(aq >= first_archetype);
          auto oldComponentInArchetypeIndex = archetypes.getArchetypeComponentIdUnsafe(aq, oldCidx);
          if (oldComponentInArchetypeIndex == INVALID_ARCHETYPE_COMPONENT_ID)
            continue;
          DAECS_EXT_ASSERT(archetypes.getArchetypeComponentIdUnsafe(aq, cidx) != INVALID_ARCHETYPE_COMPONENT_ID);

          trackedChanges.emplace_back(ScheduledArchetypeComponentTrack{aq, cidx});
        }
        // trackChangedArchetype(aq.archetype, cidx, oldCidx);
      }
      add_to_FixedTab(query.trackedChanges, query.trackedChangesCount, trackedChanges.data(), trackedChanges.size());
    }
  }

  return query.getArchetypesRelated() != 0;
}

const component_index_t *EntityManager::queryComponents(QueryId qid) const
{
  if (!isQueryValid(qid))
    return nullptr;
  uint32_t index = qid.index();
  const ResolvedQueryDesc &resDesc = resolvedQueries[index];
  return resDesc.getComponents().data() + resDesc.getRW().start;
}

bool EntityManager::makeArchetypesQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve)
{
  const ResolvedStatus status = getQueryStatus(index);

  if (DAGOR_UNLIKELY(!isFullyResolved(status)))
  {
    DEBUG_VERBOSE_QUERY("updating = %s %d, as not fully resolved", queryDescs[index].getName(), index);
    if (should_re_resolve)
    {
      if (!resolvePersistentQueryInternal(index))
      {
        DEBUG_VERBOSE_QUERY("update failed = %s", queryDescs[index].getName());
        return false;
      }
    }
    else if (!isResolved(status)) // not resolved and can't be resolved now. no need to check anything
      return false;
  }
  return makeArchetypesQuery(last_arch_count, index, isFullyResolved(status));
}

void EntityManager::queriesShrink() {} // todo: remove garbage (call reset on queries and update hash_map)
void EntityManager::invalidatePersistentQueries()
{
  DAECS_EXT_ASSERT(!isConstrainedMTMode());
  DAECS_EXT_ASSERT(getNestedQuery() == 0);
  convertArchetypeScheduledChanges();
  // since some queries can outlive entity manager, we can't just clear arrays
  for (size_t i = 0, e = archetypeQueries.size(); i < e; ++i)
  {
    resolvedQueries[i].reset();
    archetypeQueries[i].reset();
    archetypeEidQueries[i].reset();
  }
  archSubQueriesContainer.clear();
  archSubQueriesWasted = 0;
  archComponentsSizeContainers.clear();
  memset(resolvedQueryStatus.data(), 0, resolvedQueryStatus.capacity() * sizeof(resolvedQueryStatus[0]));
  for (auto &vl : archetypesES)
    vl.clear();
  for (auto &vl : archetypesRecreateES)
    vl.clear();
  allQueriesUpdatedToArch = 0;
  lastQueriesResolvedComponents = 0;
}
void EntityManager::clearQueries()
{
  currentQueryGen++;
  invalidatePersistentQueries();
  MTLinkedList<QueryStackData>::free_all();
}

uint32_t EntityManager::addOneQuery()
{
  // todo: may be keep free list instead?
  auto qi = freeQueriesCount ? eastl::find(queriesReferences.begin(), queriesReferences.end(), 0) : queriesReferences.end();
  if (qi != queriesReferences.end())
  {
    freeQueriesCount--;
    *qi = 1;
    uint32_t index = eastl::distance(queriesReferences.begin(), qi);
    archetypeQueries[index].reset();
    archetypeEidQueries[index].reset();
    resolvedQueries[index].reset();
    resetQueryStatus(index);
    queryDescs[index].clear();
    return index;
  }
  archetypeEidQueries.emplace_back();
  archetypeQueries.emplace_back();
  queryDescs.emplace_back();
  DAECS_EXT_ASSERT(archetypeQueries.size() == queryDescs.size());
  resolvedQueries.emplace_back();
  DAECS_EXT_ASSERT(archetypeQueries.size() == resolvedQueries.size());
  addOneResolvedQueryStatus();

  queriesReferences.push_back(1);
  DAECS_EXT_ASSERT(archetypeQueries.size() == queriesReferences.size());
  queriesGenerations.push_back(currentQueryGen);
  DAECS_EXT_ASSERT(archetypeQueries.size() == queriesGenerations.size());
  return archetypeQueries.size() - 1;
}

inline EntityManager::query_components_hash EntityManager::query_components_hash_calc(const BaseQueryDesc &desc)
{
  struct
  {
    uint8_t ro, rw, rq, no;
  } details;
  details.rw = desc.componentsRW.size();
  details.ro = desc.componentsRO.size();
  details.rq = desc.componentsRQ.size();
  details.no = desc.componentsNO.size();
  query_components_hash componentsHash = mem_hash_fnv1<64>((const char *)&details.ro, sizeof(details));
  auto hashComponents = [&componentsHash](dag::ConstSpan<ComponentDesc> list) {
    for (const auto &c : list)
    {
      componentsHash = mem_hash_fnv1<64>((const char *)&c.name, sizeof(c.name), componentsHash);
      componentsHash = mem_hash_fnv1<64>((const char *)&c.flags, sizeof(c.flags), componentsHash);
      componentsHash = mem_hash_fnv1<64>((const char *)&c.type, sizeof(c.type), componentsHash);
    }
  };
  hashComponents(desc.componentsRW);
  hashComponents(desc.componentsRO);
  hashComponents(desc.componentsRQ);
  hashComponents(desc.componentsNO);
  return componentsHash;
}

QueryId EntityManager::createQuery(const NamedQueryDesc &desc)
{
  QueryId h = createUnresolvedQuery(desc);
  if (!h)
    return h;
  // return h;
  uint32_t index = h.index();
  if (queriesReferences[index] == 1 && resolvePersistentQueryInternal(index)) // recently added
    makeArchetypesQuery(0, index, false);
  return h;
}

inline void EntityManager::CopyQueryDesc::init(const EntityManager &mgr, const char *name_, const BaseQueryDesc &d)
{
  G_UNUSED(mgr);
  components.clear();
  components.reserve(d.componentsRW.size() + d.componentsRO.size() + d.componentsRQ.size() + d.componentsNO.size());
  G_UNUSED(name_);
  static constexpr uint32_t max_cnt = eastl::numeric_limits<decltype(rwCnt)>::max();
  rwCnt = (uint8_t)eastl::min<uint32_t>(d.componentsRW.size(), max_cnt); //-V1029
  roCnt = (uint8_t)eastl::min<uint32_t>(d.componentsRO.size(), max_cnt); //-V1029
  rqCnt = (uint8_t)eastl::min<uint32_t>(d.componentsRQ.size(), max_cnt); //-V1029
  noCnt = (uint8_t)eastl::min<uint32_t>(d.componentsNO.size(), max_cnt); //-V1029
  components.insert(components.end(), d.componentsRW.begin(), d.componentsRW.begin() + rwCnt);
  components.insert(components.end(), d.componentsRO.begin(), d.componentsRO.begin() + roCnt);
  components.insert(components.end(), d.componentsRQ.begin(), d.componentsRQ.begin() + rqCnt);
  components.insert(components.end(), d.componentsNO.begin(), d.componentsNO.begin() + noCnt);
  components.shrink_to_fit();
#if DAGOR_DBGLEVEL != 0
  if (rwCnt != d.componentsRW.size() || roCnt != d.componentsRO.size() || rqCnt != d.componentsRQ.size() ||
      noCnt != d.componentsNO.size())
    logerr("query <%s> has too many components (rw=%d,ro=%d,rq=%d,no=%d) max is %d", name_, rwCnt, roCnt, rqCnt, noCnt, max_cnt);
  auto getNameStr = [&](const ComponentDesc &cd) { return cd.nameStr; };
#else
  auto getNameStr = [&](const ComponentDesc &cd) { return mgr.getDataComponents().findComponentName(cd.name); };
#endif
  typedef eastl::fixed_vector<component_t, 64, true, framemem_allocator> fixed_vector_components_t;
  eastl::vector_set<component_t, eastl::less<component_t>, framemem_allocator, fixed_vector_components_t> requiredComponentsSet;
  for (const auto &cd : d.componentsRQ)
  {
    if (cd.flags & CDF_OPTIONAL)
      logerr("query <%s> doesn't make sense as component %s(0x%X) is both required and optional", name_, getNameStr(cd), cd.name);
    requiredComponentsSet.emplace(cd.name);
  }
  for (const auto &cd : d.componentsRQ)
  {
    if (cd.flags & CDF_OPTIONAL)
      logerr("query <%s> doesn't make sense as component %s(0x%X) is both required and optional", name_, getNameStr(cd), cd.name);
    requiredComponentsSet.emplace(cd.name);
  }
  for (const auto &cd : d.componentsRO)
    if (!(cd.flags & CDF_OPTIONAL))
      requiredComponentsSet.emplace(cd.name);
  for (const auto &cd : d.componentsRW)
    if (!(cd.flags & CDF_OPTIONAL))
      requiredComponentsSet.emplace(cd.name);
  for (const auto &cd : d.componentsNO)
  {
    if (cd.flags & CDF_OPTIONAL)
      logerr("query <%s> doesn't make sense as component %s(0x%X) is both required_not and optional", name_, getNameStr(cd), cd.name);
    if (requiredComponentsSet.find(cd.name) != requiredComponentsSet.end())
      logerr("query <%s> doesn't make sense as component %s(0x%X) is both in required and required_not lists", name_, getNameStr(cd),
        cd.name);
  }
  requiredSetCount = eastl::min<uint32_t>(MAX_OPTIONAL_PARAMETERS, eastl::min<uint32_t>(requiredComponentsSet.size(), max_cnt));
  if (requiredComponentsSet.size() >= MAX_OPTIONAL_PARAMETERS)
    logerr("too many optional data components in query <%s>. To fix, increase bitset size or add first-optional-component-no", name_);
  DAECS_EXT_ASSERT(components.size() <= USHRT_MAX);
#if DAGOR_DBGLEVEL > 0
  name = name_;
#endif
}

QueryId EntityManager::createUnresolvedQuery(const NamedQueryDesc &desc)
{
  DAECS_EXT_ASSERT(!isConstrainedMTMode());
  G_ASSERT_RETURN(desc.componentsRW.size() + desc.componentsRO.size() < max_query_components_count, QueryId());
  query_components_hash descHash = query_components_hash_calc(desc);
  auto it = queryMap.find(descHash);
  uint32_t index;
  if (it != queryMap.end() && isQueryValidGen(it->second) && queriesReferences[it->second.index()])
  {
    index = it->second.index();
#if DAECS_EXTENSIVE_CHECKS
    auto debugComponent = [&](const char *m, dag::ConstSpan<ComponentDesc> cs) {
      for (auto &c : cs)
        debug("%s type (0x%X|%s) 0x%X|%s", m, c.type, componentTypes.findTypeName(c.type), c.name, c.nameStr);
    };
    auto debugComponents = [&](query_components_hash descHash, const char *n, const BaseQueryDesc &desc) {
      debug("<%s> == %d", n, descHash);
      debugComponent("rw", desc.componentsRW);
      debugComponent("ro", desc.componentsRO);
      debugComponent("rq", desc.componentsRQ);
      debugComponent("no", desc.componentsNO);
    };
    if (!(desc == queryDescs[index].getDesc()))
    {
      debugComponents(query_components_hash_calc(queryDescs[index].getDesc()), queryDescs[index].getName(),
        queryDescs[index].getDesc());
      debugComponents(descHash, desc.name, desc);
    }
#endif

#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(desc == queryDescs[index].getDesc(), "query %s hashed aliased with different query %s!", desc.name,
      queryDescs[index].getName());
#endif
    queriesReferences[index]++;
    return it->second;
  }
  else
  {
    index = addOneQuery();
  }
  queryDescs[index].init(*this, desc.name, desc);
#if DAGOR_DBGLEVEL > 0
  for (auto &c : queryDescs[index].components)
    c.nameStr = queriesComponentsNames.getName(queriesComponentsNames.addNameId(c.nameStr));
#endif
  QueryId ret = QueryId::make(index, queriesGenerations[index]);
  queryMap[descHash] = ret;
  // if (resolvePersistentQueryInternal(index))//recently added
  //   makeArchetypesQuery(0, index, false);
  return ret;
}

void EntityManager::destroyQuery(QueryId h)
{
  DAECS_EXT_ASSERT(!isConstrainedMTMode());
  convertArchetypeScheduledChanges();
  G_ASSERT_RETURN(isQueryValid(h), );
  auto idx = h.index();
  G_ASSERT_RETURN(queriesReferences[idx] > 0, );
  if (!(--queriesReferences[idx])) // we don't reset queries, so we can resurrect 'dead' queries (with new generation)
  {
    // remove query from hashmap. otherwise it looks like false-positive hash aliasing in a certain case
    // create query of hash A to index Y
    // destroy query of hash A
    // create query of hash B to index Y
    // create query of hash A again!
    // can be avoided by storing hashes per query, but that would simplify deletion as well
    queryMap.erase(queryMap.find(query_components_hash_calc(queryDescs[idx].getDesc()))); // todo: store hashes to avoid re-calculation
                                                                                          // of hashes
    resetQueryStatus(idx);
    archSubQueriesWasted += archetypeQueries[idx].archSubQueriesCount;
    if (DAGOR_UNLIKELY(idx == queriesReferences.size() - 1))
    {
      // specially optimize create/destroy in pairs. that's happening in inspection, and pollutes queriesReferences with unused queries
      archetypeEidQueries.pop_back();
      archetypeQueries.pop_back();
      queryDescs.pop_back();
      resolvedQueries.pop_back();
      queriesReferences.pop_back();
      queriesGenerations.pop_back();
    }
    else
    {
      queriesGenerations[idx]++;
      resolvedQueries[idx].reset();
      queryDescs[idx].clear();
      archetypeQueries[idx].reset();
      archetypeEidQueries[idx].reset();
      freeQueriesCount++;
    }
  }
}


QueryCbResult EntityManager::performQueryStoppable(QueryId h, const stoppable_query_cb_t &fun, void *user_data)
{
  uint32_t index = h.index();
  DAECS_EXT_ASSERT(isQueryValid(h));
  auto &archDesc = archetypeQueries[h.index()];
  if (!archDesc.getQueriesCount())
    return QueryCbResult::Continue;
  QueryContainer ctx;
  uint32_t totalSize;
  if ((totalSize = fillQuery(archDesc, ctx)) == 0)
  {
    DEBUG_VERY_VERBOSE_QUERY("no fit <%s> out of %d archs. %d total", queryDescs[index].getName(), archDesc.getArchetypesRelated(),
      archetypes.size());
    return QueryCbResult::Continue;
  }
  auto query = commitQuery(h, ctx, archDesc, totalSize);
  ecsdebug::track_ecs_component(queryDescs[index].getDesc(), queryDescs[index].getName());
  ScopedQueryingArchetypesCheck scopedCheck(index, *this);
  return performQueryStoppable(query, fun, user_data);
}

void EntityManager::performQuery(QueryId h, const query_cb_t &fun, void *user_data, int min_quant)
{
  uint32_t index = h.index();
  DAECS_EXT_ASSERT(isQueryValid(h));
  auto &archDesc = archetypeQueries[index];

  if (!archDesc.getQueriesCount())
    return;
  QueryContainer ctx;
  uint32_t totalSize;
  if ((totalSize = fillQuery(archDesc, ctx)) == 0)
  {
    DEBUG_VERY_VERBOSE_QUERY("no fit <%s> out of %d archs. %d total", queryDescs[index].getName(), archDesc.getArchetypesRelated(),
      archetypes.size());
    return;
  }
  auto query = commitQuery(h, ctx, archDesc, totalSize);
  ecsdebug::track_ecs_component(queryDescs[index].getDesc(), queryDescs[index].getName());
  ScopedQueryingArchetypesCheck scopedCheck(index, *this);
  performQuery(query, fun, user_data, min_quant);
}

struct DataComponentsManagerAccess
{
  static __forceinline void process_eid_components_data(uint32_t totalComponentsCount,
    QueryView::OneComponentLine *__restrict *__restrict componentData,
    const ArchetypesQuery::offset_type_t *__restrict archetypeOffsets, const DataComponentManager::Chunk &__restrict chunk,
    uint32_t idInChunk, const uint16_t *__restrict componentsSize)
  {
    for (uint32_t compI = 0; compI != totalComponentsCount; ++compI, ++componentData, ++archetypeOffsets)
    {
      // debug("comp %d offs = %d", compI, archetypeOffsets[compI]);
      auto archOfs = *archetypeOffsets;
      if (EASTL_LIKELY(archOfs != ArchetypesQuery::INVALID_OFFSET))
        *componentData = (QueryView::ComponentsData)(chunk.getCompDataUnsafe(archOfs) + idInChunk * uint32_t(componentsSize[compI]));
      // we check for optional
      else
        *componentData = nullptr;
    }
  }
};


__forceinline void EntityManager::schedule_tracked_changes(const ScheduledArchetypeComponentTrack *__restrict trackedI,
  uint32_t trackedChangesCount, EntityId eid, uint32_t archetype)
{
  auto trackedE = trackedI + trackedChangesCount;
  // we can instead store array of addresses, trading memory for speed.
  // it will also be faster on all 'big' queries (more than 8 tracked components for all archs),
  // as it is just one cache miss, though for big query we will at least miss cache twice (in binary search)
  // however, it is guaranteed cache-miss for all queries (even small)
  if (EASTL_LIKELY(trackedChangesCount <= 64)) // heurestics. On smaller arrays linear search is faster then binary search
  {
    do // same as find_if but with do_while
    {
      if (trackedI->archetype >= archetype) // we wont find it, as array is sorted
        break;
    } while (++trackedI != trackedE);
  }
  else
  {
    // yet another mutable variable may be, for initial guess?
    // make it explicitly out of line?
    trackedI = eastl::lower_bound(trackedI, trackedE, archetype,
      [](const ScheduledArchetypeComponentTrack &a, uint32_t archetype) { return a.archetype < archetype; });
  }
  if (trackedI == trackedE || trackedI->archetype != archetype)
    return;

  ScopedMTMutexT<OSSpinlock> lock(isConstrainedMTMode(), eidTrackingMutex);
  do
  {
    DAECS_EXT_ASSERT(archetypes.getArchetypeComponentId(archetype, trackedI->cidx) != INVALID_ARCHETYPE_COMPONENT_ID);
    DAECS_EXT_ASSERT(archetypeTrackChangedCheck(archetype, trackedI->cidx));
    scheduleTrackChangedNoMutex(eid, trackedI->cidx);
  } while (++trackedI != trackedE && trackedI->archetype == archetype);
}

bool EntityManager::fillEidQueryView(ecs::EntityId eid, EntityDesc entDesc, QueryId h, QueryView &__restrict qv)
{
  DAECS_EXT_ASSERT(isQueryValid(h));
  const uint32_t qIndex = h.index();
  auto &__restrict archDesc = archetypeQueries[qIndex];
  if (!archDesc.archSubQueriesCount)
    return false;
  auto &__restrict archEidDesc = archetypeEidQueries[qIndex];
  auto archetype = entDesc.archetype;
  uint32_t idInChunk = entDesc.idInChunk;
  uint32_t chunkId = entDesc.chunkId;
  const uint32_t archSubQueryId = uint32_t(archetype) - uint32_t(archDesc.firstArch);
  archetype_t itId;
  if (archSubQueryId == 0) // around 2% of all queries has one archetype, so avoid cache miss in this case with a cost of branch
    itId = 0;
  else
  {
    if (archSubQueryId >= archDesc.archSubQueriesCount)
      return false;

    itId = archSubQueriesContainer[archEidDesc.archSubQueriesAt + archSubQueryId]; // cache miss
    if (itId == INVALID_ARCHETYPE)
      return false;
  }
  // up to this it is doesEsApplyToArch, and we know for sure for coreevents, that it always passes.
  // todo: split function into two, and replace core events from fillEidQueryView into two: (itId to be part of the list)
  qv.chunkEntitiesStart = 0;
  qv.chunkEntitiesEnd = 1;
  qv.roRW = archDesc.roRW;
  qv.id = h;
  const uint32_t totalComponentsCount = archDesc.getComponentsCount();
  const auto trackedChangesCount = archDesc.trackedChangesCount;
  DAECS_EXT_ASSERT(totalComponentsCount < MAX_ONE_EID_QUERY_COMPONENTS); // if ever happen, we can change that constant in header
  // const ArchetypesQuery::offset_type_t * __restrict archetypeOffsets = archDesc.getArchetypeOffsetsPtr() +
  // totalComponentsCount*itId;

  const auto &manager = archetypes.getArchetype(archetype).manager;
  DAECS_EXT_ASSERT(manager.getChunksCount() > chunkId && manager.getChunk(chunkId).getUsed() > idInChunk);

  DataComponentsManagerAccess::process_eid_components_data(totalComponentsCount,
    const_cast<QueryView::ComponentsData *>(qv.componentData), archDesc.getArchetypeOffsetsPtr() + totalComponentsCount * itId,
    manager.getChunk(chunkId), idInChunk, archComponentsSizeContainers.data() + archEidDesc.componentsSizesAt);

  if (trackedChangesCount) // todo:can be also made template. We know for issue our core events do not have RW components
    schedule_tracked_changes(archDesc.trackedBegin(), trackedChangesCount, eid, archetype);
  return true;
}

int EntityManager::getQuerySize(QueryId h)
{
  if (!isQueryValid(h))
    return -1;
  auto &archDesc = archetypeQueries[h.index()];
  if (!archDesc.getQueriesCount())
    return 0;
  QueryContainer ctx;
  return fillQuery(archDesc, ctx); // todo: we can make a more optimized implementation, as we need only count
}

void EntityManager::defragmentArchetypeSubQueries()
{
  const bool changedOnTick = archSubLastTickSize != archSubQueriesContainer.size();
  archSubLastTickSize = archSubQueriesContainer.size();
  // avoid defragmentation on same tick that caused instantiation. instantiation is already slow, so reduce spike
  const uint32_t wasteThreshold = archSubQueriesContainer.size() / (changedOnTick ? 2 : 4);
  if (archSubQueriesWasted <= wasteThreshold)
    return;
  TIME_PROFILE_DEV(defragmentArchetypeSubQueries);
  const uint32_t nextSz = archSubQueriesContainer.size() - archSubQueriesWasted;
  const uint32_t nextCapacity = (nextSz + archSubQueriesWasted * 3 / 2 + 127) & ~127;
  decltype(archSubQueriesContainer) newContainer;
  DAECS_EXT_ASSERTF(archSubQueriesWasted < archSubQueriesContainer.size(), "%d < %d", archSubQueriesWasted,
    archSubQueriesContainer.size());
  newContainer.reserve(nextCapacity);
  newContainer.resize(nextSz);
  auto dest = newContainer.data(), src = archSubQueriesContainer.data();
  for (uint32_t cat = 0, index = 0, e = queriesReferences.size(); index < e; ++index)
  {
    if (queriesReferences[index])
    {
      const uint32_t subQCnt = archetypeQueries[index].archSubQueriesCount;
      if (subQCnt)
      {
        auto &qAt = archetypeEidQueries[index].archSubQueriesAt;
        memcpy(dest + cat, src + qAt, sizeof(archetype_t) * subQCnt);
        qAt = cat;
        cat += subQCnt;
      }
    }
  }
  newContainer.swap(archSubQueriesContainer);
  archSubQueriesWasted = 0;
}

void EntityManager::updateAllQueriesInternal()
{
  TIME_PROFILE_DEV(updateAllQueries);
  DAECS_EXT_ASSERT(allQueriesUpdatedToArch < archetypes.size());
  const bool shouldResolveQueries = lastQueriesResolvedComponents != dataComponents.size();
  for (int index = 0, e = queriesReferences.size(); index < e; ++index)
  {
    if (queriesReferences[index] && updatePersistentQueryInternal(allQueriesUpdatedToArch, index, shouldResolveQueries))
    {
      if (!allow_create_sync_within_es)
      {
        auto it = queryToEsMap.find(QueryId::make(index, queriesGenerations[index]));
        if (it != queryToEsMap.end())
        {
          for (auto esIndex : it->second)
            registerEs(esIndex);
          queryToEsMap.erase(it);
        }
      }
    }
  }

  lastQueriesResolvedComponents = dataComponents.size();
  uint32_t currentArchetypeCount = archetypes.size();
  for (int ai = allQueriesUpdatedToArch, e = currentArchetypeCount; ai < e; ++ai)
    updateArchetypeESLists(ai);
  allQueriesUpdatedToArch = currentArchetypeCount;
}

void EntityManager::validateQuery(uint32_t index)
{
  G_UNUSED(index);
#if DAGOR_DBGLEVEL > 0
  index = index % queriesReferences.size();
  if (!queriesReferences[index])
    return;
  // currently all queries are up-to-date by the time validateQuery is called;
  {
    DEBUG_VERY_VERBOSE_QUERY("validate query <%s> %d", queryDescs[index].getName(), index);
    ArchetypesQuery &archDesc = archetypeQueries[index];
    G_UNUSED(archDesc);
    if (archDesc.getComponentsCount())
    {
      DAECS_EXT_ASSERT(archDesc.getAllComponentsArchOffsetsCount() % archDesc.getComponentsCount() == 0);
      DAECS_EXT_ASSERT(archDesc.getAllComponentsArchOffsetsCount() / archDesc.getComponentsCount() == archDesc.getQueriesCount());
    }
#if DAECS_EXTENSIVE_CHECKS && VALIDATE_QUERY_THOROUGHLY // very expensive check
    {
      ResolvedQueryDesc resQuery;
      ResolvedStatus status = resolveQuery(index, ResolvedStatus::NOT_RESOLVED, resQuery);
      if (isResolved(status))
      {
        ArchetypesQuery &archQuery = archetypeQueries[index];
        if (!makeArchetypesQuery(0, index, false))
        {
          G_ASSERTF(archQuery.getQueriesCount() == 0, "%s queries=%d", archQuery.getQueriesCount());
        }
        G_ASSERT(archQuery.getComponentsCount() == archDesc.getComponentsCount());
        G_ASSERT(archQuery.getQueriesCount() == archDesc.getQueriesCount());
        G_ASSERT(archQuery.getAllComponentsArchOffsetsCount() == archDesc.getAllComponentsArchOffsetsCount());
        if (archQuery.isInplaceOffsets())
        {
          G_ASSERT(memcmp(archQuery.getArchetypeOffsetsPtrInplace(), archDesc.getArchetypeOffsetsPtrInplace(),
                     archDesc.getAllComponentsArchOffsetsCount() * sizeof(ArchetypesQuery::offset_type_t)) == 0);
        }
        else
        {
          G_ASSERT(memcmp(archQuery.allComponentsArchOffsets, archDesc.allComponentsArchOffsets,
                     archDesc.getAllComponentsArchOffsetsCount() * sizeof(ArchetypesQuery::offset_type_t)) == 0);
        }
        if (memcmp(archQuery.queriesBegin(), archDesc.queriesBegin(),
              sizeof(*archQuery.queriesBegin()) * (archQuery.queriesEnd() - archQuery.queriesBegin())) != 0)
        {
          for (int i = 0; i < archQuery.getQueriesCount(); ++i)
          {
            debug("%d: archetype %d | should be arch = %d", i, archDesc.queriesBegin()[i], archQuery.queriesBegin()[i]);
          }
          G_ASSERTF(false, "broken %s queries=%d", queryDescs[index].getName(), archQuery.getQueriesCount());
        }
        G_ASSERT(archQuery.trackedChangesCount == archDesc.trackedChangesCount);
        if (memcmp(archQuery.trackedBegin(), archDesc.trackedBegin(),
              sizeof(*archDesc.trackedBegin()) * (archQuery.trackedEnd() - archQuery.trackedBegin())) != 0)
        {
          for (int i = 0; i < archQuery.trackedChangesCount; ++i)
          {
            debug("%d: archetype %d, cidx = %d | should be arch = %d, cidx = %d", i, archDesc.trackedBegin()[i].archetype,
              archDesc.trackedBegin()[i].cidx, archQuery.trackedBegin()[i].archetype, archQuery.trackedBegin()[i].cidx);
          }
          G_ASSERTF(false, "broken %s changes=%d", queryDescs[index].getName(), archQuery.trackedChangesCount);
        }
      }
      else
      {
        G_ASSERTF(!archDesc.getQueriesCount(), "query = %s arch Resolved=%d", queryDescs[index].getName(), (int)status);
      }
    }
#endif
  }
#endif
}

void EntityManager::maintainQuery(uint32_t index)
{
  if (!queriesReferences.size())
    return;
  MTLinkedList<QueryStackData>::collapse_all();
  index = index % queriesReferences.size();
  if (!queriesReferences[index])
    return;
#if DAECS_EXTENSIVE_CHECKS
  validateQuery(index);
#endif
  if (isCompletelyResolved(index)) // free memory. can actually be moved out to tick, to be optimized lazily, remove from hot path
    queryDescs[index].clear();
}

}; // namespace ecs
