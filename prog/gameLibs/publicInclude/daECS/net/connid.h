//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

namespace net
{

class Connection;
static constexpr int INVALID_CONNECTION_ID_VALUE = -1;

struct ConnectionId
{
  int value = INVALID_CONNECTION_ID_VALUE;
  ConnectionId() = default;
  ConnectionId(int v) : value(v) {}
  operator int() const { return value; }
};
const ConnectionId INVALID_CONNECTION_ID = ConnectionId(INVALID_CONNECTION_ID_VALUE);

} // namespace net
