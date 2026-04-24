//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>


namespace websocketjsonrpc
{

// All interface methods must not modify the client object state.
// I.e. interface implementation must not call any SepClient methods.
class WsJsonRpcStatsCallback
{
public:
  virtual ~WsJsonRpcStatsCallback() = default;

  // The following methods are counting time from connection start:

  virtual void onConnectInitiate(eastl::string_view hostname, eastl::string_view ip_address) const = 0;
  virtual void onConnectSuccess(eastl::string_view hostname, eastl::string_view ip_address, int time_taken_ms) const = 0;
  virtual void onConnectFailure(eastl::string_view hostname, eastl::string_view ip_address, int time_taken_ms,
    eastl::string_view error_name) const = 0;
  virtual void onDisconnect(eastl::string_view hostname, eastl::string_view ip_address, int time_taken_ms,
    eastl::string_view error_name) const = 0;
};


using WsJsonRpcStatsCallbackPtr = eastl::shared_ptr<WsJsonRpcStatsCallback>;


// Returns empty string_view if error_code is unknown.
//
// Error codes are defined originally here: prog/gameLibs/publicInclude/websocket/websocket.h
// Duplicate error codes here:
// 0 - success
// [1..99] - CURL error codes
// [100..999] - HTTP error codes
// [1000..4999] - WebSocket status codes
// [-29'000..-29'999] - Gaijin-specific server-side JSON-RPC defined errors (see SEP_AUTHENTICATION_ERROR)
// [-32'000..-32'999] - WebSocket JSON-RPC defined errors
// [-40'000..-40'999] - Gaijin-specific client-side JSON-RPC defined errors (see INVALID_SERVER_RESPONSE)
eastl::string_view get_known_error_name(int error_code);

eastl::string_view get_error_name(int error_code, eastl::string &buffer);


class WsJsonRpcConnection;


namespace details
{

enum class StatsEventType
{
  CONNECT_INITIATE,
  CONNECT_SUCCESS,
  CONNECT_FAILURE,
  DISCONNECT,
};


void report_stats_event(const WsJsonRpcStatsCallbackPtr &stats, StatsEventType event_type, WsJsonRpcConnection &connection);

} // namespace details


} // namespace websocketjsonrpc
