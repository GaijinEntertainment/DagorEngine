// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/sepClientStats.h>

#include <sepClient/details/rpcContext.h>


namespace sepclient::details
{


void report_stats_event(const SepClientStatsCallbackPtr &stats, StatsEventType event_type, sepclient::details::RpcContext *rpc_context,
  int error_code)
{
  if (!stats)
  {
    return;
  }

  const int timeTakenMs = rpc_context ? rpc_context->getTimeElapsedFromLastSendMs() : 0;

  const eastl::string_view projectId = rpc_context ? rpc_context->getProjectId() : "";
  const eastl::string_view rpcMethod = rpc_context ? rpc_context->getRpcMethod() : "";

  const size_t firstDotPos = rpcMethod.find('.');
  const eastl::string_view service = (firstDotPos != eastl::string_view::npos) ? rpcMethod.substr(0, firstDotPos) : rpcMethod;
  const eastl::string_view action = (firstDotPos != eastl::string_view::npos) ? rpcMethod.substr(firstDotPos + 1) : "";

  switch (event_type)
  {
    case sepclient::details::StatsEventType::REQUEST_RETRY_CAUSED_BY_NETWORK:
    {
      eastl::string buffer;
      const eastl::string_view errorName = websocketjsonrpc::get_error_name(error_code, buffer);

      stats->onRetryCausedByNetwork(projectId, service, action, timeTakenMs, errorName);
      break;
    }

    case sepclient::details::StatsEventType::REQUEST_SUCCESS:
    {
      stats->onSuccess(projectId, service, action, timeTakenMs);
      break;
    }

    case sepclient::details::StatsEventType::REQUEST_FAIL_DUE_TO_NETWORK:
    {
      eastl::string buffer;
      const eastl::string_view errorName = websocketjsonrpc::get_error_name(error_code, buffer);

      stats->onRequestFailDueToNetwork(projectId, service, action, timeTakenMs, errorName);
      break;
    }

    case sepclient::details::StatsEventType::REQUEST_FAIL_DUE_TO_TIMEOUT:
    {
      stats->onRequestFailDueToTimeout(projectId, service, action, timeTakenMs);
      break;
    }

    case sepclient::details::StatsEventType::REQUEST_ERROR_RPC_RESPONSE:
    {
      eastl::string buffer;
      const eastl::string_view errorName = websocketjsonrpc::get_error_name(error_code, buffer);

      stats->onErrorRpcResponse(projectId, service, action, timeTakenMs, errorName);
      break;
    }
  }
}

} // namespace sepclient::details
