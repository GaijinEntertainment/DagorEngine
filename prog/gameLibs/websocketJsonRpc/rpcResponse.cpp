// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/rpcResponse.h>

#include "jsonRpcProtocol.h"


namespace websocketjsonrpc
{


static_assert(std::is_move_constructible_v<RpcResponse>);
static_assert(std::is_move_assignable_v<RpcResponse>);


static rapidjson::Value &find_content(rapidjson::Document &doc, bool &is_error)
{
  is_error = true;

  if (!doc.IsObject())
  {
    return doc;
  }

  auto iter = doc.FindMember(protocol::RESULT);
  if (iter != doc.MemberEnd())
  {
    is_error = false;
    return iter->value;
  }

  iter = doc.FindMember(protocol::ERROR);
  if (iter != doc.MemberEnd())
  {
    return iter->value;
  }

  return doc;
}


static rapidjson::Document create_error_jsonrpc_document(RpcMessageId message_id, const AdditionalErrorCodes error_code,
  eastl::string_view error_description, int reason_error_code)
{
  rapidjson::Document doc(rapidjson::Type::kObjectType);
  auto &alloc = doc.GetAllocator();

  doc.AddMember(protocol::VERSION, protocol::VERSION_VALUE, alloc);
  doc.AddMember(protocol::MESSAGE_ID, message_id, alloc);

  {
    rapidjson::Value error{rapidjson::Type::kObjectType};
    error.AddMember(protocol::ERROR_CODE, static_cast<std::underlying_type_t<AdditionalErrorCodes>>(error_code), alloc);
    error.AddMember(protocol::ERROR_MESSAGE,
      rapidjson::Value{error_description.data(), static_cast<rapidjson::SizeType>(error_description.size()), alloc}, alloc);

    {
      // Gaijin Char-backend-specific error details:
      rapidjson::Value errorData{rapidjson::Type::kObjectType};
      errorData.AddMember(protocol::ERROR_DATA_CONTEXT, rapidjson::Value{"WsJsonRpcConnection"}, alloc);
      if (reason_error_code != 0)
      {
        errorData.AddMember(protocol::ERROR_DATA_REASON_ERROR_CODE, rapidjson::Value{reason_error_code}, alloc);
      }

      error.AddMember(protocol::ERROR_DATA, errorData, alloc);
    }

    doc.AddMember(protocol::ERROR, error, alloc);
  }

  return doc;
}


RpcResponse::RpcResponse(rapidjson::Document &&full_json_rpc_response, size_t payload_size) :
  document(eastl::move(full_json_rpc_response)),
  isErrorResponse(true),
  content(find_content(document, isErrorResponse)),
  payloadSize(payload_size)
{}


RpcResponse::RpcResponse(RpcMessageId message_id, const AdditionalErrorCodes error_code, eastl::string_view error_description,
  int reason_error_code) :
  RpcResponse(create_error_jsonrpc_document(message_id, error_code, error_description, reason_error_code), 0)
{}


eastl::string RpcResponse::getMessageId() const
{
  if (!document.IsObject())
  {
    return "<BAD_ROOT>";
  }

  auto iter = document.FindMember(protocol::MESSAGE_ID);
  if (iter == document.MemberEnd())
  {
    return "<NO_MESSAGE_ID>";
  }
  else
  {
    const rapidjson::Value &messageId = iter->value;
    if (messageId.IsInt64())
    {
      return eastl::to_string(messageId.GetInt64());
    }
    else if (messageId.IsUint64())
    {
      return eastl::to_string(messageId.GetUint64());
    }
    else if (messageId.IsString())
    {
      eastl::string result;
      result.reserve(messageId.GetStringLength() + 2);
      result += '\"';
      result.append(messageId.GetString(), messageId.GetStringLength());
      result += '\"';
      return result;
    }
    else if (messageId.IsNull())
    {
      return "<NULL>";
    }
    else
    {
      return "<UNEXPECTED_MESSAGE_ID_TYPE>";
    }
  }
}


JsonRpcErrorCodeType RpcResponse::getErrorCode() const
{
  if (!isErrorResponse)
  {
    return 0;
  }

  if (!content.get().IsObject())
  {
    return static_cast<JsonRpcErrorCodeType>(AdditionalErrorCodes::INVALID_SERVER_RESPONSE);
  }

  const auto iter = content.get().FindMember(protocol::ERROR_CODE);
  if (iter == content.get().MemberEnd())
  {
    return static_cast<JsonRpcErrorCodeType>(AdditionalErrorCodes::INVALID_SERVER_RESPONSE);
  }
  const rapidjson::Value &codeValue = iter->value;
  if (!codeValue.IsInt())
  {
    return static_cast<JsonRpcErrorCodeType>(AdditionalErrorCodes::INVALID_SERVER_RESPONSE);
  }
  return codeValue.GetInt();
}


eastl::string RpcResponse::getErrorMessage() const
{
  if (!isErrorResponse)
  {
    return "NOT AN ERROR";
  }

  if (!content.get().IsObject())
  {
    return "MISSING ERROR OBJECT";
  }

  const auto iter = content.get().FindMember(protocol::ERROR_MESSAGE);
  if (iter == content.get().MemberEnd())
  {
    return "MISSING ERROR MESSAGE";
  }
  const rapidjson::Value &messageValue = iter->value;
  if (!messageValue.IsString())
  {
    return "ERROR MESSAGE IS NOT A STRING";
  }
  return {messageValue.GetString(), messageValue.GetStringLength()};
}


int RpcResponse::getInternalReasonErrorCode() const
{
  if (!isErrorResponse)
  {
    return 0;
  }
  if (!content.get().IsObject())
  {
    return 0;
  }
  const auto iterErrorData = content.get().FindMember(protocol::ERROR_DATA);
  if (iterErrorData == content.get().MemberEnd())
  {
    return 0;
  }
  const rapidjson::Value &errorDataValue = iterErrorData->value;
  if (!errorDataValue.IsObject())
  {
    return 0;
  }
  const auto iterReasonErrorCode = errorDataValue.FindMember(protocol::ERROR_DATA_REASON_ERROR_CODE);
  if (iterReasonErrorCode == errorDataValue.MemberEnd())
  {
    return 0;
  }
  const rapidjson::Value &reasonErrorCodeValue = iterReasonErrorCode->value;
  if (!reasonErrorCodeValue.IsInt())
  {
    return 0;
  }
  return reasonErrorCodeValue.GetInt();
}


} // namespace websocketjsonrpc
