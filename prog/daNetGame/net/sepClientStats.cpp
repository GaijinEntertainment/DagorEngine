// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sepClientStats.h"

#include <statsd/statsd.h>

#include <EASTL/fixed_string.h>


namespace sepclientstats
{


using FixedStringBase = eastl::fixed_string<char, 100, false>;


class FixedString final : public FixedStringBase
{
public:
  explicit FixedString(eastl::string_view str) : FixedStringBase(str.data(), str.size()) {}

  operator const char *() const { return FixedStringBase::c_str(); }
};


class SepClientStatsCallbackImpl final : public sepclient::SepClientStatsCallback
{
public:
  explicit SepClientStatsCallbackImpl(eastl::string_view default_project_id) : defaultProjectId(default_project_id) {}

  // JSON-RPC connection events:

  void onConnectInitiate(eastl::string_view hostname, eastl::string_view ip_address) const override
  {
    sendCounterMetric("sep_client.connect_initiate", 1,
      {
        {"project_id", FixedString(defaultProjectId)},
        {"hostname", FixedString(hostname)},
        {"ip_address", FixedString(ip_address)},
      });
  }

  void onConnectSuccess(eastl::string_view hostname, eastl::string_view ip_address, int time_taken_ms) const override
  {
    sendProfileMetric("sep_client.connect_success", time_taken_ms,
      {
        {"project_id", FixedString(defaultProjectId)},
        {"hostname", FixedString(hostname)},
        {"ip_address", FixedString(ip_address)},
      });
  }

  void onConnectFailure(
    eastl::string_view hostname, eastl::string_view ip_address, int time_taken_ms, eastl::string_view error_name) const override
  {
    sendProfileMetric("sep_client.connect_failure", time_taken_ms,
      {
        {"error_name", FixedString(error_name)},
        {"project_id", FixedString(defaultProjectId)},
        {"hostname", FixedString(hostname)},
        {"ip_address", FixedString(ip_address)},
      });
  }

  void onDisconnect(
    eastl::string_view hostname, eastl::string_view ip_address, int time_taken_ms, eastl::string_view error_name) const override
  {
    sendProfileMetric("sep_client.disconnect", time_taken_ms,
      {
        {"error_name", FixedString(error_name)},
        {"project_id", FixedString(defaultProjectId)},
        {"hostname", FixedString(hostname)},
        {"ip_address", FixedString(ip_address)},
      });
  }

  // SepClient request events:

  void onRetryCausedByNetwork(eastl::string_view project_id,
    eastl::string_view service,
    eastl::string_view action,
    int time_taken_ms,
    eastl::string_view error_name) const override
  {
    sendProfileMetric("sep_client.request_retry", time_taken_ms,
      {
        {"reason", "network_error"},
        {"error_name", FixedString(error_name)},
        {"project_id", FixedString(project_id)}, // value may be empty, skipped by statsd in this case
        {"service", FixedString(service)},
        {"action", FixedString(action)},
      });
  }

  void onSuccess(
    eastl::string_view project_id, eastl::string_view service, eastl::string_view action, int time_taken_ms) const override
  {
    sendProfileMetric("sep_client.request_success", time_taken_ms,
      {
        {"project_id", FixedString(project_id)}, // value may be empty, skipped by statsd in this case
        {"service", FixedString(service)},
        {"action", FixedString(action)},
      });
  }

  void onRequestFailDueToNetwork(eastl::string_view project_id,
    eastl::string_view service,
    eastl::string_view action,
    int time_taken_ms,
    eastl::string_view error_name) const override
  {
    sendProfileMetric("sep_client.request_fail", time_taken_ms,
      {
        {"reason", "network_error"},
        {"error_name", FixedString(error_name)},
        {"project_id", FixedString(project_id)}, // value may be empty, skipped by statsd in this case
        {"service", FixedString(service)},
        {"action", FixedString(action)},
      });
  }

  void onRequestFailDueToTimeout(
    eastl::string_view project_id, eastl::string_view service, eastl::string_view action, int time_taken_ms) const override
  {
    sendProfileMetric("sep_client.request_fail", time_taken_ms,
      {
        {"reason", "timeout"},
        {"project_id", FixedString(project_id)}, // value may be empty, skipped by statsd in this case
        {"service", FixedString(service)},
        {"action", FixedString(action)},
      });
  }

  void onErrorRpcResponse(eastl::string_view project_id,
    eastl::string_view service,
    eastl::string_view action,
    int time_taken_ms,
    eastl::string_view error_name) const override
  {
    sendProfileMetric("sep_client.request_fail", time_taken_ms,
      {
        {"reason", "json_rpc_error"},
        {"error_name", FixedString(error_name)},
        {"project_id", FixedString(project_id)}, // value may be empty, skipped by statsd in this case
        {"service", FixedString(service)},
        {"action", FixedString(action)},
      });
  }

private:
  static void sendCounterMetric(const char *stat_name, int count, std::initializer_list<statsd::MetricTag> tags)
  {
    statsd::counter(stat_name, count, tags);
  }

  static void sendProfileMetric(const char *stat_name, int time_taken_ms, std::initializer_list<statsd::MetricTag> tags)
  {
    statsd::profile(stat_name, static_cast<long>(time_taken_ms), tags);
  }

private:
  eastl::string defaultProjectId;
};


sepclient::SepClientStatsCallbackPtr spawn_callback(eastl::string_view default_project_id)
{
  return eastl::make_shared<SepClientStatsCallbackImpl>(default_project_id);
}


} // namespace sepclientstats
