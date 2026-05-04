//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/commonTypes.h>

#include <EASTL/priority_queue.h>
#include <EASTL/unordered_map.h>


namespace websocketjsonrpc::details
{


// This class is designed to be used from the same single thread
class ResponseCallbackCollection final
{
public:
  // `mesage_id` must be unique across collection lifetime
  bool addCallback(RpcMessageId mesage_id, ResponseCallback &&callback, int request_timeout_ms);

  // Extract callback by JSON RPC message id
  ResponseCallback extractCallback(RpcMessageId message_id);

  // Returns nullptr callback if no callbacks are remaining.
  // `messageId` is always filled if callback was extracted
  ResponseCallback extractAnyCallback(RpcMessageId &message_id);

  // Returns nullptr callback if no expired callbacks are remaining.
  // `messageId` is always filled if callback was extracted
  ResponseCallback extractExpiredCallback(RpcMessageId &message_id);

  size_t getInProgressCount() const { return inProgressMap.size(); }
  bool isEmpty() const { return inProgressMap.empty() && inProgressQueue.empty(); }

private:
  static constexpr TickCount TICKS_NO_VALUE = 0;

  struct CallbackInfo final
  {
    TickCount expirationTick = 0;
    RpcMessageId messageId = 0;
  };

  // Make priority_queue to store elements with smaller expirationTick on the top:
  struct LessCallbackInfoPriority final
  {
    bool operator()(const CallbackInfo &a, const CallbackInfo &b) const noexcept { return a.expirationTick > b.expirationTick; }
  };

private:
  ResponseCallback extractExpiredCallback(RpcMessageId &message_id, TickCount now_ticks);

private:
  eastl::unordered_map<RpcMessageId, ResponseCallback> inProgressMap;

  // Can contain more elements than inProgressMap
  // Smaller expirationTick is stored on the top of the heap (priority queue): eastl::greater<>.
  eastl::priority_queue<CallbackInfo, eastl::vector<CallbackInfo>, LessCallbackInfoPriority> inProgressQueue;
};


} // namespace websocketjsonrpc::details
