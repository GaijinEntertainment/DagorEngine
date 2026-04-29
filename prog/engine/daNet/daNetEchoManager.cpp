// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/daNetEchoManager.h>
#include "daNetEchoManagerInternal.h"

#include <daNet/getTime.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_critSec.h>
#include "ucr.h"

#include <enet/enet.h>
#include <zlib.h> // crc32

extern ENetAddress get_enet_address(const char *new_host);

// Read from the network thread, written from DaNetPeerInterface ctor before thread started and in dtor after thread stopped
static danet::EchoManager *echo_manager; // We only create one peer interface, which uses only one echo manager

static uint16_t calc_checksum(const EchoNetPacketBeforeChecksum &packet)
{
  // taking only the lowest 16 bits for checksum
  return static_cast<uint16_t>(crc32(0, reinterpret_cast<const unsigned char *>(&packet), sizeof(packet)));
}

static void send_echo_impl(ENetSocket socket, ENetAddress *address, bool response, danet::EchoSequenceNumber sequenceNumber)
{
  EchoNetPacket packet{ECHO_PACKET_MARKER, sequenceNumber, response};
  packet.checksum = calc_checksum(packet);

  ENetBuffer enetBuffer{};
  enetBuffer.data = static_cast<void *>(&packet); // the host is expected to always be LE
  enetBuffer.dataLength = sizeof(EchoNetPacket);

  enet_socket_send(socket, address, &enetBuffer, /*bufferCount*/ 1);
}

void handle_echo_packet(ENetSocket socket, ENetAddress *receivedFrom, const EchoNetPacket &echoNetPacket)
{
  G_ASSERTF(echo_manager != nullptr, "A packet from enet, but there is no EchoManager");

  if (echoNetPacket.checksum != calc_checksum(echoNetPacket))
    return; // checksum doesn't match

#if DAGOR_DBGLEVEL > 0
  echo_manager->sendCount += size_t{!echoNetPacket.response};
  echo_manager->receiveCount++;
#endif

  if (echoNetPacket.response)
    echo_manager->processResponse(echoNetPacket.sequenceNumber);
  else
    // sending pong immediately to ensure smaller RTT, it's not very important, but it's a tad better for correct estimations
    send_echo_impl(socket, receivedFrom, /*response*/ true, echoNetPacket.sequenceNumber);
}

namespace danet
{

// called from the main thread, before net thread is created
EchoManager::EchoManager(DaNetTime echo_timeout_ms) : timeoutMs{echo_timeout_ms}
{
  G_ASSERTF(echo_manager == nullptr, "It's expected that there is only one Echo Manager for code simplicity");
  echo_manager = this;
}

// called from the main thread, after the net thread is terminated
EchoManager::~EchoManager()
{
  echo_manager = nullptr;
#if DAGOR_DBGLEVEL > 0
  debug("Dumping echo stats: sent #%lld (%lld bytes), received #%lld (%lld bytes)", sendCount, sendCount * sizeof(EchoNetPacket),
    receiveCount, receiveCount * sizeof(EchoNetPacket));
#endif
}

// called from the main thread, after the net thread is terminated
void EchoManager::clear()
{
  clear_and_shrink(toSend);
  clear_and_shrink(inFlight);
  clear_and_shrink(received);
  sequenceCounter = 0;
  host = nullptr;
}

// called from the main thread, simultaneous with the net thread
void EchoManager::sendEcho(const char *route, uint32_t route_id)
{
  auto addr = get_enet_address(route);
  WinAutoLock l(crit); // access to "toSend" and "received"

  if (addr.host)
    toSend.push_back(RequestToSend{/*routeId*/ route_id, /*addr*/ {addr.host, addr.port}});
  else
  {
    received.push_back(EchoResponse{/*routeId*/ route_id,
      /*result*/ EchoResponse::Result::UNRESOLVED,
      /*rttOrTimeout*/ timeoutMs});
    LOGERR_ONCE("%s Can't resolve host address %s", __FUNCTION__, route);
  }
}

// called from the main thread, when the net thread isn't running
void EchoManager::setHost(_ENetHost *new_host) { host = new_host; }

// called from the net thread, simultaneous with the main thread
void EchoManager::process()
{
  WinAutoLock l(crit); // access to "received" and "toSend"
  DaNetTime now = danet::GetTime();
  while (!inFlight.empty())
  {
    const RequestInFlight &oldest = inFlight.front();
    const DaNetTime timeInFlight = now - oldest.sendTime;
    if (!oldest.receivedResponse && timeInFlight < timeoutMs)
      break;
    if (!oldest.receivedResponse)
      received.push_back(EchoResponse{/*routeId*/ oldest.routeId,
        /*result*/ EchoResponse::Result::TIMEOUT,
        /*rttOrTimeout*/ timeoutMs});
    erase_items(inFlight, 0, 1);
  }

  G_ASSERTF(host, "Host is a pointer only for late initialization, it's not optional for logic");

  for (const RequestToSend &request : toSend)
  {
    ENetAddress addr{request.addr.host, request.addr.port};
#if DAGOR_DBGLEVEL > 0
    echo_manager->sendCount++;
#endif
    send_echo_impl(host->socket, &addr, /*response*/ false, sequenceCounter);
    inFlight.push_back(RequestInFlight{/*routeId*/ request.routeId,
      /*sequenceNumber*/ sequenceCounter,
      /*sendTime*/ enet_time_get(),
      /*receivedResponse*/ false});
    ++sequenceCounter;
  }

  toSend.clear();
}

// called from the main thread, simultaneous with the net thread
eastl::optional<EchoResponse> EchoManager::receive()
{
  WinAutoLock l(crit); // access to "received"
  if (received.empty())
    return eastl::nullopt;
  const EchoResponse response = received.front();
  erase_items(received, 0, 1);
  return response;
}

// called from the net thread, simultaneous with the main thread
void EchoManager::processResponse(EchoSequenceNumber sequenceNumber)
{
  if (inFlight.empty() || sequenceNumber >= sequenceCounter || sequenceNumber < inFlight.front().sequenceNumber)
    return; // unexpected response sequenceNumber; either timed out, or corrupted, or malicious

  const size_t inFlightIndex = sequenceNumber - inFlight.front().sequenceNumber;
  G_ASSERT((inFlight.back().sequenceNumber + 1) == sequenceCounter);
  G_ASSERT(inFlightIndex < inFlight.size());
  G_ASSERT(inFlight[inFlightIndex].sequenceNumber == sequenceNumber);

  if (inFlight[inFlightIndex].receivedResponse)
    return; // duplicated packet
  inFlight[inFlightIndex].receivedResponse = true;

  const uint32_t routeId = inFlight[inFlightIndex].routeId;
  const DaNetTime rtt = danet::GetTime() - inFlight[inFlightIndex].sendTime;

  WinAutoLock l(crit);
  received.push_back(EchoResponse{/*routeId*/ routeId, /*result*/ EchoResponse::Result::SUCCESS, /*rttOrTimeout*/ rtt});
}

} // namespace danet
