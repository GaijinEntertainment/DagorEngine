//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

//
// TODO: this need to be completely reworked (demarry DaNet commands & user packet types)
//
enum DefaultMessageIDTypes : uint8_t
{
  //
  // RESERVED TYPES - DO NOT CHANGE THESE
  // All types from DaNetPeer
  //
  /// These types are never returned to the user.
  ID_MESSAGE_ID_UNKNOWN, /*0*/
  ID_UNUSED_1,           /*1*/
  /// Ping from an unconnected system.  Only reply if we have open connections. Do not update timestamps. (internal use only)
  ID_PING_OPEN_CONNECTIONS, /*2*/
  ID_UNUSED_3,              /*3*/
  /// Asking for a new connection (internal use only)
  ID_CONNECTION_REQUEST, /*4*/
  /// Connecting to a secured server/peer (internal use only)
  ID_SECURED_CONNECTION_RESPONSE, /*5*/
  /// Connecting to a secured server/peer (internal use only)
  ID_SECURED_CONNECTION_CONFIRMATION, /*6*/
  /// Packet that tells us the packet contains an integer ID to name mapping for the remote system (internal use only)
  ID_RPC_MAPPING, /*7*/
  /// A reliable packet to detect lost connections (internal use only)
  ID_DETECT_LOST_CONNECTIONS, /*8*/
  /// Packet with arbitrary size to ensure it can be delivered (internal use only)
  ID_UNUSED_9, /*9*/
  /// Response to the probe request signaling it was delivered (internal use only)
  ID_UNUSED_10, /*10*/
  /// Remote procedure call (internal use only)
  ID_RPC, /*11*/
  /// Remote procedure call reply, for RPCs that return data (internal use only)
  ID_RPC_REPLY, /*12*/
  /// DaNetPeer - Same as ID_ADVERTISE_SYSTEM, but intended for internal use rather than being passed to the user. Second byte
  /// indicates type. Used currently for NAT punchthrough for receiver port advertisement. See ID_NAT_ADVERTISE_RECIPIENT_PORT
  ID_OUT_OF_BAND_INTERNAL, /*13*/


  //
  // USER TYPES - DO NOT CHANGE THESE
  //

  /// DaNetPeer - In a client/server environment, our connection request to the server has been accepted.
  ID_CONNECTION_REQUEST_ACCEPTED, /*14*/
  /// DaNetPeer - Sent to the player when a connection request cannot be completed due to inability to connect.
  ID_UNUSED_16, /*15*/
  /// DaNetPeer - Sent a connect request to a system we are currently connected to.
  ID_ALREADY_CONNECTED, /*16*/
  /// DaNetPeer - A remote system has successfully connected.
  ID_NEW_INCOMING_CONNECTION, /*17*/
  ID_UNUSED_18,               /*18*/
  /// Disconnect event (actual reason will be in data[1] byte, see DisconnectReason enum)
  ID_DISCONNECT, /*19*/
  ID_UNUSED_20,  /*20*/
  ID_UNUSED_21,  /*21*/
  ID_UNUSED_22,  /*22*/
  /// DaNetPeer - The remote system is using a password and has refused our connection because we did not set the correct password.
  ID_INVALID_PASSWORD, /*23*/
  /// DaNetPeer - A packet has been tampered with in transit.  The sender is contained in Packet::systemAddress.
  ID_MODIFIED_PACKET, /*24*/
  ID_UNUSED_25,       /*25*/
  ID_UNUSED_26,       /*26*/
  ID_UNUSED_27,
  /// ConnectionGraph plugin - In a client/server environment, a client other than ourselves has disconnected gracefully.
  /// Packet::systemAddress is modified to reflect the systemAddress of this client.
  ID_REMOTE_DISCONNECTION_NOTIFICATION, /*28*/
  /// ConnectionGraph plugin - In a client/server environment, a client other than ourselves has been forcefully dropped.
  /// Packet::systemAddress is modified to reflect the systemAddress of this client.
  ID_REMOTE_CONNECTION_LOST, /*29*/
  /// ConnectionGraph plugin - In a client/server environment, a client other than ourselves has connected.  Packet::systemAddress is
  /// modified to reflect the systemAddress of the client that is not connected directly to us. The packet encoding is SystemAddress 1,
  /// ConnectionGraphGroupID 1, SystemAddress 2, ConnectionGraphGroupID 2
  ID_REMOTE_NEW_INCOMING_CONNECTION, /*30*/
  // DaNetPeer - Downloading a large message. Format is ID_DOWNLOAD_PROGRESS (MessageID), partCount (unsigned int), partTotal (unsigned
  // int), partLength (unsigned int), first part data (length <= MAX_MTU_SIZE). See the three parameters partCount, partTotal and
  // partLength in OnFileProgress in FileListTransferCBInterface.h
  ID_DOWNLOAD_PROGRESS, /*31*/

