#include <eventLog/eventLog.h>

#include <util/dag_baseDef.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_logSys.h>

#include <sysinfo.h>

#include <json/json.h>

#include "dataHelpers.h"
#include "httpRequest.h"

#define LOGLEVEL_DEBUG _MAKE4C('EVLG')


namespace event_log
{
char net_assert_version[64] = "";

const char *get_net_assert_version() { return net_assert_version; }
void set_net_assert_version(const char *s) { strncpy(net_assert_version, s, sizeof(net_assert_version)); }

bool fatal_on_net_assert = false;

namespace defaults
{
const uint32_t UDP_PACKET_RELIABLE_SIZE = 1300;    // In bytes
const uint32_t UDP_PACKET_INSANE_SIZE = 32 * 1024; // In bytes
const uint32_t REQUEST_TIMEOUT_SEC = 5;
const char *REQUEST_PATH = "obj";
Json::Value meta;
} // namespace defaults

static SimpleString project;

struct Configuration
{
  SimpleString host;
  short httpPort;
  short udpPort;
  SimpleString userAgent;
  time_t timeout;
  SimpleString project;
  SimpleString origin;
  SimpleString circuit;
  SimpleString version;
  String url;
  bool useHttps;

  Configuration(EventLogInitParams const &init_params) :
    host(init_params.host),
    httpPort(init_params.http_port),
    udpPort(init_params.udp_port),
    userAgent(init_params.user_agent),
    timeout(defaults::REQUEST_TIMEOUT_SEC),
    project(init_params.project),
    origin(init_params.origin),
    circuit(init_params.circuit),
    version(init_params.version),
    useHttps(init_params.use_https)
  {
    // Historically, we used 7800 so make it default
    static constexpr short DEF_HTTP_PORT = 7800;
    static constexpr short DEF_HTTPS_PORT = 443;
    const char *proto = useHttps ? "https" : "http";
    short port = (httpPort != 0) ? httpPort : (useHttps ? DEF_HTTPS_PORT : DEF_HTTP_PORT);
    if (project.empty())
      url.printf(256, "%s://%s:%d/%s", proto, host, port, defaults::REQUEST_PATH);
    else
      url.printf(256, "%s://%s:%d/%s/v2/%s", proto, host, port, defaults::REQUEST_PATH, project);
  }
}; // struct Configuration


static Configuration *config = NULL;

static sockets::SocketAddr<OSAF_IPV4> udp_addr;
static InitOnDemand<sockets::Socket, false> udp_socket;

static debug_log_callback_t orig_debug_log;
static int netlog_handler(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (lev_tag == _MAKE4C('nEvt'))
  {
    stackhelp::CallStackCaptureStore<32> stack;
    stackhelp::ext::CallStackCaptureStore extStack;
    stack.capture();
    extStack.capture();

    String s;
    if (ctx_file)
      s.printf(0, "%s,%d: ", ctx_file, ctx_line);
    s.aprintf(0, "%s:", event_log::get_net_assert_version());
    s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);
    s.aprintf(256, "\nST:\n%s", get_call_stack_str(stack, extStack));

    static constexpr int MAX_UDP_LENGTH = 900;
    if (s.length() > MAX_UDP_LENGTH)
      event_log::send_http("assert", s.str(), s.length());
    else
      event_log::send_udp("assert", s.str(), s.length());
  }

  if (orig_debug_log)
    orig_debug_log(lev_tag, fmt, arg, anum, ctx_file, ctx_line);
  return 1;
}

