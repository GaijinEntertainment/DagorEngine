//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rapidjson/document.h>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>


namespace websocketjsonrpc
{

class RpcRequest final
{
public:
  explicit RpcRequest(rapidjson::Document &&full_json_rpc_request, size_t payload_size);

  bool isValid() const { return validRequest; } // invalid RpcRequest is never returned to the program from the library
  bool isNotification() const { return serializedMessageId.empty(); }

  eastl::string &getSerializedMessageId() { return serializedMessageId; } // useful for sending sending replies, logging, debugging
  const eastl::string &getSerializedMessageId() const
  {
    return serializedMessageId;
  } // useful for sending sending replies, logging, debugging

  eastl::string &getMethod() { return method; }
  const eastl::string &getMethod() const { return method; }

  rapidjson::Value *getParams() { return params; }             // may return nullptr if `params` was omitted
  const rapidjson::Value *getParams() const { return params; } // may return nullptr if `params` was omitted

  const rapidjson::Document &getDocument() const { return document; } // useful only for logging and investigating issues

  size_t getPayloadSize() const { return payloadSize; }

private:
  rapidjson::Document document;
  eastl::string method;
  eastl::string serializedMessageId; // JSON format; empty value means lack of message id
  rapidjson::Value *params = nullptr;
  bool validRequest = false;
  size_t payloadSize = 0;
};


using RpcRequestPtr = eastl::unique_ptr<RpcRequest>;


} // namespace websocketjsonrpc