  /// FileListTransfer plugin - Setup data
  ID_FILE_LIST_TRANSFER_HEADER, /*32*/
  /// FileListTransfer plugin - A file
  ID_FILE_LIST_TRANSFER_FILE, /*33*/

  /// DirectoryDeltaTransfer plugin - Request from a remote system for a download of a directory
  ID_DDT_DOWNLOAD_REQUEST, /*34*/

  /// DaNetTransport plugin - Transport provider message, used for remote console
  ID_TRANSPORT_STRING, /*35*/

  /// ReplicaManager plugin - Create an object
  ID_REPLICA_MANAGER_CONSTRUCTION, /*36*/
  /// ReplicaManager plugin - Destroy an object
  ID_REPLICA_MANAGER_DESTRUCTION, /*37*/
  /// ReplicaManager plugin - Changed scope of an object
  ID_REPLICA_MANAGER_SCOPE_CHANGE, /*38*/
  /// ReplicaManager plugin - Serialized data of an object
  ID_REPLICA_MANAGER_SERIALIZE, /*39*/
  /// ReplicaManager plugin - New connection, about to send all world objects
  ID_REPLICA_MANAGER_DOWNLOAD_STARTED, /*40*/
  /// ReplicaManager plugin - Finished downloading all serialized objects
  ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE, /*41*/

  /// ConnectionGraph plugin - Request the connection graph from another system
  ID_CONNECTION_GRAPH_REQUEST, /*42*/
  /// ConnectionGraph plugin - Reply to a connection graph download request
  ID_CONNECTION_GRAPH_REPLY, /*43*/
  /// ConnectionGraph plugin - Update edges / nodes for a system with a connection graph
  ID_CONNECTION_GRAPH_UPDATE, /*44*/
  /// ConnectionGraph plugin - Add a new connection to a connection graph
  ID_CONNECTION_GRAPH_NEW_CONNECTION, /*45*/
  /// ConnectionGraph plugin - Remove a connection from a connection graph - connection was abruptly lost
  ID_CONNECTION_GRAPH_CONNECTION_LOST, /*46*/
  /// ConnectionGraph plugin - Remove a connection from a connection graph - connection was gracefully lost
  ID_CONNECTION_GRAPH_DISCONNECTION_NOTIFICATION, /*47*/

  /// Router plugin - route a message through another system
  ID_ROUTE_AND_MULTICAST, /*48*/

  /// RakVoice plugin - Open a communication channel
  ID_RAKVOICE_OPEN_CHANNEL_REQUEST, /*49*/
  /// RakVoice plugin - Communication channel accepted
  ID_RAKVOICE_OPEN_CHANNEL_REPLY, /*50*/
  /// RakVoice plugin - Close a communication channel
  ID_RAKVOICE_CLOSE_CHANNEL, /*51*/
  /// RakVoice plugin - Voice data
  ID_RAKVOICE_DATA, /*52*/

