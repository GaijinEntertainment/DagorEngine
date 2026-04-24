// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/daNetPeerInterface.h>
#include <daNet/getTime.h>
#include <daNet/messageIdentifiers.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <EASTL/algorithm.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <daNet/daNetStatistics.h>
#include <errno.h>
#include <memory/dag_framemem.h>
#include <enet/enet.h>
#include "ucr.h"
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

#include <enet/enet_dagor.h>
#include <arpa/inet.h>

#define DANET_DEFINED_CHANNELS 8
#define DANET_MAX_CHANNELS     ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT

#define DANET_MAX_PING                   1200
#define DANET_CONNECT_REQUEST_TIMEOUT_MS 6000 // 7500 in practice (500+1000+2000+4000), due to exp backoff and default timeout 500ms
#define DANET_LAG_MS                     100

#define MIN_RESPONSIVENESS_TOLERANCE_MS (4 * ENET_PEER_DEFAULT_ROUND_TRIP_TIME)
#define NUM_RESPONSIVENESS_ROUNDTRIPS   2
#define UPD_RESPONSIVENESS_TIMEOUT_MS   600U

#define GET_PEER(idx)  (((idx) < host->peerCount) ? &host->peers[idx] : NULL)
#define PEER2IDX(peer) ((peer) - host->peers)
#define IS_CLIENT_MODE ((host)->peerCount == 1)

static int enet_packet_payload_size(ENetPeer *peer, int mtu)
{
  int length = mtu - sizeof(ENetProtocolHeader) - sizeof(ENetProtocolSendFragment);
  if (peer && peer->host && peer->host->checksum)
    length -= sizeof(enet_uint32);
  return length;
}

static DaNetTime get_ping_timeout()
{
  DaNetTime result = DANET_MAX_PING;
  if (const DataBlock *settings = dgs_get_settings())
    result = settings->getBlockByNameEx("net")->getInt("echoTimeoutMs", result);
  return result;
}

// for use in ENetPacket::flags, should not intersect with enet ones (ENetPacketFlag)
#define DANET_PACKET_FLAG_BROADCAST          (1 << 15)
#define DANET_PACKET_FLAG_RELIABLE_UNORDERED (1 << 14)

#define debug(...) logmessage(_MAKE4C('DNET'), __VA_ARGS__)

extern ENetHost *ucr_host;

ENetAddress get_enet_address(const char *new_host) // Return zero .host on fail
{
  ENetAddress addr{0, 0};
  if (new_host)
  {
    char tmpbuf[128];
    const char *pcol = strrchr(new_host, ':');
    if (pcol) // drop port if exist
    {
      size_t l = eastl::min(size_t(pcol - new_host), sizeof(tmpbuf) - 1);
      memcpy(tmpbuf, new_host, l);
      tmpbuf[l] = '\0';
      new_host = tmpbuf; //-V507
    }
    if (!inet_pton(AF_INET, new_host, &addr.host) && enet_address_set_host(&addr, new_host) != 0)
      return {0, 0};
    if (pcol) // parse port
    {
      char *ep = NULL;
      addr.port = (uint16_t)strtol(pcol + 1, &ep, 0);
      if (ep == pcol + 1 || *ep)
        addr.port = 0;
    }
  }
  return addr;
}

extern "C"
{
  extern volatile uint64_t enet_rx_bytes, enet_rx_packets, enet_tx_bytes, enet_tx_dropped_bytes, enet_tx_packets;

  void enet_logf(const char *fmt, ...)
  {
    va_list vl;
    va_start(vl, fmt);
    cvlogmessage(_MAKE4C('ENET'), fmt, vl);
    va_end(vl);
  }
};

static DaNetStatistics g_stat;
static IDaNetTrafficEncoder *traffic_encoder = nullptr;
volatile static bool delayed_set_traffic_encoder = false;
const SystemAddress UNASSIGNED_SYSTEM_ADDRESS(0xFFFFFFFF, 0xFFFF);

struct PacketToSend : public ENetPacket
{
  ENetPeer *getPeer() { return (ENetPeer *)userData; }
  uint8_t getChannel(uint8_t &chn, size_t chn_count) const
  {
    if (!(flags & DANET_PACKET_FLAG_RELIABLE_UNORDERED))
      return flags >> 24;
    else
    {
      if (++chn < DANET_DEFINED_CHANNELS || chn >= chn_count)
        chn = DANET_DEFINED_CHANNELS;
      return chn;
    }
  }
  int getDupAt() const { return *(int *)(this + 1); }
  static PacketToSend *create(uint8_t chn, ENetPeer *peer, dag::ConstSpan<uint8_t> cdata, uint32_t flags, int dup_at = 0)
  {
    G_ASSERT(flags < (1 << 24));
    PacketToSend *pkt = (PacketToSend *)enet_malloc(sizeof(PacketToSend) + (dup_at ? sizeof(int) : 0) + data_size(cdata));
    pkt->referenceCount = dup_at ? 2 : 1;
    pkt->flags = flags | ENET_PACKET_FLAG_NO_ALLOCATE | (uint32_t(chn) << 24);
    pkt->data = (uint8_t *)(pkt + 1);
    if (dup_at)
    {
      *(int *)pkt->data = dup_at;
      pkt->data += sizeof(int);
    }
    pkt->dataLength = data_size(cdata);
#if ENET_PACKET_FREE_CALLBACK
    pkt->freeCallback = NULL;
#endif
    pkt->userData = peer;
    mem_copy_to(cdata, pkt->data);
    return pkt;
  }
};

const char *describe_disconnection_cause(DisconnectionCause cause)
{
  static const char *const dc_strs[] = {
#define DC(x) #x,
    DANET_DEFINE_DISCONNECTION_CAUSES
#undef DC
  };
  return dc_strs[cause];
}

