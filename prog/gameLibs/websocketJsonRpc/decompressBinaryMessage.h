// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <websocketJsonRpc/wsJsonRpcConnection.h> // for WsJsonRpcConnectionConfig

#include <EASTL/string.h>
#include <EASTL/string_view.h>


namespace websocketjsonrpc::details
{

struct MemoryChunk final
{
  MemoryChunk() noexcept = default;
  MemoryChunk(eastl::unique_ptr<char[]> &&data_, size_t size_) noexcept : data(eastl::move(data_)), size(size_) {}

  eastl::unique_ptr<char[]> data;
  size_t size = 0;
};


MemoryChunk decompress_binary_message(eastl::string_view binary_message_data, const WsJsonRpcConnectionConfig &config,
  eastl::string &error_details);

} // namespace websocketjsonrpc::details
