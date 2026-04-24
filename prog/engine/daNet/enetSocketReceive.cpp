// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <enet/enet.h>
#include "daNetEchoManagerInternal.h"
#include "ucr.h"
#include <debug/dag_assert.h>

extern "C" int enet_socket_receive_impl(ENetSocket socket, ENetAddress *address, ENetBuffer *buffers, size_t bufferCount);

extern "C" int enet_socket_receive(ENetSocket socket, ENetAddress *address, ENetBuffer *buffers, size_t bufferCount)
{
  G_ASSERT(bufferCount == 1); // enet_socket_receive can put received data in several buffers, but it's never used this way
  G_ASSERT(buffers && buffers->data);

  auto receive = [&] { return enet_socket_receive_impl(socket, address, buffers, bufferCount); };
  int ret = receive();
  for (; ret > 0; ret = receive())
  {
    const int receiveLength = ret; // positive result of receive is the length of received data
    if (DAGOR_UNLIKELY(ucr_is_packet(buffers->data, receiveLength)))
      ucr_handle_packet(socket, address, buffers->data, receiveLength);
    else if (auto echoNetPacket = static_cast<const EchoNetPacket *>(buffers->data);
             DAGOR_UNLIKELY(receiveLength == sizeof(EchoNetPacket) && echoNetPacket->marker == ECHO_PACKET_MARKER))
      handle_echo_packet(socket, address, *echoNetPacket);
    else
      break; // Return to enet
  }

  return ret;
}
