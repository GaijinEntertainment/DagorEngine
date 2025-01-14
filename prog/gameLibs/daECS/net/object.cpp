// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/object.h>
#include <daECS/net/connection.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/baseIo.h>
#include <daECS/core/template.h>
#include <daECS/core/componentTypes.h>
#include "objectReplica.h"
#include <daNet/bitStream.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <string.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <daECS/net/serialize.h>
#include "component_replication_filters.h"

#define REPL_BSREAD(x) \
  if (!bs.Read(x))     \
    return failRet;

namespace net
{

#define PROFILE_COMPONENTS_SERIALIZATION_STATS (DAGOR_DBGLEVEL > 0)

#if PROFILE_COMPONENTS_SERIALIZATION_STATS

static ska::flat_hash_map<uint32_t, eastl::pair<uint32_t, uint32_t>>
  replication_components_stats; // uint32_t == ecs::template_t|(ecs::component_index_t<<16)

void dump_and_clear_components_profiler_stats()
{
  if (!replication_components_stats.size())
    return;
  G_STATIC_ASSERT(sizeof(ecs::template_t) + sizeof(ecs::component_index_t) == sizeof(uint32_t)); // if ever happen, can be replaced
                                                                                                 // with uint64_t
  eastl::vector_map<ecs::component_index_t, uint32_t> sortedList;
  sortedList.reserve(replication_components_stats.size());
  typedef eastl::vector_map<ecs::component_index_t, uint32_t>::base_type list_type;
  constexpr float topThreshold = 0.95;
  auto dump_stats = [&](list_type &list) {
    eastl::sort(list.begin(), list.end(), [](auto a, auto b) { return a.second > b.second; });
    size_t total = 0;
    eastl::for_each(list.cbegin(), list.cend(), [&total](auto &a) { total += a.second; });
    size_t curSum = 0, totalThreshold = total * topThreshold;
    double toPercentMult = total ? (100. / total) : 0.;
    int i = 0;
    for (auto &it : list)
    {
      curSum += it.second;
      debug("  #%2d component %s, was replicated %.2f%%/%.2f%%(%d)", ++i,
        g_entity_mgr->getDataComponents().getComponentNameById(it.first), it.second * toPercentMult, curSum * toPercentMult,
        it.second);
      if (curSum >= totalThreshold && i >= 10)
        break;
    }
  };
  for (auto &it : replication_components_stats)
    sortedList[it.first >> 16] += it.second.first;
  debug("dumping top %g%% components replication stats by frequency...", topThreshold * 100.);
  dump_stats(sortedList);
  sortedList.clear();
  for (auto &it : replication_components_stats)
    sortedList[it.first >> 16] += it.second.second;
  debug("dumping top %g%% components replication stats by replication bits...", topThreshold * 100.);
  dump_stats(sortedList);
  eastl::string templates;
  for (int i = 0, sz = eastl::min(sortedList.size(), size_t(20)); i < sz; ++i)
  {
    templates = "";
    ecs::component_index_t componentId = ((list_type &)sortedList)[i].first;
    for (auto &s : replication_components_stats)
    {
      if ((s.first >> 16) == componentId)
        templates.append_sprintf("%s=%d,", g_entity_mgr->getTemplateName(s.first & 0xFFFF), s.second.first);
    }
    debug(" component %s:templates <%s>", g_entity_mgr->getDataComponents().getComponentNameById(componentId), templates.c_str());
  }
  replication_components_stats.clear();
  replication_components_stats.shrink_to_fit();
}
#else
void dump_and_clear_components_profiler_stats() {}
#endif

/* static */
bool Object::do_not_verify_destruction = false;
eastl::vector_set<ecs::EntityId> Object::pending_destroys;
Object::dirty_entities_list_t Object::dirtyList;
static uint32_t objectsCreationOrderSequence = 0;
static const ecs::component_index_t NO_COMPONENTS_STUB = 0;
typedef eastl::vector<const ecs::component_index_t *> components_list_type_t;
static components_list_type_t replicated_components, ignored_components; // cached list per template, begins with num components in 0th
                                                                         // element)

void clear_cached_replicated_components()
{
  auto clear = [&](components_list_type_t &components) {
    for (const ecs::component_index_t *cptr : components)
      if (cptr != &NO_COMPONENTS_STUB)
        EASTLFree(EASTLAllocatorType(), cptr, 0);
    components_list_type_t().swap(components); // clear
  };
  clear(replicated_components);
  clear(ignored_components);
}

// list of replicated components is unique for each template.
// instead of getting slow iteration over hash_sets or even regexps - just initialize once (cache)
static const ecs::component_index_t *get_template_replicated_components(ecs::EntityId eid, uint32_t &cnt)
{
  return g_entity_mgr->replicatedComponentsList(g_entity_mgr->getEntityTemplateId(eid), cnt);
}

static inline void gather_ignored(const ecs::Template *templ, eastl::vector_set<ecs::component_index_t> &ignored)
{
  for (auto comp : templ->getIgnoredInitialReplicationSet())
  {
    ecs::component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(comp);
    if (cidx != ecs::INVALID_COMPONENT_INDEX && ecs::should_replicate(cidx, *g_entity_mgr)) // if it is not serializable, no reason to
                                                                                            // do anything.
      ignored.insert(cidx);
  }
  for (auto p : templ->getParents())
    gather_ignored(g_entity_mgr->getTemplateDB().getTemplateById(p), ignored);
};

const ecs::component_index_t *get_template_ignored_initial_components(ecs::template_t tid)
{
  if (const ecs::component_index_t *ret = (tid < ignored_components.size()) ? ignored_components[tid] : nullptr)
    return (ret == &NO_COMPONENTS_STUB) ? nullptr : ret;
  const char *templateName = g_entity_mgr->getTemplateName(tid);
  const ecs::Template *templ = nullptr;
  if (!templateName)
    return nullptr;
  templ = g_entity_mgr->getTemplateDB().getTemplateByName(templateName);
  if (!templ)
    return nullptr;
  eastl::vector_set<ecs::component_index_t> ignoredComponents;
  gather_ignored(templ, ignoredComponents);
  if (tid >= ignored_components.size())
    ignored_components.resize(tid + 1);
  if (ignoredComponents.size() == 0)
  {
    ignored_components[tid] = &NO_COMPONENTS_STUB;
    return nullptr;
  }
  ignoredComponents.getContainer().insert(ignoredComponents.begin(), ignoredComponents.size());
  ignoredComponents.shrink_to_fit();
  ignored_components[tid] = ignoredComponents.data();
  ignoredComponents.reset_lose_memory();
  return ignored_components[tid];
}


void ObjectReplica::debugVerifyRemoteCompVers(const CompVersMap &local_comp_vers, bool shallow_check) const
{
  G_UNUSED(local_comp_vers);
  G_UNUSED(shallow_check);
#if DAECS_EXTENSIVE_CHECKS
  bool eq = local_comp_vers.size() == remoteCompVers.size();
  for (int i = 0, sz = local_comp_vers.size(); i < sz && eq && !shallow_check; ++i)
    eq &= local_comp_vers.data()[i].first == remoteCompVers.data()[i].first;
  G_ASSERTF(eq, "%d<%s> %d != %d %p", eidStorage, g_entity_mgr->getEntityTemplateName(ecs::EntityId(eidStorage)),
    (int)local_comp_vers.size(), (int)remoteCompVers.size(), this);
#endif
}

Object::Object(const ecs::EntityManager &, ecs::EntityId eid_, const ecs::ComponentsMap &map) :
  eid(eid_),
  controlledBy(INVALID_CONNECTION_ID),
  creationOrder(objectsCreationOrderSequence++),
  isReplicaFlag(map[ECS_HASH("serverEid")].getOr((int)ecs::ECS_INVALID_ENTITY_ID_VAL) != ecs::ECS_INVALID_ENTITY_ID_VAL),
  isMeantToBeDestroyed(!isReplicaFlag) // if it is server replica then, and only then, we should wait for destruction packet
{
  register_pending_component_filters(); // we can actually remove it from here, and make callback for new component creation instead
  if (!isReplica())
    initCompVers(); // Potentially accesses some filters, registered in register_pending_component_filters call above
}

Object::~Object()
{
  while (replicasLinkList) // auto kill all replicas for this object
    replicasLinkList->conn->killObjectReplica(replicasLinkList, this);
  dirtyList.erase(eid);
}

void Object::initCompVers()
{
  if (isReplica())
    return;
  uint32_t ncomp = 0;
  const ecs::component_index_t *components = get_template_replicated_components(eid, ncomp), *cend = components + ncomp;
  if (ncomp)
  {
    filteredComponentsBits = 0;
    DAECS_EXT_ASSERT(replicate_component_filters.size() <= sizeof(filteredComponentsBits) * 8);
    for (const ecs::component_index_t *c = components; c != cend; ++c)
    {
      replicate_component_filter_index_t filterIndex = get_replicate_component_filter_index(*c);
      if (filterIndex != replicate_everywhere_filter_id)
        filteredComponentsBits |= 1 << (filterIndex - 1);
    }
  }

  if (!ncomp || compVers.empty())
  {
    compVers.clear();
    for (ObjectReplica *repl = replicasLinkList; repl; repl = repl->nextRepl)
      repl->remoteCompVers.clear();
    return;
  }
  for (auto it = compVers.begin(); it != compVers.end();) // erase not existing
  {
    auto cit = eastl::lower_bound(components, cend, it->first);
    if (cit != cend && *cit == it->first)
      ++it;
    else
    {
      for (ObjectReplica *repl = replicasLinkList; repl; repl = repl->nextRepl)
      {
        auto rit = repl->remoteCompVers.find(it->first);
        if (rit != repl->remoteCompVers.end())
          repl->remoteCompVers.erase(rit);
      }
      it = compVers.erase(it);
    }
  }
  for (ObjectReplica *repl = replicasLinkList; repl; repl = repl->nextRepl)
    repl->debugVerifyRemoteCompVers(compVers);
}

void Object::forceReplicaVersion(CompVersMap::const_iterator cit, ecs::component_index_t cidx, ObjectReplica *replica) const
{
  if (cit != compVers.end() && cit->first == cidx) // just force correct remoteCompVer
  {
    size_t at = eastl::distance(compVers.begin(), cit);
    G_FAST_ASSERT(replica->remoteCompVers.data()[at].first == cit->first);
    replica->remoteCompVers.data()[at].second = cit->second; // now it is already replicated
  }
}

bool Object::skipInitialReplication(ecs::component_index_t cidx, Connection *conn, ObjectReplica *replica)
{
  if (!g_entity_mgr->isReplicatedComponent(g_entity_mgr->getEntityTemplateId(eid), cidx))
    return false;

  // if components should not be replicated, we should not replicate them on creation either
  // that is both optimizing traffic and hides unwanted data (on JiP/enter to scope/initial creaion).
  CompVersMap::iterator cit = compVers.lower_bound(cidx);
  G_UNUSED(conn);
  // this one should be working, but I am not entirely sure
  // to be enabled in a separate commit
  bool shouldBeSkipped = false;
  if (filteredComponentsBits != 0 && conn->isFiltering)
  {
    replicate_component_filter_index_t fit = get_replicate_component_filter_index(cidx);
    shouldBeSkipped = (fit != replicate_everywhere_filter_id &&
                       int(replicate_component_filters[fit](eid, controlledBy, conn)) <= int(CompReplicationFilter::SkipNow));
  }
  if (!shouldBeSkipped)
  {
    forceReplicaVersion(cit, cidx, replica);
    return false;
  }
  if (cit == compVers.end() || cit->first != cidx) // it has not been already changed by someone
  {
    cit = insertNewCompVer(cidx, cit);
  }
  else // if we have skipped replication, we have to ensure remote version is different
  {
    size_t at = eastl::distance(compVers.begin(), cit);
    G_FAST_ASSERT(replica->remoteCompVers.data()[at].first == cit->first);
    if (replica->remoteCompVers.data()[at].second == cit->second)
      replica->remoteCompVers.data()[at].second--; // it is not replicated
  }
  return true;
}

CompVersMap::iterator Object::insertNewCompVer(ecs::component_index_t cidx, CompVersMap::iterator cit)
{
  size_t i = eastl::distance(compVers.begin(), cit);
  for (ObjectReplica *repl = replicasLinkList; repl; repl = repl->nextRepl)
  {
    repl->debugVerifyRemoteCompVers(compVers);
    repl->remoteCompVers.insert(repl->remoteCompVers.begin() + i, eastl::make_pair(cidx, DEFAULT_COMP_VER));
  }
  compVers.insert(cit, eastl::make_pair(cidx, compver_t(DEFAULT_COMP_VER + 1)));
  addToDirty();
  return compVers.begin() + i;
}

void Object::onCompChanged(ecs::component_index_t cidx)
{
  CompVersMap::iterator it = compVers.lower_bound(cidx);
  if (it != compVers.end() && it->first == cidx)
  {
    ++it->second;
    return addToDirty();
  }
  // No active repl ver found -> search all replicated components
  insertNewCompVer(cidx, it);
}

void Object::serializeComps(const Connection *conn, danet::BitStream &bs, const ObjectReplica &repl,
  eastl::vector<CompRevision> &comps_serialized) const
{
  G_ASSERTF(!isReplica(), "%d", (ecs::entity_id_t)eid);
#if DAECS_EXTENSIVE_CHECKS
  repl.debugVerifyRemoteCompVers(compVers, /*shallow_check*/ true);
#else
  G_FAST_ASSERT(compVers.size() == repl.remoteCompVers.size());
#endif
  bool writtenSomething = false;
  for (int i = 0, sz = compVers.size(); i < sz; ++i)
  {
    auto &compVer = compVers.data()[i];
    ecs::component_index_t localCompIdx = compVer.first;
    compver_t localCompVer = compVer.second;
    DAECS_EXT_FAST_ASSERT(localCompIdx == repl.remoteCompVers.data()[i].first);
    if (localCompVer == repl.remoteCompVers.data()[i].second)
      continue;
    if (filteredComponentsBits != 0 && conn->isFiltering) // todo: isFiltering check has to be removed, as soon as we will implement
                                                          // filtering events. In that case, it will be checked on event
    {
      if (int(should_skip_component_replication(eid, localCompIdx, conn, controlledBy)) <= int(CompReplicationFilter::SkipNow))
        continue;
    }
    const auto &comp = g_entity_mgr->getComponentRef(eid, localCompIdx);
    G_ASSERTF_CONTINUE(!comp.isNull(), "There is exist comp version for unknown component <%d>(%s) in entity <%d>(%s)", localCompIdx,
      g_entity_mgr->getDataComponents().getComponentNameById(localCompIdx), (ecs::entity_id_t)eid,
      g_entity_mgr->getEntityTemplateName(eid));
    BitSize_t posBeforeCompWrite = bs.GetWriteOffset();
    bs.Write(true);
    if (conn->serializeComponentReplication(eid, comp, bs))
    {
      comps_serialized.push_back(CompRevision{localCompIdx, localCompVer});
      writtenSomething = true;
#if PROFILE_COMPONENTS_SERIALIZATION_STATS
      // FIXME: these measurements are incorrect when replication is thrown away (when packet over MTU limit)
      if (!conn->isBlackHole())
      {
        auto &stat = replication_components_stats[g_entity_mgr->getEntityTemplateId(eid) | (localCompIdx << 16)];
        stat.first++;
        stat.second += bs.GetWriteOffset() - posBeforeCompWrite;
      }
#endif
    }
    else // serialization failed (highly unlikely)
    {
      bs.SetWriteOffset(posBeforeCompWrite);
      const_cast<CompVersMap &>(repl.remoteCompVers)[localCompIdx] = localCompVer;
    }
  }
  if (writtenSomething)
    bs.Write(false); // term
}

bool Object::deserializeComps(const danet::BitStream &bs, Connection *conn)
{
  G_ASSERTF(isReplica(), "%d", (ecs::entity_id_t)eid);
  bool failRet = false;
  do
  {
    bool exist = false;
    REPL_BSREAD(exist);
    if (!exist)
      break;

    if (!conn->deserializeComponentReplication(eid, bs))
      return false;
  } while (1);
  return true;
}

void Object::addToDirty() { dirtyList.insert(eid); }
}; // namespace net


// Non-std registration due to custom type flag (COMPONENT_TYPE_REPLICATION)
ECS_REGISTER_TYPE_BASE(net::Object, ecs::ComponentTypeInfo<net::Object>::type_name, nullptr,
  (&ecs::CTMFactory<typename ecs::CreatorSelector<net::Object>::type>::create),
  (&ecs::CTMFactory<typename ecs::CreatorSelector<net::Object>::type>::destroy), ecs::COMPONENT_TYPE_REPLICATION);
ECS_AUTO_REGISTER_COMPONENT(net::Object, "replication", nullptr, 0);
