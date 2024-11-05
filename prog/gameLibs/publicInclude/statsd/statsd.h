//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_span.h>
#include <EASTL/initializer_list.h>
#include <stdarg.h>

namespace statsd
{
class ILogger
{
public:
  virtual void logerrImpl(const char *fmt, va_list) = 0;
  virtual void debugImpl(const char *fmt, va_list) = 0;
};
ILogger *get_dagor_logger();
// Initialization is done by calling either init()...
struct Env
{
  const char *value;

  // default values
  static const char *const test;
  static const char *const production;
};
struct Application
{
  const char *value;

  // default valus
  static const char *const dedicated;
  static const char *const client;
};
struct Platform
{
  const char *value;
};
struct Project
{
  const char *value;
};
struct Circuit
{
  const char *value;
};
struct Host
{
  const char *value;
};

// Example init metrics config
//  statsd::init( ...
//                statsd::Env         { "test" }         -- required ["production", "test", "stage"]
//                                                             -- production metrics live along time
//                                                             -- test metrics live a couple days
//                                                             -- stage metrics live some days
//                statsd::Circuit     { "venus" }        -- required, project type
//                statsd::Application { "client" }       -- required, client type
//                statsd::Platform    { "platform" }     -- optional, os type
//                                                             -- client set it dinamically
//                                                             -- server not used
//                statsd::Project     { "enlisted" }     -- required, project name
//                statsd::Host        { "host_client" }  -- optional, sender host name
//                                                             -- required for client
//                                                             -- server set it dynamically
bool init(ILogger *logger, const char **keys, const char *addr, int port, Env env = {nullptr}, Circuit circuit = {nullptr},
  Application application = {nullptr}, Platform platform = {nullptr}, Project project = {nullptr}, Host host = {nullptr});

bool init_async(ILogger *logger, const char **keys, const char *addr, int port, Env env = {nullptr}, Circuit circuit = {nullptr},
  Application application = {nullptr}, Platform platform = {nullptr}, Project project = {nullptr}, Host host = {nullptr});

// ...or init_socket(), setup() and connect()
// init_socket() must be called before connect()
bool init_socket(ILogger *logger);
void set_prefix(const char **keys); // NULL-terimanted array of strings
inline void setup(const char *key0, const char *key1, const char *key2)
{
  const char *keys[] = {key0, key1, key2, NULL};
  set_prefix(keys);
}

bool connect(const char *addr, int port);

void enable(bool is_enabled);

void shutdown();

// inline int64_t get_timestamp_us() { return ref_time_delta_to_usec(ref_time_ticks()-0); }
// inline int64_t get_timestamp_ms() { return get_timestamp_us() / 1000; }
struct MetricTag
{
  const char *key;
  const char *value;
};

struct MetricFormat
{
  static const char *const influx;
  static const char *const influx_test;
  static const char *const statsd;
};

// How to use:
// Sample: statsd::counter( "metricOnceTag",   1,    { "action","testAction" } );
//                             | metric name    |change    | tag key    | tag value

// Common functions
// Simple syntax when send once metric tag, using dag::ConstSpan(&tag, 1) inside
void profile(const char *metric, float time_ms, const MetricTag &tag = {nullptr, nullptr});
void profile(const char *metric, long time_ms, const MetricTag &tag = {nullptr, nullptr});
void gauge(const char *metric, long value, const MetricTag &tag = {nullptr, nullptr});
void gauge_inc(const char *metric, long value, const MetricTag &tag = {nullptr, nullptr});
void gauge_dec(const char *metric, long value, const MetricTag &tag = {nullptr, nullptr});
void counter(const char *metric, long value = 1, const MetricTag &tag = {nullptr, nullptr});
void histogram(const char *metric, float value, const MetricTag &tag = {nullptr, nullptr});
void histogram(const char *metric, long value, const MetricTag &tag = {nullptr, nullptr});

void profile(const char *metric, float value, std::initializer_list<MetricTag> tags);
void profile(const char *metric, long value, std::initializer_list<MetricTag> tags);
void gauge(const char *metric, long value, std::initializer_list<MetricTag> tags);
void gauge_inc(const char *metric, long value, std::initializer_list<MetricTag> tags);
void gauge_dec(const char *metric, long value, std::initializer_list<MetricTag> tags);
void counter(const char *metric, long value, std::initializer_list<MetricTag> tags);
void histogram(const char *metric, float value, std::initializer_list<MetricTag> tags);
void histogram(const char *metric, long value, std::initializer_list<MetricTag> tags);

// Functions with support metrics array
using MetricTags = dag::ConstSpan<MetricTag>;
void profile(const char *metric, float time_ms, const MetricTags &tags);
void profile(const char *metric, long time_ms, const MetricTags &tags);
void gauge(const char *metric, long value, const MetricTags &tags);
void gauge_inc(const char *metric, long value, const MetricTags &tags);
void gauge_dec(const char *metric, long value, const MetricTags &tags);
void counter(const char *metric, long value, const MetricTags &tags);
void histogram(const char *metric, float value, const MetricTags &tags);
void histogram(const char *metric, long value, const MetricTags &tags);

void report(void (*action)(const char *, long, const MetricTag &), long value, const char *fmt, ...);

enum MetricType
{
  COUNTER,
  PROFILE,
  GAUGE,
  HISTOGRAM,
  GAUGE_INC,
  GAUGE_DEC,
};

} // namespace statsd


#define STATSD_PROFILE_IMPL(sample_name, action, time_div)                              \
  do                                                                                    \
  {                                                                                     \
    int64_t reft = ref_time_ticks();                                                    \
    action;                                                                             \
    statsd::profile("profiling." #sample_name, (long)(get_time_usec(reft) / time_div)); \
  } while (0)


#define STATSD_PROFILE_US(sample_name, action) STATSD_PROFILE_IMPL(sample_name, action, 1)
#define STATSD_PROFILE_MS(sample_name, action) STATSD_PROFILE_IMPL(sample_name, action, 1000)
