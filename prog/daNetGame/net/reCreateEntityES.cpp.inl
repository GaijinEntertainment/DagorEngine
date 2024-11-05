// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/reCreateEntity.h"
#include <daECS/net/netEvents.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <daECS/net/netbase.h>
#include <ecs/core/entityManager.h>

static constexpr char TNAME_ADD_REM_SEPARATOR = '|'; // Any char that cannot be in valid template name

ECS_UNICAST_EVENT_TYPE(CmdRemoteRecreate, SimpleString /*tname*/, danet::BitStream /*comps*/);
ECS_REGISTER_EVENT(CmdRemoteRecreate);
ECS_REGISTER_NET_EVENT(CmdRemoteRecreate, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, RELIABLE_ORDERED);

static void serialize_cmap(ecs::EntityManager &mgr, const ecs::ComponentsInitializer &cmap, danet::BitStream &bs)
{
  bs.WriteCompressed((uint16_t)cmap.size());
  for (auto &kv : cmap)
    net::serialize_comp_nameless(mgr, kv.name, kv.second.getEntityComponentRef(), bs);
}

static bool deserialize_cmap(ecs::EntityManager &mgr, ecs::ComponentsInitializer &cmap, const danet::BitStream &bs)
{
  uint16_t cnt;
  if (!bs.ReadCompressed(cnt))
    return false;
  for (int i = 0; i < cnt; ++i)
  {
    ecs::component_t componentNameHash = 0;
    if (ecs::MaybeChildComponent maybeComp = net::deserialize_comp_nameless(mgr, componentNameHash, bs))
      cmap[ecs::HashedConstString({"!net_replicated!", componentNameHash})] = eastl::move(*maybeComp);
    else
      return false;
  }
  return true;
}

static ecs::EntityId remote_recreate_impl(ecs::EntityId eid,
  const char *new_templ_name,
  const char *sync_templ_name,
  ecs::ComponentsInitializer &&inst_comps,
  remote_recreate_entity_async_cb_t &&cb)
{
  ecs::ComponentsInitializer cinit(inst_comps);
  return g_entity_mgr->reCreateEntityFromAsync(eid, new_templ_name, eastl::move(cinit), {},
    [=, cinit = eastl::move(inst_comps), syncTemplName = SimpleString(sync_templ_name), cb = eastl::move(cb)](
      ecs::EntityId eid) mutable {
      if (cb)
        cb(eid, cinit); // Note: cb might modify cinit (hence we call it before sending it to remote system)
      CmdRemoteRecreate evt(eastl::move(syncTemplName), danet::BitStream());
      serialize_cmap(*g_entity_mgr, cinit, evt.get<1>());
      g_entity_mgr->sendEvent(eid, eastl::move(evt));
    });
}

ecs::EntityId remote_recreate_entity_from(
  ecs::EntityId eid, const char *new_templ_name, ecs::ComponentsInitializer &&inst_comps, remote_recreate_entity_async_cb_t &&cb)
{
  if (!new_templ_name || !*new_templ_name || !is_server())
    return eid;
  return remote_recreate_impl(eid, new_templ_name, new_templ_name, eastl::move(inst_comps), eastl::move(cb));
}

ecs::EntityId remote_change_sub_template(ecs::EntityId eid,
  const char *remove_templ_name,
  const char *add_templ_name,
  ecs::ComponentsInitializer &&inst_comps,
  remote_recreate_entity_async_cb_t &&cb,
  bool force)
{
  const char *entTemplName = g_entity_mgr->getEntityFutureTemplateName(eid);
  if (!entTemplName || !is_server())
    return entTemplName ? eid : ecs::INVALID_ENTITY_ID;
  ECS_DEFAULT_RECREATE_STR_T remAddName, newTemplName = entTemplName;
  force |= !inst_comps.empty() || bool(cb);
  if (remove_templ_name && *remove_templ_name)
  {
    G_ASSERT_RETURN(!strchr(remove_templ_name, TNAME_ADD_REM_SEPARATOR), ecs::INVALID_ENTITY_ID);
    if (auto ntn = remove_sub_template_name(newTemplName.c_str(), remove_templ_name, force ? nullptr : ""); !ntn.empty())
    {
      newTemplName = ntn;
      remAddName = remove_templ_name;
      remAddName += TNAME_ADD_REM_SEPARATOR;
    }
  }
  if (add_templ_name && *add_templ_name)
  {
    G_ASSERT_RETURN(!strchr(add_templ_name, TNAME_ADD_REM_SEPARATOR), ecs::INVALID_ENTITY_ID);
    if (auto ntn = add_sub_template_name(newTemplName.c_str(), add_templ_name, force ? nullptr : ""); !ntn.empty())
    {
      newTemplName = ntn;
      if (remAddName.empty())
        remAddName += TNAME_ADD_REM_SEPARATOR;
      remAddName += add_templ_name;
    }
  }
  if (remAddName.empty())
    return eid;
  return remote_recreate_impl(eid, newTemplName.c_str(), remAddName.c_str(), eastl::move(inst_comps), eastl::move(cb));
}

ECS_TAG(netClient) // Intentionally not `gameClient` to avoid executing it in offline mode
static void remote_recreate_es(const CmdRemoteRecreate &evt, ecs::EntityId eid, ecs::EntityManager &manager)
{
  ecs::ComponentsInitializer cmap;
  if (DAGOR_UNLIKELY(!deserialize_cmap(manager, cmap, evt.get<1>())))
  {
    logerr("Failed to deserialize cmap while attempting to re-create (%s) entity %d<%s>", evt.get<0>().c_str(), (ecs::entity_id_t)eid,
      manager.getEntityTemplateName(eid));
    return;
  }
  auto &tname = const_cast<SimpleString &>(evt.get<0>());
  const char *reCreateName = tname.c_str();
  ECS_DEFAULT_RECREATE_STR_T newTemplName;
  if (char *tsep = strchr(tname.c_str(), TNAME_ADD_REM_SEPARATOR)) // Add/Remove?
  {
    newTemplName = g_entity_mgr->getEntityFutureTemplateName(eid);
    if (tsep != tname.c_str()) // Has remove?
    {
      *tsep = '\0';
      newTemplName = remove_sub_template_name(newTemplName.c_str(), tname.c_str());
    }
    if (tsep[1]) // Has add?
      newTemplName = add_sub_template_name(newTemplName.c_str(), tsep + 1);
    reCreateName = newTemplName.c_str();
  }
  manager.reCreateEntityFromAsync(eid, reCreateName, eastl::move(cmap));
}
