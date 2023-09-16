//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <memory/dag_genMemAlloc.h>
#include <daECS/core/entityManager.h>
#include <EASTL/type_traits.h>
#include <EASTL/hash_set.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/bitvector.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_multiset.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/functional.h>
#include "object.h" // for CompVersMap
#include "sequence.h"
#include "connid.h"
#include "scope_query_cb.h"
#include "netbase.h"
#include "reliabilitySys.h"

namespace danet
{
class BitStream;
}
namespace ecs
{
class Template;
}

namespace net
{

class Connection;
bool write_server_eid(ecs::entity_id_t eid, danet::BitStream &bs);
bool read_server_eid(ecs::entity_id_t &eid, const danet::BitStream &bs);

struct ReplicationAck;
struct ObjectReplica;
class EncryptionCtx;
struct InternedStrings
{
  ska::flat_hash_map<eastl::string, uint32_t> index;
  eastl::vector<eastl::string> strings; // if strings[index] - "", it is not synced. strings[0] = ''
  InternedStrings();
};
struct PendingReplicationPacketInfo;

// Object that actually manages replication. Exists one per each peer.
class Connection : public IConnection
{
public:
  typedef eastl::function<void(Connection &conn, ecs::entity_id_t serverEid)> on_object_constructed_cb_t;

  Connection(ConnectionId id_, scope_query_cb_t &&scope_query = scope_query_cb_t());
  ~Connection();

  void setEncryptionCtx(EncryptionCtx *ectx) { encryptionCtx = ectx; }

  ObjectReplica *addObjectInScope(Object &obj); // Note: there is no 'remove' operation, objects removed from scope automatically on
                                                // each update (unless 'setObjectInScopeAlways' is called)
  ecs::entity_id_t setObjectInScopeAlways(Object &obj); // returns serverId of added object (or 0/invalid if addition failed)
  bool setEntityInScopeAlways(ecs::EntityId eid) override final;
  void clearObjectInScopeAlways(Object &obj);

  void setReplicatingFrom(); // whether replicas get transmitted from this side of connection (typically called on server for client's
                             // connections)
  void setReplicatingTo(); // whether replicas are allowed to be created from other side of connection (typically called on client for
                           // server's connection)
  bool isReplicatingFrom() const;
  bool isReplicatingTo() const;

  enum PrepareWriteResult
  {
    PWR_NONE = 0,
    PWR_DESTRUCTION = 1,
    PWR_CONSTRUCTION = 2,
    PWR_REPLICATION = 4,
  };
  int prepareWritePackets();
  bool readReplayKeyFrame(const danet::BitStream &bs, const on_object_constructed_cb_t &obj_constructed_cb);
  void writeReplayKeyFrame(danet::BitStream &bs, danet::BitStream &tmpbs);
  bool writeConstructionPacket(danet::BitStream &bs, danet::BitStream &tmpbs, int limit_bytes);
  bool writeDestructionPacket(danet::BitStream &bs, danet::BitStream &tmpbs, int limit_bytes);
  bool writeReplicationPacket(danet::BitStream &bs, danet::BitStream &tmpbs, int limit_bytes); // return true if something usefull was
                                                                                               // written
  bool readReplicationPacket(const danet::BitStream &bs);
  bool readConstructionPacket(const danet::BitStream &bs, float compression_ratio,
    const on_object_constructed_cb_t &obj_constructed_cb);                                                    // delayed call
  bool readDestructionPacket(const danet::BitStream &bs, const on_object_constructed_cb_t &obj_destroyed_cb); // cb called immediately

  void sendAckedPacket(const danet::BitStream &bs, int cur_time, int timeout_ms, uint8_t channel);

  void update(int cur_time);

  void writeLastRecvdPacketAcks(danet::BitStream &bs);
  bool onAcksReceived(const danet::BitStream &bs, sequence_t &baseseq); // return false if read failed

  ConnectionId getId() const override final { return id; }
  bool isEntityInScope(ecs::EntityId eid) const override final { return entityIndexToReplicaIndex.count(eid.index()) != 0; }

  void disconnect(DisconnectionCause) override { connected = false; }

  void setUserPtr(void *ptr) override final { userPtr = ptr; }
  void *getUserPtr() const override final { return userPtr; }

  uint32_t getConnFlags() const override { return connFlags; }
  uint32_t &getConnFlagsRW() override { return connFlags; }

  ObjectReplica *getReplicaByEid(ecs::EntityId eid);
  const ObjectReplica *getReplicaByEid(ecs::EntityId eid) const;
  ObjectReplica *getReplicaByObj(const Object &obj);
  const ObjectReplica *getReplicaByObj(const Object &obj) const;

  void killReplicaConfirmed(ecs::entity_id_t server_id);

  static void collapseDirtyObjects();

  void setEncryptionKey(dag::ConstSpan<uint8_t> ekey, EncryptionKeyBits ebits) override;

private:
  void killObjectReplica(ObjectReplica *repl, net::Object &obj);
  void applyDelayedAttrsUpdate(ecs::entity_id_t server_eid);

  void pushToDirty(ObjectReplica *repl);
  void pushToNonDirty(ObjectReplica *repl);

  void freeObjectReplica(ObjectReplica *repl);
  void onPacketNotify(sequence_t seq, bool lost);
  void onReplicationAckPacketNotify(sequence_t seq, bool lost);
  void onEntityReplicationLost(ObjectReplica &replica, dag::ConstSpan<CompRevision> comprevs);

  static void onPacketDropped(sequence_t seq, void *ptr);
  static void onPacketAcked(sequence_t seq, void *ptr);

protected:
  bool isFiltering = true; // this connection is filtering components. That is used for debug (when we want client to have all
                           // infornation) and for replay connection
private:
  bool connected = true; // might be false when disconnect requested, but not confirmed yet
  ConnectionId id;
  mutable InternedStrings objectKeys;

