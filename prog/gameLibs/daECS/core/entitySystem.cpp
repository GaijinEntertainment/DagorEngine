// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>
#include <debug/dag_assert.h>
#include <daECS/core/entityManager.h>
#include "tokenize_const_string.h"
#include "check_es_optional.h"

namespace ecs
{

EntitySystemDesc *EntitySystemDesc::tail = NULL;
uint32_t EntitySystemDesc::generation = 0;

void remove_system_from_list(EntitySystemDesc *desc)
{
  remove_if_systems([desc](EntitySystemDesc *s) { return s == desc; });
}


void clear_entity_systems_registry() // should be called at exit, to optimize destruction of ESD
{
  for (EntitySystemDesc *esd = EntitySystemDesc::tail; esd;)
  {
    auto next = esd->next;
    esd->freeIfDynamic();
    esd = next;
  }
  EntitySystemDesc::tail = nullptr;
}

bool EntityManager::validateQueryDesc(const BaseQueryDesc &desc) const
{
  G_UNUSED(desc);
#if DAECS_EXTENSIVE_CHECKS
  auto find = [](dag::ConstSpan<ComponentDesc> sl, const ComponentDesc &v) {
    return eastl::find_if(sl.begin(), sl.end(), [&](const ComponentDesc &s) { return s.name == v.name; }) != sl.end();
  };
  for (auto &no : desc.componentsNO)
    if (find(desc.componentsRQ, no) || find(desc.componentsRO, no) || find(desc.componentsRW, no))
      return false;
#endif
  return true;
}

static inline bool allow_name_collision(const EntitySystemDesc *a, const EntitySystemDesc *b)
{
  if (a == b) // it is same ES, probably bug
    return false;
  // we allow name collision, if it is c++ ES systems declared in same module
  return !a->isDynamic() && !b->isDynamic() && a->getModuleName() && b->getModuleName() &&
         strcmp(a->getModuleName(), b->getModuleName()) == 0;
}

// https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
template <class MarkContainer, class ListContainer, class EdgeContainer, typename LoopDetected>
static bool visit_top_sort(int node, const EdgeContainer &edges, MarkContainer &temp, MarkContainer &perm, ListContainer &result,
  LoopDetected cb)
{
  if (perm[node])
    return true;
  if (temp[node])
  {
    cb(node, temp);
    perm.set(node, true);
    return false;
  }
  temp.set(node, true);
  bool ret = true;
  if (edges.size() > node)
    for (auto child : edges[node])
      ret &= visit_top_sort(child, edges, temp, perm, result, cb);

  temp.set(node, false);
  if (!perm[node]) // this check is needed in case graph is not DAG. we will just ignore such nodes.
    result.push_back(node);
  perm.set(node, true);
  return ret;
}

typedef eastl::fixed_vector<int, 2, true> edge_container_t; // typically no more than 2 edges
template <typename T>
using SmallFmemTab = SmallTab<T, framemem_allocator>; // To consider: move to public headers?

template <typename LoopDetected, typename E, typename T>
static bool topo_sort(int N, const E &edges, T &sortedList, LoopDetected cb)
{
  sortedList.reserve(N);
  eastl::bitvector<framemem_allocator> tempMark(N, false);
  eastl::bitvector<framemem_allocator> visitedMark(N, false);
  bool isDAG = true;
  for (size_t i = 0; i < N; ++i)
    isDAG &= visit_top_sort(i, edges, tempMark, visitedMark, sortedList, cb);
  return isDAG;
}

void EntityManager::resetEsOrder()
{
  DAECS_EXT_ASSERT_RETURN(isEventSendingPossible(), );
  eventDb.validate();
  if (lastEsGen == EntitySystemDesc::generation)
    return;
  debug("reset ES");

  remove_if_systems([&](EntitySystemDesc *sd) {
    if (!validateQueryDesc(*sd))
    {
      logerr("invalid entity system <%s> (some of components are both in required_not and RW/RQ/RO components lists)", sd->name);
      sd->freeIfDynamic();
      return true;
    }
    return false;
  });

  {
    FRAMEMEM_REGION;

    // enumerate all ES
    SmallFmemTab<EntitySystemDesc *> esFullList;
    for (EntitySystemDesc *psd = EntitySystemDesc::tail; psd; psd = psd->next)
      esFullList.push_back(psd);

    // randomize list to avoid depending on native ES registration order
    // which might be different on different platforms, depend on hot-reload, etc...
    stlsort::sort(esFullList.begin(), esFullList.end(),
      [](auto a, auto b) { return ECS_HASH_SLOW(a->name).hash < ECS_HASH_SLOW(b->name).hash; });

    // check if they are correct event handlers
    for (auto sd : esFullList)
    {
      if ((sd->ops.onEvent == nullptr) != (sd->evSet.empty() && (sd->getCompSet() == NULL || *sd->getCompSet() == 0)))
      {
        logerr("entity system <%s> has %d events signed for but has %sevent handler", sd->name, sd->evSet.size(),
          sd->ops.onEvent ? "" : "no ");
        if (sd->evSet.empty())
          sd->ops.onEvent = nullptr;
        else
          sd->evSet.clear();
      }
    }

    // build graph

    eastl::hash_map<eastl::string_view, int, eastl::hash<eastl::string_view>, eastl::equal_to<eastl::string_view>, framemem_allocator>
      nameESMap;
    nameESMap.reserve(esFullList.size() * 17 / 16); // Heuristic

    SmallFmemTab<int> esToGraphNodeMap; // ES to graph vertex map
    esToGraphNodeMap.reserve(esFullList.size());
    SmallFmemTab<int> graphNodeToEsMap; // graph vertex -> ES map
    graphNodeToEsMap.reserve(esFullList.size());
    int graphNodesCount = 0;
    SmallFmemTab<edge_container_t> edgesFrom;
    auto insertEdge = [&edgesFrom](int from, int to) {
      if (edgesFrom.size() <= max(from, to))
        edgesFrom.resize(max(from, to) + 1);
      edgesFrom[from].push_back(to);
    };

    // build graph from esOrder (list of sync points)
    if (esOrder.size())
    {
      // restore linear esOrder
      SmallFmemTab<eastl::string_view> esOrderList;
      esOrderList.resize(esOrder.size());
      for (auto &i : esOrder)
        esOrderList[i.second] = eastl::string_view(i.first);

      edgesFrom.reserve(esOrderList.size());
      int prevGraphNode = -1;
      for (int i = 0, e = esOrderList.size(); i < e; ++i) // explicit order from esOrder
      {
        auto insResult = nameESMap.emplace(esOrderList[i], graphNodesCount);
        if (insResult.second)
          ++graphNodesCount;
        const int graphNode = insResult.first->second;
        if (prevGraphNode >= 0)
          insertEdge(prevGraphNode, graphNode);
        prevGraphNode = graphNode;
      }
    }

    auto insertNameEdge = [&](const char *name, int graphNode, eastl::string_view es, bool before) {
      auto insResult = nameESMap.emplace(es, graphNodesCount);
      if (insResult.second)
      {
        if (esOrder.size() && (esSkip.find_as(name, eastl::less_2<const eastl::string, const char *>()) != esSkip.end()))
        {
          const bool eventHandler = es.find("_event_handler") != es.npos;
          G_UNUSED(eventHandler);
          logerr("ES <%s> is supposed to be %s ES/sync <%.*s>, which is undeclared.%s", name, before ? "before" : "after", es.length(),
            es.data(), eventHandler ? "Just remove _event_handler in the end, as it is not part of ES name" : "");
        }
        ++graphNodesCount;
      }
      const int other = insResult.first->second;
      insertEdge(before ? graphNode : other, before ? other : graphNode);
    };

    // before/after edges
    auto insertEdges = [&](const char *name, int graphNode, const char *edges, bool before) {
      if (!edges || edges[0] == '*')
        return;
      tokenize_const_string(edges, ", ", [&](eastl::string_view es) {
        insertNameEdge(name, graphNode, es, before);
        return true;
      });
    };

    for (int i = 0, e = esFullList.size(); i < e; ++i)
    {
      eastl::string_view name(esFullList[i]->name);
      auto insResult = nameESMap.emplace(name, graphNodesCount);
      const int graphNode = insResult.first->second;
      if (!insResult.second) // was already inserted
      {
        const int j = graphNodeToEsMap.size() <= graphNode ? -1 : graphNodeToEsMap[graphNode];
        if (j >= 0 && !allow_name_collision(esFullList[i], esFullList[j]))
        {
          logerr("ES of name <%s> already registered in module <%s> now requested in module <%s>", esFullList[i]->name,
            esFullList[i]->getModuleName(), esFullList[j]->getModuleName());
          esFullList[j] = nullptr;
        }
      }
      else // inserted new one
        graphNodesCount++;

      if (graphNodeToEsMap.size() <= graphNode)
        graphNodeToEsMap.resize(graphNode + 1, -1);
      graphNodeToEsMap[graphNode] = i;

      if (esToGraphNodeMap.size() <= i)
        esToGraphNodeMap.resize(i + 1, -1);
      esToGraphNodeMap[i] = graphNode;
    }

    // insert explicit graph edges
    for (int i = 0, e = esFullList.size(); i < e; ++i)
    {
      if (!esFullList[i])
        continue;
      auto it = nameESMap.find(eastl::string_view(esFullList[i]->name));
      G_ASSERT_CONTINUE(it != nameESMap.end());
      const int graphNode = it->second;
      const char *beforeStr = esFullList[i]->getBefore(), *afterStr = esFullList[i]->getAfter();
      insertEdges(esFullList[i]->name, graphNode, beforeStr, true);
      insertEdges(esFullList[i]->name, graphNode, afterStr, false);
      if (!beforeStr || !beforeStr[0] || ::strstr(beforeStr, "__first_sync_point") == nullptr)
        insertNameEdge(esFullList[i]->name, graphNode, "__first_sync_point", false);
    }


    // topo sort
    SmallFmemTab<int> sortedList;
    auto loopDetected = [&](int i, auto &) {
      auto it = eastl::find_if(nameESMap.begin(), nameESMap.end(), [&](const auto &it) { return it.second == i; });
      eastl::string node = "n/a";
      if (it != nameESMap.end())
        node = it->first;
      logerr("syncPoint %s resulted in graph to become cyclic and was removed from sorting. ES order is non-determinstic",
        node.c_str());
    };
    topo_sort(graphNodesCount, edgesFrom, sortedList, loopDetected);
    const int lowestPrio = eastl::numeric_limits<int>::max();
    eastl::vector<int, framemem_allocator> sortedPrio;
    sortedPrio.resize(sortedList.size(), lowestPrio);
    for (size_t i = 0, e = sortedList.size(); i < e; ++i)
    {
      if (uint32_t(sortedList[i]) < sortedList.size())
        sortedPrio[sortedList[i]] = sortedList.size() - i;
    }

    // use graph for ES prio list
    struct PrioEntitySystemDesc
    {
      int id;
      int prio;
      PrioEntitySystemDesc(int i, int p) : id(i), prio(p) {}
      bool operator<(const PrioEntitySystemDesc &other) const { return prio < other.prio; }
    };
    SmallFmemTab<PrioEntitySystemDesc> prio;
    prio.reserve(esFullList.size());

    for (int i = 0, e = esFullList.size(); i < e; ++i)
    {
      EntitySystemDesc *sd = esFullList[i];
      if (!sd) // removed invalid ES
        continue;
      if (disableEntitySystems.find_as(sd->name, eastl::less_2<const eastl::string, const char *>()) != disableEntitySystems.end())
      {
        debug("skip ES <%s> due to it is switched off in inspection", sd->name);
        continue;
      }
      if (esSkip.find_as(sd->name, eastl::less_2<const eastl::string, const char *>()) != esSkip.end())
      {
        debug("skip ES <%s> due to it is not needed", sd->name);
        continue;
      }
      if (esTags.size() && !filter_needed(sd->getTags(), esTags))
      {
#if DAGOR_DBGLEVEL > 0
        debug("skip ES <%s> due to it is tags <%s>", sd->name, sd->getTags());
#endif
        continue;
      }
      const uint32_t graphNode = i < esToGraphNodeMap.size() ? esToGraphNodeMap[i] : ~0u;
      int updatePrio = (uint32_t(graphNode) < sortedPrio.size()) ? sortedPrio[graphNode] : lowestPrio;

      // if there is no before/after and not in esOrder
      if (sd->ops.onUpdate && (sd->getBefore() == nullptr && sd->getAfter() == nullptr) && !esOrder.empty() &&
          esOrder.find_as(sd->name, eastl::less_2<const eastl::string, const char *>()) == esOrder.end())
      {
        if (ignoredES.insert(ECS_HASH_SLOW(sd->name).hash).second)
          logerr("Unknown update ES order for '%s'", sd->name);
      }
      prio.push_back(PrioEntitySystemDesc({i, updatePrio}));
    }

    queryToEsMap.clear();
    clear_and_shrink(esList);
    esForAllEntities.clear();
    stlsort::sort_branchless(prio.begin(), prio.end());
    clear_and_resize(esList, prio.size());
    esForAllEntities.resize(prio.size());
    esUpdates.clear();

    uint32_t mask = 0;
    for (int i = 0; i < prio.size(); ++i)
    {
      esList[i] = esFullList[prio[i].id];
      // es will perform on all entities
      const EntitySystemDesc *es = esList[i];
      mask |= es->stageMask;
      if ((es->componentsRW.size() + es->componentsRO.size()) != 0 && allCompsAreOptional(es))
      {
        esForAllEntities.set(i, true);
#if DAGOR_DBGLEVEL > 0
        if (es->ops.onEvent)
        {
          for (auto evt : es->evSet)
          {
            const auto evtId = eventDb.findEvent(evt);
            if (evtId != EventsDB::invalid_event_id && (eventDb.getEventFlags(evtId) & EVCAST_BROADCAST))
              logerr("EntitySystem <%s> from module <%s> with all optional components subscribed to broadcast event <0x%X|%s>",
                es->name, es->getModuleName(), evt, eventDb.getEventName(evtId));
          }
        }
        if (es->stageMask)
          logerr("EntitySystem <%s> from module <%s> with all optional components subscribed to UpdateStage", es->name,
            es->getModuleName());
#endif
      }
    }

    if (mask) // total amount of stages
      esUpdates.resize(__bsr(mask) + 1);
    // todo: match with existent
    for (auto &eq : esListQueries)
      if (eq != QueryId() && isQueryValid(eq))
        destroyQuery(eq);
    esListQueries.resize(esList.size());
    for (int j = 0, ej = esList.size(); j < ej; ++j)
      esListQueries[j] = esList[j]->emptyES ? QueryId() : createUnresolvedQuery(*esList[j]);

    registerEsEvents();
    // todo: change esUpdates based on current entities
    // todo: validate hash collision in es (i.e. component_t collides with hash in componentDesc)
    /*
      esListUsed.resize(esList.size());
      esListUsed.reset();

      //register all ES
      for (int j = 0; j < esList.size(); ++j)
        useES(j);
    */
  } // End of framemem region
  clearQueries();
  updateAllQueries();
  lastEsGen = EntitySystemDesc::generation;
  broadcastEventImmediate(EventEntityManagerEsOrderSet());
}

void EntityManager::enableES(const char *es_name, bool on)
{
  auto it = disableEntitySystems.find_as(es_name, eastl::less_2<const eastl::string, const char *>());
  if (on != (it == disableEntitySystems.end()))
  {
    if (on)
      disableEntitySystems.erase(it);
    else
      disableEntitySystems.emplace(es_name);
    lastEsGen = EntitySystemDesc::generation - 1;
    resetEsOrder();
  }
}
void EntityManager::setEsOrder(dag::ConstSpan<const char *> es_order, dag::ConstSpan<const char *> es_skip)
{
  esOrder.clear();
  esOrder.reserve(es_order.size());
  for (int i = 0; i < es_order.size(); ++i)
  {
    auto inserted = esOrder.emplace(es_order[i], i);
    if (!inserted.second)
      logerr("ES <%s> appears twice in es_order", es_order[i]);
  }

  esSkip.reserve(es_skip.size());
  for (int i = 0; i < es_skip.size(); ++i)
    esSkip.insert(es_skip[i]);
  lastEsGen = EntitySystemDesc::generation - 1;
  resetEsOrder();
}


void EntityManager::setEsTags(dag::ConstSpan<const char *> es_tags)
{
  lastEsGen = EntitySystemDesc::generation - 1;
  esTags.clear();
  for (auto t : es_tags)
    esTags.insert(t);
}

} // namespace ecs
