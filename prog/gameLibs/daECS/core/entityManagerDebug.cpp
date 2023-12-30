#include <daECS/core/entityManager.h>
#include "ecsQueryInternal.h"

#include <daECS/core/internal/trackComponentAccess.h>

#include <osApiWrappers/dag_stackHlp.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_stlqsort.h>
#include <util/dag_string.h>

#include <EASTL/array.h>
#include <EASTL/vector_map.h>

#include <mutex>

namespace ecs
{

void EntityManager::dumpArchetypes(int max_a)
{
  struct AInfo
  {
    uint32_t count, size, ai;
  };
  eastl::vector<AInfo> archetypesInfo(archetypes.size());
  for (uint32_t i = 0; i < archetypes.size(); ++i)
    archetypesInfo[i] = AInfo{archetypes.getArchetype(i).componentsCnt, archetypes.getArchetype(i).entitySize, i};
  auto dump = [&]() {
    const int maxArch = min(max_a, (int)archetypesInfo.size());

    debug("top %d archetypes", maxArch);
    for (int ai = 0; ai < maxArch; ++ai)
    {
      AInfo &a = archetypesInfo[ai];
      int cnt = 0;
      String str;
      for (int ti = 0; ti < templates.size(); ++ti)
      {
        auto &t = templates.getTemplate(ti);
        if (t.archetype == a.ai)
        {
          ++cnt;
          str.aprintf(128, "%s<%s>", str.size() ? ", " : "", getTemplateName(ti));
        }
      }
      debug("archetype %d has entitySize = %d and %d components, used in templates<%s>", a.ai, a.size, a.count, str.str());
    }
  };
  stlsort::sort(archetypesInfo.begin(), archetypesInfo.end(), [](auto &a, auto &b) { return a.size > b.size; });
  dump();
  stlsort::sort(archetypesInfo.begin(), archetypesInfo.end(), [](auto &a, auto &b) { return a.count > b.count; });
  dump();
}

bool EntityManager::dumpArchetype(uint32_t a)
{
  if (a >= archetypes.size())
    return false;
  String str;

  for (int ti = 0; ti < templates.size(); ++ti)
  {
    auto &t = templates.getTemplate(ti);
    if (t.archetype == a)
      str.aprintf(128, "%s<%s>", str.size() ? ", " : "", getTemplateName(ti));
  }
  debug("archetype %d has entitySize = %d and %d components, used in templates<%s>", a, archetypes.getArchetype(a).componentsCnt,
    archetypes.getArchetype(a).entitySize, str.str());
  str.clear();
  for (int ci = 0; ci < archetypes.getArchetype(a).componentsCnt; ++ci)
  {
    component_index_t cidx = archetypes.getComponent(a, ci);
    str.aprintf(128, "%s%s:%s", str.size() ? "\n" : "", dataComponents.getComponentNameById(cidx),
      componentTypes.getTypeNameById(dataComponents.getComponentById(cidx).componentType));
  }
  debug("components:\n%s", str.str());
  return true;
}

size_t EntityManager::dumpMemoryUsage()
{
  size_t totalMem = 0;
  size_t entitiesMem = entDescs.capacity() * sizeof(EntityDesc) + freeIndices.size() * sizeof(uint32_t);
  size_t allocMem = creationAllocator.calcMemAllocated() + creationAllocator.chunks.capacity() * 16;
  debug("entitiesCount = %d freeIndices = %d entitiesMem = %d bytes creating allocated = %d(%d)bytes", entDescs.size(),
    freeIndices.size(), entitiesMem, creationAllocator.calcMemAllocated(), allocMem);
  totalMem += entitiesMem + allocMem;

  size_t componentsMem = dataComponents.components.capacity() *
                           (sizeof(DataComponent) + sizeof(ComponentSerializer *) + sizeof(eastl::string) + // todo: add strings
                                                                                                            // themselves
                             sizeof(uint32_t) + sizeof(component_t) + sizeof(component_index_t)) +
                         dataComponents.dependencies.capacity() * sizeof(component_t) +
                         dataComponents.componentIndex.bucket_count() *
                           sizeof(ska::detailv3::sherwood_v3_entry<eastl::pair<component_t, component_index_t>>) +
                         sizeof(DataComponents);

  totalMem += componentsMem;
  debug("components = %d mem = %d bytes", dataComponents.size(), componentsMem);
  debug("componentTypes = %d", componentTypes.getTypeCount());
  totalMem += componentTypes.getTypeCount() * (8 * 4); // some approximation. todo: calculate correctly!

  size_t templInitialData = 0, templData = 0;
  for (int i = 0; i < templates.size(); ++i)
  {
    auto &t = templates.getTemplate(i);
    templInitialData += t.alignedEntitySize;
    templData += (t.componentsCount + 7) / 8;
  }
  templData += sizeof(InstantiatedTemplate) * templates.size(); // should be capacity
  templData += sizeof(ecs::string) * templates.size();          // should be capacity
  totalMem += templData + templInitialData;
  debug("templates count = %d initial=%d totalData= %d", templates.size(), templInitialData, templData);

  size_t archData = 0, data = 0, index = 0, needData = 0, chunkData = 0, chunksCount = 0, emptyData = 0;
  for (int i = 0; i < archetypes.size(); ++i)
  {
    auto &a = archetypes.getArchetype(i);
    const Archetypes::ArchetypeInfo &ai = archetypes.getArchetypeInfoUnsafe(i);
    index += ai.count * sizeof(component_index_t);
    chunkData += a.manager.getChunksCount() * 16; // sizeof(DataComponentManager::Chunk);//should be capacity
    chunksCount += a.manager.getChunksCount();
    for (int ci = 0, ce = a.manager.getChunksCount(); ci < ce; ++ci)
    {
      data += a.manager.getChunkCapacity(ci) * a.entitySize;
      needData += a.manager.getChunkUsed(ci) * a.entitySize;
      emptyData += a.manager.getChunkUsed(ci) == 0 ? a.manager.getChunkCapacity(ci) * a.entitySize : 0;
      if (a.manager.getChunkUsed(ci) == 0 && a.manager.getChunkCapacity(ci) != 0)
        debug("arch %d of %d chunks, chunk %d has capacity of %d, but is empty", i, ce, ci, a.manager.getChunkCapacity(ci));
    }
  }
  archData += archetypes.archetypeComponents.capacity() * (sizeof(component_index_t) + sizeof(uint16_t) * 2);
  archData += archetypes.archetypes.capacity() * sizeof(typename decltype(archetypes.archetypes)::value_tuple);
  uint32_t creationQueueCount = 0;

  for (auto &chunk : delayedCreationQueue)
    creationQueueCount += chunk.capacity;
  debug("delayedCreationQueue.capacity()=%d(%d), mem=%d+%d", creationQueueCount, delayedCreationQueue.capacity(),
    creationQueueCount * sizeof(DelayedEntityCreation),
    delayedCreationQueue.capacity() * sizeof(decltype(delayedCreationQueue)::value_type));
  totalMem += creationQueueCount * sizeof(DelayedEntityCreation) +
              delayedCreationQueue.capacity() * sizeof(decltype(delayedCreationQueue)::value_type);

  debug("deferredEvents mem=%d", eventsStorage.capacity());
  totalMem += eventsStorage.capacity();

  totalMem += index + archData + data + chunkData;
  debug("archetypes count = %d chunks=%d index= %d archData=%d allocatedData= %d neededData = %d emptyData = %d "
        "chunkData = %d, total = %d",
    archetypes.size(), chunksCount, index, archData, data, needData, emptyData, chunkData, index + archData + data + chunkData);
  size_t archetypeQueriesMem = 0;
  for (auto &aq : archetypeQueries)
    archetypeQueriesMem += aq.memUsage();
  const size_t archetypeQueriesSize =
    archetypeQueries.capacity() * sizeof(ArchetypesQuery) + archetypeEidQueries.capacity() * sizeof(ArchetypesEidQuery);
  const size_t resQueriesSize = resolvedQueries.capacity() * sizeof(ResolvedQueryDesc);

  const size_t copyQueriesSize = queryDescs.capacity() * sizeof(CopyQueryDesc);

  size_t resQueriesMem = 0;
  for (auto &q : queryDescs)
    resQueriesMem += q.components.capacity() * sizeof(ComponentDesc);
  const size_t refAndGenSize = queriesReferences.capacity() * sizeof(uint16_t) + queriesGenerations.capacity() * sizeof(uint8_t);
  size_t referencesCount = 0;
  for (auto &ref : queriesReferences)
    referencesCount += ref;

  debug("queries count %d (%d references), totalMem = %d: archMem = %d, resMem=%d+%d, copymem = %d, refAndGen = %d",
    archetypeQueries.size(), referencesCount,
    archetypeQueriesSize + resQueriesSize + resQueriesMem + copyQueriesSize + refAndGenSize + archetypeQueriesMem,
    archetypeQueriesSize + archetypeQueriesMem, resQueriesSize, resQueriesMem, copyQueriesSize, refAndGenSize);
  totalMem += archetypeQueriesSize + resQueriesSize + copyQueriesSize + refAndGenSize;

  debug("totalMemory = %gKb", totalMem / 1024.);
  return totalMem;
}


void EntityManager::accessError(EntityId eid, const HashedConstString name) const
{
  G_UNUSED(eid);
  G_UNUSED(name);
#if DAECS_EXTENSIVE_CHECKS
  G_ASSERTF(0, "component '%s'(0x%X) is not present in entity %d of template '%s'", name.str, name.hash, eid,
    getEntityTemplateName(eid));
#endif
  logmessage((errorCount++) == 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN, "component '%s'(0x%X) is not present in entity %d of template '%s'",
    name.str, name.hash, eid, getEntityTemplateName(eid));
}

void EntityManager::accessError(EntityId eid, component_index_t cidx, const LTComponentList *list = 0) const
{
  G_UNUSED(eid);
  G_UNUSED(cidx);
#if DAECS_EXTENSIVE_CHECKS
  if (list)
  {
    G_ASSERTF(0, "component '%s'(0x%X) of type (%s|0x%X) requested at %s, line %d - is not present in entity %d of template '%s'",
      list->nameStr, list->name, componentTypes.findTypeName(list->type), list->type, list->fileStr, list->line, eid,
      getEntityTemplateName(eid));
  }
  else
    G_ASSERTF(0, "component '%s'(0x%X) is not present in entity %d of template '%s'", dataComponents.getComponentNameById(cidx),
      dataComponents.getComponentTpById(cidx), eid, getEntityTemplateName(eid));
#endif
  if (list)
  {
#if DAGOR_DBGLEVEL > 0
    logmessage((errorCount++) == 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN,
      "component '%s'(0x%X) of type (%s|0x%X) requested at %s, line %d - is not present in entity %d of template '%s'", list->nameStr,
      list->name, componentTypes.getTypeNameById(list->type), list->type, list->fileStr, list->line, eid, getEntityTemplateName(eid));
#else
    logmessage((errorCount++) == 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN, "component '%s'(0x%X) is not present in entity %d of template '%s'",
      dataComponents.getComponentNameById(list->getCidx()), dataComponents.getComponentTpById(list->getCidx()), eid,
      getEntityTemplateName(eid));
#endif
  }
  else
    logmessage((errorCount++) == 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN, "component '%s'(0x%X) is not present in entity %d of template '%s'",
      dataComponents.getComponentNameById(cidx), dataComponents.getComponentTpById(cidx), eid, getEntityTemplateName(eid));
  // This is potentially rather dangerous error which might lead to crashes later (due access to non-resized arrays, etc...)
  // So make sure that log is flushed (to make it visible in crash logs)
  debug_flush(false);
}


const char *Event::getName() const
{
  if (!g_entity_mgr)
  {
#if DAGOR_DBGLEVEL > 0
    logerr("attempting to get Event name event 0x%X when EntityManager is dead", type);
#endif
    auto find_type = [this](const EventInfoLinkedList *l) { return l->getEventType() == type; };
    auto it = EventInfoLinkedList::find_if(EventInfoLinkedList::get_tail(), find_type);
    if (it != nullptr)
      return it->getEventName();
// even less sense, if it is already registered, it should be in database!
#if DAGOR_DBGLEVEL > 0
    it = EventInfoLinkedList::find_if(EventInfoLinkedList::get_registered_tail(), find_type);
    if (it != nullptr)
    {
      logerr("event <0x%X | %s> is in registered list, while is not in database", type, it->getEventName());
      return it->getEventName();
    }
#endif
    return "#Unknown#";
  }
  const char *n = g_entity_mgr->getEventsDb().findEventName(type);
  if (!n)
  {
#if DAGOR_DBGLEVEL > 0
    logerr("attempting to get Event name of unregistered event, event type 0x%X", type);
#endif
    return "#Unknown#";
  }
  return n;
}

int EntityManager::getEntitySystemSize(uint32_t es)
{
  if (es >= esListQueries.size() || !isQueryValid(esListQueries[es]))
    return -1;
  return getQuerySize(esListQueries[es]);
}

QueryId EntityManager::getQuery(uint32_t id) const
{
  if (id >= queriesReferences.size() || !queriesReferences[id])
    return QueryId();
  return QueryId::make(id, queriesGenerations[id]);
}

const char *EntityManager::getQueryName(QueryId h) const
{
  if (!isQueryValid(h))
    return nullptr;

  return queryDescs[h.index()].getName();
}

const ecs::BaseQueryDesc EntityManager::getQueryDesc(QueryId h) const
{
  if (!isQueryValid(h))
    return ecs::BaseQueryDesc{empty_span(), empty_span(), empty_span(), empty_span()};
  return queryDescs[h.index()].getDesc();
}

}; // namespace ecs

