// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/details/responseCallbackCollection.h>

#include <websocketJsonRpc/details/abstractionLayer.h>

#include <debug/dag_assert.h>


namespace websocketjsonrpc::details
{


bool ResponseCallbackCollection::addCallback(RpcMessageId message_id, ResponseCallback &&callback, int request_timeout_ms)
{
  G_ASSERT(callback);
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount nowTicks = abstraction::get_current_tick_count();

  CallbackInfo info;
  info.expirationTick = nowTicks + request_timeout_ms * ticksPerSecond / 1000;
  info.messageId = message_id;
  inProgressQueue.emplace(info);

  const bool insertedNew = inProgressMap.try_emplace(message_id, eastl::move(callback)).second;
  G_ASSERTF(insertedNew, "JSON RPC message id must be unique across the collection lifetime; id: %lld", (long long)message_id);
  return insertedNew;
}


ResponseCallback ResponseCallbackCollection::extractCallback(RpcMessageId messageId)
{
  const auto iter = inProgressMap.find(messageId);
  if (iter == inProgressMap.end())
  {
    return {};
  }

  ResponseCallback callback = eastl::move(iter->second);
  inProgressMap.erase(iter);

  // Every item in inProgressMap must have a twin item in inProgressQueue.
  // The corresponding element in inProgressQueue would be removed later
  G_ASSERT(!inProgressQueue.empty());

  G_ASSERT(callback);
  return callback;
}


ResponseCallback ResponseCallbackCollection::extractAnyCallback(RpcMessageId &message_id)
{
  return extractExpiredCallback(message_id, TICKS_NO_VALUE);
}


ResponseCallback ResponseCallbackCollection::extractExpiredCallback(RpcMessageId &message_id)
{
  const TickCount nowTicks = abstraction::get_current_tick_count();
  return extractExpiredCallback(message_id, nowTicks);
}


ResponseCallback ResponseCallbackCollection::extractExpiredCallback(RpcMessageId &message_id, TickCount now_ticks)
{
  message_id = RpcMessageId{};

  while (!inProgressQueue.empty())
  {
    const CallbackInfo &info = inProgressQueue.top();

    const auto iter = inProgressMap.find(info.messageId);
    if (iter == inProgressMap.end()) // callback could be already called and removed from inProgressMap
    {
      inProgressQueue.pop();
      continue;
    }

    if (now_ticks != TICKS_NO_VALUE && info.expirationTick > now_ticks)
    {
      return {};
    }

    message_id = info.messageId;
    ResponseCallback callback = eastl::move(iter->second);
    inProgressMap.erase(iter);
    inProgressQueue.pop();

    G_ASSERT(callback);
    return callback;
  }

  return {};
}


} // namespace websocketjsonrpc::details
