// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bodiesCleanup.h"
#include <ecs/core/entityManager.h>
#include <EASTL/vector_set.h>
#include "game/player.h"
#include "game/dasEvents.h"
#include <daECS/net/object.h>

struct BodiesClnRec
{
  ecs::EntityId eid;
  float ttl;
  bool operator<(const BodiesClnRec &rhs) const { return eid < rhs.eid; }
};

typedef eastl::vector_set<BodiesClnRec> BodiesCleanup;
template <>
struct ecs::TypeComparable<BodiesCleanup>
{
  static constexpr bool value = false;
};

ECS_REGISTER_EVENT(CmdBodyCleanup);          // [0]float (ttl). if ttl < 0 - resurrects */
ECS_REGISTER_EVENT(CmdBodyCleanupUpdateTtl); // [0]float (new ttl)*/
ECS_DECLARE_RELOCATABLE_TYPE(BodiesCleanup);
ECS_REGISTER_RELOCATABLE_TYPE(BodiesCleanup, nullptr);
ECS_AUTO_REGISTER_COMPONENT(BodiesCleanup, "bodies_cleanup", nullptr, 0);

inline void bodies_cmd_cleanup_es_event_handler(const CmdBodyCleanup &evt, ecs::EntityId eid, net::Object *replication)
{
  if (replication && replication->isReplica()) // networking entities are cleaned by server
    return;
  ecs::EntityId bodiesEid = g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("bodies_cleanup"));
  if (!bodiesEid)
    return;
  BodiesCleanup &bodies_cleanup = ECS_GET_RW(BodiesCleanup, bodiesEid, bodies_cleanup);

  float ttl = evt.get<0>();
  if (ttl >= 0.f)
    bodies_cleanup.insert(BodiesClnRec{eid, ttl});
  else
    bodies_cleanup.erase(BodiesClnRec{eid, 0.f});
}

inline void update_body_cleanup_ttl_es(const CmdBodyCleanupUpdateTtl &evt, ecs::EntityId eid)
{
  ecs::EntityId bodiesEid = g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("bodies_cleanup"));
  if (!bodiesEid)
    return;
  BodiesCleanup &bodiesCleanup = ECS_GET_RW(BodiesCleanup, bodiesEid, bodies_cleanup);

  float newTtl = evt.get<0>();
  BodiesClnRec *body = bodiesCleanup.find(BodiesClnRec{eid, 0.f});
  if (body)
    body->ttl = newTtl;
}

inline void bodies_resurrected_es_event_handler(const EventAnyEntityResurrected &evt, BodiesCleanup &bodies_cleanup)
{
  bodies_cleanup.erase(BodiesClnRec{evt.get<0>(), 0.f});
}

ECS_NO_ORDER
static inline void bodies_cleanup_es(const ecs::UpdateStageInfoAct &info, BodiesCleanup &bodies_cleanup)
{
  for (auto bi = bodies_cleanup.begin(); bi != bodies_cleanup.end();)
  {
    BodiesClnRec &b = *bi;
    b.ttl -= info.dt;
    if (b.ttl > 0.f)
    {
      ++bi;
      continue;
    }
    if (!ECS_GET_OR(b.eid, canRemoveWhilePossessed, true))
    {
      const ecs::EntityId playerEid = game::find_player_eid_that_possess(b.eid);
      const bool isPossessed = playerEid != ecs::INVALID_ENTITY_ID;
      if (isPossessed)
      {
        const float recheck_possessed_ttl = 5.0f; // check 5 seconds later
        b.ttl = recheck_possessed_ttl;
        ++bi;
        continue;
      }
    }
    g_entity_mgr->destroyEntity(b.eid);
    bi = bodies_cleanup.erase(bi);
  }
}
