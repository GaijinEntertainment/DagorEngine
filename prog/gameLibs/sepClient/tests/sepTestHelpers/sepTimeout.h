// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepClient/details/rpcContext.h>

#include <websocketJsonRpc/details/abstractionLayer.h>


namespace tests
{

inline int get_send_expiration_duration_ms(int receiveExpirationDurationMs)
{
  const sepclient::TickCount ticksPerSecond = websocketjsonrpc::abstraction::get_ticks_per_second();

  const sepclient::TickCount sendExpirationDurationTicks =
    sepclient::details::RpcContext::get_send_expiration_duration(receiveExpirationDurationMs * ticksPerSecond / 1000);

  return static_cast<int>(sendExpirationDurationTicks * 1000 / ticksPerSecond);
}

} // namespace tests