#if DAECS_EXTENSIVE_CHECKS

using Callstack = eastl::array<void *, 16>;

namespace eastl
{

template <>
struct hash<Callstack>
{
  size_t operator()(const Callstack &st) const
  {
    return ecs_mem_hash((const char *)st.data(), sizeof(Callstack::value_type) * st.max_size());
  }
};

} // namespace eastl

struct TrackAccessRecordRAIILock
{
  std::recursive_mutex *mtx;

  TrackAccessRecordRAIILock(std::recursive_mutex &_mtx) : mtx(g_entity_mgr->isConstrainedMTMode() ? &_mtx : nullptr)
  {
    if (EASTL_UNLIKELY(mtx != nullptr))
      mtx->lock();
  }

  ~TrackAccessRecordRAIILock()
  {
    if (EASTL_UNLIKELY(mtx != nullptr))
      mtx->unlock();
  }
};

struct TrackAccessRecord
{
  ecsdebug::TrackOp op = ecsdebug::TRACK_READ;
  eastl::string details;
  Callstack stack;

  ecs::template_t templateId = ecs::INVALID_TEMPLATE_INDEX;

  bool operator==(const TrackAccessRecord &rhs) const
  {
    return op == rhs.op && templateId == rhs.templateId && details == rhs.details &&
           eastl::equal(stack.begin(), stack.end(), rhs.stack.begin());
  }