  /// Autopatcher plugin - Get a list of files that have changed since a certain date
  ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE, /*53*/
  /// Autopatcher plugin - A list of files to create
  ID_AUTOPATCHER_CREATION_LIST, /*54*/
  /// Autopatcher plugin - A list of files to delete
  ID_AUTOPATCHER_DELETION_LIST, /*55*/
  /// Autopatcher plugin - A list of files to get patches for
  ID_AUTOPATCHER_GET_PATCH, /*56*/
  /// Autopatcher plugin - A list of patches for a list of files
  ID_AUTOPATCHER_PATCH_LIST, /*57*/
  /// Autopatcher plugin - Returned to the user: An error from the database repository for the autopatcher.
  ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR, /*58*/
  /// Autopatcher plugin - Finished getting all files from the autopatcher
  ID_AUTOPATCHER_FINISHED_INTERNAL, /*59*/
  ID_AUTOPATCHER_FINISHED,          /*60*/
  /// Autopatcher plugin - Returned to the user: You must restart the application to finish patching.
  ID_AUTOPATCHER_RESTART_APPLICATION, /*61*/

  /// NATPunchthrough plugin - Intermediary got a request to help punch through a nat
  ID_NAT_PUNCHTHROUGH_REQUEST, /*62*/
  /// NATPunchthrough plugin - Intermediary cannot complete the request because the target system is not connected
  ID_NAT_TARGET_NOT_CONNECTED, /*63*/
  /// NATPunchthrough plugin - While attempting to connect, we lost the connection to the target system
  ID_NAT_TARGET_CONNECTION_LOST, /*64*/
  /// NATPunchthrough plugin - Internal message to connect at a certain time
  ID_NAT_CONNECT_AT_TIME, /*65*/
  /// NATPunchthrough plugin - Internal message to send a message (to punch through the nat) at a certain time
  ID_NAT_SEND_OFFLINE_MESSAGE_AT_TIME, /*66*/
  /// NATPunchthrough plugin - The facilitator is already attempting this connection
  ID_NAT_IN_PROGRESS, /*67*/

  /// LightweightDatabase plugin - Query
  ID_DATABASE_QUERY_REQUEST, /*68*/
  /// LightweightDatabase plugin - Update
  ID_DATABASE_UPDATE_ROW, /*69*/
  /// LightweightDatabase plugin - Remove
  ID_DATABASE_REMOVE_ROW, /*70*/
  /// LightweightDatabase plugin - A serialized table.  Bytes 1+ contain the table.  Pass to TableSerializer::DeserializeTable
  ID_DATABASE_QUERY_REPLY, /*71*/
  /// LightweightDatabase plugin - Specified table not found
  ID_DATABASE_UNKNOWN_TABLE, /*72*/
  /// LightweightDatabase plugin - Incorrect password
  ID_DATABASE_INCORRECT_PASSWORD, /*73*/

  /// ReadyEvent plugin - Set the ready state for a particular system
  ID_READY_EVENT_SET, /*74*/
  /// ReadyEvent plugin - Unset the ready state for a particular system
  ID_READY_EVENT_UNSET, /*75*/
  /// All systems are in state ID_READY_EVENT_SET
  ID_READY_EVENT_ALL_SET, /*76*/
  /// ReadyEvent plugin - Request of ready event state - used for pulling data when newly connecting
  ID_READY_EVENT_QUERY, /*77*/

  /// Lobby packets. Second byte indicates type.
  ID_LOBBY_GENERAL, /*78*/

  /// Auto RPC procedure call
  ID_AUTO_RPC_CALL, /*79*/

  /// Auto RPC functionName to index mapping
  ID_AUTO_RPC_REMOTE_INDEX, /*80*/

  /// Auto RPC functionName to index mapping, lookup failed. Will try to auto recover
  ID_AUTO_RPC_UNKNOWN_REMOTE_INDEX, /*81*/

  /// Auto RPC error code
  /// See AutoRPC.h for codes, stored in packet->data[1]
  ID_RPC_REMOTE_ERROR, /*82*/

  /// FileListTransfer transferring large files in chunks that are read only when needed, to save memory
  ID_FILE_LIST_REFERENCE_PUSH, /*83*/

  /// Force the ready event to all set
  ID_READY_EVENT_FORCE_ALL_SET, /*84*/

  // Rooms function
  ID_ROOMS_EXECUTE_FUNC, /*85*/

  // For the user to use.  Start your first enumeration at this value.
  ID_USER_PACKET_ENUM, /*86*/
  //-------------------------------------------------------------------------------------------------------------

};
