/**
 @file  enet_dagor.c
 @brief ENet Dagor extensions

 Contains all Dagor-specific additions so that diffs against upstream ENet
 are confined to minimal hook-points in the original source files.

 Features:
   - Unconnected messages  (ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED)
   - Manual framed send    (enet_dagor_manual_send_command / _immediately)
   - Relay ping-target     (enet_dagor_protocol_handle_ping_target_port_for_relay)
   - Relay pong / 2nd RTT  (ENET_PROTOCOL_COMMAND_DAGOR_RELAY_PONG)
*/
#include <stdio.h>
#include <string.h>
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"
#include "enet/enet_dagor.h"
#include "enet/time.h"
/* inet_pton lives in <ws2tcpip.h> on Windows (not <winsock2.h>); on Unix it comes from
   <arpa/inet.h> via enet/unix.h which the platform header chain already pulls in. */
#if defined(_WIN32) || defined(_WIN64) || _TARGET_PC_WIN || _TARGET_XBOX
#include <ws2tcpip.h>
#endif
/* The macro in enet_dagor.h routes callers' enet_address_set_host() to our wrapper; undef
   it inside this TU so the wrapper's fallback can reach the upstream function. */
#undef enet_address_set_host
#if _DAGOR_ENET_EXT
#define HOSTPORT_FMT    "%d.%d.%d.%d:%d"
#define HOSTPORT(h, p)  (h) & 255, ((h) >> 8) & 255, ((h) >> 16) & 255, (h) >> 24, (p)

/* =========================================================================
   Unconnected message delivery
   ========================================================================= */

/**
 Dequeues one incoming unconnected message from the special peer.

 @param peer        the host->unconnectedMessagesPeer
 @param addressFrom filled with the sender's address; may be NULL
 @returns packet on success, NULL when the queue is empty
*/
ENetPacket *
enet_dagor_peer_receive_unconnected_messages (ENetPeer * peer, ENetAddress * addressFrom)
{
    ENetIncomingCommand * incomingCommand;
    ENetPacket * packet;

    if (enet_list_empty (& peer -> dispatchedCommands))
      return NULL;

    incomingCommand = (ENetIncomingCommand *) enet_list_remove (enet_list_begin (& peer -> dispatchedCommands));

    if (addressFrom != NULL)
    {
        addressFrom -> host = incomingCommand -> command.dagorSendUnconnected.hostFrom;
        addressFrom -> port = incomingCommand -> command.dagorSendUnconnected.portFrom;
    }

    packet = incomingCommand -> packet;

    -- packet -> referenceCount;

    enet_free (incomingCommand);

    peer -> totalWaitingData -= packet -> dataLength;

    return packet;
}


/** Initiates a connection to a foreign host. Wrapps normal enet_host_connect but find peer from the end of the list
 * this is useful when you have peers that are order dependent which need to be allocated first and match index by index some outside array and order independent.
    @param host host seeking the connection
    @param address destination for the connection
    @param channelCount number of channels to allocate
    @param data user data supplied to the receiving host
    @returns a peer representing the foreign host on success, NULL on failure
    @remarks The peer returned will have not completed the connection until enet_host_service()
    notifies of an ENET_EVENT_TYPE_CONNECT event for the peer.
*/
ENetPeer *
enet_dagor_host_connect_peer_from_the_end (ENetHost * host, const ENetAddress * address, size_t channelCount, enet_uint32 data)
{
    ENetPeer * currentPeer = NULL;
    size_t i;

    for (i = host -> peerCount; i > 0; --i)
    {
       if (host -> peers [i - 1].state == ENET_PEER_STATE_DISCONNECTED)
       {
          currentPeer = & host -> peers [i - 1];
          break;
       }
    }

    if (currentPeer == NULL)
      return NULL;
    host -> dagorConnectPeerOverride = currentPeer;
    return enet_host_connect (host, address, channelCount, data);
}

/* =========================================================================
   Protocol handler - SEND_UNCONNECTED (incoming)
   ========================================================================= */

