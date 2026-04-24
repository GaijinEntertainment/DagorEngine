/**
 @file  enet_dagor.c
 @brief ENet Dagor extensions

 Contains all Dagor-specific additions so that diffs against upstream ENet
 are confined to minimal hook-points in the original source files.

 Features:
   - Unconnected messages  (ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED)
   - Manual immediate send (enet_dagor_manual_send_immediately)
   - Relay ping-target     (enet_dagor_protocol_handle_ping_target_port_for_relay)
*/
#include <stdio.h>
#include <string.h>
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"
#include "enet/enet_dagor.h"
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
    ENetPeer * currentPeer;

    for (currentPeer = & host -> peers [host -> peerCount - 1];
         currentPeer >= & host -> peers [0];
        --currentPeer)
    {
       if (currentPeer -> state == ENET_PEER_STATE_DISCONNECTED)
         break;
    }

    if (currentPeer < & host -> peers [0])
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

    dataLength = ENET_NET_TO_HOST_16 (command -> dagorSendUnconnected.dataLength);
    * currentData += dataLength;

    if (dataLength > host -> maximumPacketSize)
    {
        enet_logf ("unconnected message data length is bad %d %d", dataLength, host -> receivedDataLength);
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

static ENetBuffer immediate_send_buffers [3];
static enet_uint32 emptyBufferPlaceholder;
/**
 Sends a single ENet-framed datagram directly without a peer.

 @param host         source host (socket + checksum callback used)
 @param command_type one of the supported ENetProtocolCommand values above
 @param addr         destination address
 @param ebuf         payload buffer (required for SEND_UNCONNECTED)
*/
void
enet_dagor_manual_send_immediately (ENetHost * host, const ENetAddress * addr, const ENetBuffer * ebuf)
{
    enet_uint8 headerData [sizeof (ENetProtocolHeader) + sizeof (enet_uint32)];
    ENetProtocolHeader * protocolHeader = (ENetProtocolHeader *) headerData;
    ENetProtocol command;
    size_t commandSize;
    int buffersCount;
    enet_uint16 flags;

    flags  = ENET_PROTOCOL_MAXIMUM_PEER_ID;
    flags |= ENET_PROTOCOL_HEADER_FLAG_SENT_TIME;
    protocolHeader -> peerID    = ENET_HOST_TO_NET_16 (flags);
    protocolHeader -> sentTime  = ENET_HOST_TO_NET_16 (host -> serviceTime & 0xFFFF);

    command.header.command   = ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED;
    command.header.channelID = 0;
    commandSize              = enet_protocol_command_size (command.header.command);

    buffersCount = 3;

    command.dagorSendUnconnected.dataLength = ENET_HOST_TO_NET_16 ((enet_uint16) (ebuf ? ebuf -> dataLength : 0));

    /* Set up all buffers before computing the checksum so the checksum
       covers the real payload, not uninitialised memory. */
    immediate_send_buffers [0].data       = (void *) protocolHeader;
    immediate_send_buffers [0].dataLength = sizeof (ENetProtocolHeader);
    immediate_send_buffers [1].data       = (void *) & command;
    immediate_send_buffers [1].dataLength = commandSize;
    if (ebuf != NULL)
    {
        immediate_send_buffers [2].data       = ebuf -> data;
        immediate_send_buffers [2].dataLength = ebuf -> dataLength;
    }
    else
    {
        immediate_send_buffers [2].data       = &emptyBufferPlaceholder;
        immediate_send_buffers [2].dataLength = 0;
    }

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
        enet_logf ("enet_dagor_manual_send_immediately buffers:");
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

    enet_logf ("manual_send_immediately: -> " HOSTPORT_FMT " dataLen=%d",
               HOSTPORT (addr -> host, addr -> port), (int) (ebuf ? ebuf -> dataLength : 0));
    enet_socket_send (host -> socket, addr, immediate_send_buffers, buffersCount);
}

/* =========================================================================
   Protocol handler - PING_TARGET_PORT_FOR_RELAY (triggers UDP punch)
   ========================================================================= */

int
enet_dagor_protocol_handle_ping_target_port_for_relay (ENetHost * host, ENetPeer * peer, const ENetProtocol * command)
{
    ENetAddress addr;
    addr.host = command -> dagorPingTargetPortForRelay.host;
    if (!addr.host)
        addr.host = peer -> address.host;
    addr.port = command -> dagorPingTargetPortForRelay.port;
    enet_dagor_manual_send_immediately (host, & addr, NULL);
    enet_logf ("punch udp hole for peer " HOSTPORT_FMT, HOSTPORT (addr.host, addr.port));
    return 0;
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

#endif