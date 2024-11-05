//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "daNetTypes.h"

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>

#include <EASTL/optional.h>

#include <cstdint>

struct _ENetHost;

namespace danet
{

using EchoSequenceNumber = uint32_t;

struct EchoResponse
{
  enum class Result
  {
    SUCCESS,
    UNRESOLVED,
    TIMEOUT
  };

  uint32_t routeId{};
  Result result{};
  DaNetTime rttOrTimeout{};
};

class EchoManager
{
private:
  struct RequestToSend
  {
    uint32_t routeId{};
    struct
    {
      uint32_t host;
      uint16_t port;
    } addr{};
  };

  struct RequestInFlight
  {
    uint32_t routeId{};
    EchoSequenceNumber sequenceNumber{};
    DaNetTime sendTime{};
    bool receivedResponse{}; // Mark received packets to avoid potentially costly deletion from the random inFlight queue position
  };

public:
  EchoManager(DaNetTime echo_timeout_ms);
  ~EchoManager();
  void clear();
  void sendEcho(const char *route_addr, uint32_t route_id);
  void setHost(_ENetHost *new_host);
  void process();
  eastl::optional<EchoResponse> receive();

  void processResponse(EchoSequenceNumber sequenceNumber);

#if DAGOR_DBGLEVEL > 0
  size_t sendCount{};
  size_t receiveCount{};
#endif

private:
  WinCritSec crit{};
  Tab<RequestToSend> toSend{};
  Tab<RequestInFlight> inFlight{};
  Tab<EchoResponse> received{};

  _ENetHost *host{};
  DaNetTime timeoutMs{};
  EchoSequenceNumber sequenceCounter{};
};

} // namespace danet
