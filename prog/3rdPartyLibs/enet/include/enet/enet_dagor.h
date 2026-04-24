/**
 @file  enet_dagor_protocol.h
 @brief ENet Dagor extensions - private protocol-layer declarations

 This header is for internal use by protocol.c and enet_dagor.c only.
 Public API additions live in enet.h as usual.
*/
#ifndef __ENET_DAGOR_H__
#define __ENET_DAGOR_H__
#include "enet/protocol.h"
#include "enet/enet.h"
#include "enet/enet_dagor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if _DAGOR_ENET_EXT
/** Called from enet_protocol_handle_incoming_commands for peerless SEND_UNCONNECTED datagrams. */
int enet_dagor_protocol_handle_send_unconnected (ENetHost * host, ENetProtocol * command, enet_uint8 ** currentData);

/** Called from enet_protocol_handle_incoming_commands for PING_TARGET_PORT_FOR_RELAY. */
int enet_dagor_protocol_handle_ping_target_port_for_relay (ENetHost * host, ENetPeer * peer, const ENetProtocol * command);

ENET_API void enet_dagor_manual_send_immediately (ENetHost * host, const ENetAddress * addr, const ENetBuffer * ebuf);
ENET_API void                enet_dagor_peer_ping_target_request (ENetPeer *, enet_uint32 host, enet_uint16 port);
ENET_API ENetPacket *        enet_dagor_peer_receive_unconnected_messages (ENetPeer *, ENetAddress * addressFrom);
ENET_API ENetPeer * enet_dagor_host_connect_peer_from_the_end (ENetHost *, const ENetAddress *, size_t, enet_uint32);
ENET_API int        enet_dagor_host_service_only_incoming_commands (ENetHost *, ENetEvent *);

//only here to share the method, not really part of an API
//not having dagor prefix is intentional, it's a native enet method
int enet_protocol_receive_incoming_commands (ENetHost * host, ENetEvent * event);

/** Debug hex-dump helpers (compiled only when ENET_DEBUG is set). */
#if ENET_DEBUG

#define ENET_DAGOR_HEX_DUMP_MAX_SIZE 2048

typedef struct _ENetDagorHexDebugDumpBuffer
{
    char  buf [ENET_DAGOR_HEX_DUMP_MAX_SIZE];
    char *current;  /**< write cursor; NULL means uninitialised (enet_dagor_build_buffer_hex will init it) */
} ENetDagorHexDebugDumpBuffer;

void enet_dagor_build_buffer_hex (ENetDagorHexDebugDumpBuffer * buffer, const unsigned char * data, size_t len);
void enet_dagor_print_buffer_hex (ENetDagorHexDebugDumpBuffer * buffer, const char * prefix);

#endif /* ENET_DEBUG */

#endif /* _DAGOR_ENET_EXT */

#ifdef __cplusplus
}
#endif

#if _DAGOR_ENET_EXT
#define _DAGOR_ENET_INJECT_CASES_TO_HANDLE_CUSTOM_COMMANDS \
       case ENET_PROTOCOL_COMMAND_DAGOR_PING_TARGET_PORT_FOR_RELAY: \
          if (enet_dagor_protocol_handle_ping_target_port_for_relay (host, peer, command)) \
            goto commandError; \
          break; \
       case ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED: \
          enet_logf("received ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED"); \
          if (enet_dagor_protocol_handle_send_unconnected (host, command, & currentData)) \
            goto commandError; \
          break;
#define _DAGOR_ENET_INJECT_UNCONNECTED_PEER_RECEIVE_MESSAGES \
if (!enet_list_empty (& ENET_DAGOR_UNCONNECTED_PEER (host) -> dispatchedCommands)) \
{ \
  event -> packet = enet_dagor_peer_receive_unconnected_messages (ENET_DAGOR_UNCONNECTED_PEER (host), & event -> address); \
  if (event -> packet == NULL) \
    return 0; \
  event -> type = ENET_EVENT_TYPE_DAGOR_RECEIVE_UNCONNECTED; \
  event -> peer = NULL; \
  event -> channelID = 0; \
  return 1; \
}

#define _DAGOR_ENET_INJECT_INITIALIZE_UNCONNECTED_PEER \
    { \
       ENetAddress fakeAddr; \
       fakeAddr.host = 0; fakeAddr.port = 0; \
       host -> dagorConnectPeerOverride = ENET_DAGOR_UNCONNECTED_PEER (host); \
       enet_host_connect (host, & fakeAddr, 1, 0); \
    }

/* Extra conditions for the peer==NULL peerless-command check in protocol.c.
   Expands to nothing when _DAGOR_ENET_EXT is off. */
#define _DAGOR_ENET_EXTRA_PEERLESS_COMMANDS(n) \
    && (n) != ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED \

