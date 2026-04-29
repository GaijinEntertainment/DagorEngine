//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClient/details/rpcContext.h>

#include <EASTL/priority_queue.h>


namespace sepclient::details
{


class DelayedRpcCallCollection final
{
public:
  DelayedRpcCallCollection(TickCount requestTimeoutWithoutNetworkTicks)
  {
    callQueue.comp.haveNetworkConnection = false;
    callQueue.comp.requestTimeoutWithoutNetworkTicks = requestTimeoutWithoutNetworkTicks;
  }

  void addRpc(RpcContextPtr &&context)
  {
    if (context)
    {
      callQueue.emplace(eastl::move(context));
    }
  }

  bool getHaveNetworkConnection() const { return callQueue.comp.haveNetworkConnection; }

  void setHaveNetworkConnection(bool have_connection)
  {
    if (callQueue.comp.haveNetworkConnection == have_connection)
    {
      return;
    }

    callQueue.comp.haveNetworkConnection = have_connection;

    // Rebuild the heap since comparator was changed
    eastl::make_heap(callQueue.c.begin(), callQueue.c.end(), callQueue.comp);
  }

  RpcContextPtr extractAnyCall()
  {
    if (callQueue.empty())
    {
      return nullptr;
    }

    RpcContextPtr result;
    callQueue.pop(result);
    return eastl::move(result);
  }

  RpcContextPtr extractExpiredCall(TickCount now_ticks)
  {
    if (callQueue.empty())
    {
      return nullptr;
    }

    const RpcContextPtr &topCall = callQueue.top();
    const TickCount topExpirationTick = callQueue.comp.getExpirationTick(*topCall);
    if (topExpirationTick > now_ticks)
    {
      return nullptr;
    }

    RpcContextPtr result;
    callQueue.pop(result);
    return eastl::move(result);
  }

  bool empty() const { return callQueue.empty(); }

private:
  // Make priority_queue to store elements with smaller expirationTick on the top:
  struct LessDelayedCallPriority final
  {
    TickCount getExpirationTick(const RpcContext &context) const noexcept
    {
      return haveNetworkConnection ? context.getSendExpirationTick() : context.getCreationTick() + requestTimeoutWithoutNetworkTicks;
    }

    bool operator()(const RpcContextPtr &a, const RpcContextPtr &b) const noexcept
    {
      const TickCount expirationTickA = getExpirationTick(*a);
      const TickCount expirationTickB = getExpirationTick(*b);

      if (expirationTickA == expirationTickB)
      {
        // keep original order for calls with the same expirationTick
        return a->getMessageId() > b->getMessageId();
      }
      return expirationTickA > expirationTickB;
    }

    bool haveNetworkConnection = false;
    TickCount requestTimeoutWithoutNetworkTicks = 0;
  };

private:
  eastl::priority_queue<RpcContextPtr, eastl::vector<RpcContextPtr>, LessDelayedCallPriority> callQueue;
};


} // namespace sepclient::details
