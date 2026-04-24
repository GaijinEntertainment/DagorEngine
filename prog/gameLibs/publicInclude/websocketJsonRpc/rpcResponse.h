//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/details/commonTypes.h>

#include <rapidjson/document.h>

#include <EASTL/string.h>


namespace websocketjsonrpc
{

using JsonRpcErrorCodeType = int;


enum class AdditionalErrorCodes : JsonRpcErrorCodeType
{
  // Invalid game client errors (-40100..):
  WEBSOCKET_CONNECTION_IS_NOT_READY = -40101,
  LOST_WEBSOCKET_CONNECTION_AFTER_SENDING = -40102,

  // Local client runtime errors (-40200..):
  CLIENT_LIBRARY_IS_NOT_INITIALIZED = -40201,

  // Received network data errors (-40300..):
  INVALID_SERVER_RESPONSE = -40301,
  RPC_CALL_TIMEOUT = -40302,
};


class RpcResponse final
{
public:
  RpcResponse(rapidjson::Document &&full_json_rpc_response, size_t payload_size);

  RpcResponse(RpcMessageId message_id, AdditionalErrorCodes error_code, eastl::string_view error_description,
    int reason_error_code = 0);

  bool isError() const { return isErrorResponse; }

  eastl::string getMessageId() const; // useful only for logging and investigating issues

  JsonRpcErrorCodeType getErrorCode() const; // valid only if isError() == true
  eastl::string getErrorMessage() const;     // valid only if isError() == true

  rapidjson::Value &getResult() { return content.get(); }             // valid if isError() == false
  const rapidjson::Value &getResult() const { return content.get(); } // valid if isError() == false

  const rapidjson::Document &getDocument() const { return document; } // useful only for logging and investigating issues
  rapidjson::Document &&extractDocument() { return eastl::move(document); }

  size_t getPayloadSize() const { return payloadSize; }

  // valid only if isError() == true && RpcResponse was created with special constructor; 0 by default
  int getInternalReasonErrorCode() const;

private:
  // NOTE! All these members MUST be declared in this particular order!
  rapidjson::Document document;
  bool isErrorResponse = false;
  eastl::reference_wrapper<rapidjson::Value> content; // `result` or `error` (or document in case of error)
  size_t payloadSize = 0;
};


} // namespace websocketjsonrpc
