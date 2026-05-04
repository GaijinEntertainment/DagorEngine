// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/details/rpcContext.h>

#include "sepProtocol.h"

#include <websocketJsonRpc/details/abstractionLayer.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>

#include <rapidjson/writer.h>


namespace sepclient::details
{


namespace abstraction = websocketjsonrpc::abstraction;


static TickCount get_relative_tick(TickCount now_ticks, int duration_since_now_ms)
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  return now_ticks + duration_since_now_ms * ticksPerSecond / 1000;
}


TickCount RpcContext::get_send_expiration_duration(TickCount receiveExpirationDurationTicks)
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests

  // 25% of request_timeout_ms, not less than 500 ms, no more than request_timeout_ms:
  const TickCount timeReservedForCallExecutionTicks =
    eastl::min(eastl::max(receiveExpirationDurationTicks / 4, ticksPerSecond / 2), receiveExpirationDurationTicks);

  return receiveExpirationDurationTicks - timeReservedForCallExecutionTicks;
}


static TickCount get_send_expiration_tick(TickCount now_ticks, int request_timeout_ms)
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests

  const TickCount maxTicksBeforeSend = RpcContext::get_send_expiration_duration(request_timeout_ms * ticksPerSecond / 1000);

  return now_ticks + maxTicksBeforeSend;
}


static eastl::string construct_json_rpc_method(const RpcRequest &request)
{
  return {eastl::string::CtorSprintf{}, "%s.%s", request.getService().c_str(), request.getAction().c_str()};
}


static rapidjson::StringBuffer construct_json_rpc_params(const RpcRequest &request, TransactionId transactionId)
{
  rapidjson::StringBuffer buffer;
  const eastl::string transactionIdStr = eastl::to_string(transactionId);
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  // Data schema is defined in `sep_protocol.md` file (in the same directory as this file).

  writer.StartObject();

  writer.Key(protocol::rpc::params::TRANSACTION_ID);
  writer.String(transactionIdStr.data(), transactionIdStr.size());

  const eastl::string_view projectId = request.getProjectId();
  if (!projectId.empty())
  {
    writer.Key(protocol::rpc::params::PROJECT_ID);
    writer.String(projectId.data(), projectId.size());
  }

  const eastl::string_view profileId = request.getProfileId();
  if (!profileId.empty())
  {
    writer.Key(protocol::rpc::params::PROFILE_ID);
    writer.String(profileId.data(), profileId.size());
  }

  const eastl::string_view actionParams = request.getActionParams();
  if (!actionParams.empty())
  {
    writer.Key(protocol::rpc::params::ACTION_PARAMS);
    writer.RawValue(actionParams.data(), actionParams.size(), rapidjson::Type::kObjectType);
  }

  writer.EndObject();

  G_ASSERT(writer.IsComplete());
  return buffer;
}


RpcContext::RpcContext(const RpcRequest &request, ResponseCallback &&response_callback, const RpcContextSettings &settings_) :
  settings(settings_),
  creationTick(abstraction::get_current_tick_count()),
  sendExpirationTick(get_send_expiration_tick(creationTick, settings_.requestTimeoutMs)),
  receiveExpirationTick(get_relative_tick(creationTick, settings_.requestTimeoutMs)),
  messageId(request.getJsonRpcMessageId()),
  rpcMethod(construct_json_rpc_method(request)),
  transactionId(request.getTransactionId()),
  projectId(request.getProjectId()),
  profileId(request.getProfileId()),
  rpcParamsBuffer(construct_json_rpc_params(request, transactionId)),
  callback(eastl::move(response_callback))
{
  G_ASSERT(settings.requestTimeoutMs >= 0);
  G_ASSERT(settings.maxExecutionTries > 0);

  G_ASSERT(sendExpirationTick >= 0);
  G_ASSERT(receiveExpirationTick >= 0);
  G_ASSERT(transactionId >= 0);
  G_ASSERT(rpcParamsBuffer.GetSize() > 0);
  G_ASSERT(!getRpcParamsInJsonFormat().empty());
  G_ASSERT(callback);

  if (settings.extraVerboseLogging)
  {
    const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
    logdbg("[sep][sep_client] RpcContext is created at tick %lld; tick frequency: %lld; receiveExpirationTick: %lld; Context: %s",
      (long long)creationTick, (long long)ticksPerSecond, (long long)receiveExpirationTick, formatState().c_str());
  }
}


