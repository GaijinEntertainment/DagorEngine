#include <daECS/net/network.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/template.h>
#include <daECS/net/connection.h>
#include <daECS/net/message.h>
#include <EASTL/algorithm.h>
#include <EASTL/tuple.h>
#include <math/dag_bits.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <daNet/bitStream.h>
#include <daNet/messageIdentifiers.h>
#include <debug/dag_assert.h>
#include <perfMon/dag_cpuFreq.h>
#include <daNet/daNetPeerInterface.h>
#include <memory/dag_framemem.h>
#include "utils.h"
#include <daECS/net/netEvent.h>
#include <daECS/net/netEvents.h>
#include "compression.h"
#include "encryption.h"
#include <stdlib.h> // alloca

namespace net
{
extern void dump_and_clear_components_profiler_stats();
extern void reset_replicate_component_filters();

extern void clear_cached_replicated_components();

#define DISCONNECT_WAIT_TIME_MS               600
#define INITIAL_REPLICATION_PACKET_TIMEOUT_MS 500
#define MAX_REPLICATION_PACKET_TIMEOUT_MS     5000
#define MSECINSEC                             1000

// If earliest non-acked reliable packet's life time >= this value -> disconnect
#define DANET_PEER_MAX_TIMEOUT_MS 30000

#define DEFAULT_COMPRESSION_THRESHOLD 256

static int cachedCreationBandwith = -1, cachedCreationMaxDeltaTime = -1;
static inline eastl::pair<int, int> get_creation_bw_limits() // returns (bandwidth, maxDeltaTime)
{
  if (EASTL_UNLIKELY(cachedCreationBandwith < 0))
  {
    const DataBlock *blk = dgs_get_settings();
    blk = blk->getBlockByNameEx("net");
    blk = blk->getBlockByNameEx("bwlimit");
    blk = blk->getBlockByNameEx("creation");
    cachedCreationBandwith = blk->getInt("rate", 0); // Bytes per second. 0 - unlimited.
    cachedCreationMaxDeltaTime = blk->getInt("maxDeltaTime", 0);
  }
  return eastl::make_pair(cachedCreationBandwith, cachedCreationMaxDeltaTime);
}

#define ID_MSG_BASE ID_FILE_LIST_TRANSFER_HEADER // To consider: make this random in runtime (to increase reverse-engineering efforts)
enum
{
  ID_ENTITY_MSG = ID_MSG_BASE,
  ID_ENTITY_MSG_COMPRESSED,
  ID_ENTITY_REPLICATION, // from server - state sync, from client - acks for state sync
  ID_ENTITY_REPLICATION_COMPRESSED,
  ID_ENTITY_CREATION,
  ID_ENTITY_CREATION_COMPRESSED,
  ID_ENTITY_DESTRUCTION
};

#if DAGOR_DBGLEVEL > 0
#define NET_STAT_ENABLED 1
static const char *const str_msg_ids[] = {"ID_ENTITY_MSG", "ID_ENTITY_MSG_COMPRESSED", "ID_ENTITY_REPLICATION",
  "ID_ENTITY_REPLICATION_COMPRESSED", "ID_ENTITY_CREATION", "ID_ENTITY_CREATION_COMPRESSED", "ID_ENTITY_DESTRUCTION"};
static const char ID_ENTITY_REPLICATION_ACKS_STR[] = "ID_ENTITY_REPLICATION_ACKS";
struct NetStatRecord
{
  const char *strType;
  size_t bytes, cbytes;
  uint32_t packets, maxMsgSize, minMsgSize;
  bool isCompressed() const { return bytes != cbytes; }
  size_t effectiveBytes() const { return isCompressed() ? cbytes : bytes; }
  const char *name() const { return isCompressed() ? (strType - 1) : strType; }
  bool operator<(const NetStatRecord &rhs) const { return strType < rhs.strType; }
};
static int performNetStat = 20;
static void trace_net_stat(eastl::vector_set<NetStatRecord> &netstat, const char *strt, uint32_t bytes, uint32_t cbytes)
{
  const char *strType = (bytes == cbytes) ? strt : strt + 1;
  auto it = netstat.find(NetStatRecord{strType});
  if (it != netstat.end())
  {
    it->bytes += bytes;
    it->cbytes += cbytes;
    it->maxMsgSize = eastl::max_alt(cbytes, it->maxMsgSize);
    it->minMsgSize = eastl::min_alt(cbytes, it->minMsgSize);
    it->packets++;
  }
  else
    netstat.emplace(NetStatRecord{strType, bytes, cbytes, 1, cbytes, cbytes});
}
static void dump_net_stat(eastl::vector_set<NetStatRecord> &netstat, const char *trans_type)
{
  if (netstat.empty())
    return;
  size_t maxNameLen = 0;
  size_t totalBytes = 0, totalPackets = 0;
  for (auto &rec : netstat)
  {
    totalBytes += rec.effectiveBytes();
    totalPackets += rec.packets;
    maxNameLen = eastl::max_alt(strlen(rec.name()), maxNameLen);
  }
  eastl::sort(netstat.begin(), netstat.end(),
    [](const NetStatRecord &a, const NetStatRecord &b) { return a.effectiveBytes() > b.effectiveBytes(); });
  debug("Dumping %s %d netstat records (message types) of total %d records, %d KBytes, %d packets:", trans_type, performNetStat,
    netstat.size(), totalBytes >> 10, totalPackets);
  debug("  #N compressed_flag name msg_size_bytes(avg/min/max) kilobytes(%%) packets(%%) compression_ratio"); // legend
  for (int i = 0; i < performNetStat && i < netstat.size(); ++i)
  {
    const NetStatRecord &r = netstat[i];
    size_t ebytes = r.effectiveBytes();
    debug("  #%2d %c %*s %4u %4u %5u %7u(%5.2f%%) %7u(%5.2f%%) %.3f", i + 1, r.isCompressed() ? 'C' : ' ', maxNameLen, r.name(),
      ebytes / r.packets, r.minMsgSize, r.maxMsgSize, ebytes >> 10, double(ebytes) / totalBytes * 100., r.packets,
      double(r.packets) / totalPackets * 100., r.isCompressed() ? (double(r.bytes) / r.cbytes) : 0.);
  }
}

#define TRACE_NET_STAT(tx, n, bs, bsc, conn)                                 \
  if (performNetStat > 0 && n && is_main_thread() && !(conn)->isBlackHole()) \
  trace_net_stat(tx##NetStat, n, (bs).GetNumberOfBytesUsed(),                \
    (&(bs) == &(bsc)) ? (bs).GetNumberOfBytesUsed() : (bsc).GetNumberOfBytesUsed())
#else
#define TRACE_NET_STAT(...) ((void)0)
#endif

extern const int ID_ENTITY_MSG_HEADER_SIZE = sizeof(char) + sizeof(ecs::entity_id_t) + sizeof(uint8_t);

static inline bool check_routing(const IMessage &msg, const CNetwork &cnet, ecs::EntityId toEid, const net::Object *robj,
  ConnectionId receiver, bool send)
{
  const MessageClass &mcls = msg.getMsgClass();
  switch (mcls.routing)
  {
    case ROUTING_CLIENT_TO_SERVER: return send ? cnet.isClient() : cnet.isServer();
    case ROUTING_SERVER_TO_CLIENT: return send ? cnet.isServer() : cnet.isClient();
    case ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER:
    {
      if (send ? cnet.isClient() : cnet.isServer())
      {
        if (!robj)
          robj = g_entity_mgr->getNullable<net::Object>(toEid, ECS_HASH("replication"));
#if DAECS_EXTENSIVE_CHECKS
        G_ASSERTF(robj, "%d", (ecs::entity_id_t)toEid);
#endif
        return robj && receiver == robj->getControlledBy();
      }
      else
        return false;
    }
  };
  return false;
}

void CNetwork::ObjMsg::apply(const net::Object &robj, net::Connection &from, CNetwork &cnet, const danet::BitStream &data) const
{
  ecs::EntityManager &mgr = *g_entity_mgr;
  ecs::EntityId toEid = robj.getEid();
  G_ASSERT(!mgr.isLoadingEntity(toEid));
  alignas(16) char tmpBuf[256];
  void *dataPtr = tmpBuf;
  if (EASTL_UNLIKELY(msgCls->memSize > sizeof(tmpBuf)))
    dataPtr = framemem_ptr()->alloc(msgCls->memSize);
  eastl::unique_ptr<IMessage, MessageDeleter> msg(&msgCls->create(dataPtr), MessageDeleter(/*heap*/ false));
  if (check_routing(*msg, cnet, toEid, &robj, from.getId(), /*send*/ false))
  {
    if (msg->unpack(data, from))
    {
      if (msgCls->flags & MF_TIMED)
        static_cast<IMessageTimed *>(msg.get())->rcvTime = rcvTime;
      msg->connection = &from;
      if (net::event::try_receive(*msg, mgr, toEid))
        ;
      else
      {
        // Send event immediately because we need this message to be processed before futher state syncs messages
        mgr.sendEventImmediate(toEid, ecs::EventNetMessage(eastl::move(msg)));
      }
    }
    else
      logerr("network message #%d/%s/%x to %d<%s> from conn #%d is failed to unpack", msgCls->classId, msgCls->debugClassName,
        msgCls->classHash, (ecs::entity_id_t)toEid, mgr.getEntityTemplateName(toEid), (int)from.getId());
  }
  else
    logwarn("network message #%d/%s/%x to %d<%s> from conn #%d is dropped due to failed routing (%d) check", msgCls->classId,
      msgCls->debugClassName, msgCls->classHash, (ecs::entity_id_t)toEid, mgr.getEntityTemplateName(toEid), (int)from.getId(),
      msgCls->routing);
  if (dataPtr != tmpBuf)
    framemem_ptr()->free(dataPtr);
}

struct RTTStat
{
  sequence_t largestAcked = 0;
  uint16_t latest = 0, smoothed = 0;
  void update(sequence_t baseseq, unsigned rtt)
  {
    if (!is_seq_gt(baseseq, largestAcked) && largestAcked != baseseq) // ignore out-of-order ack samples
      return;
    largestAcked = baseseq;
    if (latest)
    {
      latest = rtt;
      smoothed = (smoothed * 7u + rtt) / 8u;
    }
    else // initial
      latest = smoothed = rtt;
  }
  void clear() { memset(this, 0, sizeof(*this)); }
};


CNetwork::CNetwork(INetDriver *drv_, INetworkObserver *obsrv, uint16_t protov, uint64_t session_rand,
  scope_query_cb_t &&scope_query_) :
  drv(drv_), observer(obsrv), scope_query(eastl::move(scope_query_)), protocolVersion(protov)
{
  G_ASSERT(observer);
  G_ASSERT(drv_);
  bServer = drv->isServer();
  uint32_t numMessageClasses = MessageClass::init(bServer);
  if (bServer)
    net::event::init_server();
  else
    net::event::init_client();

  if (protocolVersion != PROTO_VERSION_UNKNOWN)
    protocolVersion |= numMessageClasses << 16;
  if (auto ctrlIface = static_cast<DaNetPeerInterface *>(drv->getControlIface()))
  {
    encryptionCtx.reset(EncryptionCtx::create(ctrlIface->GetMaximumIncomingConnections(), session_rand));
    ctrlIface->SetTrafficEncoder(encryptionCtx.get());
  }
#ifdef NET_STAT_ENABLED
  performNetStat = dgs_get_settings()->getBlockByNameEx("net")->getInt("netstat", 20);
#endif
}

extern void dump_initial_construction_stats();

void CNetwork::dumpStats()
{
  dump_and_clear_components_profiler_stats();
  dump_initial_construction_stats();
#if NET_STAT_ENABLED
  dump_net_stat(txNetStat, "tx (outgoing)");
  dump_net_stat(rxNetStat, "rx (incoming)");
#endif
}

CNetwork::~CNetwork()
{
  dumpStats();
  drv->shutdown(DISCONNECT_WAIT_TIME_MS);
  clear_cached_replicated_components();
  reset_replicate_component_filters();
}

void CNetwork::stopAll(DisconnectionCause cause) { drv->stopAll(cause); }

void CNetwork::receivePackets(int cur_time_ms, uint8_t replication_channel)
{
  int numEntitiesDestroyed = 0;
  while (Packet *pkt = drv->receive(cur_time_ms))
  {
    onPacket(pkt, cur_time_ms, replication_channel, numEntitiesDestroyed);
    drv->free(pkt);
  }

  while (eastl::optional<danet::EchoResponse> echo = drv->receiveEchoResponse())
    g_entity_mgr->broadcastEvent(NetEchoReponse{echo->routeId, static_cast<int>(echo->result), echo->rttOrTimeout});
}

void CNetwork::updateConnections(int cur_time)
{
  if (isServer())
  {
    for (auto &conn : clientConnections)
      if (conn && conn->connected)
        conn->update(cur_time);
  }
  else if (serverConnection && serverConnection->connected)
    serverConnection->update(cur_time);
}

void CNetwork::update(int curTime, uint8_t replication_channel)
{
  receivePackets(curTime, replication_channel);
  updateConnections(curTime); // update timeouts after packets handling
  syncStateUpdates(curTime, replication_channel);
}

bool CNetwork::readReplayKeyFrame(const danet::BitStream &bs)
{
  return serverConnection->readReplayKeyFrame(bs, [this](Connection &conn, ecs::entity_id_t serverEid) {
    conn.applyDelayedAttrsUpdate(serverEid);
    flushClientWaitMsgs(serverEid);
  });
}

void CNetwork::syncStateUpdates(int cur_time, uint8_t replication_channel)
{
  if (!isServer())
    return;
  Connection::collapseDirtyObjects();
  danet::BitStream bs(2 << 10, framemem_ptr()), bsCompressed(framemem_ptr()), tmpBs(framemem_ptr());
  for (auto &conn : clientConnections)
  {
    if (!conn || !conn->connected || !conn->isResponsive())
      continue;

    int pwrRes = conn->prepareWritePackets();
    if (pwrRes == Connection::PWR_NONE)
      continue;

    int realMTU = conn->getMTU() + ID_ENTITY_MSG_HEADER_SIZE;
    const uint32_t limitConstBytes = realMTU * (2 + COMPRESSION_ENABLED) - sizeof(char); // intentionally use more then MTU for object
                                                                                         // creation because enet splits/reassembles
                                                                                         // packets more efficiently then we can

    if (pwrRes & Connection::PWR_DESTRUCTION)
    {
      bs.ResetWritePointer();
      bs.Write((char)ID_ENTITY_DESTRUCTION);
      do
      {
        bs.SetWriteOffset(CHAR_BIT);
        if (!conn->writeDestructionPacket(bs, tmpBs, limitConstBytes))
          break;
        TRACE_NET_STAT(tx, str_msg_ids[ID_ENTITY_DESTRUCTION - ID_MSG_BASE], bs, bs, conn);
        conn->send(cur_time, bs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0);
      } while (1);
    }

    if (pwrRes & Connection::PWR_CONSTRUCTION)
    {
      bs.ResetWritePointer();
      bs.Write((char)ID_ENTITY_CREATION);
      const uint32_t threshold = DEFAULT_COMPRESSION_THRESHOLD;
      const uint8_t ptype = ID_ENTITY_CREATION_COMPRESSED;
      auto sendContructionPacket = [&]() {
        bs.SetWriteOffset(CHAR_BIT);
        if (!conn->writeConstructionPacket(bs, tmpBs, limitConstBytes))
          return 0;
        const danet::BitStream &bsToSend = bitstream_compress(bs, sizeof(char), ptype, bsCompressed, threshold);
        TRACE_NET_STAT(tx, str_msg_ids[ID_ENTITY_CREATION - ID_MSG_BASE], bs, bsToSend, conn);
        conn->send(cur_time, bsToSend, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0);
        return (int)bsToSend.GetNumberOfBytesUsed();
      };
      int creationBwLimit, creationBwMaxDeltaTime;
      eastl::tie(creationBwLimit, creationBwMaxDeltaTime) = get_creation_bw_limits();
      if (creationBwLimit)
      {
        int creationDeltaTime = cur_time - conn->creationLastSentTime;
        int creationDeltaBytes = creationDeltaTime * creationBwLimit / MSECINSEC;
        if (creationDeltaTime > creationBwMaxDeltaTime || conn->creationCurBytes <= creationDeltaBytes)
        {
          int creationMaxDeltaBytes = creationBwMaxDeltaTime * creationBwLimit / MSECINSEC;
          int totalSent = 0;
          while (int bytesSent = sendContructionPacket())
          {
            totalSent += bytesSent;
            if (totalSent > creationMaxDeltaBytes)
              break;
          }
          conn->creationCurBytes = totalSent;
          conn->creationLastSentTime = cur_time;
        }
      }
      else
        while (sendContructionPacket())
          ;
    }

    if (pwrRes & Connection::PWR_REPLICATION)
    {
      const auto &rtt = rttstat[eastl::distance(clientConnections.begin(), &conn)];
      // Note: what is better: this formula (used in QUIC for ack timeouts) or smoothed + 4*variance (enet/QUIC PTO)?
      int connTimeout = rtt.latest ? (eastl::max_alt(rtt.smoothed, rtt.latest) * 9u / 8u) : INITIAL_REPLICATION_PACKET_TIMEOUT_MS;
      connTimeout = eastl::min(connTimeout, MAX_REPLICATION_PACKET_TIMEOUT_MS);
      constexpr int headerSize = sizeof(char) + sizeof(cur_time);
      const uint32_t limitReplBytes = realMTU - headerSize;
      bs.ResetWritePointer();
      bs.Write((char)ID_ENTITY_REPLICATION);
      bs.Write(cur_time); // for RTT calculations, TODO: 16 bit
      int iterGuard = 0;
      do
      {
        G_ASSERT(iterGuard++ < (16 << 10));
        G_UNUSED(iterGuard);
        bs.SetWriteOffset(BYTES_TO_BITS(headerSize));
        if (!conn->writeReplicationPacket(bs, tmpBs, limitReplBytes)) // limit replication packet size to MTU
          break;
        const uint32_t threshold = DEFAULT_COMPRESSION_THRESHOLD;
        const uint8_t ptype = ID_ENTITY_REPLICATION_COMPRESSED;
        const danet::BitStream &bsToSend = bitstream_compress(bs, headerSize, ptype, bsCompressed, threshold);
        TRACE_NET_STAT(tx, str_msg_ids[ID_ENTITY_REPLICATION - ID_MSG_BASE], bs, bsToSend, conn);
        conn->sendAckedPacket(bsToSend, cur_time, connTimeout, replication_channel);
      } while (1);
    }
  }
}

static inline bool calc_parity_bit(int msgid) { return (__popcount(msgid) & 1) != 0; }

extern ecs::EntityId msg_sink_eid;
bool CNetwork::sendto(int cur_time, ecs::EntityId to_eid, const IMessage &msg, IConnection *conn_, const MessageNetDesc *msg_net_desc)
{
  net::Connection *conn = static_cast<net::Connection *>(conn_);
  if (!conn)
    return false;
  // Explicitly check msg_sink for 2 reasons: 1. it's faster then hash lup 2. this allows send to it from threads
  if (isServer() && to_eid != msg_sink_eid && !conn->isEntityInScope(to_eid))
    return false;
  ecs::entity_id_t serverEid = (ecs::entity_id_t)to_eid;
  if (serverEid == ecs::ECS_INVALID_ENTITY_ID_VAL)
    return false;
  if (!check_routing(msg, *this, to_eid, nullptr, conn->getId(), /*send*/ true))
    return false;

  const MessageClass &mcls = msg.getMsgClass();
  int numClassIdBits = MessageClass::getNumClassIdBits();
#if DAECS_EXTENSIVE_CHECKS
  G_ASSERTF(numClassIdBits >= 0, "MessageClass::init() wasn't called?");
  G_ASSERTF(mcls.classId >= 0, "%s", mcls.debugClassName);
  G_ASSERT(mcls.classId < (1 << numClassIdBits));
#endif
  danet::BitStream bs(512, framemem_ptr());
  bs.Write((char)ID_ENTITY_MSG);
  net::write_server_eid(serverEid, bs);
  bs.WriteBits((uint8_t *)&mcls.classId, numClassIdBits);
  if (numClassIdBits & 7)
    bs.Write(calc_parity_bit(mcls.classId));
  bs.AlignWriteToByteBoundary();
  int headerSize = BITS_TO_BYTES(bs.GetNumberOfBitsUsed());
  G_FAST_ASSERT(headerSize <= ID_ENTITY_MSG_HEADER_SIZE);

  msg.pack(bs);

  const net::MessageNetDesc &mdsc = msg_net_desc ? *msg_net_desc : mcls;

  danet::BitStream bsCompressed(framemem_ptr());
  const danet::BitStream &bsToSend = (mdsc.flags & MF_DONT_COMPRESS) ? bs
                                                                     : bitstream_compress(bs, headerSize, ID_ENTITY_MSG_COMPRESSED,
                                                                         bsCompressed, DEFAULT_COMPRESSION_THRESHOLD);
  TRACE_NET_STAT(tx, mcls.debugClassName, bs, bsToSend, conn);
  PacketPriority pprio = (mdsc.flags & MF_URGENT) ? SYSTEM_PRIORITY : MEDIUM_PRIORITY;
  return conn->send(cur_time, bsToSend, pprio, mdsc.reliability, mdsc.channel, mdsc.dupDelay);
}

#define GET_CONN_OR_LEAVE(idx)                                                              \
  Connection *conn = getConnection(idx);                                                    \
  if (!conn)                                                                                \
  {                                                                                         \
    logwarn("net: packet of size %d bytes from unkwnown connection #%d", pkt->length, idx); \
    break;                                                                                  \
  }

void CNetwork::onPacket(const Packet *pkt, int cur_time_ms, uint8_t replication_channel, int &numEntitiesDestroyed)
{
  const danet::BitStream bs(pkt->data, pkt->length, false);
  danet::BitStream bsTmp(framemem_ptr());
  uint8_t ptype = pkt->data[0];
  bs.IgnoreBits(CHAR_BIT);
  auto readCompressedIfPacketType = [&bs, &bsTmp, ptype, pkt](uint8_t ctype) -> const danet::BitStream * {
    if (ptype != ctype)
      return &bs;
    if (bitstream_decompress(bs, bsTmp))
      return &bsTmp;
    else
    {
      logerr("failed to read compressed packet %d of %d bytes from conn #%d", ctype, BITS_TO_BYTES(bs.GetNumberOfUnreadBits()),
        pkt->systemIndex);
      return nullptr;
    }
  };
  auto onUnknownPacket = [&](const Packet *pkt) {
    if (!observer->onPacket(pkt))
      logerr("Unknown packet data of type %d size %d from peer #%d @ %s", (int)pkt->data[0], pkt->length, pkt->systemIndex,
        pkt->systemAddress.ToString());
  };
  switch (ptype)
  {
    case ID_NEW_INCOMING_CONNECTION: // server
    {
      if (!isServer())
      {
        onUnknownPacket(pkt);
        break;
      }
      auto ctrlIface = static_cast<DaNetPeerInterface *>(drv->getControlIface());
      if (!ctrlIface) // it's replay's driver which doesn't need additional connection
        break;

      if (protocolVersion != PROTO_VERSION_UNKNOWN && pkt->length >= 5)
      {
        uint32_t connectData = PROTO_VERSION_UNKNOWN;
        G_VERIFY(bs.Read(connectData));
        if (connectData != PROTO_VERSION_UNKNOWN && protocolVersion != connectData)
        {
          logwarn("peer #%d protocol version mismatch %#x (ours) != %#x (theirs)", pkt->systemIndex, protocolVersion, connectData);
          ctrlIface->CloseConnection(pkt->systemIndex, DC_NET_PROTO_MISMATCH);
          break;
        }
      }

      addConnection(create_net_connection(drv.get(), pkt->systemIndex, scope_query_cb_t(scope_query)), pkt->systemIndex);
    }
    break;
    case ID_CONNECTION_REQUEST_ACCEPTED: // client
    {
      if (isServer())
        onUnknownPacket(pkt);
      else if (auto ctrlIface = static_cast<DaNetPeerInterface *>(drv->getControlIface()))
        addConnection(create_net_connection(drv.get(), pkt->systemIndex, scope_query_cb_t(scope_query)), pkt->systemIndex);
    }
    break;
    case ID_DISCONNECT: destroyConnection(pkt->systemIndex, (DisconnectionCause)pkt->data[1]); break;
    case ID_ENTITY_MSG:
    case ID_ENTITY_MSG_COMPRESSED:
    {
      GET_CONN_OR_LEAVE(pkt->systemIndex);
      ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
      if (!net::read_server_eid(serverEid, bs))
      {
        logerr("Failed to read server eid from conn #%d, incompatible network protocol?", (int)conn->getId());
        break;
      }
      int msgId = 0;
      int nClsIdBits = MessageClass::getNumClassIdBits();
      if (!bs.ReadBits((uint8_t *)&msgId, nClsIdBits))
        msgId = -1;
      else if (nClsIdBits & 7)
      {
        bool parityBit = false;
        if (!bs.Read(parityBit))
          msgId = -1;
        else if (parityBit != calc_parity_bit(msgId))
          msgId = -2;
      }
      const MessageClass *msgCls = MessageClass::getById(msgId);
      if (!msgCls)
      {
        logerr("Failed to resolve net message with id %d from conn #%d, incompatible network protocol?", msgId, (int)conn->getId());
        break;
      }
      bs.AlignReadToByteBoundary();
      const danet::BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_MSG_COMPRESSED);
      if (!bsToRead)
        break;
      TRACE_NET_STAT(rx, msgCls->debugClassName, *bsToRead, bs, conn);
      ObjMsg omsg;
      omsg.msgCls = msgCls;
      if (msgCls->flags & MF_TIMED)
        omsg.rcvTime = pkt->receiveTime;
      if (const Object *robj = Object::getByEid(ecs::EntityId(serverEid)))
        omsg.apply(*robj, *conn, *this, *bsToRead);
      else if (isClient())
      {
        if ((msgCls->flags & MF_DISCARD_IF_NO_ENTITY) != 0)
          ; // do nothing (i.e. discard)
        else
        {
          auto it = clientWaitMsgs.insert(ClientWaitMsg{serverEid}).first;
          it->msgs.push_back(eastl::move(omsg));
          it->msgs.back().bs = *bsToRead; // copy
        }
      }
      else
        logwarn("Can't resolve network entity by eid %d for connection #%d, with msg = %s/%x. Message will be dropped.", serverEid,
          (int)conn->getId(), msgCls->debugClassName, msgCls->classHash);
    }
    break;
    case ID_ENTITY_REPLICATION:
    case ID_ENTITY_REPLICATION_COMPRESSED:
    {
      GET_CONN_OR_LEAVE(pkt->systemIndex);
      int sentTime = 0;
      if (!bs.Read(sentTime))
      {
        logerr("failed to read time from conn #%d", pkt->systemIndex);
        break;
      }
      if (isServer()) // Acks for replication
      {
        if (ptype != ID_ENTITY_REPLICATION)
          break;
        sequence_t baseseq = 0;
        if (!conn->onAcksReceived(bs, baseseq))
          break;
        TRACE_NET_STAT(rx, ID_ENTITY_REPLICATION_ACKS_STR, bs, bs, conn);
        int rtt = cur_time_ms - sentTime;
        if (rtt > 0)
          rttstat[conn->getId()].update(baseseq, eastl::min(rtt, USHRT_MAX));
        else
          logwarn("Ignore bad rtt value %d (packet from future?)", rtt);
      }
      else // replication packet
      {
        const danet::BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_REPLICATION_COMPRESSED);
        if (!bsToRead)
          break;
        if (!conn->readReplicationPacket(*bsToRead))
        {
          logerr("Failed to read replication packet (%d) of %d bytes from conn #%d", ptype, bsToRead->GetNumberOfBytesUsed(),
            pkt->systemIndex);
          break;
        }
        TRACE_NET_STAT(rx, str_msg_ids[ID_ENTITY_REPLICATION - ID_MSG_BASE], *bsToRead, bs, conn);

        // Reply with acks for this state sync
        // To consider: reply rarer then once per each replication packet
        danet::BitStream acks(12, framemem_ptr());
        acks.Write((char)ID_ENTITY_REPLICATION);
        acks.Write(sentTime);
        conn->writeLastRecvdPacketAcks(acks);
        TRACE_NET_STAT(tx, ID_ENTITY_REPLICATION_ACKS_STR, acks, acks, conn);
        conn->send(cur_time_ms, acks, MEDIUM_PRIORITY, UNRELIABLE, replication_channel);
      }
    }
    break;
    case ID_ENTITY_CREATION:
    case ID_ENTITY_CREATION_COMPRESSED:
    {
      GET_CONN_OR_LEAVE(pkt->systemIndex);
      if (isServer())
        ;  // do nothing (old clients might sends us kill confirmations, not used anymore)
      else // client
      {
        const danet::BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_CREATION_COMPRESSED);
        if (!bsToRead)
          break;
        // Make sure that entities actually destroyed before creating new ones (in order to free potentially colliding eid slots)
        if (EASTL_UNLIKELY(numEntitiesDestroyed != 0))
        {
          g_entity_mgr->performDelayedCreation(/*flush_all*/ false);
          numEntitiesDestroyed = 0;
        }
        TRACE_NET_STAT(rx, str_msg_ids[ID_ENTITY_CREATION - ID_MSG_BASE], *bsToRead, bs, conn);
        auto constructCb = [this](Connection &conn, ecs::entity_id_t serverEid) {
          if (&conn != serverConnection.get()) // Note: in theory different instance might get reallocated to the same exact address as
                                               // old one
            return;
          conn.applyDelayedAttrsUpdate(serverEid);
          flushClientWaitMsgs(serverEid);
        };
#if DAECS_EXTENSIVE_CHECKS
        float cratio = (bsToRead == &bs) ? 0.f : float(bs.GetNumberOfBytesUsed()) / bsToRead->GetNumberOfBytesUsed();
#else
        float cratio = 0;
#endif
        bool r = conn->readConstructionPacket(*bsToRead, cratio, constructCb);
        if (!r)
          logerr("Failed to read %sconstruction packet of %d bytes from conn #%d",
            ptype == ID_ENTITY_CREATION_COMPRESSED ? "compresssed " : "", bsToRead->GetNumberOfBytesUsed(), pkt->systemIndex);
      }
    }
    break;
    case ID_ENTITY_DESTRUCTION:
    {
      GET_CONN_OR_LEAVE(pkt->systemIndex);
      if (isServer())
        ;  // fallthrough
      else // client
      {
        auto destroyCb = [this, &numEntitiesDestroyed](Connection &, ecs::entity_id_t serverEid) {
          ++numEntitiesDestroyed;
          clientWaitMsgs.erase(ClientWaitMsg{serverEid}); // might exist some data if creation cb is not called yet (i.e. if entity was
                                                          // still in loading queue)
        };
        TRACE_NET_STAT(rx, str_msg_ids[ID_ENTITY_DESTRUCTION - ID_MSG_BASE], bs, bs, conn);
        if (!conn->readDestructionPacket(bs, destroyCb))
          logerr("Failed to read destruction packet of %d bytes from conn #%d", bs.GetNumberOfBytesUsed(), pkt->systemIndex);
        break;
      }
    }
    default: onUnknownPacket(pkt);
  }
}

