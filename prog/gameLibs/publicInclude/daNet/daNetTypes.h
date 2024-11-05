//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

//
// misc defines
//
#define BITS_TO_BYTES(x)              (((x) + 7) >> 3)
#define BITS_TO_BYTES_WORD_ALIGNED(x) ((((x) + 31) >> 5) << 2)
#define BYTES_TO_BITS(x)              ((x) << 3)

#define DANET_MEM tmpmem // runtime instead ?

struct _ENetPacket;

//
// DaNet types and structs
//
struct NetworkID
{
  uint16_t localSystemAddress;

  inline bool operator==(const NetworkID &rhs) const { return localSystemAddress == rhs.localSystemAddress; }

  inline bool operator!=(const NetworkID &rhs) const { return localSystemAddress != rhs.localSystemAddress; }

  static inline bool IsPeerToPeerMode() { return false; }
};

struct SystemAddress // extended ENetAddress
{
  union
  {
    uint32_t host;
    uint32_t binaryAddress;
  };
  uint16_t port;
  uint16_t _unused;

  const char *ToString(bool withPort = true) const;

  SystemAddress() : host(0), port(0), _unused(0) {}
  SystemAddress(uint32_t h, uint16_t p)
  {
    host = h;
    port = p;
    _unused = 0;
  }

  inline bool operator==(const SystemAddress &rhs) const { return host == rhs.host && port == rhs.port; }

  inline bool operator!=(const SystemAddress &rhs) const { return host != rhs.host || port != rhs.port; }

  void SetBinaryAddress(const char *str);
};
namespace danet
{
class BitStream;
void write_type(BitStream &bs, const SystemAddress &sa);
bool read_type(const BitStream &bs, SystemAddress &sa);
} // namespace danet

struct SocketDescriptor
{
  enum
  {
    STR,
    SOCKET,
    BINARY
  };
  union
  {
    char hostAddress[32];
    intptr_t socket;
    uint32_t host;
  };
  uint16_t port;
  uint8_t type;

  SocketDescriptor() : port(0), type(STR) { hostAddress[0] = 0; }
  SocketDescriptor(uint16_t _port, const char *_hostAddress);
};

typedef uint32_t DaNetTime; // in milliseconds
typedef uint8_t MessageID;
typedef uint32_t BitSize_t;
typedef uint16_t SystemIndex;

struct Packet
{
  SystemIndex systemIndex;
  SystemAddress systemAddress;
  uint32_t length;
  BitSize_t bitSize;
  uint8_t *data;
  DaNetTime receiveTime;
  _ENetPacket *enet_packet;
};

namespace danet
{
class BitStream;
enum Options
{
  DUMP_REFLECTION = 1,        // dump all reflection var serialization and deserialization in log
  AUTO_CONVERT_ENDIANESS = 2, // aut convert endianess in danet::BitStream
};
extern uint32_t option_flags;

struct PeerQoSStat
{
  DaNetTime connectionStartTime = 0;
  int averagePing = 0;
  int averagePingVariance = 0;
  int lowestPing = 0;
  int highestPingVariance = 0;
  float packetLoss = 0; // [0..1]
};
}; // namespace danet

extern const SystemAddress UNASSIGNED_SYSTEM_ADDRESS;
const SystemIndex UNASSIGNED_SYSTEM_INDEX = SystemIndex(-1);
const SystemIndex SERVER_SYSTEM_INDEX = SystemIndex(0);
const NetworkID UNASSIGNED_NETWORK_ID = {65535};
