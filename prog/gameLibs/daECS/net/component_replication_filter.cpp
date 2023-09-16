#include "component_replication_filters.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <daECS/core/entityManager.h>
#include <debug/dag_logSys.h>

#define VERBOSE_DEBUG(...)

namespace net
{

SmallTab<replicate_component_filter_index_t> replicate_component_filter_index; // we use SmallTab, as it's size() is faster.
SmallTab<component_replication_filter> replicate_component_filters;

uint32_t dirty_component_filter_mask = 0;
static uint32_t last_components_count = 0;

ska::flat_hash_map<eastl::string, replicate_component_filter_index_t> replicate_component_filter_map;
static eastl::vector<eastl::pair<ecs::component_t, replicate_component_filter_index_t>> delayed_components_filter;

static CompReplicationFilter filter_everywhere(ecs::EntityId, ConnectionId, const IConnection *)
{
  return CompReplicationFilter::ReplicateForConnection;
}

static void init_replicate_component_filters()
{
  if (replicate_component_filters.empty())
  {
    replicate_component_filter_map["replicate_everywhere"] = 0;
    replicate_component_filters.emplace_back(filter_everywhere);
  }
  else
  {
    auto filterEveryWhere = component_replication_filter(filter_everywhere);
    replicate_component_filters[0].swap(filterEveryWhere);
  }
}

void reset_replicate_component_filters()
{
  delayed_components_filter.clear();
  replicate_component_filter_map.clear();
  replicate_component_filters.clear();
  replicate_component_filter_index.clear();
  init_replicate_component_filters();
  last_components_count = 0;
}

const char *get_component_filter_name(replicate_component_filter_index_t fit)
{
  auto it = eastl::find_if(replicate_component_filter_map.begin(), replicate_component_filter_map.end(),
    [fit](auto a) { return a.second == fit; });
  return it == replicate_component_filter_map.end() ? nullptr : it->first.c_str();
}

replicate_component_filter_index_t find_component_filter(const char *name)
{
  auto it = replicate_component_filter_map.find_as(name);
  return it == replicate_component_filter_map.end() ? invalid_filter_id : it->second;
}

replicate_component_filter_index_t find_component_filter_for_component(ecs::component_t component)
{
  ecs::component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(component);
  if (cidx != ecs::INVALID_COMPONENT_INDEX)
    return cidx < replicate_component_filter_index.size() ? replicate_component_filter_index[cidx] : replicate_everywhere_filter_id;
  auto it = eastl::find_if(delayed_components_filter.rbegin(), delayed_components_filter.rend(),
    [component](auto a) { return a.first == component; });
  return it == delayed_components_filter.rend() ? replicate_everywhere_filter_id : it->second;
}

replicate_component_filter_index_t register_component_filter(const char *name, component_replication_filter filter)
{
  init_replicate_component_filters(); // init on demand, just in case
  auto it = replicate_component_filter_map.find_as(name);
  if (it != replicate_component_filter_map.end())
  {
    replicate_component_filters[it->second] = filter;
    return it->second;
  }
  if (replicate_component_filters.size() == eastl::numeric_limits<replicate_component_filter_index_t>::max())
  {
    logerr("too many filters already registered, replace replicate_component_filter_index_t type");
    return replicate_everywhere_filter_id;
  }
  replicate_component_filter_index_t index = replicate_component_filters.size();
  replicate_component_filters.emplace_back(filter);
  replicate_component_filter_map[name] = index;
  if (sizeof(dirty_component_filter_mask) * 8 == index)
    logerr("too many filters already registered %d, dirty_component_filter_mask can't work with", index);
  return index;
}

void set_component_filter_cidx(ecs::component_index_t cidx, replicate_component_filter_index_t filter)
{
  if (filter >= replicate_component_filters.size())
  {
    logerr("invalid replication  filter# %d, maximum registered %d", filter, replicate_component_filters.size());
    return;
  }
  if (cidx >= replicate_component_filter_index.size())
    replicate_component_filter_index.resize(cidx + 1, replicate_everywhere_filter_id);
  VERBOSE_DEBUG("cidx: set <%s> repl. filter for <%s> component", get_component_filter_name(filter),
    g_entity_mgr->getDataComponents().getComponentNameById(cidx));
  replicate_component_filter_index[cidx] = filter;
}

static bool set_component_filter_internal(ecs::component_t component, replicate_component_filter_index_t filter)
{
  ecs::component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(component);
  if (cidx != ecs::INVALID_COMPONENT_INDEX)
  {
    set_component_filter_cidx(cidx, filter);
    return true;
  }
  VERBOSE_DEBUG("unresolved <0x%X|%s> component for repl. filter", component,
    g_entity_mgr->getDataComponents().findComponentName(component), get_component_filter_name(filter));
  return false;
}

void set_component_filter(ecs::component_t component, const char *filter_name)
{
  VERBOSE_DEBUG("set <%s> repl. filter for <0x%X|%s> component", filter_name, component,
    g_entity_mgr->getDataComponents().findComponentName(component));
  auto it = replicate_component_filter_map.find_as(filter_name);
  if (it == replicate_component_filter_map.end())
  {
    logerr("replication filter %s is not registered", filter_name);
    return;
  }
  if (!set_component_filter_internal(component, it->second))
    delayed_components_filter.emplace_back(component, it->second);
}

void set_component_filter(ecs::component_t component, replicate_component_filter_index_t filter)
{
  VERBOSE_DEBUG("set <%s> filter for <0x%X|%s> component", get_component_filter_name(filter), component,
    g_entity_mgr->getDataComponents().findComponentName(component));
  if (filter >= replicate_component_filters.size())
  {
    logerr("replication filter %d is not registered", filter);
    return;
  }
  if (!set_component_filter_internal(component, filter))
    delayed_components_filter.emplace_back(component, filter);
}

void register_pending_component_filters()
{
  if (last_components_count == g_entity_mgr->getDataComponents().size()) // no sense to check delayed, unless number haschanged - there
                                                                         // would be nothing to do
    return;
  last_components_count = g_entity_mgr->getDataComponents().size();
  for (auto i = delayed_components_filter.begin(); i != delayed_components_filter.end();)
  {
    if (set_component_filter_internal(i->first, i->second))
      i = delayed_components_filter.erase(i);
    else
      ++i;
  }
}

void update_component_filter_events() { dirty_component_filter_mask |= (1 << replicate_component_filters.size()) - 1; }

extern void mark_some_objects_as_dirty(uint32_t mask);

void update_dirty_component_filter_mask()
{
  dirty_component_filter_mask &= (1 << replicate_component_filters.size()) - 1; // all other filters are invalid anyway
  if (dirty_component_filter_mask != 0)
    mark_some_objects_as_dirty(dirty_component_filter_mask);
  dirty_component_filter_mask = 0;
}

}; // namespace net