//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/string.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include "connid.h"
#include "compver.h"

namespace danet
{
class BitStream;
}
namespace ecs
{
class EventEntityRecreated;
struct Event;
} // namespace ecs

namespace net
{
void clear_cached_replicated_components();

struct ObjectReplica;
class Connection;

typedef eastl::vector_map<ecs::component_index_t, compver_t, eastl::less<ecs::component_index_t>, EASTLAllocatorType,
  dag::Vector<eastl::pair<ecs::component_index_t, compver_t>>>
  CompVersMap;
struct CompRevision
{
  ecs::component_index_t compIdx;
  compver_t version;
};


// Replication Component (one per each entity)
class Object
{
public:
  ecs::EntityId getEid() const { return eid; }

  EA_NON_COPYABLE(Object);

  bool isReplica() const;
  ConnectionId getControlledBy() const { return controlledBy; }
  void setControlledBy(ConnectionId cid) { controlledBy = cid; }
  uint32_t getCreationOrder() const { return creationOrder; }

  Object(const ecs::EntityManager &mgr, ecs::EntityId eid_, const ecs::ComponentsMap &map);
  ~Object();

  static Object *getByEid(ecs::EntityId);
  bool meantToBeDestroyed() const { return do_not_verify_destruction | isMeantToBeDestroyed; }
  static void doNotVerifyDestruction(bool v) { do_not_verify_destruction = v; }
  static void clear_pending_destroys() { pending_destroys.clear(); }
  static bool remove_from_pending_destroys(ecs::EntityId eid) { return pending_destroys.erase(eid) != 0; }
  static void add_to_pending_destroys(ecs::EntityId eid) { pending_destroys.emplace(eid); }

private:
  void onCompChanged(ecs::component_index_t);

  void initCompVers();
  void addToDirty();
  void serializeComps(const Connection *conn, danet::BitStream &bs, const ObjectReplica &repl,
    eastl::vector<CompRevision> &comps_serialized) const;
  bool deserializeComps(const danet::BitStream &bs, Connection *conn);

  friend class Connection;
  friend struct ObjectReplica;
  friend void server_replication_cb(ecs::EntityId eid, ecs::component_index_t cidx);
  friend void client_validate_replication_cb(ecs::EntityId eid, ecs::component_index_t cidx);
  friend void replication_es_event_handler(const ecs::EventEntityRecreated &, net::Object &);
  friend void replication_validation_es_event_handler(const ecs::Event &, net::Object &);
  friend struct ComponentFiltersHelper;

  typedef eastl::vector_set<ecs::EntityId, eastl::less<ecs::EntityId>, EASTLAllocatorType,
    eastl::fixed_vector<ecs::EntityId, 32, /*bOverflow*/ true>>
    dirty_entities_list_t;
  static dirty_entities_list_t dirtyList; // list of objects that need to be updated
  CompVersMap compVers;                   // versions of active (i.e. changed at least once) replicated components
  ObjectReplica *replicasLinkList = NULL; // linked list of replicas for this object (one for each connection)
  ecs::EntityId eid;                      // id of entity this component belongs to
  ConnectionId controlledBy;
  uint32_t creationOrder;
  uint16_t filteredComponentsBits = 0; // up to 16 filters. can be easily extended to 32 or 64 bits
  bool isReplicaFlag = false; // it actually is not even a bool (isReplica). if it's replica or not can be determ by connection type
  bool isMeantToBeDestroyed = false; // to be written by server-driven code
  static bool do_not_verify_destruction;
  static eastl::vector_set<ecs::EntityId> pending_destroys;

  CompVersMap::iterator insertNewCompVer(ecs::component_index_t cidx, CompVersMap::iterator cit); // iterator in compVers, returns
                                                                                                  // position at compVers
  bool skipInitialReplication(ecs::component_index_t cidx, Connection *conn, ObjectReplica *replica);
  void forceReplicaVersion(CompVersMap::const_iterator cit, ecs::component_index_t cidx, ObjectReplica *replica) const;
};

inline bool Object::isReplica() const { return isReplicaFlag; }

}; // namespace net

ECS_DECLARE_RELOCATABLE_TYPE(net::Object);