  ska::flat_hash_map<uint16_t, uint16_t> entityIndexToReplicaIndex;

  eastl::vector<eastl::string> serverTemplates; // string table for templates
  uint32_t constructionPacketSequence = 0;

  int creationCurBytes = 0;
  int creationLastSentTime = 0;

  // If there is componentsSynced[server_cidx] is true
  //  serverToClientCidx[server_cidx] maps to client component. if serverToClientCidx[server_cidx] == invalid, then server component
  //  use unknown type, and protocol is severily different
  SmallTab<SmallTab<ecs::component_index_t>> clientTemplatesComponents; // only on client. direct map of templates to server components
                                                                        // index (in serverToClientCidx)
  eastl::vector<ecs::component_index_t> serverToClientCidx; // only on client. Direct map of server to client component indices.
  eastl::bitvector<> componentsSynced; // if component_index is already relpicated to/from, it will be true here. Both client and
                                       // server
  SmallTab<ecs::template_t> serverTemplatesIdx;          // only on server. indices of written templates
  SmallTab<ecs::template_t> serverIdxToTemplates;        // reverse for serverTemplatesIdx
  eastl::vector<uint16_t> serverTemplateComponentsCount; // only on server. addressed by serverTemplatesIdx[template_id].
  ecs::template_t syncedTemplate = 0;
  bool serializeComponentReplication(ecs::EntityId eid, const ecs::EntityComponentRef &attr, danet::BitStream &bs) const;
  bool deserializeComponentReplication(ecs::EntityId eid, const danet::BitStream &bs);

  const char *deserializeTemplate(const danet::BitStream &bs, ecs::template_t &server_template_id, bool &tpl_deserialized);
  bool syncReadComponent(ecs::component_index_t serverCidx, const danet::BitStream &bs, ecs::template_t templateId,
    bool produce_errors);
  bool syncReadTemplate(const danet::BitStream &bs, ecs::template_t templateId);

  enum class CanSkipInitial
  {
    No,
    Yes
  };
  void serializeConstruction(ecs::EntityId eid, danet::BitStream &bs, CanSkipInitial canSkipInitial = CanSkipInitial::Yes);
  void serializeTemplate(danet::BitStream &bs, ecs::template_t templateId, eastl::bitvector<> &componentsSyncedTmp) const;
  ecs::EntityId deserializeConstruction(const danet::BitStream &bs, ecs::entity_id_t serverId, uint32_t sz, float cratio,
    ecs::create_entity_async_cb_t &&cb);
  bool deserializeComponentConstruction(ecs::template_t server_template, const danet::BitStream &bs, ecs::ComponentsInitializer &init,
    int &out_ncomp);

  ecs::component_index_t resolveComponent(ecs::component_index_t server_cidx, ecs::component_t &name, ecs::component_type_t &type);

  enum
  {
    MaxReplicas = 24 << 10,
  };
  Tab<ObjectReplica> replicaRefs; // allocated array of ObjectReplica's, empty if replicating to
  Tab<ObjectReplica *> replicas;  // empty if replicating to; split onto 3 segments: dirty, in scope & free
  eastl::conditional_t<MaxReplicas <= USHRT_MAX, uint16_t, uint32_t> numReplicas = 0, // num used replicas in 'replicas' array
    numDirtyReplicas = 0; // num dirty replicas in 'replicas' array (<= numReplicas)
  uint32_t connFlags = 0;
  void *userPtr = NULL;
  EncryptionCtx *encryptionCtx = nullptr;

  scope_query_cb_t scopeQuery;
  ReliabilitySystem reliabilitySys;

  uint32_t lastCreationSeq = 0;

  struct DelayedAttrsUpdateRec
  {
    ecs::entity_id_t serverEid;
    mutable eastl::fixed_vector<SmallTab<uint8_t, MidmemAlloc>, 1> data;
  };
  struct DelayedAttrsUpdateRecHash
  {
    size_t operator()(const DelayedAttrsUpdateRec &val) const { return size_t(unsigned(val.serverEid * 2166136261U)); }
  };
  struct DelayedAttrsUpdateEqual
  {
    size_t operator()(const DelayedAttrsUpdateRec &a, const DelayedAttrsUpdateRec &b) const { return a.serverEid == b.serverEid; }
  };
  typedef eastl::hash_set<DelayedAttrsUpdateRec, DelayedAttrsUpdateRecHash, DelayedAttrsUpdateEqual> DelayedAttrsList;
  DelayedAttrsList delayedAttrsUpdate;

  eastl::vector<PendingReplicationPacketInfo> pendingReplInfo; // To consider: replace to deque (after update EASTL >= 3.13)

  enum
  {
    REPLICATING_INIT = 0,
    REPLICATING_TO = 1,
    REPLICATING_FROM = 2
  } replicatingType = REPLICATING_INIT;

  friend class Object;
  friend class ReplicationSys;
  friend class CNetwork;
  friend struct BitstreamDeserializer;
  friend struct BitstreamSerializer;
  friend struct ComponentFiltersHelper;
};

inline bool Connection::isReplicatingFrom() const { return replicatingType == REPLICATING_FROM; }
inline bool Connection::isReplicatingTo() const { return replicatingType == REPLICATING_TO; }

inline const ObjectReplica *Connection::getReplicaByEid(ecs::EntityId eid) const
{
  return const_cast<Connection *>(this)->getReplicaByEid(eid);
}
inline ObjectReplica *Connection::getReplicaByObj(const Object &obj) { return getReplicaByEid(obj.getEid()); }
inline const ObjectReplica *Connection::getReplicaByObj(const Object &obj) const { return getReplicaByEid(obj.getEid()); }

}; // namespace net
