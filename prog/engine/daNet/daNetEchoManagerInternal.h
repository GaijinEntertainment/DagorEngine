// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

#ifdef _WIN32
using ENetSocket = uintptr_t;
#else
using ENetSocket = int;
#endif
struct _ENetAddress;

// Supposedly the enet doesn't send the first 2 bytes with all bits set
using EchoPacketMarkerType = uint16_t;
static constexpr EchoPacketMarkerType ECHO_PACKET_MARKER = 0xFFFF;

#pragma pack(push, 1)
struct EchoNetPacketBeforeChecksum
{
  EchoNetPacketBeforeChecksum(EchoPacketMarkerType marker, uint32_t sequenceNumber, bool response) :
    marker{marker}, sequenceNumber{sequenceNumber}, response{uint32_t{response}}
  {}

  EchoPacketMarkerType marker;
  uint32_t sequenceNumber : 31;
  uint32_t response : 1; // MSB
};

struct EchoNetPacket : EchoNetPacketBeforeChecksum
{
  using EchoNetPacketBeforeChecksum::EchoNetPacketBeforeChecksum;
  uint16_t checksum{};
};
#pragma pack(pop)

void handle_echo_packet(ENetSocket socket, _ENetAddress *receivedFrom, const EchoNetPacket &echoNetPacket);

static_assert(sizeof(EchoNetPacket) == 8, "size of EchoNetPacket is expected to be independent of platform/compiler/anything");
