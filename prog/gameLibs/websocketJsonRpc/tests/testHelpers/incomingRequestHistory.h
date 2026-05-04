// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocketJsonRpc/rpcRequest.h>

#include <catch2/catch_test_macros.hpp>

#include <EASTL/deque.h>


class IncomingRequestHistory final
{
public:
  size_t size() const { return requests.size(); }

  const websocketjsonrpc::RpcRequest &last() const
  {
    REQUIRE(requests.empty() == false);
    return *requests.back();
  }

  void clear() { requests.clear(); }

  auto createCallback()
  {
    return [this](websocketjsonrpc::RpcRequestPtr &&incoming_request) { requests.emplace_back(eastl::move(incoming_request)); };
  }

public:
  eastl::deque<websocketjsonrpc::RpcRequestPtr> requests;
};
