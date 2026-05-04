// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/rpcRequest.h>

#include "jsonRpcProtocol.h"

#include <debug/dag_assert.h>

#include <rapidjson/writer.h>


namespace websocketjsonrpc
{

static_assert(std::is_move_constructible_v<RpcRequest>);
static_assert(std::is_move_assignable_v<RpcRequest>);


static void extract_data(rapidjson::Document &doc, bool &valid_request, eastl::string &method, eastl::string &serialized_message_id,
  rapidjson::Value *&params)
{
  valid_request = true;
  method.clear();
  serialized_message_id.clear();
  params = nullptr;

  if (!doc.IsObject())
  {
    valid_request = false;
    return;
  }

  auto iter = doc.FindMember(protocol::METHOD);
  if (iter != doc.MemberEnd())
  {
    if (iter->value.IsString())
    {
      method.assign(iter->value.GetString(), iter->value.GetStringLength());
    }
    else
    {
      valid_request = false;
      method = "<INVALID_METHOD_VALUE_TYPE>";
    }
  }
  else
  {
    valid_request = false;
    method = "<NO_METHOD_VALUE>";
  }

  iter = doc.FindMember(protocol::MESSAGE_ID);
  if (iter != doc.MemberEnd())
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    G_VERIFY(iter->value.Accept(writer));
    G_ASSERT(writer.IsComplete());

    serialized_message_id.assign(buffer.GetString(), buffer.GetSize());
  }

  iter = doc.FindMember(protocol::PARAMS);
  if (iter != doc.MemberEnd())
  {
    params = &iter->value;
  }
  else
  {
    // Missing `params`. This is OK for JSON RPC 2.0
    params = nullptr;
  }
}


RpcRequest::RpcRequest(rapidjson::Document &&full_json_rpc_request, size_t payload_size) :
  document(eastl::move(full_json_rpc_request)), payloadSize(payload_size)
{
  extract_data(document, validRequest, method, serializedMessageId, params);
}


} // namespace websocketjsonrpc