static thread_local char sysaddr_str_buf[24];
const char *SystemAddress::ToString(bool withPort) const
{
  if (*this == UNASSIGNED_SYSTEM_ADDRESS)
    return "";
  const char *hname = ::ip_to_string(((in_addr *)&host)->s_addr);
  char *saBuf = sysaddr_str_buf;
  if (withPort)
    _snprintf(saBuf, sizeof(sysaddr_str_buf), "%s:%d", hname, port);
  else
    _snprintf(saBuf, sizeof(sysaddr_str_buf), "%s", hname);
  saBuf[sizeof(sysaddr_str_buf) - 1] = 0;
  return saBuf;
}
void danet::write_type(danet::BitStream &bs, const SystemAddress &sa)
{
  bs.Write(sa.host);
  bs.Write(sa.port);
}
bool danet::read_type(const danet::BitStream &bs, SystemAddress &sa) { return bs.Read(sa.host) && bs.Read(sa.port); }

#pragma pack(push, 1)
struct InplaceDataPacket
{
  uint8_t packetId;
  uint32_t data;
  static ENetPacket *create(const InplaceDataPacket &p) { return enet_packet_create(&p, sizeof(p), 0); }
};
#pragma pack(pop)

void SystemAddress::SetBinaryAddress(const char *a)
{
  host = ENET_HOST_ANY;
  enet_address_set_host((ENetAddress *)this, a); //-V1027 layout-compatible with ENetAddress
}

SocketDescriptor::SocketDescriptor(unsigned short _port, const char *_hostAddress) : type(STR)
{
  port = _port;
  if (_hostAddress && *_hostAddress)
  {
    strncpy(hostAddress, _hostAddress, sizeof(hostAddress));
    hostAddress[sizeof(hostAddress) - 1] = 0;
  }
  else
    hostAddress[0] = 0;
}

static ENetPeer *get_peer_by_addr(const ENetHost *host, const SystemAddress &a)
{
  if (a != UNASSIGNED_SYSTEM_ADDRESS)
    for (ENetPeer *cp = host->peers; cp < &host->peers[host->peerCount]; ++cp)
      if (a == (const SystemAddress &)cp->address && cp->state == ENET_PEER_STATE_CONNECTED)
        return cp;
  return NULL;
}

DaNetPeerInterface::DaNetPeerInterface(_ENetHost *ehost, bool is_threaded) :
  host(ehost),
  // Note: overcommit thread stack because unknown third-party software (e.g. "smart" net drivers) might hook socket functions
  DaThread("danet_thread", 128 << 10),
  receivedPackets(DANET_MEM),
  packetsToSend(DANET_MEM),
  disconnectCommands(DANET_MEM),
  sleep_time(0),
  maximumIncomingConnections(0),
  responsivenessUpdateStamp(0U),
  echoManager(get_ping_timeout()),
  is_threaded(is_threaded)
{
  receivedPackets.reserve(64);
  packetsToSend.reserve(64);
  disconnectCommands.reserve(4);
  G_STATIC_ASSERT(sizeof(SystemAddress) == sizeof(ENetAddress));
  G_ASSERT(!host || (char *)host == (char *)(this + 1));

  G_VERIFY(enet_initialize() == 0);
  enet_time_set(1); // not 0, because 0 has special meaning ("invalid time")

  echoManager.setHost(host); // it's ok, if host == nullptr

  os_event_create(&packetsEvent);
}

DaNetPeerInterface::~DaNetPeerInterface()
{
  if (host)
    Shutdown(DEF_BLOCK_DURATION);

  enet_deinitialize();

  os_event_destroy(&packetsEvent);
}

/* static */
DaNetPeerInterface *DaNetPeerInterface::create(size_t asz, void **outp)
{
  // To consider: preallocate peer related data as well
  asz = (asz + alignof(DaNetPeerInterface) - 1) & ~(alignof(DaNetPeerInterface) - 1);
  void *mem = memalloc(asz + sizeof(DaNetPeerInterface) + sizeof(ENetHost), defaultmem);
  auto host = (_ENetHost *)((char *)mem + asz + sizeof(DaNetPeerInterface));
  memset(host, 0, sizeof(*host));
  enet_list_clear(&host->dispatchQueue);
  host->socket = ENET_SOCKET_NULL;
  auto ret = new ((char *)mem + asz, _NEW_INPLACE) DaNetPeerInterface(host);
  if (outp)
    *outp = asz ? mem : nullptr;
  return ret;
}

void DaNetPeerInterface::SetBandwidthLimits(uint32_t incoming_bps, uint32_t outgoing_bps, uint32_t outgoing_bps_per_peer)
{
  if (host)
    enet_host_bandwidth_limit(host, BITS_TO_BYTES(incoming_bps), BITS_TO_BYTES(outgoing_bps), BITS_TO_BYTES(outgoing_bps_per_peer));
}