int
enet_dagor_protocol_handle_send_unconnected (ENetHost * host, ENetProtocol * command, enet_uint8 ** currentData)
{
    size_t dataLength;

    /* enet_peer_queue_incoming_command indexes peer->channels[channelID] unchecked; the
       unconnected peer has one channel, so reject out-of-range channelID before queueing. */
    if (command -> header.channelID >= ENET_DAGOR_UNCONNECTED_PEER (host) -> channelCount)
    {
        enet_logf ("unconnected message channelID out of range: %d", (int) command -> header.channelID);
        return -1;
    }

    dataLength = ENET_NET_TO_HOST_16 (command -> dagorSendUnconnected.dataLength);
    * currentData += dataLength;

    if (dataLength > host -> maximumPacketSize)
    {
        enet_logf ("unconnected message data length is bad %d %d", (int) dataLength, (int) host -> receivedDataLength);
        return -1;
    }
    if (* currentData < host -> receivedData)
    {
        enet_logf ("unconnected message data location is bad %p %p", * currentData, host -> receivedData);
        return -1;
    }
    if (* currentData > & host -> receivedData [host -> receivedDataLength])
    {
        enet_logf ("unconnected message data end location is bad %p %p",
                   * currentData, & host -> receivedData [host -> receivedDataLength]);
        return -1;
    }

    command -> dagorSendUnconnected.hostFrom = host -> receivedAddress.host;
    command -> dagorSendUnconnected.portFrom = host -> receivedAddress.port;

    if (enet_peer_queue_incoming_command (
        ENET_DAGOR_UNCONNECTED_PEER (host),
        command,
        (const enet_uint8 *) command + sizeof (ENetProtocolDagorSendUnconnected),
        dataLength, 0, 0
    ) == NULL)
    {
        enet_logf ("couldn't enqueue unconnected message");
        return -1;
    }

    return 0;
}

/* =========================================================================
   Manual immediate send (peerless, no ENet connection state)
   ========================================================================= */

/**
 Sends a peerless ENet-framed datagram: protocol header + one command (+ optional payload).
 Relay uses this for SEND_UNCONNECTED punches and for RELAY_PONG answers.
*/
void
enet_dagor_manual_send_command (ENetHost * host, const ENetAddress * addr, const ENetProtocol * command,
                                const ENetBuffer * ebuf)
{
    enet_uint8 headerData [sizeof (ENetProtocolHeader) + sizeof (enet_uint32)];
    ENetProtocolHeader * protocolHeader = (ENetProtocolHeader *) headerData;
    size_t commandSize;
    int buffersCount;
    enet_uint16 flags;
    ENetBuffer immediate_send_buffers [3];

    if (host == NULL || addr == NULL || command == NULL)
      return;

    flags  = ENET_PROTOCOL_MAXIMUM_PEER_ID;
    flags |= ENET_PROTOCOL_HEADER_FLAG_SENT_TIME;
    protocolHeader -> peerID    = ENET_HOST_TO_NET_16 (flags);
    protocolHeader -> sentTime  = ENET_HOST_TO_NET_16 (host -> serviceTime & 0xFFFF);

    commandSize = enet_protocol_command_size (command -> header.command);
    if (commandSize == 0)
      return;

    immediate_send_buffers [0].data       = (void *) protocolHeader;
    immediate_send_buffers [0].dataLength = sizeof (ENetProtocolHeader);
    immediate_send_buffers [1].data       = (void *) command;
    immediate_send_buffers [1].dataLength = commandSize;
    if (ebuf != NULL)
    {
        immediate_send_buffers [2].data       = ebuf -> data;
        immediate_send_buffers [2].dataLength = ebuf -> dataLength;
        buffersCount = 3;
    }
    else
      buffersCount = 2;

    if (host -> checksum != NULL)
    {
        enet_uint32 * checksum = (enet_uint32 *) & headerData [sizeof (ENetProtocolHeader)];
        * checksum = 0;
        immediate_send_buffers [0].dataLength += sizeof (enet_uint32);
        * checksum = host -> checksum (immediate_send_buffers, buffersCount);
    }

#if ENET_DEBUG
    {
        int i;
        int total = 0;
        enet_logf ("enet_dagor_manual_send_command buffers:");
        for (i = 0; i < buffersCount; i++)
        {
            ENetDagorHexDebugDumpBuffer hexbuf;
            hexbuf.current = hexbuf.buf;
            enet_dagor_build_buffer_hex (& hexbuf, (unsigned char *) immediate_send_buffers [i].data,
                              immediate_send_buffers [i].dataLength);
            enet_dagor_print_buffer_hex (& hexbuf, "  ");
            total += (int) immediate_send_buffers [i].dataLength;
        }
        enet_logf ("total bytes: %d", total);
    }
#endif

    enet_logf ("manual_send_command: cmd=%u -> " HOSTPORT_FMT " payloadLen=%d",
               (unsigned) (command -> header.command & ENET_PROTOCOL_COMMAND_MASK),
               HOSTPORT (addr -> host, addr -> port), (int) (ebuf ? ebuf -> dataLength : 0));
    enet_socket_send (host -> socket, addr, immediate_send_buffers, buffersCount);
}