  uint32_t getHash() const { return ecs::ecs_hash(details) ^ hash_int(templateId) ^ eastl::hash<Callstack>()(stack); }
};

struct TrackAccessRecordWithDups : TrackAccessRecord
{
  uint32_t dupsCount = 0;
};

struct HashedTrackAccessRecord
{
  uint32_t hash;
  uint32_t frameNo;
  uint32_t dupsCount;
};

using TrackAccessRecordsList = eastl::vector<TrackAccessRecord>;
using TrackAccessRecordWithDupsList = eastl::vector<TrackAccessRecordWithDups>;
using HashedTrackAccessRecordList = eastl::vector<HashedTrackAccessRecord>;
static ska::flat_hash_map<uint32_t, TrackAccessRecord> records_by_hash;
static eastl::vector_map<ecs::component_t, HashedTrackAccessRecordList> records_by_component;
static std::recursive_mutex track_mutex;

void ecsdebug::start_track_ecs_component(ecs::component_t comp)
{
  TrackAccessRecordRAIILock scopeLock(track_mutex);
  records_by_component.insert(comp);
}

void ecsdebug::track_ecs_component(ecs::component_t comp, ecsdebug::TrackOp op, const char *details, ecs::EntityId eid,
  bool need_stack)
{
  if (EASTL_LIKELY(records_by_component.empty())) // intentionally before lock
    return;
  TrackAccessRecordRAIILock scopeLock(track_mutex);

  auto curRecords = records_by_component.find(comp);
  if (curRecords == records_by_component.end())
    return;

  TrackAccessRecord rec;
  if (need_stack)
    stackhlp_fill_stack(rec.stack.data(), rec.stack.max_size(), 2);
  else
    eastl::fill(rec.stack.begin(), rec.stack.end(), nullptr);

  rec.op = op;
  rec.details = details;
  rec.templateId = g_entity_mgr->getEntityTemplateId(eid);

  if (!curRecords->second.empty())
  {
    HashedTrackAccessRecord &lastHashedRecord = curRecords->second.back();
    TrackAccessRecord &lastRecord = records_by_hash.find(lastHashedRecord.hash)->second;
    if (lastRecord == rec)
    {
      lastHashedRecord.dupsCount++;
      return;
    }
  }

  const uint32_t recordHash = rec.getHash();
  auto insertRes = records_by_hash.emplace(recordHash, TrackAccessRecord{});
  if (insertRes.second)
    insertRes.first->second = eastl::move(rec);
  else
  {
    // Check collisions
    G_ASSERT(insertRes.first->second == rec);
  }

  curRecords->second.push_back({recordHash, dagor_frame_no(), 0});
}

