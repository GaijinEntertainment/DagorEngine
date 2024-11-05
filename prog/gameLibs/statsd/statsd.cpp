// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <statsd/statsd.h>
#include <util/dag_simpleString.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_sockets.h>
#include <debug/dag_debug.h>
#include <EASTL/fixed_vector.h>
#include <memory/dag_framemem.h>
#include <initializer_list>
#include <asyncResolveAddr/asyncResolveAddr.h>

namespace statsd
{

#define STATSD_PACKET_MAX_SIZE (1024 - 28) // this should fit in MTU (28 is UDP header size)

static sockets::Socket stats_sock(OSAF_IPV4, OST_UDP, false);
static bool sending_enabled = true;
static bool sockets_initialized = false;
static SimpleString prefix;
static ILogger *logger = NULL;
static bool useNewMetricFormat = false;

const char *const Env::test = "test";
const char *const Env::production = "production";

const char *const Application::dedicated = "dedicated";
const char *const Application::client = "client";

const char *const MetricFormat::influx = "influx";
const char *const MetricFormat::influx_test = "influx_test";
const char *const MetricFormat::statsd = "statsd";

#undef logerr
#undef debug

namespace internal
{
static SimpleString env;
static SimpleString circuit;
static SimpleString application;
static SimpleString platform;
static SimpleString project;
static SimpleString hostcl;
} // namespace internal

static void debug(const char *fmt, ...)
{
  if (logger)
  {
    va_list vl;
    va_start(vl, fmt);
    logger->debugImpl(fmt, vl);
    va_end(vl);
  }
}

static void logerr(const char *fmt, ...)
{
  if (logger)
  {
    va_list vl;
    va_start(vl, fmt);
    logger->logerrImpl(fmt, vl);
    va_end(vl);
  }
}


#if _TARGET_C1 | _TARGET_C2















#else
static bool try_restore_network(int /*err*/) { return false; }
#endif


static void try_recover_on_error(int sent, const char *buf, int size, const char *origin)
{
  int lastError = stats_sock.getLastError();
  if (sent < 0 && lastError != OS_SOCKET_ERR_WOULDBLOCK)
  {
    char err[255] = {0};
    debug("%s: failed to send batch to statsd: %s", origin, stats_sock.getLastErrorStr(err, sizeof(err)));
    if (try_restore_network(lastError))
      stats_sock.send(buf, size);
  }
}

bool init_socket(ILogger *logger_)
{
  char errStr[512];
  logger = logger_;

  stats_sock.reset();
  if (!stats_sock)
  {
    logerr("%s: failed to reinitialize statsd socket: %s (%d)", __FUNCTION__, stats_sock.getLastErrorStr(errStr, sizeof(errStr)),
      stats_sock.getLastError());
    return false;
  }
  if (stats_sock.setopt(OSO_NONBLOCK, 1) != 0)
  {
    logerr("%s: failed to set non-blocking mode for statsd socket: %s (%d)", __FUNCTION__,
      stats_sock.getLastErrorStr(errStr, sizeof(errStr)), stats_sock.getLastError());
    stats_sock.close();
    return false;
  }

  return true;
}

void set_prefix(const char **keys)
{
  G_ASSERT(keys);
  char tag_buffer[256];
  int bufLen = 0;
  for (const char *key = *keys; key; key = *(++keys))
  {
    int written = _snprintf(tag_buffer + bufLen, sizeof(tag_buffer) - bufLen, "%s.", key);
    if ((size_t)written < sizeof(tag_buffer) - bufLen)
      bufLen += written;
    else
    {
      tag_buffer[sizeof(tag_buffer) - 1] = '\0';
      G_ASSERT_LOG(0, "%s: too long prefix '%s'", __FUNCTION__, tag_buffer);
      sending_enabled = false;
      return;
    }
  }
  if (prefix.empty() || strcmp(tag_buffer, prefix.str()) != 0)
  {
    debug("%s: setting statsd prefix: '%s'", __FUNCTION__, tag_buffer);
    prefix = tag_buffer;
  }
}

static bool g_async_resolve_in_progress = false;
static eastl::fixed_vector<SimpleString, 64, false /* bEnableOverflow*/> g_send_queue;

static void on_resolved(const sockets::SocketAddr<OSAF_IPV4> &ipv4_addr)
{
  if (!stats_sock)
    return;

  auto shutdownAndFailWithMsg = [](auto... args) {
#if DAGOR_DBGLEVEL > 0
    logerr(args...);
#else
    logwarn(args...);
#endif
    shutdown();
  };

  g_async_resolve_in_progress = false;

  if (!ipv4_addr.isValid())
  {
    shutdownAndFailWithMsg("%s: bad statsd address", __FUNCTION__);
    return;
  }

  if (stats_sock.connect(ipv4_addr) != 0)
  {
    char errStr[512];
    shutdownAndFailWithMsg("%s: connect() call failed: %s", __FUNCTION__, stats_sock.getLastErrorStr(errStr, sizeof(errStr)));
    return;
  }

  debug("%s: [STATSD] flush message queue after resolve: %d", __FUNCTION__, g_send_queue.size());

  for (const SimpleString &msg : g_send_queue)
  {
    int nsent = (int)stats_sock.send(msg.str(), msg.length());
    try_recover_on_error(nsent, msg.str(), msg.length(), __FUNCTION__);
  }

  g_send_queue.clear();
}

static bool connect_async(const char *addr, int port)
{
  G_ASSERT(addr && *addr);

  if (!stats_sock)
  {
    logerr("%s: uninitialized socket", __FUNCTION__);
    return false;
  }

  debug("%s: statsd server: %s:%d", __FUNCTION__, addr, port);

  g_async_resolve_in_progress = true;

  sockets::resolve_socket_addr_async(addr, (uint16_t)port, on_resolved);

  return true;
}

bool connect(const char *addr, int port)
{
  G_ASSERT(addr && *addr);

  if (!stats_sock)
  {
    logerr("%s: uninitialized socket", __FUNCTION__);
    return false;
  }

  debug("%s: statsd server: %s:%d", __FUNCTION__, addr, port);

  sockets::SocketAddr<OSAF_IPV4> ipv4Addr{addr, (uint16_t)port};
  if (!ipv4Addr.isValid())
  {
    logerr("%s: bad statsd address: %s:%d", __FUNCTION__, addr, port);
    return false;
  }

  if (stats_sock.connect(ipv4Addr) != 0)
  {
    char errStr[512];
    logerr("%s: connect() call failed: %s", __FUNCTION__, stats_sock.getLastErrorStr(errStr, sizeof(errStr)));
    return false;
  }

#if _TARGET_C1 | _TARGET_C2


#endif

  return true;
}

void enable(bool is_enabled) { sending_enabled = is_enabled; }

template <typename ValueType>
constexpr const char *value_spec_string_new = ":%s%ld|%s";

template <>
constexpr const char *value_spec_string_new<float> = ":%s%f|%s";

template <typename ValueType>
static void send_internal(const char *metric, dag::ConstSpan<MetricTag> tags, MetricType mtype, ValueType value)
{
  if (!sending_enabled)
    return;

  if (!stats_sock && !g_async_resolve_in_progress)
  {
    logerr("%s: uninitialized socket, init_statsd isn't called yet?", __FUNCTION__);
    return;
  }

  if (!useNewMetricFormat)
  {
    logerr("[STATSD] Tags array support only with new metric format, metric=%s", metric);
    return;
  }

  char buf[STATSD_PACKET_MAX_SIZE];

  const char *type = "";
  const char *val_prefix = "";
  switch (mtype)
  {
    case COUNTER: type = "c"; break;
    case PROFILE: type = "ms"; break;
    case GAUGE: type = "g"; break;
    case HISTOGRAM: type = "h"; break;
    case GAUGE_INC:
      type = "g";
      val_prefix = "+";
      break;
    case GAUGE_DEC:
      type = "g";
      val_prefix = "-";
      break;
    default: G_ASSERT(0);
  }

  char *pos = buf;
  size_t bytes_left = sizeof(buf);
  int bytesAdded = 0;

  // permanent header
  {
    bytesAdded = _snprintf(pos, bytes_left, "%s,env=%s,circuit=%s,application=%s,project=%s", metric, internal::env.c_str(),
      internal::circuit.c_str(), internal::application.c_str(), internal::project.c_str());
    G_ASSERTF((size_t)bytesAdded < bytes_left, "send_internal: should never happens, something went wrong");
    pos += bytesAdded;
    bytes_left -= bytesAdded;
  }

  if (!internal::platform.empty())
  {
    bytesAdded = _snprintf(pos, bytes_left, ",platform=%s", internal::platform.c_str());
    pos += bytesAdded;
    bytes_left -= bytesAdded;
  }

  if (!internal::hostcl.empty())
  {
    bytesAdded = _snprintf(pos, bytes_left, ",host=%s", internal::hostcl.c_str());
    pos += bytesAdded;
    bytes_left -= bytesAdded;
  }

  // temporary tags
  for (auto &tag : tags)
  {
    // check that key and value are correct string
    if (!tag.key || !tag.value || !*tag.key || !*tag.value)
      continue;

    bytesAdded = _snprintf(pos, bytes_left, ",%s=%s", tag.key, tag.value);
    if ((size_t)bytesAdded >= bytes_left)
    {
      logerr("%s: Metric too long for batch, skipping: %s", buf);
      return;
    }

    pos += bytesAdded;
    bytes_left -= bytesAdded;
  }

  // value specification
  {
    bytesAdded = _snprintf(pos, bytes_left, value_spec_string_new<ValueType>, val_prefix, value, type);
    if ((size_t)bytesAdded >= bytes_left)
    {
      logerr("%s: Metric too long for batch, skipping: %s", buf);
      return;
    }

    pos += bytesAdded;
    bytes_left -= bytesAdded;
  }

  G_ASSERT(pos < (buf + sizeof(buf)));
  int total = (int)(pos - buf);

  if (!g_async_resolve_in_progress)
  {
    int nsent = (int)stats_sock.send(buf, total);
    try_recover_on_error(nsent, buf, total, __FUNCTION__);
  }
  else
  {
    debug("%s: [STATSD] schedule message for send after resolve", __FUNCTION__);
    if (g_send_queue.full())
      debug("%s: [STATSD] send queue is full, dropping message", __FUNCTION__);
    else
      g_send_queue.emplace_back(buf, total);
  }
}

template <MetricType mtype, typename ValueType, typename... Args>
void send_internal_args(const char *metric, ValueType value, const Args &...args)
{
  MetricTag n[] = {args...};
  send_internal(metric, dag::ConstSpan<MetricTag>(n, sizeof...(args)), mtype, value);
}

template <MetricType mtype, typename ValueType>
void send_internal_tags(const char *metric, ValueType value, std::initializer_list<MetricTag> _tags)
{
  eastl::fixed_vector<MetricTag, 3, true, framemem_allocator> tags;
  for (auto &a : _tags)
    tags.push_back(a);
  send_internal(metric, dag::ConstSpan<MetricTag>(tags.data(), tags.size()), mtype, value);
}

template <MetricType mtype, typename ValueType>
void send_internal(const char *metric, dag::ConstSpan<MetricTag> tags, ValueType value)
{
  send_internal(metric, tags, mtype, value);
}

template <typename ValueType>
constexpr const char *value_spec_string_old = "%s%s:%s%ld|%s";

template <>
constexpr const char *value_spec_string_old<float> = "%s%s:%s%g|%s";

template <typename ValueType>
static void statsd_send(const char *metric, const char *type, const char *val_pref, ValueType value)
{
  G_ASSERT(metric && *metric);
  G_ASSERT(type && *type);
  G_ASSERT(val_pref); // can be empty string

  if (prefix.empty() || !sending_enabled)
    return;

  if (!stats_sock && !g_async_resolve_in_progress)
  {
    logerr("%s: uninitialized socket, init_statsd isn't called yet?", __FUNCTION__);
    return;
  }

  char buf[STATSD_PACKET_MAX_SIZE];
  int n = _snprintf(buf, sizeof(buf), value_spec_string_old<ValueType>, prefix.str(), metric, val_pref, value, type);
  if ((size_t)n >= sizeof(buf))
  {
    logerr("%s: failed to format string data for statsd (tag: '%s', metric: '%s', "
           "val_pref: '%s', value: %ld, type: '%s'",
      __FUNCTION__, prefix.str(), metric, val_pref, value, type);
    return;
  }

  if (!g_async_resolve_in_progress)
  {
    int nsent = stats_sock.send(buf, n);
    try_recover_on_error(nsent, buf, n, __FUNCTION__);
  }
  else
  {
    debug("%s: [STATSD] schedule message for send after resolve", __FUNCTION__);
    if (g_send_queue.full())
      debug("%s: [STATSD] send queue is full, dropping message", __FUNCTION__);
    else
      g_send_queue.emplace_back(buf, n);
  }
}

// Common functions
void gauge(const char *metric, long value, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<GAUGE>(metric, value, tag);
  else
    statsd_send(metric, "g", "", value);
}

void gauge(const char *metric, long value, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<GAUGE>(metric, value, tags);
  else
    statsd_send(metric, "g", "", value);
}

void gauge_inc(const char *metric, long value, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<GAUGE_INC>(metric, value, tag);
  else
    statsd_send(metric, "g", "+", value);
}

void gauge_inc(const char *metric, long value, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<GAUGE_INC>(metric, value, tags);
  else
    statsd_send(metric, "g", "+", value);
}

void gauge_dec(const char *metric, long value, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<GAUGE_DEC>(metric, value, tag);
  else
    statsd_send(metric, "g", "-", abs(value));
}

void gauge_dec(const char *metric, long value, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<GAUGE_DEC>(metric, value, tags);
  else
    statsd_send(metric, "g", "-", abs(value));
}

void counter(const char *metric, long value, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<COUNTER>(metric, value, tag);
  else
    statsd_send(metric, "c", "", value);
}

void counter(const char *metric, long value, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<COUNTER>(metric, value, tags);
  else
    statsd_send(metric, "c", "", value);
}

void profile(const char *metric, float time_ms, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<PROFILE>(metric, time_ms, tag);
  else
    statsd_send(metric, "ms", "", time_ms);
}

void profile(const char *metric, long time_ms, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<PROFILE>(metric, time_ms, tag);
  else
    statsd_send(metric, "ms", "", time_ms);
}

void profile(const char *metric, float time_ms, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<PROFILE>(metric, time_ms, tags);
  else
    statsd_send(metric, "ms", "", time_ms);
}

void profile(const char *metric, long time_ms, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<PROFILE>(metric, time_ms, tags);
  else
    statsd_send(metric, "ms", "", time_ms);
}

void histogram(const char *metric, float value, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<HISTOGRAM>(metric, value, tag);
  else
    statsd_send(metric, "h", "", value);
}

void histogram(const char *metric, long value, const MetricTag &tag)
{
  if (useNewMetricFormat)
    send_internal_args<HISTOGRAM>(metric, value, tag);
  else
    statsd_send(metric, "h", "", value);
}

void histogram(const char *metric, float value, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<HISTOGRAM>(metric, value, tags);
  else
    statsd_send(metric, "h", "", value);
}

void histogram(const char *metric, long value, std::initializer_list<MetricTag> tags)
{
  if (useNewMetricFormat)
    send_internal_tags<HISTOGRAM>(metric, value, tags);
  else
    statsd_send(metric, "h", "", value);
}

// Tag arrays functions
void gauge(const char *metric, long value, const MetricTags &tags) { send_internal(metric, tags, COUNTER, value); }

void gauge_inc(const char *metric, long value, const MetricTags &tags) { send_internal(metric, tags, GAUGE_INC, value); }

void gauge_dec(const char *metric, long value, const MetricTags &tags) { send_internal(metric, tags, GAUGE_DEC, value); }

void counter(const char *metric, long value, const MetricTags &tags) { send_internal(metric, tags, COUNTER, value); }

void profile(const char *metric, float time_ms, const MetricTags &tags) { send_internal(metric, tags, PROFILE, time_ms); }

void profile(const char *metric, long time_ms, const MetricTags &tags) { send_internal(metric, tags, PROFILE, time_ms); }

void histogram(const char *metric, float value, const MetricTags &tags) { send_internal(metric, tags, HISTOGRAM, value); }

void histogram(const char *metric, long value, const MetricTags &tags) { send_internal(metric, tags, HISTOGRAM, value); }

template <typename ConnectFunc>
static bool init_impl(ILogger *logger_, const char **keys, Env env, Circuit circuit, Application application, Platform platform,
  Project project, Host hostcl, ConnectFunc connect_func)
{
  if (sockets_initialized)
  {
    G_ASSERT(stats_sock);
    return true;
  }
  if (os_sockets_init() != 0)
  {
    logerr("%s: Fail to init sockets", __FUNCTION__);
    return false;
  }
  if (!init_socket(logger_))
  {
    os_sockets_shutdown();
    return false;
  }
  sockets_initialized = true;
  set_prefix(keys);

  if (connect_func())
  {
    if (env.value != nullptr)
    {
      internal::env = env.value;
      internal::circuit = circuit.value;
      internal::application = application.value;
      internal::platform = platform.value;
      internal::project = project.value;
      internal::hostcl = hostcl.value;
      useNewMetricFormat = true;
    }

    return true;
  }
  shutdown();
  return false;
}

bool init(ILogger *logger_, const char **keys, const char *addr, int port, Env env, Circuit circuit, Application application,
  Platform platform, Project project, Host hostcl)
{
  return init_impl(logger_, keys, env, circuit, application, platform, project, hostcl,
    [addr, port]() { return connect(addr, port); });
}

bool init_async(ILogger *logger_, const char **keys, const char *addr, int port, Env env, Circuit circuit, Application application,
  Platform platform, Project project, Host hostcl)
{
  return init_impl(logger_, keys, env, circuit, application, platform, project, hostcl,
    [addr, port]() { return connect_async(addr, port); });
}

void report(void (*action)(const char *, long, const MetricTag &), long value, const char *fmt, ...)
{
  G_ASSERT(action);

  char metric[128];

  va_list ap;
  va_start(ap, fmt);
  int written = _vsnprintf(metric, sizeof(metric), fmt, ap);
  va_end(ap);

  G_ASSERT((size_t)written < sizeof(metric));
  (void)written;

  (*action)(metric, value, {nullptr, nullptr});
}

void shutdown()
{
  stats_sock.close();
  prefix.clear();
  if (sockets_initialized)
  {
    os_sockets_shutdown();
    sockets_initialized = false;
  }
}


} // namespace statsd