/** SEND_UNCONNECTED convenience wrapper (payload optional). */
void
enet_dagor_manual_send_immediately (ENetHost * host, const ENetAddress * addr, const ENetBuffer * ebuf)
{
    ENetProtocol command;

    memset (& command, 0, sizeof (command));
    command.header.command   = ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED;
    command.header.channelID = 0;
    command.dagorSendUnconnected.dataLength = ENET_HOST_TO_NET_16 ((enet_uint16) (ebuf ? ebuf -> dataLength : 0));
    enet_dagor_manual_send_command (host, addr, & command, ebuf);
}

/* =========================================================================
   Protocol handler - PING_TARGET_PORT_FOR_RELAY (triggers UDP punch)
   ========================================================================= */

int
enet_dagor_protocol_handle_ping_target_port_for_relay (ENetHost * host, ENetPeer * peer, const ENetProtocol * command)
{
    ENetAddress addr;

    /* Peer-supplied host:port = source-spoofing reflector; reject non-connected peers.
       Return -1 (not 0) so the generic ACK path cannot emit a packet for this command. */
    if (peer -> state != ENET_PEER_STATE_CONNECTED)
    {
        enet_logf ("ping_target relay from non-connected peer (state=%d), dropping", (int) peer -> state);
        return -1;
    }

    addr.host = command -> dagorPingTargetPortForRelay.host;
    if (!addr.host)
        addr.host = peer -> address.host;
    addr.port = command -> dagorPingTargetPortForRelay.port;
    enet_dagor_manual_send_immediately (host, & addr, NULL);
    enet_logf ("punch udp hole for peer " HOSTPORT_FMT, HOSTPORT (addr.host, addr.port));
    return 0;
}

/* =========================================================================
   Protocol handler - RELAY_PONG (second RTT on the session peer)
   ========================================================================= */

static ENetPeer *
enet_dagor_find_connected_peer_by_address (ENetHost * host, const ENetAddress * addr)
{
    size_t i;
    for (i = 0; i < host -> peerCount; ++ i)
    {
        ENetPeer * peer = & host -> peers [i];
        if (peer -> state == ENET_PEER_STATE_CONNECTED &&
            peer -> address.host == addr -> host &&
            peer -> address.port == addr -> port)
          return peer;
    }
    return NULL;
}

int
enet_dagor_protocol_handle_relay_pong (ENetHost * host, ENetPeer * peer, const ENetProtocol * command)
{
    enet_uint32 receivedSentTime;
    enet_uint32 sample;

    if (peer == NULL)
      peer = enet_dagor_find_connected_peer_by_address (host, & host -> receivedAddress);
    if (peer == NULL || peer -> state != ENET_PEER_STATE_CONNECTED)
      return 0;

    /* Same 16-bit sentTime unwrap as enet_protocol_handle_acknowledge. */
    receivedSentTime = ENET_NET_TO_HOST_16 (command -> dagorRelayPong.receivedSentTime);
    receivedSentTime |= host -> serviceTime & 0xFFFF0000;
    if ((receivedSentTime & 0x8000) > (host -> serviceTime & 0x8000))
        receivedSentTime -= 0x10000;

    if (ENET_TIME_LESS (host -> serviceTime, receivedSentTime))
      return 0;

    sample = ENET_TIME_DIFFERENCE (host -> serviceTime, receivedSentTime);
    if (peer -> relayRoundTripTime == 0)
      peer -> relayRoundTripTime = sample;
    else if (sample >= peer -> relayRoundTripTime)
      peer -> relayRoundTripTime += (sample - peer -> relayRoundTripTime) / 8;
    else
      peer -> relayRoundTripTime -= (peer -> relayRoundTripTime - sample) / 8;
    return 0;
}

int
enet_dagor_datagram_header_sent_time (const enet_uint8 * data, size_t data_length, enet_uint16 * out_sent_time)
{
    const ENetProtocolHeader * header;
    enet_uint16 peerID;

    if (data == NULL || out_sent_time == NULL || data_length < sizeof (ENetProtocolHeader))
      return 0;

    header = (const ENetProtocolHeader *) data;
    peerID = ENET_NET_TO_HOST_16 (header -> peerID);
    if ((peerID & ENET_PROTOCOL_HEADER_FLAG_SENT_TIME) == 0)
      return 0;

    * out_sent_time = ENET_NET_TO_HOST_16 (header -> sentTime);
    return 1;
}

