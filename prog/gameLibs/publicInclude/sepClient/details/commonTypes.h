//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/commonTypes.h>

#include <EASTL/functional.h>


namespace sepclient
{

using TickCount = websocketjsonrpc::TickCount;

using RpcResponse = websocketjsonrpc::RpcResponse;
using RpcResponsePtr = websocketjsonrpc::RpcResponsePtr;

using RpcMessageId = websocketjsonrpc::RpcMessageId;
using TransactionId = int64_t;

using ResponseCallback = websocketjsonrpc::ResponseCallback;


enum class LogLevel
{
  NONE,         // no logging
  MAJOR_EVENTS, // log only main events
  FULL_DUMP,    // maximum logging

  DEFAULT = MAJOR_EVENTS,
};


} // namespace sepclient
