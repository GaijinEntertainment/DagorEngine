#include <daECS/net/compBlacklist.h>
#include <daECS/core/entityManager.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/vector_map.h>
#include <EASTL/algorithm.h>

#if ECS_NET_COMP_BLACKLIST_ENABLED

namespace net
{

struct BlacklistedComponentInfo
{
  int deserializeCounter = 0;
  int changeCounter = 0;
};

using CompHashSet = ska::flat_hash_set<ecs::component_t, ska::power_of_two_std_hash<ecs::component_index_t>>;
static CompHashSet replicated_component_client_modify_blacklist;
static eastl::vector_map<uint64_t, BlacklistedComponentInfo> blacklisted_components;
static inline uint64_t make_bl_info_key(ecs::EntityId eid, ecs::component_index_t cidx)
{
  return (uint64_t(ecs::entity_id_t(eid)) << 32) | cidx;
}

void replicated_component_client_modify_blacklist_reset() { replicated_component_client_modify_blacklist.clear(); }

void replicated_component_client_modify_blacklist_add(ecs::component_t component)
{
  replicated_component_client_modify_blacklist.insert(component);
}

void replicated_component_on_client_deserialize(ecs::EntityId eid, ecs::component_index_t cidx)
{
  if (replicated_component_client_modify_blacklist.count(g_entity_mgr->getDataComponents().getComponentTpById(cidx)) <= 0)
    return;
  BlacklistedComponentInfo &info = blacklisted_components[make_bl_info_key(eid, cidx)];
  ++info.deserializeCounter;
  if ((info.changeCounter > 0) && (info.deserializeCounter == 1))
  {
    // This component was already changed on client and now it's deserialized, log error.
    info.deserializeCounter = info.changeCounter + 1; // We don't want to spam error messages.
    logerr("entity %d(templ='%s'): modifying replicated component %d('%s') from client (1), this is "
           "blacklisted by _replicatedComponentClientModifyBlacklist",
      (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid), (int)cidx,
      g_entity_mgr->getDataComponents().getComponentNameById(cidx));
  }
}

void replicated_component_on_client_change(ecs::EntityId eid, ecs::component_index_t cidx)
{
  if (replicated_component_client_modify_blacklist.count(g_entity_mgr->getDataComponents().getComponentTpById(cidx)) <= 0)
    return;
  BlacklistedComponentInfo &info = blacklisted_components[make_bl_info_key(eid, cidx)];
  ++info.changeCounter;
  if ((info.deserializeCounter > 0) && (info.changeCounter > info.deserializeCounter))
  {
    // This component was already deserialized and changed on client more times than it was deserialized, log error.
    info.deserializeCounter = info.changeCounter + 1; // We don't want to spam error messages.
    logerr("entity %d(templ='%s'): modifying replicated component %d('%s') from client (2), this is "
           "blacklisted by _replicatedComponentClientModifyBlacklist",
      (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid), (int)cidx,
      g_entity_mgr->getDataComponents().getComponentNameById(cidx));
  }
}

void replicated_component_on_client_destroy(ecs::EntityId eid)
{
  auto it = blacklisted_components.lower_bound(make_bl_info_key(eid, 0)), end = blacklisted_components.end();
  if (it == end || (it->first >> 32) != (ecs::entity_id_t)eid)
    return;
  auto ite = it + 1;
  for (; ite != end && (ite->first >> 32) == (ecs::entity_id_t)eid; ++ite)
    ;
  G_FAST_ASSERT(blacklisted_components.validate_iterator(ite) & eastl::isf_valid);
  blacklisted_components.erase(it, ite);
}

} // namespace net

#endif // #if ECS_NET_COMP_BLACKLIST_ENABLED