bool DaNetPeerInterface::Startup(uint16_t maxCon, int st, const SocketDescriptor *sd)
{
  G_ASSERT(!host || (char *)host == (char *)(this + 1));
  G_ASSERT(maxCon);
  sleep_time = st;

  ENetAddress addr;
  if (sd)
  {
    addr.host = ENET_HOST_ANY;
    addr.port = sd->port;
    if (sd->type == SocketDescriptor::STR)
    {
      if (sd->hostAddress[0])
        enet_address_set_host(&addr, sd->hostAddress);
    }
    else if (sd->type == SocketDescriptor::BINARY)
      addr.host = sd->host;
  }
  maximumIncomingConnections = maximumIncomingConnections ? min((uint16_t)maximumIncomingConnections, maxCon) : maxCon;
  G_ASSERT(maximumIncomingConnections);
  _ENetHost *ehost = enet_host_create((sd && sd->type != SocketDescriptor::SOCKET) ? &addr : NULL, maximumIncomingConnections,
    DANET_MAX_CHANNELS, 0, 0);
  if (!ehost)
    return false;
  if ((char *)host == (char *)(this + 1))
  {
    // move host data
    memcpy(host, ehost, sizeof(*host));
    enet_list_clear(&host->dispatchQueue);
    for (ENetPeer *cp = host->peers; cp < &host->peers[ENET_DAGOR_ALLOC_PEER_COUNT(host->peerCount)]; ++cp)
      cp->host = host;
    enet_free(ehost);
  }
  else
    host = ehost;
  ucr_host = host;

  if (sd && sd->type == SocketDescriptor::SOCKET) // replace created socket with passed one
  {
    enet_socket_destroy(host->socket);
    // binded udp socket expected here
    G_ASSERT(sd->socket != ENET_SOCKET_NULL);
    host->socket = sd->socket;
    enet_socket_get_address(host->socket, &host->address);
    enet_socket_set_option(host->socket, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option(host->socket, ENET_SOCKOPT_BROADCAST, 1);
    enet_socket_set_option(host->socket, ENET_SOCKOPT_RCVBUF, ENET_HOST_RECEIVE_BUFFER_SIZE);
    enet_socket_set_option(host->socket, ENET_SOCKOPT_SNDBUF, ENET_HOST_SEND_BUFFER_SIZE);
  }

  peerResponsiveness.resize(maximumIncomingConnections);
  mem_set_0(peerResponsiveness);

  ResetTrafficStatistics(); // it's safe to call it here before thread is not yet started

  echoManager.setHost(host); // doing this strictly before the network thread is started for concurrency reasons

  if (sd && !is_threaded)
    DaThread::start();

  return true;
}

static void send_enet_packet(ENetPeer *peer, uint8_t orderingChannel, ENetPacket *packet)
{
  if (enet_peer_send(peer, orderingChannel, packet) < 0)
    debug("enet_peer_send to %s failed, peer state %d", ((const SystemAddress &)peer->address).ToString(), peer->state);
}

const _ENetPeer *DaNetPeerInterface::GetENetPeer(SystemIndex sys_idx) const { return GET_PEER(sys_idx); }

void DaNetPeerInterface::Stop(DaNetTime block_duration, DisconnectionCause cause)
{
  if (host)
  {
    if (!is_threaded)
      DaThread::terminate(true, -1, &packetsEvent);

    enet_host_flush(host);

    // disconnect peers reliably
    Tab<ENetPeer *> peers_to_disconnect(framemem_ptr());
    peers_to_disconnect.reserve(host->peerCount);
    for (ENetPeer *cp = host->peers; cp < &host->peers[host->peerCount]; ++cp)
      if (cp->state == ENET_PEER_STATE_CONNECTED)
      {
        peers_to_disconnect.push_back(cp);
        enet_peer_disconnect(cp, cause);
      }
    if (peers_to_disconnect.size())
    {
      ENetEvent event;
      DaNetTime deadlineTime = danet::GetTime() + block_duration;
      do // wait for disconnect acks
      {
        event.type = ENET_EVENT_TYPE_NONE;
        int r = enet_host_service(host, &event, 0);
        if (r <= 0)
          sleep_msec(1);
        else
          switch (event.type)
          {
            case ENET_EVENT_TYPE_RECEIVE: enet_packet_destroy(event.packet); break;
            case ENET_EVENT_TYPE_CONNECT: enet_peer_disconnect_now(event.peer, cause); break;
            case ENET_EVENT_TYPE_DISCONNECT: erase_item_by_value(peers_to_disconnect, event.peer); break;
            case ENET_EVENT_TYPE_DAGOR_INTERCEPTED: /*this should never really happen but neither does it break anything*/ break;
            case ENET_EVENT_TYPE_DAGOR_RECEIVE_UNCONNECTED: /*currently no logic outside relaying/STUN requires unconnected messaging*/
              break;
            case ENET_EVENT_TYPE_NONE: break;
          }
      } while (peers_to_disconnect.size() && danet::GetTime() <= deadlineTime);
    }
  }
}

void DaNetPeerInterface::Shutdown(DaNetTime block_duration)
{
  if (host)
  {
    Stop(block_duration, DC_CONNECTION_CLOSED);
    enet_socket_destroy(host->socket);
    for (_ENetPeer *cp = host->peers; cp < &host->peers[ENET_DAGOR_ALLOC_PEER_COUNT(host->peerCount)]; ++cp)
      enet_peer_reset(cp);
    enet_free(host->peers);
    if ((char *)host != (char *)(this + 1))
      enet_free(host);
    host = NULL;
    ucr_host = nullptr;
  }

  for (int i = 0; i < receivedPackets.size(); ++i)
    DeallocatePacket(receivedPackets[i]);
  clear_and_shrink(receivedPackets);

  echoManager.clear(); // strictly after the network thread is terminated for concurrency reasons

  for (int i = 0; i < packetsToDup.size(); ++i)
    if (--packetsToDup[i]->referenceCount == 0)
      enet_packet_destroy(packetsToDup[i]);
  clear_and_shrink(packetsToDup);
  for (int i = 0; i < packetsToSend.size(); ++i)
    if (--packetsToSend[i]->referenceCount == 0)
      enet_packet_destroy(packetsToSend[i]);
  clear_and_shrink(packetsToSend);

  clear_and_shrink(disconnectCommands);

  relayPeerIdx = UNASSIGNED_SYSTEM_INDEX;
  if (relayStatusHandler)
    relayStatusHandler(false);
}

danet::PeerQoSStat DaNetPeerInterface::GetPeerQoSStat(SystemIndex peerIdx) const
{
  danet::PeerQoSStat ret;
  const ENetPeer *peer = GET_PEER(peerIdx);
  const PeerResponsiveness *p = peer ? &peerResponsiveness[peerIdx] : nullptr;
  if (!p || interlocked_relaxed_load(p->state) != ENET_PEER_STATE_CONNECTED)
    return ret;
  ret.connectionStartTime = interlocked_relaxed_load(p->connectionStartTime);
  if (interlocked_relaxed_load(p->packetThrottleEpoch) != 0)
  {
    ret.averagePing = interlocked_relaxed_load(p->roundTripTime);
    ret.averagePingVariance = interlocked_relaxed_load(p->roundTripTimeVariance);
    // enet actually updates lastRoundTripTime[Variance] with lowestRoundTripTime/highestRoundTripTimeVariance each
    // packetThrottleInterval (see enet/protocol.c:enet_protocol_handle_acknowledge())
    ret.lowestPing = interlocked_relaxed_load(p->lastRoundTripTime);
    ret.highestPingVariance = interlocked_relaxed_load(p->lastRoundTripTimeVariance);
    ret.packetLoss = interlocked_relaxed_load(p->packetLoss) / (float)ENET_PEER_PACKET_LOSS_SCALE;
  }
  return ret;
}

int DaNetPeerInterface::GetLastPing(SystemIndex peerIdx) const
{
  ENetPeer *peer = GET_PEER(peerIdx);
  return peer ? interlocked_relaxed_load(peerResponsiveness[peerIdx].roundTripTime) : -1;
}

int DaNetPeerInterface::GetLowestPing(SystemIndex peerIdx) const
{
  ENetPeer *peer = GET_PEER(peerIdx);
  return peer ? interlocked_relaxed_load(peerResponsiveness[peerIdx].lowestRoundTripTime) : -1;
}

int DaNetPeerInterface::GetAveragePing(SystemIndex peerIdx) const
{
  ENetPeer *peer = GET_PEER(peerIdx);
  return peer ? interlocked_relaxed_load(peerResponsiveness[peerIdx].roundTripTime) : -1;
}

int DaNetPeerInterface::GetMaximumPacketSize(SystemIndex sys_idx) const
{
  ENetPeer *peer = GET_PEER(sys_idx);
  return enet_packet_payload_size(peer, peer ? peer->mtu : ENET_HOST_DEFAULT_MTU);
}

uint32_t DaNetPeerInterface::GetPeerConnectID(SystemIndex sys_idx) const
{
  const ENetPeer *peer = GET_PEER(sys_idx);
  return peer ? peer->connectID : 0;
}

void DaNetPeerInterface::SetupPeerTimeouts(SystemIndex idx, uint32_t max_timeout_ms, uint32_t min_timeout_ms,
  uint32_t min_timeout_limit)
{
  ENetPeer *peer = GET_PEER(idx);
  if (peer)
    enet_peer_timeout(peer, min_timeout_limit, min_timeout_ms, max_timeout_ms);
}


bool DaNetPeerInterface::IsPeerResponsive(SystemIndex sys_idx) const
{
  const ENetPeer *peer = GET_PEER(sys_idx);
  return peer ? !interlocked_relaxed_load(peerResponsiveness[sys_idx].notResponsive) : false;
}

DaNetTime DaNetPeerInterface::GetConnectionStartTime(SystemIndex sys_idx) const
{
  const ENetPeer *peer = GET_PEER(sys_idx);
  return peer ? interlocked_relaxed_load(peerResponsiveness[sys_idx].connectionStartTime) : 0;
}

DaNetTime DaNetPeerInterface::GetLastReceivedPacketTime(SystemIndex sys_idx) const
{
  const ENetPeer *peer = GET_PEER(sys_idx);
  return peer ? interlocked_relaxed_load(peerResponsiveness[sys_idx].lastReceiveTime) : 0;
}

void DaNetPeerInterface::updatePeerResponsiveness(DaNetTime currTime)
{
  if (currTime < responsivenessUpdateStamp)
    return;
  responsivenessUpdateStamp = currTime + UPD_RESPONSIVENESS_TIMEOUT_MS;

  for (int i = 0; i < host->peerCount; ++i)
  {
    const ENetPeer *peer = &host->peers[i];
    PeerResponsiveness &pr = peerResponsiveness[i];
    if (interlocked_relaxed_load(pr.lastReceiveTime) != 0)
    {
      DaNetTime lastTime = max(peer->lastReceiveTime, interlocked_relaxed_load(pr.lastReceiveTime));
      // Note: despite it's name 'last' here is actullay minRTT+maxRTTVar (see enet_protocol_handle_acknowledge())
      int32_t criterion = NUM_RESPONSIVENESS_ROUNDTRIPS * (peer->lastRoundTripTime + peer->lastRoundTripTimeVariance * 2);
      interlocked_relaxed_store(pr.notResponsive, int32_t(currTime - lastTime) > max(criterion, MIN_RESPONSIVENESS_TOLERANCE_MS));
    }
    interlocked_relaxed_store(pr.state, peer->state);
    interlocked_relaxed_store(pr.packetThrottleEpoch, peer->packetThrottleEpoch);
    interlocked_relaxed_store(pr.roundTripTime, peer->roundTripTime);
    interlocked_relaxed_store(pr.roundTripTimeVariance, peer->roundTripTimeVariance);
    interlocked_relaxed_store(pr.lastRoundTripTime, peer->lastRoundTripTime);
    interlocked_relaxed_store(pr.lowestRoundTripTime, peer->lowestRoundTripTime);
    interlocked_relaxed_store(pr.lastRoundTripTimeVariance, peer->lastRoundTripTimeVariance);
    interlocked_relaxed_store(pr.packetLoss, peer->packetLoss);
  }
}


void DaNetPeerInterface::SendUnconnected(const SystemAddress &target, dag::ConstSpan<uint8_t> data)
{
  debug("DaNetPeerInterface::SendUnconnected: -> %s data=%zu bytes", target.ToString(), data.size());
  _ENetPacket *packet = enet_packet_create(data.data(), data.size(), 0);
  if (!packet)
  {
    logerr("DaNetPeerInterface::SendUnconnected: enet_packet_create failed");
    return;
  }
  packet->userData = (void *)(uintptr_t)(((uint64_t)target.host << 16) | target.port);
  WinAutoLock l(packetsCrit);
  unconnectedToSend.push_back(packet);
  os_event_set(&packetsEvent);
}

int DaNetPeerInterface::sendPacketsInQueue(Tab<PacketToSend *> &packetsTmp, DaNetTime cur_time)
{
  int countPackets = 0;
  {
    WinAutoLock l(packetsCrit);
    countPackets = packetsToSend.size();
    packetsTmp.swap(packetsToSend);
    interlocked_relaxed_store(packetsToSendActualSize, packetsToSend.size());

    for (int i = 0; i < packetsToDup.size(); ++i)
    {
      PacketToSend *p2d = packetsToDup[i];
      if (cur_time >= p2d->getDupAt())
      {
        packetsTmp.push_back(p2d);
        erase_items(packetsToDup, i--, 1);
      }
      else
        break;
    }
  }

  for (PacketToSend *pkt : packetsTmp)
  {
#ifdef _DEBUG_TAB_
    G_FAST_ASSERT(pkt->referenceCount);
#endif
    --pkt->referenceCount;

    if (DAGOR_LIKELY(!(pkt->flags & DANET_PACKET_FLAG_BROADCAST)))
    {
      ENetPeer *peer = pkt->getPeer();
      uint8_t chn = pkt->getChannel(peerResponsiveness[PEER2IDX(peer)].unordChannel, peer->channelCount);
      send_enet_packet(peer, chn, pkt);
    }
    else // broadcast
    {
      int pi = 0;
      for (ENetPeer *cp = host->peers, *cpe = &host->peers[host->peerCount]; cp < cpe; ++cp, ++pi)
      {
        if (cp->state != ENET_PEER_STATE_CONNECTED || (pkt->getPeer() && cp == pkt->getPeer()))
          continue;
        uint8_t chn = pkt->getChannel(peerResponsiveness[pi].unordChannel, cp->channelCount);
        send_enet_packet(cp, chn, pkt);
      }
    }

    if (!pkt->referenceCount) // send failed?
      enet_packet_destroy(pkt);
  }
  packetsTmp.clear();
  return countPackets;
}

void DaNetPeerInterface::sendUnconnectedQueue()
{
  Tab<_ENetPacket *> unconn(DANET_MEM);
  {
    WinAutoLock l(packetsCrit);
    unconn.swap(unconnectedToSend);
  }
  if (unconn.empty())
    return;
  host->serviceTime = enet_time_get();
  for (_ENetPacket *packet : unconn)
  {
    uint64_t packed = (uint64_t)(uintptr_t)packet->userData;
    ENetAddress addr;
    addr.host = (enet_uint32)(packed >> 16);
    addr.port = (enet_uint16)(packed & 0xFFFF);
    ENetBuffer ebuf;
    ebuf.data = packet->data;
    ebuf.dataLength = packet->dataLength;
    debug("DaNetPeerInterface::sendUnconnectedQueue: -> %s data=%u bytes", ((const SystemAddress &)addr).ToString(),
      packet->dataLength);
    enet_dagor_manual_send_immediately(host, &addr, packet->dataLength > 0 ? &ebuf : nullptr);
    enet_packet_destroy(packet);
  }
}

void DaNetPeerInterface::processDisconnectCommands()
{
  WinAutoLock l(packetsCrit);
  for (auto &dc : disconnectCommands)
    enet_peer_disconnect_later(dc.peer, dc.cause);
  disconnectCommands.clear();
}

void DaNetPeerInterface::receivePacket(SystemIndex sys_idx, const SystemAddress &sys_addr, _ENetPacket *packet, DaNetTime cur_time)
{
  G_FAST_ASSERT(packet);
  if (!packet->data && packet->dataLength > 0)
  {
    logerr("empty packet from %s", sys_addr.ToString());
    enet_packet_destroy(packet);
    return;
  }

  Packet &p = *new Packet;
  p.systemIndex = sys_idx;
  p.systemAddress = sys_addr;
  p.systemAddress._unused = 0;
  p.length = (uint32_t)packet->dataLength;
  p.bitSize = (uint32_t)BYTES_TO_BITS(packet->dataLength);
  p.data = (uint8_t *)packet->data;
  p.receiveTime = cur_time;
  p.enet_packet = packet;

  WinAutoLock l(packetsCrit);
  receivedPackets.push_back(&p);
}

/* static */
int DaNetPeerInterface::client_update_last_server_receive_time_cb(_ENetHost *host, _ENetEvent *)
{
  DaNetPeerInterface *peerIf = ((DaNetPeerInterface *)host) - 1; //-V1027 intentional container-of pattern
  if (!peerIf->forcedHostAddr.host || peerIf->forcedHostAddr.host == host->receivedAddress.host)
    interlocked_relaxed_store(peerIf->peerResponsiveness[0].lastReceiveTime, host->serviceTime);
  return 0;
}

void DaNetPeerInterface::updateEnet(DaNetTime cur_time)
{
  int numPacketsInBatch = 0;
  ENetEvent event;
  auto receivePacketInBatch = [this, &numPacketsInBatch, cur_time](SystemIndex idx, const SystemAddress &addr, _ENetPacket *packet) {
    receivePacket(idx, addr, packet, cur_time);
    return ++numPacketsInBatch < 64;
  };
  if (interlocked_acquire_load(delayed_set_traffic_encoder))
  {
    setTrafficEncoderImpl(traffic_encoder);
    traffic_encoder = nullptr;
    interlocked_release_store(delayed_set_traffic_encoder, false);
  }
  int r = enet_host_service(host, /*event*/ NULL, /*timeout*/ 0); // send/receive
  if (r < 0)
  {
#if _TARGET_PC_WIN | _TARGET_XBOX
    int last_error = WSAGetLastError();
#else
    int last_error = errno;
#endif
    G_UNUSED(last_error);
    logwarn("enet_host_service() failed, last_error = %d", last_error);
  }
  do // dispatch incoming events...
  {
    enet_host_check_events(host, &event);
    switch (event.type)
    {
      case ENET_EVENT_TYPE_NONE:
      {
        if (interlocked_relaxed_load(packetsToSendActualSize) == 0)
          os_event_wait(&packetsEvent, sleep_time);
        return;
      }
      break;

      case ENET_EVENT_TYPE_RECEIVE:
      {
        G_ASSERT(event.peer);
        G_ASSERT(event.packet);

        if (!forcedHostAddr.host || forcedHostAddr.host == event.peer->address.host)
          interlocked_relaxed_store(peerResponsiveness[PEER2IDX(event.peer)].lastReceiveTime, cur_time);
        if (!receivePacketInBatch(PEER2IDX(event.peer), (const SystemAddress &)event.peer->address, event.packet))
          return;
      }
      break;

      case ENET_EVENT_TYPE_CONNECT:
      {
        G_ASSERT(event.peer);
        int peerIdx = PEER2IDX(event.peer);
        if (peerIdx == relayPeerIdx)
        {
          if (relayStatusHandler)
            relayStatusHandler(true);
        }
        debug("ENET_EVENT_TYPE_CONNECT from %s, peer #%d, data=%x", ((SystemAddress &)event.peer->address).ToString(), peerIdx,
          event.data);

        int numConnectedPeers = 0;
        for (ENetPeer *cp = host->peers; cp < &host->peers[host->peerCount]; ++cp)
          numConnectedPeers += (int)(cp->state >= ENET_PEER_STATE_CONNECTING && cp->state <= ENET_PEER_STATE_CONNECTED);
        if (numConnectedPeers <= maximumIncomingConnections)
        {
          uint8_t b = IS_CLIENT_MODE ? ID_CONNECTION_REQUEST_ACCEPTED : ID_NEW_INCOMING_CONNECTION;
          if (b == ID_CONNECTION_REQUEST_ACCEPTED)
            host->peers[0].timeoutMaximum = ENET_PEER_TIMEOUT_MAXIMUM;

          interlocked_relaxed_store(peerResponsiveness[peerIdx].connectionStartTime, cur_time);

          if (!receivePacketInBatch(peerIdx, (const SystemAddress &)event.peer->address, InplaceDataPacket::create({b, event.data})))
            return;
        }
        else
        {
          logwarn("No free slots to connect, # connected peers %u > maximumIncomingConnections %u", numConnectedPeers,
            maximumIncomingConnections);
          enet_peer_disconnect(event.peer, DC_SERVER_FULL);
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        G_ASSERT(event.peer);
        if (PEER2IDX(event.peer) == relayPeerIdx)
        {
          relayPeerIdx = UNASSIGNED_SYSTEM_INDEX;
          if (relayStatusHandler)
            relayStatusHandler(false);
        }

        memset(&peerResponsiveness[PEER2IDX(event.peer)], 0, sizeof(PeerResponsiveness));

        DisconnectionCause dc;
        if (IS_CLIENT_MODE && host->totalReceivedPackets == 0)
          dc = DC_CONNECTION_ATTEMPT_FAILED;
        else if (event.data < DC_NUM)
          dc = (DisconnectionCause)event.data;
        else
          dc = DC_CONNECTION_LOST;


        debug("ENET_EVENT_TYPE_DISCONNECT from %s, peer #%d, cause=%s(%d)", ((SystemAddress &)event.peer->address).ToString(),
          (int)PEER2IDX(event.peer), describe_disconnection_cause(dc), event.data);

        if (!receivePacketInBatch(PEER2IDX(event.peer), (const SystemAddress &)event.peer->address,
              InplaceDataPacket::create({ID_DISCONNECT, dc})))
          return;
      }
      break;

      case ENET_EVENT_TYPE_DAGOR_RECEIVE_UNCONNECTED:
        G_ASSERT(event.packet);
        // if unconnected has no data it requires to further processing, it's likely simply udp punch, it's job is done at this point
        if (event.packet->data == nullptr)
          break;
        if (!receivePacketInBatch(UNASSIGNED_SYSTEM_INDEX, (const SystemAddress &)event.address, event.packet))
          return;
        break;
      default: G_ASSERTF(0, "invalid enet event.type %d", event.type);
    };
  } while (interlocked_relaxed_load(packetsToSendActualSize) == 0); // ...while outgoing queue is empty
}

DaNetPeerExecutionContext::DaNetPeerExecutionContext()
{
  curTime = enet_time_get();
  prevTime = DaNetTime();
  packetsTmp = Tab<PacketToSend *>();
}

void DaNetPeerInterface::execute() // thread func
{
  G_ASSERT(!is_threaded);
  if (IS_CLIENT_MODE)
    DaThread::applyThisThreadAffinity(WORKER_THREADS_AFFINITY_USE);
  G_ASSERT(host);
  DaNetPeerExecutionContext context = DaNetPeerExecutionContext();
  while (!isThreadTerminating())
  {
    ExecuteSingleUpdate(context);
  }
}

void DaNetPeerInterface::ExecuteSingleUpdate(DaNetPeerExecutionContext &context)
{
  int sendQSize = 0;
  {
    sendQSize = sendPacketsInQueue(context.packetsTmp, context.curTime);
    sendUnconnectedQueue();
    echoManager.process();
    processDisconnectCommands();
    updateEnet(context.curTime);
    if (forcedHostAddr.host) // we have to rewrite it every cycle as it's might get overridden within enet on receives from prev host
    {
      host->peers[0].address.host = forcedHostAddr.host;
      if (forcedHostAddr.port)
        host->peers[0].address.port = forcedHostAddr.port;
    }
    updatePeerResponsiveness(context.curTime);
  }
  context.prevTime = context.curTime, context.curTime = enet_time_get();
  if ((context.curTime - context.prevTime) > DANET_LAG_MS)
    debug("danet_thread lag %d > %d (before/after enet update %d/%d) ms, sendq size=%d", (context.curTime - context.prevTime),
      host->serviceTime - context.prevTime, context.curTime - host->serviceTime, DANET_LAG_MS, sendQSize);
  G_UNUSED(sendQSize);
}

eastl::optional<danet::EchoResponse> DaNetPeerInterface::ReceiveEchoResponse() { return echoManager.receive(); }

Packet *DaNetPeerInterface::Receive()
{
  WinAutoLock l(packetsCrit);
  if (!receivedPackets.size())
    return NULL;
  Packet *r = receivedPackets[0];
  erase_items(receivedPackets, 0, 1);
  return r;
}

void DaNetPeerInterface::DeallocatePacket(Packet *p)
{
  G_ASSERT(p);
  if (p)
  {
    enet_packet_destroy(p->enet_packet);
    delete p;
  }
}

void DaNetPeerInterface::SendEcho(const char *route, uint32_t route_id) { echoManager.sendEcho(route, route_id); }

bool DaNetPeerInterface::Send(dag::ConstSpan<uint8_t> data, PacketPriority pri, PacketReliability reliability, uint8_t orderingChannel,
  SystemIndex system_index, bool broadcast, int dup_delay_ms)
{
  G_ASSERT_RETURN(!data.empty(), false);

  if (orderingChannel >= DANET_DEFINED_CHANNELS)
  {
    logerr("invalid orderingChannel %d", orderingChannel);
    return false;
  }

  if (!broadcast && system_index == UNASSIGNED_SYSTEM_INDEX)
  {
    logerr("can't send packet to UNASSIGNED_SYSTEM_INDEX");
    return false;
  }

  ENetPeer *peer = NULL;
  if (system_index != UNASSIGNED_SYSTEM_INDEX)
  {
    peer = GET_PEER(system_index);
    if (!peer)
      return false;
  }

  uint32_t flags = 0;
  if (reliability == RELIABLE_ORDERED)
    flags |= ENET_PACKET_FLAG_RELIABLE;
  else if (reliability == RELIABLE_UNORDERED)
    flags |= ENET_PACKET_FLAG_RELIABLE | DANET_PACKET_FLAG_RELIABLE_UNORDERED;
  else if (reliability == UNRELIABLE) // unsequenced flag is not supported along with reliable flag
    flags |= ENET_PACKET_FLAG_UNSEQUENCED;

  if (broadcast)
    flags |= DANET_PACKET_FLAG_BROADCAST;

  PacketToSend *p2s = PacketToSend::create(orderingChannel, peer, data, flags, dup_delay_ms ? (danet::GetTime() + dup_delay_ms) : 0);
  {
    WinAutoLock l(packetsCrit);
    packetsToSend.push_back(p2s);

    if (dup_delay_ms)
    {
      if (packetsToDup.empty() || p2s->getDupAt() >= packetsToDup.back()->getDupAt()) // fast/most common path
        packetsToDup.push_back(p2s);
      else
      {
        // Keep it sorted in order to be able exit early if time is not yet come
        auto it = eastl::lower_bound(packetsToDup.begin(), packetsToDup.end(), p2s->getDupAt(),
          [](const PacketToSend *a, int at) { return a->getDupAt() < at; });
        insert_item_at(packetsToDup, eastl::distance(packetsToDup.begin(), it), p2s);
#ifdef _DEBUG_TAB_
        for (int i = 1; i < packetsToDup.size(); ++i) // ensure that it's sorted
          G_ASSERT(packetsToDup[i]->getDupAt() >= packetsToDup[i - 1]->getDupAt());
#endif
      }
    }
    interlocked_relaxed_store(packetsToSendActualSize, packetsToSend.size());
  }

  if (interlocked_relaxed_load(packetsToSendActualSize) > 4096) // Note: number is pretty arbitrary. To consider: gather high water
                                                                // mark of send queue sizes and use that data for tuning it
    os_event_set(&packetsEvent);

  if (pri == SYSTEM_PRIORITY)
    os_event_set(&packetsEvent);

  return true;
}

void DaNetPeerInterface::CloseConnection(const SystemAddress &addr, DisconnectionCause cause)
{
  G_ASSERT(host);
  if (ENetPeer *peer = get_peer_by_addr(host, addr))
    CloseConnection(PEER2IDX(peer), cause);
}

void DaNetPeerInterface::CloseConnection(SystemIndex idx, DisconnectionCause cause)
{
  G_ASSERT(host);
  if (ENetPeer *peer = GET_PEER(idx))
  {
    WinAutoLock l(packetsCrit);
    new (&disconnectCommands.push_back(), _NEW_INPLACE) DisconnectCommand{peer, cause};
  }
}

bool DaNetPeerInterface::Connect(const char *hosta, uint16_t port, uint32_t connect_data, bool is_relay_connection)
{
  G_ASSERT(host);
  G_ASSERT(hosta);
  ENetAddress addr;
  if (enet_address_set_host(&addr, hosta) < 0)
  {
    debug("enet_address_set_host(%s) failed", hosta);
    return false;
  }
  addr.port = port;
  ENetPeer *hostpeer = nullptr;
  if (is_relay_connection && relayPeerIdx != UNASSIGNED_SYSTEM_INDEX)
  {
    logerr("Can't establish relay connection when one is already established");
    return false;
  }
  if (is_relay_connection)
    hostpeer = enet_dagor_host_connect_peer_from_the_end(host, &addr, DANET_MAX_CHANNELS, connect_data);
  else
    hostpeer = enet_host_connect(host, &addr, DANET_MAX_CHANNELS, connect_data);
  if (hostpeer)
  {
    ucr_send_hello(host->socket, &addr);
    hostpeer->timeoutMaximum = DANET_CONNECT_REQUEST_TIMEOUT_MS;
    if ((char *)host == (char *)(this + 1))
      host->intercept = client_update_last_server_receive_time_cb;
    host->totalReceivedPackets = host->totalReceivedData = 0;
    if (is_relay_connection)
      relayPeerIdx = PEER2IDX(hostpeer);
    if (!is_threaded)
      DaThread::start();
    return true;
  }
  else
  {
    logerr("enet_host_connect failed");
    return false;
  }
}

void DaNetPeerInterface::SetMaximumIncomingConnections(uint32_t max_incoming_connections)
{
  G_ASSERTF(!peerResponsiveness.size() || max_incoming_connections <= peerResponsiveness.size(),
    "SetMaximumIncomingConnections()"
    " after Startup() can be called only for reduction (%d >= %d)",
    max_incoming_connections, peerResponsiveness.size());
  maximumIncomingConnections =
    peerResponsiveness.size() ? min(max_incoming_connections, (uint32_t)peerResponsiveness.size()) : max_incoming_connections;
}


SystemAddress DaNetPeerInterface::GetPeerSystemAddress(SystemIndex sys_idx) const
{
  const ENetPeer *peer = GET_PEER(sys_idx);
  if (peer && peer->state == ENET_PEER_STATE_CONNECTED)
    return SystemAddress(peer->address.host, peer->address.port);
  else
    return UNASSIGNED_SYSTEM_ADDRESS;
}

bool DaNetPeerInterface::IsPeerConnected(const SystemAddress &sa) const { return get_peer_by_addr(host, sa) != NULL; }

bool DaNetPeerInterface::ChangeHostAddress(const char *new_host)
{
  if (auto addr = get_enet_address(new_host); addr.host)
  {
    forcedHostAddr.host = addr.host;
    forcedHostAddr.port = addr.port;
    return true;
  }
  return false;
}

void DaNetPeerInterface::AllowReceivePlaintext(SystemIndex system_index, bool allow)
{
  G_ASSERT(host);
  ENetPeer *peer = GET_PEER(system_index);
  if (peer)
  {
    peer->useStickyCompressionFlag = !allow;
    peer->stickyCompressionFlag &= !allow;
  }
}

const DaNetStatistics *DaNetPeerInterface::GetStatistics(SystemIndex sys_idx) const
{
  if (sys_idx == UNASSIGNED_SYSTEM_INDEX)
  {
    g_stat.bytesSent = interlocked_relaxed_load(enet_tx_bytes);
    g_stat.bytesReceived = interlocked_relaxed_load(enet_rx_bytes);
    g_stat.bytesDropped = interlocked_relaxed_load(enet_tx_dropped_bytes);
    g_stat.packetsSent = interlocked_relaxed_load(enet_tx_packets);
    g_stat.packetsReceived = interlocked_relaxed_load(enet_rx_packets);
    return &g_stat;
  }
  else
    return NULL; // not supported
}

void DaNetPeerInterface::ResetTrafficStatistics()
{
  memset(&g_stat, 0, sizeof(g_stat));
  g_stat.connectionStartTime = danet::GetTime();
  interlocked_relaxed_store(enet_rx_bytes, 0);
  interlocked_relaxed_store(enet_rx_packets, 0);
  interlocked_relaxed_store(enet_tx_bytes, 0);
  interlocked_relaxed_store(enet_tx_dropped_bytes, 0);
  interlocked_relaxed_store(enet_tx_packets, 0);
}

static size_t ENET_CALLBACK compress_proxy(void *context, enet_uint16 peer_id, const ENetBuffer *inBuffers, size_t inBufferCount,
  size_t inLimit, enet_uint8 *outData, size_t outLimit)
{
  return ((IDaNetTrafficEncoder *)context)->encode(peer_id, inBuffers, inBufferCount, inLimit, outData, outLimit);
}

static size_t ENET_CALLBACK decompress_proxy(void *context, enet_uint16 peer_id, const enet_uint8 *inData, size_t inLimit,
  enet_uint8 *outData, size_t outLimit)
{
  return ((IDaNetTrafficEncoder *)context)->decode(peer_id, inData, inLimit, outData, outLimit);
}

void DaNetPeerInterface::SetTrafficEncoder(IDaNetTrafficEncoder *enc)
{
  traffic_encoder = enc;
  interlocked_release_store(delayed_set_traffic_encoder, true);
}

void DaNetPeerInterface::setTrafficEncoderImpl(IDaNetTrafficEncoder *enc)
{
  if (enc)
  {
    ENetCompressor c;
    memset(&c, 0, sizeof(c));
    c.context = enc;
    c.compress = compress_proxy;
    c.decompress = decompress_proxy;
    enet_host_compress(host, &c);
  }
  else
    enet_host_compress(host, NULL);
}
