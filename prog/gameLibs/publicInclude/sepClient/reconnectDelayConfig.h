//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

#include <utility>


namespace sepclient
{

class ReconnectDelayConfig final
{
public:
  // Note: connect_retry_number is 0-based (0 means first try, 1 - first retry, ...)
  int getDelayMs(int connect_retry_number, uint8_t random_number) const;

public:
  // Delays for different retries:
  // first:  [MinMs   .. MaxMs],
  // second: [MinMs*2 .. MaxMs*2],
  // third:  [MinMs*4 .. MaxMs*4], ...
  // It is important to make random delay (jitter) to avoid DDoS for backend in case of short mass network outage

  int initialDelayMinMs = 100;
  int initialDelayMaxMs = 1500;

  // I hope it's enough to reconnect all users back after a full backend restart during peak hour (worst case):
  int maxDelayMs = 300'000; // 5 min
};


} // namespace sepclient