int
enet_dagor_datagram_is_relay_pong (const enet_uint8 * data, size_t data_length)
{
    const ENetProtocolHeader * header;
    const ENetProtocolCommandHeader * cmd;
    enet_uint16 peerID;

    if (data == NULL || data_length < sizeof (ENetProtocolHeader) + sizeof (ENetProtocolCommandHeader))
      return 0;

    header = (const ENetProtocolHeader *) data;
    peerID = ENET_NET_TO_HOST_16 (header -> peerID);
    if ((peerID & ENET_PROTOCOL_HEADER_FLAG_COMPRESSED) != 0)
      return 0;
    if ((peerID & ~ (ENET_PROTOCOL_HEADER_FLAG_MASK | ENET_PROTOCOL_HEADER_SESSION_MASK)) != ENET_PROTOCOL_MAXIMUM_PEER_ID)
      return 0;
    if ((peerID & ENET_PROTOCOL_HEADER_FLAG_SENT_TIME) == 0)
      return 0;

    cmd = (const ENetProtocolCommandHeader *) (data + sizeof (ENetProtocolHeader));
    return ((cmd -> command & ENET_PROTOCOL_COMMAND_MASK) == ENET_PROTOCOL_COMMAND_DAGOR_RELAY_PONG) ? 1 : 0;
}

int
enet_dagor_host_service_only_incoming_commands (ENetHost * host, ENetEvent * event)
{
  if (event != NULL)
  {
    event -> type = ENET_EVENT_TYPE_NONE;
    event -> peer = NULL;
    event -> packet = NULL;
  }

  host -> serviceTime = enet_time_get ();

  switch (enet_protocol_receive_incoming_commands (host, event))
  {
  case 1:
    return 1;

  case -1:
#ifdef ENET_DEBUG
    perror ("Error receiving incoming packets");
#endif

    return -1;

  default:
    break;
  }
  return 0;
}

/** Sends a ping_target request to a peer.
    @param peer destination for the ping request
    @param port destination for the answer unreliable message
    @remarks meant for relay NAT punchthrough, it asks remote peer to send message to target port on this host
*/
void
enet_dagor_peer_ping_target_request (ENetPeer * peer, enet_uint32 host, enet_uint16 port)
{
    ENetProtocol command;

    if (peer -> state != ENET_PEER_STATE_CONNECTED)
    {
      enet_logf ("ping_target_request: peer not connected (state=%d), dropping", (int) peer -> state);
      return;
    }

    command.header.command = ENET_PROTOCOL_COMMAND_DAGOR_PING_TARGET_PORT_FOR_RELAY;
    command.header.channelID = 0xFF;
    command.dagorPingTargetPortForRelay.host = host;
    command.dagorPingTargetPortForRelay.port = port;

    enet_logf ("ping_target_request: queuing command to peer " HOSTPORT_FMT " -> punch " HOSTPORT_FMT,
               HOSTPORT (peer -> address.host, peer -> address.port), HOSTPORT (host, port));
    enet_peer_queue_outgoing_command (peer, & command, NULL, 0, 0);
}

/* =========================================================================
   Debug hex-dump helpers
   ========================================================================= */

#if ENET_DEBUG

void
enet_dagor_build_buffer_hex (ENetDagorHexDebugDumpBuffer * buffer, const unsigned char * data, size_t len)
{
    int currentOffset;

    if (buffer -> current == NULL)
        buffer -> current = buffer -> buf;

    currentOffset = (int) (buffer -> current - buffer -> buf);
    if (currentOffset + (int) len > ENET_DAGOR_HEX_DUMP_MAX_SIZE - 1)
        len = (size_t) (ENET_DAGOR_HEX_DUMP_MAX_SIZE - currentOffset - 1);

    for (; len > 0; -- len, ++ data)
        buffer -> current += sprintf (buffer -> current, "%02X ", (unsigned) * data);

    * buffer -> current = '\0';
}

void
enet_dagor_print_buffer_hex (ENetDagorHexDebugDumpBuffer * buffer, const char * prefix)
{
    enet_logf ("%s%s", prefix, buffer -> buf);
}

#endif /* ENET_DEBUG */

/* =========================================================================
   Host-name resolution wrapper
   ========================================================================= */

/**
 IP-literal-first wrapper around enet_address_set_host. Short-circuits IP literals to avoid
 the upstream gethostbyname / sceNetResolverStartNtoa, which can block for seconds on Windows
 and PS even when the input is "127.0.0.1".

 No recursion: enet_dagor.h's macro renames enet_address_set_host to this wrapper, but the
 #undef above and the fact that win32.c/unix.c do NOT include enet_dagor.h keep the fallback
 below pointed at the upstream definition. Do not include enet_dagor.h from those TUs.
*/
int
enet_dagor_address_set_host (ENetAddress * address, const char * name)
{
    if (inet_pton (AF_INET, name, & address -> host) == 1)
        return 0;
    return enet_address_set_host (address, name);
}

#endif