//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClient/details/commonTypes.h>
#include <sepClient/rpcRequest.h>

#include <rapidjson/stringbuffer.h>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>


namespace sepclient::details
{

struct RpcContextSettings final
{
  int requestTimeoutMs = -1;
  int maxExecutionTries = -1;
  bool extraVerboseLogging = false;
};


class RpcContext final
{
public:
  RpcContext(const RpcRequest &request, ResponseCallback &&response_callback, const RpcContextSettings &settings);
  ~RpcContext();

  RpcMessageId getMessageId() const { return messageId; }

  eastl::string_view getRpcMethod() const { return rpcMethod; }

  TransactionId getTransactionId() const { return transactionId; }

  eastl::string_view getProjectId() const { return projectId; }

  eastl::string_view getRpcParamsInJsonFormat() const { return {rpcParamsBuffer.GetString(), rpcParamsBuffer.GetSize()}; }

  eastl::string formatState() const;

  int getRemainingTimeBeforeSendMs() const;
  int getRemainingReceiveTimeMs() const;
  int getTimeElapsedFromCreationMs() const;
  int getTimeElapsedFromLastSendMs() const;

  void executeCallback(websocketjsonrpc::RpcResponsePtr &&response);

  void onBeforeCall();

  TickCount getCreationTick() const { return creationTick; }
  TickCount getSendExpirationTick() const { return sendExpirationTick; }

public:
  static TickCount get_send_expiration_duration(TickCount receiveExpirationDurationTicks);

private:
  const RpcContextSettings settings;

  const TickCount creationTick = 0;
  const TickCount sendExpirationTick = 0;    // would not send request after this time
  const TickCount receiveExpirationTick = 0; // would not wait for response after this time
  TickCount lastSendTimeTick = 0;

  const RpcMessageId messageId{};      // positive value
  const eastl::string rpcMethod;       // non-empty string
  const TransactionId transactionId{}; // positive value
  const eastl::string projectId;       // empty string means no value
  const eastl::string profileId;       // empty string means no value
  const rapidjson::StringBuffer rpcParamsBuffer;

  int executionTry = 0;

  ResponseCallback callback; // will be nullptr after the call is done
};


using RpcContextPtr = eastl::unique_ptr<RpcContext>;


} // namespace sepclient::details
