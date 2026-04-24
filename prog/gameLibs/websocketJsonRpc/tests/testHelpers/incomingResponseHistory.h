// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocketJsonRpc/rpcResponse.h>

#include <catch2/catch_test_macros.hpp>

#include <EASTL/deque.h>


class IncomingResponseHistory final
{
public:
  size_t size() const { return responses.size(); }

  const websocketjsonrpc::RpcResponse &last() const
  {
    REQUIRE(responses.empty() == false);
    return *responses.back();
  }

  void clear() { responses.clear(); }

  auto createCallback()
  {
    return [this](websocketjsonrpc::RpcResponsePtr &&incoming_response) { responses.emplace_back(eastl::move(incoming_response)); };
  }

public:
  eastl::deque<websocketjsonrpc::RpcResponsePtr> responses;
};
