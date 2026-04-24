//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

#include <generic/dag_moveOnlyFunction.h>
#include <EASTL/unique_ptr.h>


namespace websocketjsonrpc
{

class RpcResponse;
using RpcResponsePtr = eastl::unique_ptr<RpcResponse>;

using TickCount = int64_t;

using RpcMessageId = int64_t;

using ResponseCallback = dag::MoveOnlyFunction<void(RpcResponsePtr &&received_response)>;

enum class ConnectionId : uint32_t
{
};

} // namespace websocketjsonrpc