void CNetwork::flushClientWaitMsgs(ecs::entity_id_t serverEid)
{
  G_ASSERT(isClient());
  G_ASSERT(serverConnection);
  auto it = clientWaitMsgs.find(ClientWaitMsg{serverEid});
  if (it == clientWaitMsgs.end())
    return;
  if (const Object *robj = Object::getByEid(ecs::EntityId(serverEid)))
  {
    G_ASSERT(!it->msgs.empty());
    for (auto &msg : it->msgs)
      msg.apply(*robj, *serverConnection, *this, msg.bs);
  }
  else
    G_ASSERTF(0, "Failed to resolve serverEid %d or replication component", serverEid);
  clientWaitMsgs.erase(it);
}

Connection *CNetwork::getConnection(unsigned idx)
{
  if (isServer())
    return idx < clientConnections.size() ? clientConnections[idx].get() : NULL;
  else
    return idx == 0 ? serverConnection.get() : NULL;
}

bool CNetwork::debugVerifyNetConnectionPtr(void *conn) const
{
  return (conn != nullptr) && (!debugConnVtbl || *(void **)conn == debugConnVtbl); // hack for fast connection verification
}

void CNetwork::addConnection(Connection *conn, unsigned idx)
{
  G_ASSERT(conn);
  auto ctrlIface = static_cast<DaNetPeerInterface *>(drv->getControlIface());
  if (auto ectx = encryptionCtx.get())
  {
    conn->setEncryptionCtx(ectx);
    if (uint32_t connectID = ctrlIface ? ctrlIface->GetPeerConnectID(idx) : 0)
      ectx->setPeerRandom(idx, connectID);
  }
  if (!debugConnVtbl && !conn->isBlackHole())
    debugConnVtbl = *(void **)conn;
  if (ctrlIface)
    ctrlIface->SetupPeerTimeouts(conn->getId(), DANET_PEER_MAX_TIMEOUT_MS);
  if (isServer())
  {
    if (idx >= clientConnections.size())
    {
      clientConnections.resize(idx + 1);
      rttstat.resize(idx + 1);
    }
    if (!clientConnections[idx])
    {
      clientConnections[idx].reset(conn);
      conn->setReplicatingFrom();
    }
    else
    {
      logerr("%s conn slot #%d is already occupied!", __FUNCTION__, idx);
      delete conn;
      return;
    }
  }
  else
  {
    G_ASSERT(idx == 0);
    G_ASSERT(!serverConnection);
    serverConnection.reset(conn);
    conn->setReplicatingTo();
  }
  observer->onConnect(*conn);
}