/* Extra OR condition in enet_peer_dispatch_incoming_unreliable_commands skip check. */
#define _DAGOR_ENET_EXTRA_UNSEQUENCED_DISPATCH_SKIP(cmd) \
    || ((cmd) & ENET_PROTOCOL_COMMAND_MASK) == ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED

/* Extra AND condition in enet_peer_queue_incoming_command unsequenced guard. */
#define _DAGOR_ENET_EXTRA_SEQUENCED_COMMANDS_CHECK(cmd) \
    && ((cmd) & ENET_PROTOCOL_COMMAND_MASK) != ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED

/* Switch case injected into enet_peer_queue_incoming_command. */
#define _DAGOR_ENET_INJECT_QUEUE_INCOMING_UNCONNECTED_CASE \
    case ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED: \
       currentCommand = enet_list_end (& channel -> incomingUnreliableCommands); \
       peer -> needsDispatch = 1; \
       break;

#define _DAGOR_ENET_INJECT_CLEAR_NEEDS_DISPATCH_ON_UNCONNECTED_PEER ENET_DAGOR_UNCONNECTED_PEER (host) -> needsDispatch = 0;
/* Override peer selection in enet_host_connect: if set, skip the free-peer search loop. */
#define _DAGOR_ENET_OVERRIDE_CURRENT_PEER(peer, host) \
    (peer) = (host)->dagorConnectPeerOverride; \
    (host)->dagorConnectPeerOverride = NULL; \
    if (!peer) \
    {
#define _DAGOR_ENET_OVERRIDE_CURRENT_PEER_END }

/* Connect guard: skip sending the CONNECT command for the fake unconnected peer init. */
#define _DAGOR_ENET_INJECT_CONNECT_GUARD(peer, address) \
    if ((address)->host == 0 && (address)->port == 0) return (peer);

/* Replaces case 1: body in enet_protocol_receive_incoming_commands intercept switch. */
#define _DAGOR_ENET_INTERCEPT_CASE_HANDLING(event, packets) \
             { \
               if ((event) -> type == ENET_EVENT_TYPE_DAGOR_INTERCEPTED) \
               { \
                 if ((packets) == 255) \
                   return 1; \
               } \
               else \
                 return 1; \
             }
#define _DAGOR_ENET_INTERCEPT_CASE_HANDLING_CASE_2_FORCE_INTERRUPT(event, packets) \
          case 2: \
             if ((event) != NULL && (event) -> type != ENET_EVENT_TYPE_NONE) \
               return 1; \
             continue;

/* Debug: log received datagram with hex dump. No-op when ENET_DEBUG is not set. */
#if ENET_DEBUG
#define _DAGOR_ENET_INJECT_DEBUG_LOG_RECEIVED(host, peerID) \
    enet_logf("received some incoming command with peerID %d", (peerID)); \
    { \
        ENetDagorHexDebugDumpBuffer _dbgbuf; \
        _dbgbuf.current = _dbgbuf.buf; \
        enet_dagor_build_buffer_hex (& _dbgbuf, (unsigned char *) (host) -> receivedData, (host) -> receivedDataLength); \
        enet_dagor_print_buffer_hex (& _dbgbuf, "buffer data:"); \
    }
#else
#define _DAGOR_ENET_INJECT_DEBUG_LOG_RECEIVED(host, peerID)
#endif

#else
#define _DAGOR_ENET_INJECT_CASES_TO_HANDLE_CUSTOM_COMMANDS
#define _DAGOR_ENET_INJECT_INITIALIZE_UNCONNECTED_PEER
#define _DAGOR_ENET_INJECT_UNCONNECTED_PEER_RECEIVE_MESSAGES
#define _DAGOR_ENET_EXTRA_PEERLESS_COMMANDS(n)
#define _DAGOR_ENET_EXTRA_UNSEQUENCED_DISPATCH_SKIP(cmd)
#define _DAGOR_ENET_EXTRA_SEQUENCED_COMMANDS_CHECK(cmd)
#define _DAGOR_ENET_INJECT_QUEUE_INCOMING_UNCONNECTED_CASE
#define _DAGOR_ENET_INJECT_CLEAR_NEEDS_DISPATCH_ON_UNCONNECTED_PEER
#define _DAGOR_ENET_INJECT_CONNECT_GUARD(peer, address)
#define _DAGOR_ENET_OVERRIDE_CURRENT_PEER(peer, host)
#define _DAGOR_ENET_OVERRIDE_CURRENT_PEER_END
#define _DAGOR_ENET_INTERCEPT_CASE_HANDLING(event, packets) \
             if ((event) != NULL && (event) -> type != ENET_EVENT_TYPE_NONE) \
               return 1;
#define _DAGOR_ENET_INTERCEPT_CASE_HANDLING_CASE_2_FORCE_INTERRUPT(event, packets)
#define _DAGOR_ENET_INJECT_DEBUG_LOG_RECEIVED(host, peerID)
#endif

#endif /* __ENET_DAGOR_H__ */
