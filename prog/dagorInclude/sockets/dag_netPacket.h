//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>

struct NetMsgPacketHeader
{
  unsigned size;
  unsigned generation;
};

struct NetMsgPacket
{
  NetMsgPacketHeader hdr;
  union
  {
    char data[24];
    unsigned int dwData[6];
    unsigned queryId;
  };

  static NetMsgPacket *create(int sz, IMemAlloc *m = tmpmem)
  {
    NetMsgPacket *p = (NetMsgPacket *)new (m) char[sizeof(NetMsgPacketHeader) + sz];
    p->hdr.size = sz;
    return p;
  }
  static NetMsgPacket *create(void *ptr, int sz)
  {
    NetMsgPacket *p = (NetMsgPacket *)ptr;
    p->hdr.size = sz;
    return p;
  }

  NetMsgPacket *clone(IMemAlloc *m = tmpmem) const
  {
    NetMsgPacket *p = create(hdr.size, m);
    p->hdr = hdr;
    memcpy(p->data, data, hdr.size);
    return p;
  }
};

enum
{ // special meaning of generation!
  NGEN_Query = 0xFFFFFF00U,
  NGEN_Response = 0xFFFFFF01U,
};

enum
{
  // common server queries (processed by server implementation)
  NSQP_GetInfo = 1, // Get info about server
  NSQP_CheckLocal,  // Sends client's computername for server to compare with its own
  NSQP_SERVER_SPEC_QUERY_NUM,

  // specific server queries (processed by server's processor)
  NSQP_USER_START = 2048,
};

struct NetShortQueryPacket : public NetMsgPacket
{
  NetShortQueryPacket(int query, bool response = false, int dwUsed = 2)
  {
    hdr.size = dwUsed * 4;
    hdr.generation = response ? NGEN_Response : NGEN_Query;
    queryId = query;
    dwData[1] = 0;
  }
};

static constexpr int MAX_PACKET_BURST_SIZE = 32 << 10;