void CNetwork::destroyConnection(unsigned idx, DisconnectionCause cause)
{
  Connection *conn = getConnection(idx);
  if (isServer())
  {
    if (!conn)
      return;
    observer->onDisconnect(conn, cause);
    clientConnections[idx].reset(); // TODO: do actual daNet/enet disconnect here
    rttstat[idx].clear();
  }
  else
  {
    observer->onDisconnect(conn, cause);
    clientWaitMsgs.clear();
    serverConnection.reset();
  }
  if (encryptionCtx)
  {
    encryptionCtx->setPeerRandom(idx, 0);
    encryptionCtx->setPeerKey(idx, dag::ConstSpan<uint8_t>(), EncryptionKeyBits::None);
  }
}

extern void mark_all_objects_as_dirty();
void CNetwork::enableComponentFiltering(ConnectionId id, bool on)
{
  auto enable = [](IConnection *c, bool on) {
    if (c && !c->isBlackHole())
      static_cast<Connection *>(c)->isFiltering = on;
  };
  if (id == INVALID_CONNECTION_ID)
  {
    for (auto &c : getClientConnections())
      enable(c.get(), on);
  }
  else if (size_t(id) < getClientConnections().size())
    enable(getClientConnections()[id].get(), on);
  if (!on)
    mark_all_objects_as_dirty();
}

bool CNetwork::isComponentFilteringEnabled(ConnectionId id)
{
  auto isEnabled = [](IConnection *c) {
    if (c && !c->isBlackHole())
      if (static_cast<Connection *>(c)->isFiltering)
        return true;
    return false;
  };
  if (id == INVALID_CONNECTION_ID)
  {
    for (auto &c : getClientConnections())
      if (isEnabled(c.get()))
        return true;
    return false;
  }
  else
    return size_t(id) < getClientConnections().size() ? isEnabled(getClientConnections()[id].get()) : false;
}


} // namespace net