RpcContext::~RpcContext()
{
  if (settings.extraVerboseLogging)
  {
    const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
    logdbg("[sep][sep_client] RpcContext is destroyed at tick %lld; tick frequency: %lld; Context: %s", (long long)creationTick,
      (long long)ticksPerSecond, formatState().c_str());
  }

  if (callback)
  {
    logerr("[sep][sep_client][sep_error] ERROR! RpcContext was destroyed without calling response callback!"
           " Game client code may hung! Context: %s",
      formatState().c_str());
  }
}


eastl::string RpcContext::formatState() const
{
  const int remainingTimeBeforeSendMs = getRemainingTimeBeforeSendMs();
  const int remainingReceiveTimeMs = getRemainingReceiveTimeMs();

  eastl::string result{eastl::string::CtorSprintf{},
    "{ method: \"%s\"; messageId: %lld; transactId: %lld; projectId: \"%s\"; profileId: \"%s\""
    "; time (remaining before send/remaining for receive/total): %d / %d / %d ms; try %d / %d }",
    rpcMethod.c_str(), (long long)messageId, (long long)transactionId, projectId.c_str(), profileId.c_str(), remainingTimeBeforeSendMs,
    remainingReceiveTimeMs, settings.requestTimeoutMs, executionTry, settings.maxExecutionTries};

  if (settings.extraVerboseLogging)
  {
    const int elapsedFromCreationMs = getTimeElapsedFromCreationMs();
    const int elapsedFromLastSendMs = getTimeElapsedFromLastSendMs();
    result.append_sprintf("; { time (elapsed from create/elapsed from last send): %d / %d ms }", elapsedFromCreationMs,
      elapsedFromLastSendMs);
  }

  return result;
}


int RpcContext::getRemainingTimeBeforeSendMs() const
{
  if (executionTry >= settings.maxExecutionTries)
  {
    return 0;
  }

  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount nowTicks = abstraction::get_current_tick_count();
  const TickCount remainingTicks = sendExpirationTick - nowTicks;

  return static_cast<int>(remainingTicks * 1000 / ticksPerSecond);
}


int RpcContext::getRemainingReceiveTimeMs() const
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount nowTicks = abstraction::get_current_tick_count();
  const TickCount remainingTicks = receiveExpirationTick - nowTicks;

  return static_cast<int>(remainingTicks * 1000 / ticksPerSecond);
}


int RpcContext::getTimeElapsedFromCreationMs() const
{
  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount nowTicks = abstraction::get_current_tick_count();
  const TickCount elapsedTicks = nowTicks - creationTick;

  return static_cast<int>(elapsedTicks * 1000 / ticksPerSecond);
}


int RpcContext::getTimeElapsedFromLastSendMs() const
{
  if (lastSendTimeTick == 0)
  {
    return 0;
  }

  const TickCount ticksPerSecond = abstraction::get_ticks_per_second(); // do not save since it can change during runtime in tests
  const TickCount nowTicks = abstraction::get_current_tick_count();
  const TickCount elapsedTicks = nowTicks - lastSendTimeTick;

  return static_cast<int>(elapsedTicks * 1000 / ticksPerSecond);
}


void RpcContext::executeCallback(websocketjsonrpc::RpcResponsePtr &&response)
{
  if (!callback)
  {
    G_ASSERT_LOG(false, "[sep][sep_client][sep_error] ERROR! RpcContext::executeCallback() was called twice! Context: %s",
      formatState().c_str());
    return;
  }

  callback(eastl::move(response));

  // Ensure callback is not called twice:
  callback = nullptr;
}


void RpcContext::onBeforeCall()
{
  executionTry++;
  lastSendTimeTick = abstraction::get_current_tick_count();
}


} // namespace sepclient::details
