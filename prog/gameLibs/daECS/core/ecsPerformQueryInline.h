// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ecsQueryManager.h"
#include "daECS/core/internal/chunkedVectorMT.h"

namespace ecs
{

struct QueryContainer
{
  QueryStackData &__restrict data;
  uint32_t *__restrict entitiesCnt = nullptr;
  QueryView::OneComponentLine **__restrict componentsData = nullptr;
  const uint32_t *entitiesCntEnd = nullptr;
  QueryView::OneComponentLine *const *__restrict componentsDataEnd = nullptr;

  QueryContainer(QueryStackData &__restrict data_) :
    data(data_),
    entitiesCnt(data.chunkEntitiesCnt.getStack()),
    entitiesCntEnd(data.chunkEntitiesCnt.getStackEnd()),
    componentsData(data.componentData.getStack()),
    componentsDataEnd(data.componentData.getStackEnd())
  {}

  __forceinline void pushChunkEntitiesCnt(uint32_t c)
  {
    if (DAGOR_UNLIKELY(entitiesCnt >= entitiesCntEnd))
      data.chunkEntitiesCnt.growStack(entitiesCnt, 1, entitiesCntEnd);
    *(entitiesCnt++) = c;
  }
  QueryView::OneComponentLine *__restrict *__restrict addComponentData(uint32_t c)
  {
    if (DAGOR_UNLIKELY(componentsData + c > componentsDataEnd))
      data.componentData.growStack(componentsData, c, componentsDataEnd);
    auto ret = componentsData;
    componentsData += c;
    return ret;
  }
};

struct CommittedQuery : public Query
{
  QueryStackData &__restrict data;
  CommittedQuery(const CommittedQuery &) = delete;
  CommittedQuery(QueryId h, QueryContainer &__restrict ctx, const ArchetypesQuery &__restrict archDesc, uint32_t total) :
    Query(h), data(ctx.data)
  {
    roRW = archDesc.roRW; // same as rwCount = archDesc.rwCount; roCount = archDesc.roCount;
    chunkEntitiesCnt = data.chunkEntitiesCnt.getStack();
    componentData = data.componentData.getStack();
    chunks = ctx.entitiesCnt - chunkEntitiesCnt;
    totalSize = total;

    data.chunkEntitiesCnt.commit(ctx.entitiesCnt);
    data.componentData.commit(ctx.componentsData);
  }
  ~CommittedQuery()
  {
    data.chunkEntitiesCnt.decommit((uint32_t *)chunkEntitiesCnt);
    data.componentData.decommit((QueryView::OneComponentLine **)componentData);
  }
};

__forceinline CommittedQuery EntityManager::commitQuery(QueryId h, QueryContainer &__restrict ctx,
  const ArchetypesQuery &__restrict arch, uint32_t total)
{
  if (arch.trackedChangesCount)
    sheduleArchetypeTracking(arch);
  return CommittedQuery(h, ctx, arch, total);
}

__forceinline uint32_t EntityManager::fillQuery(const ArchetypesQuery &__restrict archDesc, QueryContainer &__restrict ctx)
{
  const auto *__restrict aqi = archDesc.queriesBegin(), *__restrict aqEnd = archDesc.queriesEnd();
  DAECS_EXT_ASSERT(aqi != aqEnd);
  const uint32_t totalComponentsCount = archDesc.getComponentsCount();
  uint32_t totalSize = 0;
  // reserve doesn't make much sense, since we are using fixed_vector, and initial grow value will be of reasonable size anyway
  if (DAGOR_UNLIKELY(totalComponentsCount == 0)) // this is very rare case, where we have only require/require_not in broadcast message
  {
    do
    {
      totalSize += archetypes.getArchetype(*aqi).manager.getTotalEntities();
    } while (++aqi != aqEnd);
    // query.chunkEntitiesCnt.push_back(query.totalSize);//amount of chunks doesn't matter
    ctx.pushChunkEntitiesCnt(totalSize);
    return totalSize;
  }

  const ArchetypesQuery::offset_type_t *__restrict archetypeOffsets = archDesc.getArchetypeOffsetsPtr();
  do
  {
    const ArchetypesQuery::offset_type_t *__restrict archetypeOffsetsPtrEnd = archetypeOffsets + totalComponentsCount;
    const auto &__restrict manager = archetypes.getArchetype(*aqi).manager;
    const uint32_t totalEntities = manager.getTotalEntities();
    if (!totalEntities)
    {
      archetypeOffsets = archetypeOffsetsPtrEnd; //
      continue;
    }
    totalSize += totalEntities;
    // debug("%d: ofs ro = %d", query.ro.start-1, archetypeOffsets[query.ro.start-1]);
    auto chunks = manager.getChunksConst();
    DAECS_EXT_ASSERT(chunks.size() > 0);
    const auto *__restrict chunksPtr = chunks.data();
    auto chunksE = chunksPtr + chunks.size();
    do
    {
      uint32_t chunkUsed = chunksPtr->getUsed();
      if (!chunkUsed)
        continue;
      const uint32_t capacityBits = chunksPtr->getCapacityBits();
      uint8_t *__restrict data = chunksPtr->getData();
      const auto *__restrict archetypeOffsetsPtr = archetypeOffsets;
      ctx.pushChunkEntitiesCnt(chunkUsed);
      QueryView::OneComponentLine *__restrict *__restrict componentDataPtr = ctx.addComponentData(totalComponentsCount);
      do
      {
        const ArchetypesQuery::offset_type_t ofs = *(archetypeOffsetsPtr++);
        *(componentDataPtr++) =
          (ofs != ArchetypesQuery::INVALID_OFFSET) ? (QueryView::OneComponentLine *__restrict)(data + (ofs << capacityBits)) : nullptr;
      } while (archetypeOffsetsPtr != archetypeOffsetsPtrEnd);
    } while (DAGOR_UNLIKELY(++chunksPtr != chunksE));
    archetypeOffsets = archetypeOffsetsPtrEnd;
  } while (++aqi != aqEnd);

  return totalSize;
}

// actual perform query. The final one
__forceinline QueryCbResult EntityManager::performQueryStoppable(const Query &__restrict query,
  const stoppable_query_cb_t &__restrict fun, void *__restrict user_data)
{
  RaiiOptionalCounter nested(!isConstrainedMTMode(), nestedQuery);
  uint32_t chunk = 0, e = query.chunksCount();
  QueryView qv(*this, 0, 0, 0, query.id, nullptr, query.roRW, user_data);
  do
  {
    qv.componentData = (QueryView::ComponentsData *__restrict)query.getChunkData(chunk);
    qv.chunkEntitiesEnd = query.chunkEntitiesCnt[chunk];
    if (DAGOR_UNLIKELY(fun(qv) == QueryCbResult::Stop))
      return QueryCbResult::Stop;
  } while (++chunk != e);

  return QueryCbResult::Continue;
}

template <typename Cb>
__forceinline void EntityManager::performSTQuery(const Query &__restrict query, void *__restrict user_data, const Cb &__restrict fun)
{
  uint32_t chunk = 0, e = query.chunksCount();
  QueryView qv(*this, 0, 0, 0, query.id, nullptr, query.roRW, user_data);
  do
  {
    qv.componentData = (QueryView::ComponentsData *__restrict)query.getChunkData(chunk);
    qv.chunkEntitiesEnd = query.chunkEntitiesCnt[chunk];
    fun(qv);
  } while (++chunk != e);
}

__forceinline void EntityManager::performQuery(const Query &__restrict pQuery, const query_cb_t &__restrict fun,
  void *__restrict user_data, int min_quant)
{
  RaiiOptionalCounter nested(!isConstrainedMTMode(), nestedQuery);
  if (!min_quant || !maxNumJobs || pQuery.totalSize <= min_quant * 2 // doesn't make sense to run multithreading if there is less work
                                                                     // than for one additional thread. We use 3 as heurestics, because
                                                                     // threadpool has it's own cost as well
      || !performMTQuery(pQuery, fun, user_data, min_quant))
    performSTQuery(pQuery, user_data, fun);
}

#define QUERY_CONTAINER(name, aDesc) QueryContainer name;

inline void EntityManager::performQueryES(QueryId h, ESFuncType fun, const ESPayLoad &__restrict evt, void *user_data, int min_quant)
{
  uint32_t index = h.index();
  auto &__restrict archDesc = archetypeQueries[index];
  if (!archDesc.getQueriesCount())
    return;
  QueryContainer ctx(queryStack.getData());
  uint32_t querySize;
  if ((querySize = fillQuery(archDesc, ctx)) == 0)
  {
    DEBUG_VERY_VERBOSE_QUERY("no fit <%s> out of %d archs. %d in q, %d total", queryDescs[index].getName(),
      archDesc.getArchetypesRelated(), archDesc.lastArchetypesGeneration, archetypes.size());
    return;
  }
  auto query = commitQuery(h, ctx, archDesc, querySize);
  trackComponent(queryDescs[index].getDesc(), queryDescs[index].getName());
  EntityManager::ScopedQueryingArchetypesCheck scopedCheck(index, *this);
  RaiiOptionalCounter nested(!isConstrainedMTMode(), nestedQuery);
  if (DAGOR_UNLIKELY(min_quant && maxNumJobs && querySize > min_quant * 2)) // doesn't make sense to run multithreading if there is
                                                                            // less work than for one additional thread. We use 3 as
                                                                            // heurestics, because threadpool has it's own cost as well
    if (performMTQuery(query, [&evt, &fun](const QueryView &qv) { fun(evt, qv); }, user_data, min_quant))
      return;
  performSTQuery(query, user_data, [&fun, &evt](const QueryView &__restrict qv) { fun(evt, qv); });
}

__forceinline void EntityManager::performQueryEmptyAllowed(QueryId h, ESFuncType fun, const ESPayLoad &evt, void *user_data,
  int min_quant)
{
  if (h)
    performQueryES(h, fun, evt, user_data, min_quant);
  else
    fun(evt, QueryView(*this, user_data));
}

} // namespace ecs