bool init(EventLogInitParams const &init_params)
{
  G_ASSERT(init_params.host && init_params.user_agent);

  auto shutdownAndFailWithMsg = [](auto... args) {
#if DAGOR_DBGLEVEL > 0
    logerr(args...);
#else
    logwarn(args...);
#endif
    shutdown();
    return false;
  };

  os_sockets_init();

  config = new Configuration(init_params);
  udp_socket.demandInit(OSAF_IPV4, OST_UDP);

#define RET_ON_SOCK_FAIL(s, msg)                                \
  if (!s)                                                       \
  {                                                             \
    char b[512] = {0};                                          \
    s->getLastErrorStr(b, sizeof(b));                           \
    return shutdownAndFailWithMsg(msg ": %s", (const char *)b); \
  }
  udp_socket->reset();
  RET_ON_SOCK_FAIL(udp_socket, "[network] Could not init UDP socket");

  udp_socket->setopt(OSO_NONBLOCK, 1);
  RET_ON_SOCK_FAIL(udp_socket, "[network] Could not set UDP socket NONBLOCK");
#undef RET_ON_SOCK_FAIL

  udp_addr.setHost(config->host);
  udp_addr.setPort(config->udpPort);

  if (!udp_addr.isValid())
    return shutdownAndFailWithMsg("[network] Could not set UDP socket addr '%s:%d", config->host, config->udpPort);

  // Now set default metadata
  defaults::meta["platform"] = get_platform_string_id();
  ConsoleModel consoleModel = get_console_model();
  if (consoleModel != ConsoleModel::UNKNOWN)
    defaults::meta["console_model"] = get_console_model_revision(consoleModel);
  if (!config->origin.empty())
    defaults::meta["origin"] = config->origin.str();
  if (!config->circuit.empty())
    defaults::meta["circuit"] = config->circuit.str();
  if (!config->project.empty())
    defaults::meta["project"] = config->project.str();
  if (!config->version.empty())
    defaults::meta["version"] = config->version.str();

  const eastl::string id = systeminfo::get_system_id();
  if (!id.empty())
    defaults::meta["system_id"] = id.c_str();


  orig_debug_log = debug_set_log_callback(netlog_handler);
  return true;
}


void shutdown()
{
  if (!is_enabled())
    return;

  udp_socket.demandDestroy();
  del_it(config);
  os_sockets_shutdown();
}


bool is_enabled() { return (config != NULL); }


template <typename Func>
void send_http_impl(const Func &func_send, const char *type, const void *data, uint32_t size, Json::Value *meta)
{
  if (!is_enabled())
    return;

  Packet pkt(type, meta, data, size, defaults::meta);
  debug("[network] Event log send HTTP packet %zu bytes to %s:%d", pkt.size(), config->host, config->udpPort);
  func_send(config->url.str(), pkt.data(), pkt.size(), config->userAgent.str(), config->timeout);
}

void send_http_instant(const char *type, const void *data, uint32_t size, Json::Value *meta)
{
  send_http_impl(http::post, type, data, size, meta);
}

void send_http(const char *type, const void *data, uint32_t size, Json::Value *meta)
{
  send_http_impl(http::post_async, type, data, size, meta);
}

void send_udp(const char *type, const void *data, uint32_t size, Json::Value *meta)
{
  if (!is_enabled())
    return;

  Packet pkt(type, meta, data, size, defaults::meta);
  debug("[network] Event log send UDP packet %zu bytes to %s:%d", pkt.size(), config->host, config->udpPort);
  if (pkt.size() > defaults::UDP_PACKET_RELIABLE_SIZE)
    logwarn("[network] Sending UDP packet bigger than %d bytes, size is %d", defaults::UDP_PACKET_RELIABLE_SIZE, pkt.size());

  G_UNUSED(defaults::UDP_PACKET_INSANE_SIZE);
  G_ASSERT(pkt.size() <= defaults::UDP_PACKET_INSANE_SIZE);

  int r = udp_socket->sendtoWithResetOnError(udp_addr, (const char *)pkt.data(), (uint32_t)pkt.size());
  if (r == -1)
  {
    char buf[256];
    logerr("[network] Could not send %zu bytes to %s:%d via UDP: %s", pkt.size(), config->host, config->udpPort,
      udp_socket->getLastErrorStr(buf, sizeof(buf)));
  }
}
} // namespace event_log
