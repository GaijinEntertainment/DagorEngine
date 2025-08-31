// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <generic/dag_sort.h>
#include <daECS/net/message.h>
#include <daECS/net/netEvent.h>
#include <daNet/bitStream.h>
#include <EASTL/functional.h>
#include <EASTL/vector_map.h>
#include <math/dag_adjpow2.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <daECS/net/msgSink.h>
#include <util/dag_globDef.h>

namespace net
{

extern void clear_net_msg_handlers();
extern void register_net_msg_handler(const net::MessageClass &klass, net::msg_handler_t handler);

MessageClass *MessageClass::classLinkList = NULL;
int MessageClass::numClassIdBits = -1;

enum class MessageSysSyncState : uint8_t
{
  SERVER,         // sync in server mode
  NOT_SYNCED,     // client - not using message sync, it is either unused, or not yet connected to server
  WAITING_SYNC,   // client - connected to server, sent hashes, and waiting for sync data
  SYNCED_MATCHED, // client - synced, no data received, because messages on client and server match
  SYNCED_FULL,    // client - synced, server message data is received
  SYNC_FAILED,    // client - sync has failed
};
static MessageSysSyncState msgSysSyncState = MessageSysSyncState::NOT_SYNCED;
static uint64_t allMessagesHash = 0u;

static dag::Vector<const MessageClass *> incomingMessageById;

// client-to-server message hashes, then server-to-client message hashes
static dag::Vector<uint32_t> syncedServerMessageHashes;
static uint16_t syncedClientToServerMsgCount = 0u;

dag::Span<net::IConnection *> broadcast_rcptf(Tab<net::IConnection *> &, ecs::EntityId, const IMessage &)
{
  // no body, since it's never actually called (only address of this function is used)
  return dag::Span<net::IConnection *>();
}

dag::Span<net::IConnection *> direct_connection_rcptf(Tab<net::IConnection *> &, ecs::EntityId, const IMessage &msg)
{
  if (msg.connection)
    return make_span((net::IConnection **)&msg.connection, 1);
  else
    return dag::Span<net::IConnection *>();
}

MessageClass::MessageClass(const char *class_name, uint32_t class_hash, uint32_t class_sz, MessageRouting rout, bool timed,
  recipient_filter_t rcptf, PacketReliability rlb, uint8_t chn, uint32_t flags_, int dup_delay_ms,
  void (*msg_sink_handler)(const IMessage *)) :
  msgSinkHandler(msg_sink_handler), debugClassName(class_name), classHash(class_hash), memSize(class_sz)
{
  routing = rout, reliability = rlb;
  channel = chn;
  flags = flags_ | (timed ? MF_TIMED : 0);
  rcptFilter = rcptf;
  dupDelay = dup_delay_ms;
  G_FAST_ASSERT(memSize != 0);
  // add to list
  next = MessageClass::classLinkList;
  MessageClass::classLinkList = this;
}

/* static */
uint32_t MessageClass::initImpl(bool server, bool dbg_output_table)
{
  allMessagesHash = calcAllMessagesHash();

  if (!classLinkList)
    return 0;
  auto is_outgoing = [server](const MessageClass *cls) {
    MessageRouting rout = cls->routing;
    return server ? (rout == ROUTING_SERVER_TO_CLIENT)
                  : (rout == ROUTING_CLIENT_TO_SERVER || rout == ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER);
  };
  clear_net_msg_handlers();

  FRAMEMEM_REGION;
  FRAMEMEM_VALIDATE;

  using MessageByHashMap = eastl::vector_map<uint32_t, MessageClass *, eastl::less<uint32_t>, framemem_allocator>;
  MessageByHashMap outgoingMessagesMap;
  MessageByHashMap incomingMessagesMap;
  dag::Vector<MessageClass *, framemem_allocator> skippedMsgClasses;
  outgoingMessagesMap.reserve(64);
  incomingMessagesMap.reserve(64);
  G_ASSERT(!server || syncedServerMessageHashes.empty()); // when initializing for server, there must not be any synced message hashes
  if (!server)
    for (uint32_t i = 0; i < syncedServerMessageHashes.size(); i++)
      // their incoming - our outgoing
      (i < syncedClientToServerMsgCount ? outgoingMessagesMap : incomingMessagesMap).emplace(syncedServerMessageHashes[i], nullptr);
  uint32_t netMessagesCount = 0, maxMessageClassLen = 0;
  for (MessageClass *cls = classLinkList; cls; cls = cls->next)
  {
    MessageRouting rout = cls->routing;
    if (rout == ROUTING_SERVER_TO_CLIENT && !cls->rcptFilter)
    {
      logerr("Server to client message '%s'(%x) must have recipientFilter. Check message registration options.", cls->debugClassName,
        cls->classHash);
      continue;
    }
    const bool outgoing = is_outgoing(cls);
    MessageByHashMap &messageMap = outgoing ? outgoingMessagesMap : incomingMessagesMap;
    // if we are syncing messages, ignore messages, that are not registered on server
    if (EASTL_UNLIKELY(!syncedServerMessageHashes.empty() && !server && messageMap.find(cls->classHash) == messageMap.end()))
    {
      skippedMsgClasses.push_back(cls);
      continue;
    }
    // skip optional messages to re-add them later
    if (EASTL_UNLIKELY(cls->flags & MF_OPTIONAL))
    {
      skippedMsgClasses.push_back(cls);
      continue;
    }
    // put message in appropriate map, and check for hash collision
    MessageClass *&msgClsSlot = messageMap[cls->classHash];
    G_ASSERTF(msgClsSlot == nullptr, "Net messages hash collision %x for %s/%s!", cls->classHash, msgClsSlot->debugClassName,
      cls->debugClassName);
    msgClsSlot = cls;
    ++netMessagesCount;
    if (cls->msgSinkHandler && !outgoing)
      register_net_msg_handler(*cls, cls->msgSinkHandler);
    if (cls->debugClassName)
      maxMessageClassLen = eastl::max(maxMessageClassLen, uint32_t(strlen(cls->debugClassName)));
  }

  dag::Vector<const MessageClass *> outgoingMessagesList;
  dag::Vector<const MessageClass *> incomingMessagesList;
  outgoingMessagesList.resize_noinit(uint32_t(outgoingMessagesMap.size()));
  incomingMessagesList.resize_noinit(uint32_t(incomingMessagesMap.size()));
  {
    for (int i = 0; i < int(incomingMessagesMap.size()); ++i)
    {
      MessageClass *cls = incomingMessagesMap.data()[i].second;
      incomingMessagesList[i] = cls;
      if (cls != nullptr)
        cls->classId = int16_t(i); // for receival (getById)
    }
    for (int i = 0; i < int(outgoingMessagesMap.size()); ++i)
    {
      MessageClass *cls = outgoingMessagesMap.data()[i].second;
      outgoingMessagesList[i] = cls;
      if (cls != nullptr)
        cls->classId = int16_t(i); // for sending
    }
    G_ASSERT(syncedServerMessageHashes.empty() || outgoingMessagesList.size() == syncedClientToServerMsgCount);
    // sort and add skipped messages
    stlsort::sort_branchless(skippedMsgClasses.begin(), skippedMsgClasses.end(),
      [&](const MessageClass *a, const MessageClass *b) { return a->classHash < b->classHash; });
    for (MessageClass *cls : skippedMsgClasses)
    {
      const bool outgoing = is_outgoing(cls);
      dag::Vector<const MessageClass *> &list = outgoing ? outgoingMessagesList : incomingMessagesList;
      cls->classId = int16_t(list.size());
      list.push_back(cls);
    }
  }
  const int numMessageClasses = int(eastl::max(incomingMessagesList.size(), outgoingMessagesList.size()));

  G_ASSERT(numMessageClasses >= 1);
#if DAGOR_DBGLEVEL > 0
  if (dbg_output_table && maxMessageClassLen && dgs_get_settings()->getBlockByNameEx("net")->getBool("debugDumpMessageClasses", true))
  {
    debug("%3s %*s/hash     %*s/hash     (all msg hash %016llx)", "#", maxMessageClassLen, "incoming_msg", maxMessageClassLen,
      "outgoing_msg", allMessagesHash);
    for (int j = 0; j < numMessageClasses; j++)
    {
      const MessageClass *ic = j < incomingMessagesList.size() ? incomingMessagesList[j] : nullptr;
      const MessageClass *oc = j < outgoingMessagesList.size() ? outgoingMessagesList[j] : nullptr;
      debug("%3d %*s/%08x %*s/%08x%s", j, maxMessageClassLen, ic ? ic->debugClassName : nullptr, ic ? ic->classHash : 0,
        maxMessageClassLen, oc ? oc->debugClassName : nullptr, oc ? oc->classHash : 0,
        j == syncedClientToServerMsgCount - 1 && !syncedServerMessageHashes.empty() ? " < last outgoing" : "");
    }
  }
#else
  G_UNUSED(maxMessageClassLen);
  G_UNUSED(dbg_output_table);
#endif

  // store result in global variables
  incomingMessageById = eastl::move(incomingMessagesList);
  incomingMessageById.shrink_to_fit();
  // To consider: use different  number bits for incoming outgoing messages
  numClassIdBits = int((numMessageClasses == 1) ? 1 : (get_log2i(numMessageClasses) + 1));

  // when message ids change, we always want to re-init net events too, as they are
  // directly linked to message ids, and may crash, if they are not re-inited
  if (server)
    net::event::init_server(g_entity_mgr.get());
  else
    net::event::init_client(g_entity_mgr.get());

  return netMessagesCount;
}

static void reset_message_ids_sync_impl()
{
  syncedServerMessageHashes.clear();
  syncedClientToServerMsgCount = 0u;
}

uint32_t MessageClass::init(bool server)
{
  if (server)
  {
    msgSysSyncState = MessageSysSyncState::SERVER;
    reset_message_ids_sync_impl();
  }
  else if (msgSysSyncState == MessageSysSyncState::SERVER)
  {
    G_ASSERT(syncedServerMessageHashes.empty());
    G_ASSERT(syncedClientToServerMsgCount == 0);
    reset_message_ids_sync_impl();
    msgSysSyncState = MessageSysSyncState::NOT_SYNCED;
  }
  return initImpl(server, /* debug output */ numClassIdBits < 0);
}

void MessageClass::startWaitingForMessageIdsSync()
{
  G_ASSERT(msgSysSyncState == MessageSysSyncState::NOT_SYNCED || msgSysSyncState == MessageSysSyncState::WAITING_SYNC);
  reset_message_ids_sync_impl();
  msgSysSyncState = MessageSysSyncState::WAITING_SYNC;
}

void MessageClass::resetMessageIdsSync()
{
  if (!syncedServerMessageHashes.empty())
    debug("resetting synced ECS messages ids");
  reset_message_ids_sync_impl();
  msgSysSyncState = MessageSysSyncState::NOT_SYNCED;
}

void MessageClass::writeMessageIdsSync(danet::BitStream &bs, uint64_t client_hash)
{
  if (msgSysSyncState == MessageSysSyncState::SERVER && client_hash != 0u && client_hash == allMessagesHash)
  {
    // signal client, that everything is synced
    bs.Write(uint16_t(0xffff));
    return;
  }
  if (msgSysSyncState == MessageSysSyncState::SERVER || msgSysSyncState == MessageSysSyncState::SYNCED_MATCHED)
  {
    uint16_t count = 0u;
    uint16_t incomingCnt = 0u;
    const auto countOfs = bs.GetWriteOffset();
    bs.Write(count);
    for (int i = 0; i < 2; i++)
    {
      const bool gatherServerIncoming = i == 0; // first we write incoming to server, then - outgoing
      for (const MessageClass *cls = classLinkList; cls; cls = cls->next)
      {
        if (!gatherServerIncoming && cls->routing != ROUTING_SERVER_TO_CLIENT)
          continue;
        if (gatherServerIncoming && cls->routing != ROUTING_CLIENT_TO_SERVER &&
            cls->routing != ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER)
          continue;
        if (cls->flags & MF_OPTIONAL)
          continue;
        bs.Write(cls->classHash);
        count++;
        if (gatherServerIncoming)
          incomingCnt++;
      }
    }
    bs.WriteAt(count, countOfs);
    bs.Write(incomingCnt);
  }
  else if (msgSysSyncState == MessageSysSyncState::SYNCED_FULL)
  {
    G_ASSERT(!syncedServerMessageHashes.empty());
    bs.Write(uint16_t(syncedServerMessageHashes.size()));
    for (const uint32_t hash : syncedServerMessageHashes)
      bs.Write(hash);
    bs.Write(uint16_t(syncedClientToServerMsgCount));
  }
  else
  {
    bs.Write(uint16_t(0));
    bs.Write(uint16_t(0));
    G_ASSERT_LOG(0, "writing message ids sync on client in invalid state %i", int(msgSysSyncState));
  }
}

bool MessageClass::applyMessageIdsSync(const danet::BitStream &bs)
{
  G_ASSERT_RETURN(msgSysSyncState == MessageSysSyncState::WAITING_SYNC, false);
  reset_message_ids_sync_impl();
  bool ok = true;
  uint16_t count = 0u;
  ok &= bs.Read(count);
  const bool isMatched = count == 0xffffu;
  if (isMatched)
  {
    syncedClientToServerMsgCount = 0;
    syncedServerMessageHashes.clear();
  }
  else
  {
    for (uint16_t j = 0u; j < count; j++)
    {
      uint32_t hash = 0u;
      ok &= bs.Read(hash);
      syncedServerMessageHashes.push_back(hash);
    }
    ok &= bs.Read(syncedClientToServerMsgCount);
    // validate incoming msg count
    ok &= syncedClientToServerMsgCount <= syncedServerMessageHashes.size();
  }
  if (!ok)
  {
    debug("message sync read failed");
    msgSysSyncState = MessageSysSyncState::SYNC_FAILED;
    reset_message_ids_sync_impl();
  }
  else
  {
    msgSysSyncState = isMatched ? MessageSysSyncState::SYNCED_MATCHED : MessageSysSyncState::SYNCED_FULL;
    if (isMatched)
      debug("server and client messages match, no syncing will be done");
    else
      debug("server and client messages successfully synced");
  }
  initImpl(/* server */ false, /* debug output */ !isMatched);
  return ok;
}

bool MessageClass::shouldIgnoreOutgoingMessage(int msg_class_id)
{
  // we don't allow to send messages from client while it is waiting for sync,
  // no sending/receiving messages is done at this point
  if (msgSysSyncState == MessageSysSyncState::WAITING_SYNC || msgSysSyncState == MessageSysSyncState::SYNC_FAILED)
  {
    logerr("Sending net message with id %d, %s", msg_class_id,
      msgSysSyncState == MessageSysSyncState::SYNC_FAILED ? "after sync has failed" : "while still waiting for sync");
    return true;
  }
  if (msgSysSyncState == MessageSysSyncState::SYNCED_FULL)
  {
    return uint32_t(msg_class_id) >= syncedClientToServerMsgCount && syncedClientToServerMsgCount > 0u;
  }
  return false;
}

bool MessageClass::validateIncomingMessage(int msg_class_id, int dbg_conn_id)
{
  // We don't log error on incoming messages, while waiting for sync
  // because it is normal scenario to receive messages from server at this stage.
  // In this case message is silently ignored, as if connection is not yet established
  if (msgSysSyncState == MessageSysSyncState::WAITING_SYNC)
    return true;

  if (msgSysSyncState == MessageSysSyncState::SYNC_FAILED)
  {
    logerr("Net message received with id %d from conn #%d, after sync has failed", msg_class_id, dbg_conn_id);
    return false;
  }
  if (msgSysSyncState == MessageSysSyncState::SYNCED_FULL && unsigned(msg_class_id) >= incomingMessageById.size())
  {
    logerr("Failed to resolve net message with id %d from conn #%d (out of range), incompatible network protocol?", msg_class_id,
      dbg_conn_id);
    return false;
  }
  if (unsigned(msg_class_id) >= incomingMessageById.size() ||
      (msgSysSyncState != MessageSysSyncState::SYNCED_FULL && incomingMessageById[unsigned(msg_class_id)] == nullptr))
  {
    logerr("Failed to resolve net message with id %d from conn #%d, incompatible network protocol?", msg_class_id, dbg_conn_id);
    return false;
  }
  return true;
}

/* static */
const MessageClass *MessageClass::getById(int msg_class_id)
{
  if (msgSysSyncState == MessageSysSyncState::WAITING_SYNC)
    return nullptr;
  return unsigned(msg_class_id) < incomingMessageById.size() ? incomingMessageById[msg_class_id] : nullptr;
}

/* static */
int MessageClass::getNumClassIdBits() { return numClassIdBits; }

/* static */
uint32_t MessageClass::calcNumMessageClasses()
{
  uint32_t nmc = 0;
  for (const MessageClass *cls = classLinkList; cls; cls = cls->next)
    if (!(cls->flags & MF_OPTIONAL))
      ++nmc;
  return nmc;
}

uint64_t MessageClass::calcAllMessagesHash()
{
  // this must be order-independent, due to classLinkList being potentially different order in different builds
  uint64_t hash = FNV1Params<64>::offset_basis;
  for (const MessageClass *cls = MessageClass::classLinkList; cls; cls = cls->next)
    if (!(cls->flags & MF_OPTIONAL))
      hash += uint64_t(cls->classHash) * FNV1Params<64>::prime;
  return hash;
}


} // namespace net
