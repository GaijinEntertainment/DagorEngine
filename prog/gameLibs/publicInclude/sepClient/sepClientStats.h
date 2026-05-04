//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <websocketJsonRpc/wsJsonRpcStats.h>

#include <EASTL/shared_ptr.h>


namespace sepclient
{


// All interface methods must not modify the client object state.
// I.e. interface implementation must not call any SepClient methods.
class SepClientStatsCallback : public websocketjsonrpc::WsJsonRpcStatsCallback
{
public:
  // The following methods are counting time from the last sending request:

  // On initiate request retry after `onDisconnect()`
  virtual void onRetryCausedByNetwork(eastl::string_view project_id, eastl::string_view service, eastl::string_view action,
    int time_taken_ms, eastl::string_view error_name) const = 0;

  virtual void onSuccess(eastl::string_view project_id, eastl::string_view service, eastl::string_view action,
    int time_taken_ms) const = 0;

  virtual void onRequestFailDueToNetwork(eastl::string_view project_id, eastl::string_view service, eastl::string_view action,
    int time_taken_ms, eastl::string_view error_name) const = 0;

  virtual void onRequestFailDueToTimeout(eastl::string_view project_id, eastl::string_view service, eastl::string_view action,
    int time_taken_ms) const = 0;

  virtual void onErrorRpcResponse(eastl::string_view project_id, eastl::string_view service, eastl::string_view action,
    int time_taken_ms, eastl::string_view error_name) const = 0;
};


using SepClientStatsCallbackPtr = eastl::shared_ptr<SepClientStatsCallback>;


namespace details
{

class RpcContext;


enum class StatsEventType
{
  REQUEST_RETRY_CAUSED_BY_NETWORK,
  REQUEST_SUCCESS,
  REQUEST_FAIL_DUE_TO_NETWORK,
  REQUEST_FAIL_DUE_TO_TIMEOUT,
  REQUEST_ERROR_RPC_RESPONSE,
};


inline constexpr int NO_ERROR = 0;


void report_stats_event(const SepClientStatsCallbackPtr &stats, StatsEventType event_type, sepclient::details::RpcContext *rpc_context,
  int error_code);

} // namespace details


} // namespace sepclient