void ecsdebug::track_ecs_component(const ecs::BaseQueryDesc &desc, const char *details, ecs::EntityId eid, bool need_stack)
{
  TrackAccessRecordRAIILock scopeLock(track_mutex);

  if (EASTL_LIKELY(records_by_component.empty()))
    return;

  for (const ecs::ComponentDesc &c : desc.componentsRO)
    track_ecs_component(c.name, TRACK_READ, details, eid, need_stack);

  for (const ecs::ComponentDesc &c : desc.componentsRW)
    track_ecs_component(c.name, TRACK_WRITE, details, eid, need_stack);
}

void ecsdebug::stop_dump_track_ecs_components()
{
  TrackAccessRecordRAIILock scopeLock(track_mutex);

  ska::flat_hash_map<Callstack, eastl::string> solvedCallstacks;

  for (const auto &kv : records_by_component)
  {
    eastl::vector_map<uint32_t, TrackAccessRecordWithDupsList> recordsByFrame;

    for (const auto &h : kv.second)
    {
      auto findRes = records_by_hash.find(h.hash);
      G_ASSERT_CONTINUE(findRes != records_by_hash.end());

      TrackAccessRecordWithDups record;
      static_cast<TrackAccessRecord &>(record) = findRes->second;
      record.dupsCount = h.dupsCount;

      auto res = recordsByFrame.insert(h.frameNo);
      res.first->second.push_back(record);
    }

    debug("====[ %s ]====", g_entity_mgr->getDataComponents().findComponentName(kv.first));

    eastl::string report;
    report.reserve(16 << 20);

    const TrackAccessRecordWithDupsList *lastPrinted = nullptr;
    for (const auto &kv : recordsByFrame)
    {
      const uint32_t frameNo = kv.first;

      const TrackAccessRecordWithDupsList &curRecords = kv.second;
      if (lastPrinted && lastPrinted->size() == curRecords.size() &&
          eastl::equal(lastPrinted->begin(), lastPrinted->end(), curRecords.begin()))
        continue;

      lastPrinted = &curRecords;
      for (const auto &r : curRecords)
      {
        report.append_sprintf("  [%06d][%s]: %s", frameNo, r.op == TRACK_WRITE ? "W" : "_", r.details.c_str());

        if (r.templateId != ecs::INVALID_TEMPLATE_INDEX)
          report.append_sprintf(" (%s)", g_entity_mgr->getTemplateName(r.templateId));

        if (r.dupsCount != 0)
          report.append_sprintf(" (dupsCount: %d)\n", r.dupsCount);
        else
          report.append("\n");

        if (r.stack[0])
        {
          auto solved = solvedCallstacks.find(r.stack);
          if (solved != solvedCallstacks.end())
            report.append_sprintf("%s\n", solved->second.c_str());
          else
          {
            auto insertRes = solvedCallstacks.emplace(r.stack, "");
            insertRes.first->second = stackhlp_get_call_stack_str((void **)r.stack.data(), r.stack.max_size()).str();
            report.append_sprintf("%s\n", insertRes.first->second.c_str());
          }
        }
      }
      report.append("\n\n");
    }

    debug("\n\n%s", report.c_str());

    debug("====[ %s ]====", g_entity_mgr->getDataComponents().findComponentName(kv.first));
  }

  records_by_component.clear();
  records_by_component.shrink_to_fit();
  records_by_hash.clear();
  records_by_hash.shrink_to_fit();
}

#endif
