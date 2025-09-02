//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "daNetEchoManager.h"
#include "daNetTypes.h"
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_events.h>
#include "bitStream.h"
#include <generic/dag_tab.h>
#include "packetPriority.h"
#include "disconnectionCause.h"

#include <EASTL/optional.h>

struct DaNetStatistics;
struct _ENetEvent;
struct _ENetHost;
struct _ENetPeer;
struct _ENetAddress;
struct _ENetBuffer;
struct _ENetPacket;
struct PacketToSend;

class IDaNetTrafficEncoder
{
public:
  virtual ~IDaNetTrafficEncoder() = default;
  // return encoded size or 0 on error
  virtual size_t encode(SystemIndex sys_idx, const _ENetBuffer *buffers, size_t bufCount, size_t inLimit, uint8_t *outData,
    size_t outLimit) = 0;
  // return decoded size or 0 on error
  virtual size_t decode(SystemIndex sys_idx, const uint8_t *inData, size_t inLimit, uint8_t *outData, size_t outLimit) = 0;
};

struct DaNetPeerExecutionContext
{
  DaNetPeerExecutionContext();

public:
  Tab<PacketToSend *> packetsTmp;
  DaNetTime curTime;
  DaNetTime prevTime;
};

class DaNetPeerInterface : public DaThread
{
public:
  static constexpr int DEF_BLOCK_DURATION = 600;

  DaNetPeerInterface(_ENetHost *ehost = nullptr, bool is_threaded = false);
  ~DaNetPeerInterface();

  static DaNetPeerInterface *create(size_t asz = 0, void **outp = nullptr);

  bool Startup(uint16_t maxCon, int sleep_time, const SocketDescriptor *sd = NULL);
  void Stop(DaNetTime block_duration = DEF_BLOCK_DURATION, DisconnectionCause cause = DC_CONNECTION_STOPPED);
  void Shutdown(DaNetTime block_duration = DEF_BLOCK_DURATION);

  eastl::optional<danet::EchoResponse> ReceiveEchoResponse();
  Packet *Receive();
  void DeallocatePacket(Packet *);

  void SendEcho(const char *route, uint32_t route_id);

  bool Send(dag::ConstSpan<uint8_t> data, PacketPriority priority, PacketReliability reliability, uint8_t orderingChannel,
    SystemIndex system_index, bool broadcast,
    int dup_delay_ms = 0); // If not 0 - this message will be duplicated after this number of milliseconds

  bool Send(const danet::BitStream &bs, PacketPriority priority_, PacketReliability reliability, uint8_t orderingChannel,
    SystemIndex system_index, bool broadcast, int dup_delay_ms = 0)
  {
    return Send(bs.getSlice(), priority_, reliability, orderingChannel, system_index, broadcast, dup_delay_ms);
  }

  bool Connect(const char *host, uint16_t port, uint32_t connect_data = 0, bool is_relay_connection = false);
  void CloseConnection(const SystemAddress &addr, DisconnectionCause cause = DC_CONNECTION_CLOSED);
  void CloseConnection(SystemIndex idx, DisconnectionCause cause = DC_CONNECTION_CLOSED);

  SystemAddress GetPeerSystemAddress(SystemIndex sys_idx) const; // return UNASSIGNED_SYSTEM_ADDRESS on error

  bool IsPeerConnected(const SystemAddress &a) const;
  bool IsPeerResponsive(SystemIndex sys_idx) const;

  bool IsRelayConnection(SystemIndex sys_idx) const { return sys_idx == relayPeerIdx; }


  const DaNetStatistics *GetStatistics(SystemIndex sys_idx) const;

  danet::PeerQoSStat GetPeerQoSStat(SystemIndex sys_idx) const;
  int GetLastPing(SystemIndex sys_idx) const;
  int GetLowestPing(SystemIndex sys_idx) const;
  int GetAveragePing(SystemIndex sys_idx) const;
  int GetMaximumPacketSize(SystemIndex sys_idx) const;

  void SetBandwidthLimits(uint32_t incoming_bps = 0, uint32_t outgoing_bps = 0, uint32_t outgoing_bps_per_peer = 0);

  DaNetTime GetLastReceivedPacketTime(SystemIndex idx) const; // return 0 if unknown
  DaNetTime GetConnectionStartTime(SystemIndex idx) const;    // return 0 if unknown

  void SetMaximumIncomingConnections(uint32_t max_incoming_connections);
  uint32_t GetMaximumIncomingConnections() const
  {
    return maximumIncomingConnections;
  } // might return 0 if Startup() & SetMaximumIncomingConnections() is not called

  uint32_t GetPeerConnectID(SystemIndex idx) const; // return 0 on error

  void SetupPeerTimeouts(SystemIndex idx, uint32_t max_timeout_ms, uint32_t min_timeout_ms = 0, uint32_t min_timeout_limit = 0);

  void SetTrafficEncoder(IDaNetTrafficEncoder *enc);
  void ResetTrafficStatistics();

  bool ChangeHostAddress(const char *host_addr); // 0th peer is implied (hence this is rather client-oriented API)

  void AllowReceivePlaintext(SystemIndex system_index, bool allow);

  const _ENetHost *GetHostRaw() const { return host; }

  _ENetHost *GetHostMutable() const { return host; }

  const _ENetPeer *GetENetPeer(SystemIndex sys_idx) const;

  void ExecuteSingleUpdate(DaNetPeerExecutionContext &context);

private:
  virtual void execute();
  bool handleInternalPacket(_ENetEvent *e, DaNetTime cur_time);
  void sendPacketsInQueue(Tab<PacketToSend *> &packetsTmp, DaNetTime cur_time);
  void receivePacket(_ENetPeer *from, _ENetPacket *packet, DaNetTime cur_time);
  void updateNetworkSimulator(DaNetTime cur_time);
  void processDisconnectCommands();
  void updateEnet(DaNetTime cur_time);
  void updatePeerResponsiveness(DaNetTime cur_time);

  static int __cdecl client_update_last_server_receive_time_cb(_ENetHost *host, _ENetEvent *);

private:
  _ENetHost *host; // client or server host actually
  int sleep_time;  // how often wake up in thread for handling network events
  uint32_t maximumIncomingConnections;
  bool is_threaded;
  WinCritSec packetsCrit; // guard send queue, receive queue & packets pool
  SystemIndex relayPeerIdx = UNASSIGNED_SYSTEM_INDEX;

  struct DisconnectCommand
  {
    _ENetPeer *peer;
    DisconnectionCause cause;
  };

  // queues
  Tab<Packet *> receivedPackets;
  Tab<PacketToSend *> packetsToSend;
  Tab<PacketToSend *> packetsToDup;          // sorted by (dup at) time
  Tab<DisconnectCommand> disconnectCommands; // To consider: unify with packetsToSend in one cmdbuf
  struct
  {
    uint32_t host;
    uint16_t port;
  } forcedHostAddr = {0, 0}; // force host (peer[0]) address (in network byte order)

  os_event_t packetsEvent;

  struct PeerResponsiveness
  {
    DaNetTime connectionStartTime;
    DaNetTime lastReceiveTime;
    bool notResponsive;
    uint8_t unordChannel;
  };
  Tab<PeerResponsiveness> peerResponsiveness;
  DaNetTime responsivenessUpdateStamp;

  danet::EchoManager echoManager;
};
