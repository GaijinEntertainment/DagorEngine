/**
 @file  enet_dagor_protocol.h
 @brief ENet Dagor extensions - private protocol-layer declarations

 This header is for internal use by protocol.c and enet_dagor.c only.
 Public API additions live in enet.h as usual.
*/
#ifndef __ENET_DAGOR_TYPES_H__
#define __ENET_DAGOR_TYPES_H__
#if _DAGOR_ENET_EXT
/** Use this instead of peerCount when allocating the peers array to get one extra slot for unconnected messages. */
#define ENET_DAGOR_ALLOC_PEER_COUNT(n) ((n) + 1)
/** Access the extra peer slot used for storing unconnected incoming messages. */
#define ENET_DAGOR_UNCONNECTED_PEER(host) (&(host)->peers[(host)->peerCount])
#else
#define ENET_DAGOR_ALLOC_PEER_COUNT(n) (n)
#endif

#if _DAGOR_ENET_EXT
#define _DAGOR_ENET_INJECT_PROTOCOL_COMMANDS_AND_COUNT \
   ENET_PROTOCOL_COMMAND_DAGOR_PING_TARGET_PORT_FOR_RELAY = 13, \
   ENET_PROTOCOL_COMMAND_DAGOR_SEND_UNCONNECTED = 14,          \
   ENET_PROTOCOL_COMMAND_COUNT              = 15
#else
#define _DAGOR_ENET_INJECT_PROTOCOL_COMMANDS_AND_COUNT \
   ENET_PROTOCOL_COMMAND_COUNT              = 13
#endif

#if _DAGOR_ENET_EXT
#define _DAGOR_ENET_EXT_PROTOCOL_TYPES                     \
typedef struct _ENetProtocolDagorPingTargetPortForRelay    \
{                                                          \
   ENetProtocolCommandHeader header;                       \
   enet_uint16 padding_discard;                            \
   enet_uint16 port;                                       \
   enet_uint32 host;                                       \
} ENET_PACKED ENetProtocolDagorPingTargetPortForRelay;     \
typedef struct _ENetProtocolDagorSendUnconnected           \
{                                                          \
   ENetProtocolCommandHeader header;                       \
   enet_uint16 dataLength;                                 \
   enet_uint16 portFrom;                                   \
   enet_uint32 hostFrom;                                   \
} ENET_PACKED ENetProtocolDagorSendUnconnected;
#else
#define _DAGOR_ENET_EXT_PROTOCOL_TYPES
#endif

#if _DAGOR_ENET_EXT
#define _DAGOR_ENET_INJECT_PROTOCOL_TYPES                               \
   ENetProtocolDagorPingTargetPortForRelay dagorPingTargetPortForRelay; \
   ENetProtocolDagorSendUnconnected dagorSendUnconnected;

#define _DAGOR_ENET_INJECT_PROTOCOL_TYPES_SIZES       \
    sizeof (ENetProtocolDagorPingTargetPortForRelay), \
    sizeof (ENetProtocolDagorSendUnconnected)


#define _DAGOR_ENET_INJECT_EVENTS                                                     \
   /* a packet has been received from an unconnected endpoint. The peer field is NULL.             \
      The channelID field is empty. The packet field contains                                      \
      the packet that was received;                                                                \
      The address field contains sender endpoint for identification.                               \
      It's possible that a connected endpoint sent an unconnected message, in theory.              \
      The meaning of this is to be determined by the application logic.                            \
      This packet must be destroyed with enet_packet_destroy after use. */                         \
   ENET_EVENT_TYPE_DAGOR_RECEIVE_UNCONNECTED = 4,                                                  \
                                                                                                   \
   /** a packet has been received and intercepted via host->intercept callback.                    \
      Interceptor states that enet should not process as whatever meaning                         \
      it had has already been processed */                                                        \
   ENET_EVENT_TYPE_DAGOR_INTERCEPTED = 5

/** Extra fields injected into ENetEvent. */
#define _DAGOR_ENET_EXTEND_EVENT_STRUCT \
   ENetAddress          address;   /**< Internet address of the unconnected/peerless event */

/** Extra fields injected into ENetHost. */
#define _DAGOR_ENET_EXTEND_HOST_STRUCT \
   ENetPeer *           dagorConnectPeerOverride; /**< when set, enet_host_connect uses this peer directly instead of searching */

#else
   #define _DAGOR_ENET_INJECT_PROTOCOL_TYPES
   #define _DAGOR_ENET_INJECT_PROTOCOL_TYPES_SIZES
   #define _DAGOR_ENET_INJECT_EVENTS
   #define _DAGOR_ENET_EXTEND_EVENT_STRUCT
   #define _DAGOR_ENET_EXTEND_HOST_STRUCT
#endif
#endif /* __ENET_DAGOR_TYPES_H__ */
