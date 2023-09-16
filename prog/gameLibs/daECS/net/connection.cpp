#include <daECS/net/serialize.h>
#include <daECS/net/connection.h>
#include <daECS/net/object.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/component.h>
#include <util/dag_stlqsort.h>
#include "objectReplica.h"
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>
#include <math/dag_adjpow2.h>
#include <generic/dag_tab.h>
#include <daNet/bitStream.h>
#include <math/random/dag_random.h>
#include <memory/dag_framemem.h>
#include "utils.h"
#include "encryption.h"
#include <daECS/core/template.h>

#define SCOPE_QUERYING_ENABLED 0 // Currently not used, hence disabled

#define REPL_VER(x) \
  if (!(x))         \
    return failRet;

namespace net
{
extern void update_dirty_component_filter_mask();

bool write_server_eid(ecs::entity_id_t eidVal, danet::BitStream &bs)
{
  // bs.WriteCompressed(eidVal);//unoptimized version
  // return true;
  // we optimize it by writing generation separately from idx
  ecs::EntityId id(eidVal);
  uint32_t index = id.index();
  uint32_t generation = ecs::get_generation(id);
  G_ASSERTF(generation <= UCHAR_MAX, "%u", eidVal);
  const bool isShortEid = index < (1 << 14);
  const bool isVeryCompressed = isShortEid && generation <= 1;
  if (isVeryCompressed)
  {
    uint16_t compressedData = 1;         // one bit
    compressedData |= (generation << 1); // one bit
    compressedData |= index << 2;        // 14 bit
    bs.Write(compressedData);            // 16 bits
  }
  else if (isShortEid)
  {
    // 3 bytes version will assume generation is less than 1<<7, and 4 bytes version that index is 1<<22 (which is currently limit!)
    uint16_t compressedIndex = 2; // less significant is zero meaning uncompressed, next one means that it's 3 byte version
    compressedIndex |= index << 2;
    bs.Write(compressedIndex);
    bs.Write((uint8_t)generation); // 16 bits
  }
  else // !isShortEid
  {
    G_ASSERTF(index < (1 << 22), "%u", eidVal);
    uint32_t compressedData = 0; // two zeroes at the end means uncompressed + 4byte version
    compressedData |= index << 2;
    compressedData |= generation << 24;
    bs.Write(uint16_t(compressedData));
    bs.Write(uint16_t(compressedData >> 16));
  }
  return true;
}

bool read_server_eid(ecs::entity_id_t &eidVal, const danet::BitStream &bs)
{
  // return bs.ReadCompressed(eidVal); //unoptimized version
  uint16_t first16Bit = 0;
  if (!bs.Read(first16Bit))
    return false;
  if (first16Bit & 1) // 2 byte version
    eidVal = ecs::make_eid(first16Bit >> 2, (first16Bit & 2) >> 1);
  else if (first16Bit & 2) // short eid: 3 byte version
  {
    uint8_t generation = 0;
    if (!bs.Read(generation))
      return false;
    eidVal = ecs::make_eid(first16Bit >> 2, generation);
  }
  else // long eid: 4 byte version
  {
    uint16_t tailData = 0;
    if (!bs.Read(tailData))
      return false;
    uint32_t compressedData = (uint32_t(tailData) << 16) | uint32_t(first16Bit);
    eidVal = ecs::make_eid((compressedData & 0x00ffffff) >> 2, compressedData >> 24);
  }
  return true;
}

void write_eid(danet::BitStream &bs, ecs::EntityId eid) { write_server_eid((ecs::entity_id_t)eid, bs); }

bool read_eid(const danet::BitStream &bs, ecs::EntityId &eid)
{
  ecs::entity_id_t serverEid;
  if (read_server_eid(serverEid, bs))
  {
    eid = ecs::EntityId(serverEid);
    return true;
  }
  return false;
}

namespace detail
{
bool read_value(const danet::BitStream &, ConnectionId &cid, const IConnection &conn)
{
  cid = static_cast<const net::Connection &>(conn).getId();
  return true;
}
} // namespace detail

#define MAKE_REPLICA_HANDLE(repl) ((unsigned((repl)->gen) << 16) | unsigned((repl)-replicaRefs.data()))
#define GET_REPLICA_BY_HANDLE(h)  ((replicaRefs[(h)&USHRT_MAX].gen == ((h) >> 16)) ? &replicaRefs[(h)&USHRT_MAX] : NULL)

struct PendingReplicationPacketInfo
{
  sequence_t waitSequence; // sequence we waiting for
  // To consider: profile actual numbers of number of entities/revs in real battle and use fixed_vector<> with proper fixed capacity as
  // necessary
  eastl::vector<eastl::pair<unsigned /*replicaHandle*/, unsigned /*revcount*/>> pendingReplicas;
  eastl::vector<CompRevision> compRevisions;
  PendingReplicationPacketInfo() = default;
  PendingReplicationPacketInfo(sequence_t seq) : waitSequence(seq) {}
};

Connection::Connection(ConnectionId id_, scope_query_cb_t &&scope_query) :
  id(id_), scopeQuery(eastl::move(scope_query)), reliabilitySys(grnd()) // randomize initial local sequence in order to avoid relying
                                                                        // on lack of sequence oveflowing
{
  G_ASSERT(id != net::INVALID_CONNECTION_ID);
  G_ASSERT(SCOPE_QUERYING_ENABLED || !scopeQuery);
}

Connection::~Connection()
{
  if (isReplicatingFrom()) // remove references to this connection's replicas from object's linked list
    for (int i = 0; i < numReplicas; ++i)
      if (net::Object *obj = replicas[i]->isToKill() ? nullptr : Object::getByEid(replicas[i]->getEid())) // if object marked for kill
                                                                                                          // then it's already detached
                                                                                                          // (see killObjectReplica)
        replicas[i]->detachFromObj(*obj);
}

ObjectReplica *Connection::getReplicaByEid(ecs::EntityId eid)
{
  auto it = entityIndexToReplicaIndex.find(eid.index()); // To consider: check for index overflow?
  if (it == entityIndexToReplicaIndex.end())
    return nullptr;
  ObjectReplica *repl = &replicaRefs[it->second];
#if DAECS_EXTENSIVE_CHECKS
  G_FAST_ASSERT(repl->getEid() == eid);
#endif
  return repl;
}

ObjectReplica *Connection::addObjectInScope(Object &obj)
{
  if (!isReplicatingFrom())
    return NULL;
  ObjectReplica *repl = getReplicaByObj(obj);
  if (repl)
  {
    repl->flags |= ObjectReplica::InScope;
    return repl;
  }
  if (numReplicas >= MaxReplicas)
    return NULL;
  uint32_t eidIndex = obj.getEid().index();
  if (eidIndex > eastl::numeric_limits<decltype(entityIndexToReplicaIndex)::key_type>::max())
    return NULL;

  // allocate new replica
  repl = replicas[numReplicas++];
  G_ASSERT(repl->isFree());
  repl->eidStorage = (ecs::entity_id_t)obj.getEid();
  repl->cmp = uint32_t(MAX_OBJ_CREATION_SEQ - (lastCreationSeq++ & MAX_OBJ_CREATION_SEQ)) | // Inverted for reverse iteration
              (uint32_t(ObjectReplica::InScope | ObjectReplica::NotYetReplicated) << 24);   // Note: 'flags' is union with MSB of cmp
  repl->remoteCompVers.assign(obj.compVers.begin(), obj.compVers.end());

  // attach to list of all replicas for this object
  repl->nextRepl = obj.replicasLinkList;
  repl->prevRepl = NULL;
  if (obj.replicasLinkList)
    obj.replicasLinkList->prevRepl = repl;
  obj.replicasLinkList = repl;

  uint32_t replIndex = (uint32_t)eastl::distance(replicaRefs.begin(), repl);
  G_ASSERT(MaxReplicas <= (USHRT_MAX + 1) || replIndex <= USHRT_MAX);
  G_VERIFY(entityIndexToReplicaIndex.emplace(eidIndex, replIndex).second); // shall not exist already

  pushToDirty(repl); // by default new in scope object considered dirty

  return repl;
}

void Connection::killObjectReplica(ObjectReplica *repl, Object &obj)
{
  if (!isReplicatingFrom())
    return;
  repl->detachFromObj(obj);
  repl->flags |= ObjectReplica::ToKill;
  G_VERIFY(entityIndexToReplicaIndex.erase(repl->getEid().index()));
  repl->prevRepl = repl->nextRepl = NULL;

  pushToDirty(repl);
}

void Connection::pushToNonDirty(ObjectReplica *repl)
{
  G_ASSERT(isReplicatingFrom());
  G_ASSERT(replicas[repl->arrayIndex] == repl);
  G_ASSERT(repl->arrayIndex < numDirtyReplicas);
  G_ASSERT(!repl->isFree());
  numDirtyReplicas--;
  if (repl->arrayIndex != numDirtyReplicas)
  {
    eastl::swap(replicas[repl->arrayIndex], replicas[numDirtyReplicas]);
    eastl::swap(replicas[repl->arrayIndex]->arrayIndex, replicas[numDirtyReplicas]->arrayIndex);
  }
}

void Connection::pushToDirty(ObjectReplica *repl)
{
  DAECS_EXT_ASSERT(isReplicatingFrom() && repl->arrayIndex < numReplicas && replicas[repl->arrayIndex] == repl);
  G_ASSERT(!repl->isFree());
  if (repl->arrayIndex < numDirtyReplicas) // already in dirty segment
    return;
  eastl::swap(replicas[repl->arrayIndex], replicas[numDirtyReplicas]);
  eastl::swap(replicas[repl->arrayIndex]->arrayIndex, replicas[numDirtyReplicas]->arrayIndex);
  numDirtyReplicas++;
  G_ASSERT(numDirtyReplicas <= numReplicas);
}

void Connection::freeObjectReplica(ObjectReplica *repl)
{
  DAECS_EXT_ASSERT(isReplicatingFrom() && repl->arrayIndex < numReplicas && replicas[repl->arrayIndex] == repl);
  G_ASSERT(!repl->isFree());
  if (repl->arrayIndex < numDirtyReplicas)
    pushToNonDirty(repl);
  numReplicas--;
  eastl::swap(replicas[repl->arrayIndex], replicas[numReplicas]);
  eastl::swap(replicas[repl->arrayIndex]->arrayIndex, replicas[numReplicas]->arrayIndex);
  repl->flags = ObjectReplica::Free;
  CompVersMap().swap(repl->remoteCompVers); // clear
  repl->gen++;                              // invalidate all handles to this replica
}

bool Connection::setEntityInScopeAlways(ecs::EntityId eid)
{
  net::Object *robj = net::Object::getByEid(eid);
  return robj != nullptr && setObjectInScopeAlways(*robj) != ecs::ECS_INVALID_ENTITY_ID_VAL;
}

ecs::entity_id_t Connection::setObjectInScopeAlways(Object &obj)
{
  if (!isReplicatingFrom())
    return ecs::ECS_INVALID_ENTITY_ID_VAL;
  if (ObjectReplica *repl = addObjectInScope(obj))
  {
    repl->flags |= ObjectReplica::AlwaysInScope;
    return (ecs::entity_id_t)obj.eid;
  }
  else
    return ecs::ECS_INVALID_ENTITY_ID_VAL;
}

void Connection::clearObjectInScopeAlways(Object &obj)
{
  if (!isReplicatingFrom())
    return;
  if (ObjectReplica *repl = getReplicaByObj(obj))
  {
    repl->flags &= ~ObjectReplica::AlwaysInScope;
    pushToDirty(repl); // object might need to be killed
  }
}

void Connection::setReplicatingFrom()
{
  G_ASSERT(!isReplicatingTo());
  if (isReplicatingFrom())
    return;

  replicatingType = REPLICATING_FROM;
  G_STATIC_ASSERT(MaxReplicas <= eastl::numeric_limits<decltype(ObjectReplica::arrayIndex)>::max());
  replicaRefs.resize(MaxReplicas);
  replicas.resize(MaxReplicas);
  int i = 0;
  for (ObjectReplica &rr : replicaRefs)
  {
    rr.arrayIndex = i;
    rr.conn = this;
    rr.flags = ObjectReplica::Free;
    rr.gen = 0;
    replicas[i] = &rr;
    i++;
  }

  lastCreationSeq = 0;
}

void Connection::setReplicatingTo()
{
  G_ASSERT(!isReplicatingFrom());
  if (isReplicatingTo())
    return;
  replicatingType = REPLICATING_TO;
  lastCreationSeq = 0;
}

/* static */
void Connection::collapseDirtyObjects()
{
  update_dirty_component_filter_mask();
  for (ecs::EntityId dirtyEid : Object::dirtyList)
  {
    Object *obj = Object::getByEid(dirtyEid);
    for (ObjectReplica *repl = obj->replicasLinkList; repl; repl = repl->nextRepl)
      repl->conn->pushToDirty(repl);
  }
  Object::dirtyList.getContainer().clear(/*freeOverflow*/ true);
}

int Connection::prepareWritePackets()
{
  if (!isReplicatingFrom() || !numDirtyReplicas)
    return PWR_NONE;

  if (SCOPE_QUERYING_ENABLED && scopeQuery)
  {
    dag::Span<ObjectReplica *> dirtyReplicas(replicas.data(), numDirtyReplicas);
    for (ObjectReplica *repl : dirtyReplicas)
      if (!(repl->flags & ObjectReplica::AlwaysInScope))
        repl->flags &= ~ObjectReplica::InScope;

    scopeQuery(this);
  }

  int res = PWR_NONE;
  for (int i = numDirtyReplicas - 1; i >= 0; --i) // reverse iteration to be able remove (freeObjectReplica)
  {
    ObjectReplica *repl = replicas[i];

    // mark for kill replicas that wasn't added in scope
    if (!(repl->flags & ObjectReplica::InScope))
      killObjectReplica(replicas[i], *net::Object::getByEid(replicas[i]->getEid()));

    // free replicas that already marked for kill but not yet replicated
    if ((repl->flags & (ObjectReplica::ToKill | ObjectReplica::NotYetReplicated)) ==
        (ObjectReplica::ToKill | ObjectReplica::NotYetReplicated))
      freeObjectReplica(repl);
    else
    {
      if (repl->isToKill())
        res |= PWR_DESTRUCTION;
      else if (repl->isNeedInitialUpdate())
        res |= PWR_CONSTRUCTION;
      else
        res |= PWR_REPLICATION;
    }
  }

  if (numDirtyReplicas > 1)
  {
    dag::Span<ObjectReplica *> dirtyReplicas(replicas.data(), numDirtyReplicas);
    stlsort::sort_branchless(dirtyReplicas.begin(), dirtyReplicas.end(),
      [](const ObjectReplica *a, const ObjectReplica *b) { return a->cmp < b->cmp; });
    int ai = 0;
    for (ObjectReplica *repl : dirtyReplicas)
      repl->arrayIndex = ai++;
  }

  return res;
}

template <typename F>
static inline uint32_t write_block(danet::BitStream &bs, danet::BitStream &tmpbs, const F &writefn)
{
  memset(tmpbs.GetData(), 0, tmpbs.GetNumberOfBytesUsed());
  tmpbs.ResetWritePointer();

  writefn(tmpbs);

  uint32_t blockSizeBytes = tmpbs.GetNumberOfBytesUsed();
  if (blockSizeBytes)
  {
    bs.WriteCompressed(blockSizeBytes);
    bs.Write((const char *)tmpbs.GetData(), blockSizeBytes);
  }

  return blockSizeBytes;
}

bool Connection::writeDestructionPacket(danet::BitStream &bs, danet::BitStream & /*tmpbs*/, int limit_bytes)
{
  G_ASSERT(isReplicatingFrom());
  if (!numDirtyReplicas)
    return false;

  BitSize_t afterHdrPos = bs.GetWriteOffset();
  int written = 0;
  bs.Write((uint8_t)0);

  BitSize_t limitBits = BYTES_TO_BITS(limit_bytes);
  for (int i = numDirtyReplicas - 1; i >= 0 && bs.GetNumberOfBitsUsed() < limitBits; --i) // reverse iteration to be able delete dirty
                                                                                          // replicas without breaking sort order
  {
    ObjectReplica *repl = replicas[i];
    G_FAST_ASSERT(!repl->isFree());
    if (!repl->isToKill())
    {
      // all to kill replicas should be in the end of 'replicas' (sorting invariant)
#if DAECS_EXTENSIVE_CHECKS
      for (int j = i - 1; j >= 0; --j)
        G_ASSERTF(!replicas[j]->isToKill(), "%d: 0x%x", j, replicas[j]->flags);
#endif
      break;
    }

    net::write_server_eid(repl->eidStorage, bs);

    pushToNonDirty(repl);
    freeObjectReplica(repl);

    if (++written > UCHAR_MAX)
      break;
  }

  if (written)
  {
    bs.WriteAt(uint8_t(written - 1), afterHdrPos);
    return true;
  }
  else
    return false;
}

void Connection::writeReplayKeyFrame(danet::BitStream &bs, danet::BitStream &tmpbs)
{
  // Step 1. Save current network state
  SmallTab<ecs::template_t> serverTemplatesIdxSaved;
  SmallTab<ecs::template_t> serverIdxToTemplatesSaved;
  eastl::bitvector<> componentsSyncedSaved;
  eastl::vector<uint16_t> serverTemplateComponentsCountSaved;
  ecs::template_t syncedTemplateSaved = 0;
  InternedStrings objectKeysSaved;

  serverTemplatesIdx.swap(serverTemplatesIdxSaved);
  serverIdxToTemplates.swap(serverIdxToTemplatesSaved);
  componentsSynced.swap(componentsSyncedSaved);
  serverTemplateComponentsCount.swap(serverTemplateComponentsCountSaved);
  eastl::swap(objectKeys, objectKeysSaved);
  eastl::swap(syncedTemplate, syncedTemplateSaved);

  // Step 2. Sync all entities as fresh entities
  BitSize_t blockSizePos = bs.GetWriteOffset();
  uint16_t serializeBlockCount = 0;
  bs.Write(serializeBlockCount);
  for (int i = numDirtyReplicas; i < numReplicas; ++i)
  {
    ObjectReplica *repl = replicas[i];
    if (repl->isToKill() || repl->isNeedInitialUpdate())
      continue;

    ecs::EntityId eid = repl->getEid();
    if (g_entity_mgr->isLoadingEntity(eid))
      continue;

    ++serializeBlockCount;
    bs.Write(uint8_t(0));
    net::write_server_eid((ecs::entity_id_t)eid, bs);

    write_block(bs, tmpbs,
      [this, eid](danet::BitStream &bs2) { serializeConstruction(eid, bs2, net::Connection::CanSkipInitial::No); });
  }
  bs.WriteAt(serializeBlockCount, blockSizePos);

  // Step 3. Restore network state
  serverTemplatesIdx.swap(serverTemplatesIdxSaved);
  serverIdxToTemplates.swap(serverIdxToTemplatesSaved);
  componentsSynced.swap(componentsSyncedSaved);
  serverTemplateComponentsCount.swap(serverTemplateComponentsCountSaved);
  eastl::swap(objectKeys, objectKeysSaved);
  eastl::swap(syncedTemplate, syncedTemplateSaved);

  // Step 4. Serialize exist network templates
  eastl::bitvector<> componentsSyncedTmp;
  blockSizePos = bs.GetWriteOffset();
  serializeBlockCount = 0;
  bs.Write(serializeBlockCount);
  for (ecs::template_t i = 0, serverTemplateSize = serverIdxToTemplates.size(); i < serverTemplateSize; ++i)
  {
    serializeTemplate(bs, i, componentsSyncedTmp);
    ++serializeBlockCount;
  }
  bs.WriteAt(serializeBlockCount, blockSizePos);

  // Step 5. Serialize string to index
  serializeBlockCount = 0;
  blockSizePos = bs.GetWriteOffset();
  bs.Write(serializeBlockCount);
  for (auto const &it : objectKeys.index)
  {
    bs.Write(it.second);
    bs.Write((uint16_t)it.first.length());
    bs.Write(it.first.c_str(), it.first.length());
    ++serializeBlockCount;
  }
  bs.WriteAt(serializeBlockCount, blockSizePos);
}

bool Connection::readReplayKeyFrame(const danet::BitStream &bs, const on_object_constructed_cb_t &obj_constructed_cb)
{
  serverTemplates.clear();
  clientTemplatesComponents.clear();
  serverToClientCidx.clear();
  componentsSynced.clear();
  uint16_t serializeBlockCount = 0;
  const bool failRet = false;
  REPL_VER(bs.Read(serializeBlockCount));
  for (uint16_t i = 0; i < serializeBlockCount; ++i)
  {
    bool success = readConstructionPacket(bs, 0 /*compression_ratio*/, obj_constructed_cb);
    G_ASSERT_RETURN(success, false);
  }

  serverTemplates.clear();
  clientTemplatesComponents.clear();
  serverToClientCidx.clear();
  componentsSynced.clear();

  REPL_VER(bs.Read(serializeBlockCount));
  for (uint16_t i = 0; i < serializeBlockCount; ++i)
  {
    ecs::template_t templateId;
    bool tpl_deserialized;
    deserializeTemplate(bs, templateId, tpl_deserialized);
    G_ASSERT_RETURN(tpl_deserialized, false);
  }

  auto objectKeys_index = objectKeys.index;
  auto objectKeys_strings = objectKeys.strings;
  objectKeys.index.clear();
  objectKeys.strings.clear();

  REPL_VER(bs.Read(serializeBlockCount));
  for (int i = 0; i < serializeBlockCount; ++i)
  {
    uint32_t key;
    uint16_t size;
    REPL_VER(bs.Read(key));
    REPL_VER(bs.Read(size));
    eastl::string str(size, '\0');
    REPL_VER(bs.Read(str.data(), size));
    if (objectKeys.strings.size() <= key)
      objectKeys.strings.resize(key + 1);
    objectKeys.strings[key] = str;
    objectKeys.index.emplace(str, key);
  }
  return true;
}

bool Connection::writeConstructionPacket(danet::BitStream &bs, danet::BitStream &tmpbs, int limit_bytes)
{
  G_ASSERT(isReplicatingFrom());
  if (!numDirtyReplicas)
    return false;

  BitSize_t afterHdrPos = bs.GetWriteOffset();
  int written = 0;
  bs.Write((uint8_t)0);

  BitSize_t limitBits = BYTES_TO_BITS(limit_bytes);
  for (int i = numDirtyReplicas - 1; i >= 0 && bs.GetNumberOfBitsUsed() < limitBits; --i) // reverse iteration to be able delete dirty
                                                                                          // replicas without breaking sort order
  {
    ObjectReplica *repl = replicas[i];
    G_FAST_ASSERT(!repl->isFree());
    if (repl->isToKill())
      continue; // handled by writeDestructionPacket
    else if (!repl->isNeedInitialUpdate())
    {
      // all to create/kill replicas should be in the end of 'replicas' (sorting invariant)
#if DAECS_EXTENSIVE_CHECKS
      for (int j = i - 1; j >= 0; --j)
        G_ASSERTF(!replicas[j]->isNeedInitialUpdate() && !replicas[j]->isToKill(), "%d: 0x%x", j, replicas[j]->flags);
#endif
      break;
    }
    ecs::EntityId eid = repl->getEid();
    DAECS_EXT_ASSERTF(!g_entity_mgr->isLoadingEntity(eid), "%d", (ecs::entity_id_t)eid);
    net::write_server_eid((ecs::entity_id_t)eid, bs);

    uint32_t blockSizeBytes = write_block(bs, tmpbs, [this, eid](danet::BitStream &bs2) { serializeConstruction(eid, bs2); });

    DAECS_EXT_ASSERT(blockSizeBytes);
    G_UNUSED(blockSizeBytes);
#if DAECS_EXTENSIVE_CHECKS
    if (blockSizeBytes >= (1 << 14))
      logwarn("Construction packet of entity %d(%s) is %dK!", (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid),
        blockSizeBytes >> 10);
#endif

    pushToNonDirty(repl);

    repl->flags &= ~ObjectReplica::NotYetReplicated;

    if (++written > UCHAR_MAX)
      break;
  }

  if (written)
  {
    bs.WriteAt(uint8_t(written - 1), afterHdrPos);
    return true;
  }
  else
    return false;
}

bool Connection::writeReplicationPacket(danet::BitStream &bs, danet::BitStream &tmpbs, int limit_bytes)
{
  G_ASSERT(isReplicatingFrom());
  if (!numDirtyReplicas)
    return false;

  sequence_t localSequence = reliabilitySys.getLocalSequence();

  bs.Write(localSequence);

  BitSize_t afterHdrPos = bs.GetWriteOffset();
  G_UNUSED(afterHdrPos);

  PendingReplicationPacketInfo pendingPacketRepl(localSequence);

  BitSize_t limitBits = BYTES_TO_BITS(limit_bytes);
  constexpr bool exist = true;
  uint32_t prevCompRevsSize = 0;
  for (int i = numDirtyReplicas - 1; i >= 0 && bs.GetNumberOfBitsUsed() < limitBits; --i) // reverse iteration to be able delete dirty
                                                                                          // replicas without breaking sort order
  {
    ObjectReplica *repl = replicas[i];
    if (repl->isNeedInitialUpdate() || repl->isToKill()) // such replicas are handled by 'writeConstructionPacket'
      continue;
    // Don't try to replicate entity until it's loading is done (as it might be consequence of re-creation with resources,
    // and re-creation might add or remove replication components)
    if (g_entity_mgr->isLoadingEntity(repl->getEid()))
      continue;

    BitSize_t startPos = bs.GetWriteOffset();
    bs.Write(exist);
    net::write_server_eid(repl->eidStorage, bs);

    const net::Object *robj = Object::getByEid(repl->getEid());
    uint32_t blockSizeBytes = write_block(bs, tmpbs, [this, robj, &pendingPacketRepl, repl](danet::BitStream &bs2) {
      robj->serializeComps(this, bs2, *repl, pendingPacketRepl.compRevisions);
    });
    G_UNUSED(blockSizeBytes);
    if (pendingPacketRepl.compRevisions.size() == prevCompRevsSize) // nothing was written?
    {
      G_ASSERT(blockSizeBytes == 0);
      pushToNonDirty(repl);
      bs.SetWriteOffset(startPos);
      continue;
    }
    G_ASSERT(blockSizeBytes != 0);
    if (bs.GetNumberOfBitsUsed() > limitBits && prevCompRevsSize) // over the limit (except for the first object)?
    {
      // rewind to prev
      bs.SetWriteOffset(startPos);
      pendingPacketRepl.compRevisions.resize(prevCompRevsSize);
      break;
    }
    else
    {
      uint32_t revcount = pendingPacketRepl.compRevisions.size() - prevCompRevsSize;
      pendingPacketRepl.pendingReplicas.push_back(eastl::make_pair(MAKE_REPLICA_HANDLE(repl), revcount));

      // Update to new version immediately until nack or timeout occurs (optimistic replication)
      for (auto &crev : make_span_const(&pendingPacketRepl.compRevisions[prevCompRevsSize], revcount))
      {
        auto ins = repl->remoteCompVers.insert(crev.compIdx);
        G_ASSERTF(!ins.second, "%d %u %p", crev.compIdx, pendingPacketRepl.pendingReplicas.back().first,
          repl); // shall not change container size (see debugVerifyRemoteCompVers())
        ins.first->second = crev.version;
      }

      prevCompRevsSize = pendingPacketRepl.compRevisions.size();
      pushToNonDirty(repl);
    }
  }

  G_ASSERT(pendingPacketRepl.compRevisions.empty() == (bs.GetWriteOffset() == afterHdrPos));
  if (!pendingPacketRepl.compRevisions.empty())
  {
    bs.Write(!exist); // term bit
    pendingReplInfo.push_back(eastl::move(pendingPacketRepl));
    return true;
  }
  else
    return false;
}

void Connection::sendAckedPacket(const danet::BitStream &bs, int cur_time, int timeout_ms, uint8_t channel)
{
  auto reliability = UNRELIABLE_SEQUENCED; // TODO: do something more smart then just drop out-of-order packets
  G_VERIFY(send(cur_time, bs, MEDIUM_PRIORITY, reliability, channel));
  if (!isBlackHole())
    reliabilitySys.packetSent(cur_time, timeout_ms);
  else
    onPacketNotify(reliabilitySys.getLocalSequence(), /*lost*/ false);
}

void Connection::applyDelayedAttrsUpdate(ecs::entity_id_t server_eid)
{
  G_ASSERT(isReplicatingTo());
  auto it = delayedAttrsUpdate.find(DelayedAttrsUpdateRec{server_eid});
  if (it != delayedAttrsUpdate.end())
  {
    net::Object *nobj = Object::getByEid(ecs::EntityId(server_eid));
    G_FAST_ASSERT(nobj);
    G_FAST_ASSERT(!it->data.empty());
    for (auto &data : it->data)
    {
      danet::BitStream bs2(data.data(), data.size(), false);
      G_VERIFY(nobj->deserializeComps(bs2, this));
    }
    delayedAttrsUpdate.erase(it);
  }
}

bool Connection::readDestructionPacket(const danet::BitStream &bs, const on_object_constructed_cb_t &obj_destroyed_cb)
{
  G_ASSERT(isReplicatingTo());

  const bool failRet = false;

  uint8_t written = 0;
  REPL_VER(bs.Read(written));

  for (int i = 0, n = written + 1; i < n; ++i)
  {
    ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
    REPL_VER(read_server_eid(serverEid, bs));
    const ecs::EntityId resolvedEid(serverEid);
#if DAECS_EXTENSIVE_CHECKS
    if (EASTL_LIKELY(g_entity_mgr->doesEntityExist(resolvedEid)))
#else
    if (EASTL_LIKELY(g_entity_mgr->destroyEntity(resolvedEid)))
#endif
    {
      if (Object *obj = Object::getByEid(resolvedEid)) // entity destruction is always deferred, so this get after destroy is fine
        obj->isMeantToBeDestroyed = true;
      else
#if DAECS_EXTENSIVE_CHECKS // Note: don't do this check/logerr in release since in theory this might due to tampered server -> client
                           // network packet
      {
        if (!g_entity_mgr->getEntityFutureTemplateName(resolvedEid))
          logerr("Attempt to destroy unknown networking entity %d", (ecs::entity_id_t)resolvedEid);
        Object::add_to_pending_destroys(resolvedEid);
      }
      g_entity_mgr->destroyEntity(resolvedEid);
#else
      {
        Object::add_to_pending_destroys(resolvedEid);
      }
#endif
    }
    else
      logerr("Entity %d does not exist (already destroyed?). Only server has the authority to destroy network entities.",
        ecs::entity_id_t(resolvedEid));
    delayedAttrsUpdate.erase(DelayedAttrsUpdateRec{serverEid}); // might exist some data if entity was killed before creation (but
                                                                // after EntityId (handle) allocation)
    obj_destroyed_cb(*this, serverEid);
  }

  return true;
}

bool Connection::readConstructionPacket(const danet::BitStream &bs, float compression_ratio,
  const on_object_constructed_cb_t &obj_constructed_cb)
{
  G_ASSERT(isReplicatingTo());

  auto failRet = false;

  uint8_t written = 0;
  REPL_VER(bs.Read(written));

  for (int i = 0, n = written + 1; i < n; ++i)
  {
    ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
    REPL_VER(read_server_eid(serverEid, bs));
    ecs::EntityId resolvedEid(serverEid);
    uint32_t blockSizeBytes = 0;
    REPL_VER(bs.ReadCompressed(blockSizeBytes));
    G_ASSERT(blockSizeBytes > 0);
    BitSize_t startPos = bs.GetReadOffset();
    if (EASTL_LIKELY(g_entity_mgr->getEntityTemplateId(resolvedEid) == ecs::INVALID_TEMPLATE_INDEX))
    {
      ecs::EntityId eid = deserializeConstruction(bs, serverEid, blockSizeBytes, compression_ratio,
        [this, obj_constructed_cb, serverEid](ecs::EntityId) { obj_constructed_cb(*this, serverEid); });
      if (!eid)
        logerr("Construction of entity of eid %d failed", serverEid);
    }
    else
      logerr("Attempt to create already existing network entity %d<%s>", serverEid, g_entity_mgr->getEntityTemplateName(resolvedEid));
    bs.SetReadOffset(startPos + BYTES_TO_BITS(blockSizeBytes)); // rewind to next block
  }

#if DAECS_EXTENSIVE_CHECKS
  constructionPacketSequence++;
#endif

  return true;
}

bool Connection::readReplicationPacket(const danet::BitStream &bs)
{
  G_ASSERT(isReplicatingTo());

  constexpr bool failRet = false;
  sequence_t remoteSequence = sequence_t(-1);
  REPL_VER(bs.Read(remoteSequence));

  do
  {
    bool exists = false;
    REPL_VER(bs.Read(exists));
    if (!exists)
      break;
    ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
    REPL_VER(read_server_eid(serverEid, bs));
    uint32_t blockSizeBytes = 0;
    REPL_VER(bs.ReadCompressed(blockSizeBytes));
    G_ASSERT(blockSizeBytes > 0);
    BitSize_t startPos = bs.GetReadOffset();

    Object *nobj = Object::getByEid(ecs::EntityId(serverEid));
    if (nobj && !delayedAttrsUpdate.count(DelayedAttrsUpdateRec{serverEid}))
    {
      REPL_VER(nobj->deserializeComps(bs, this));
      G_ASSERT(BITS_TO_BYTES(bs.GetReadOffset() - startPos) == blockSizeBytes);
    }
    else // attrs update for missing local object (assume that creation packet is not received yet, and save it for future apply)
    {
      auto it = delayedAttrsUpdate.insert(DelayedAttrsUpdateRec{serverEid}).first;
      SmallTab<uint8_t, MidmemAlloc> &tab = it->data.emplace_back();
      clear_and_resize(tab, blockSizeBytes);
      G_VERIFY(bs.ReadBits(&tab[0], BYTES_TO_BITS(blockSizeBytes)));
    }
    bs.SetReadOffset(startPos + BYTES_TO_BITS(blockSizeBytes)); // rewind to next block
  } while (1);

  reliabilitySys.packetReceived(remoteSequence);

  return true;
}

void Connection::writeLastRecvdPacketAcks(danet::BitStream &bs)
{
  G_ASSERT(isReplicatingTo());
  auto ackData = reliabilitySys.genLastSeqAckData();
  bs.Write(ackData.rseq);
  bs.Write(ackData.ackBits);
}

bool Connection::onAcksReceived(const danet::BitStream &bs, sequence_t &baseseq)
{
  G_ASSERT(isReplicatingFrom());
  ack_bits_t ackBits = 0;
  if (!bs.Read(baseseq) || !bs.Read(ackBits))
    return false;
  reliabilitySys.processAck(baseseq, ackBits, onPacketAcked, onPacketDropped, this);
  return true;
}

/* static */ void Connection::onPacketDropped(sequence_t seq, void *ptr) { ((Connection *)ptr)->onPacketNotify(seq, true); }
/* static */ void Connection::onPacketAcked(sequence_t seq, void *ptr) { ((Connection *)ptr)->onPacketNotify(seq, false); }

void Connection::onPacketNotify(sequence_t seq, bool lost)
{
  if (!isReplicatingFrom())
    return;
  onReplicationAckPacketNotify(seq, lost);
}

void Connection::onEntityReplicationLost(ObjectReplica &replica, dag::ConstSpan<CompRevision> comprevs)
{
  bool dirty = false;
  for (auto &crev : comprevs)
  {
    auto it = replica.remoteCompVers.find(crev.compIdx);
    if (it == replica.remoteCompVers.end())
      ; // Lost ack for unknown component. Probably entity was re-created while this ack was in flight, not a big deal
    else if (it->second == crev.version) // component wasn't changed since replication of this ack took place (i.e. still has same
                                         // ver)?
    {
      --it->second; // replicate it again then
      dirty = true;
    }
    else
      ; // This component was changed while this ack was in flight, new ack was issued, this ack is not relevant anymore
  }
  if (dirty)
    pushToDirty(&replica);
}

void Connection::onReplicationAckPacketNotify(sequence_t seq, bool lost)
{
  G_ASSERT(isReplicatingFrom());

  for (auto &curPendInfo : pendingReplInfo)
    if (curPendInfo.waitSequence == seq)
    {
      if (EASTL_UNLIKELY(lost))
      {
        uint32_t curOffs = 0;
        for (auto &pendRepl : curPendInfo.pendingReplicas)
        {
          if (ObjectReplica *replica = GET_REPLICA_BY_HANDLE(pendRepl.first))
            onEntityReplicationLost(*replica, make_span_const(&curPendInfo.compRevisions[curOffs], pendRepl.second));
          curOffs += pendRepl.second;
        }
        G_ASSERT(curOffs == curPendInfo.compRevisions.size());
      }

      pendingReplInfo.erase(&curPendInfo);
      return;
    }

  G_ASSERTF(0, "Failed to lookup pending packet replication info for seq %d", seq);
}

void Connection::update(int cur_time) { reliabilitySys.update(cur_time, onPacketDropped, this); }

void Connection::setEncryptionKey(dag::ConstSpan<uint8_t> ekey, EncryptionKeyBits ebits)
{
  if (encryptionCtx)
    encryptionCtx->setPeerKey(getId(), ekey, ebits);
}

} // namespace net
