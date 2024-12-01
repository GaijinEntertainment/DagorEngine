// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "telemetry.h"
#include <statsd/statsd.h>
#include <eventLog/eventLog.h>
#include <eventLog/errorLog.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_hash.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_logSys.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_gpuConfig.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "main/circuit.h"
#include "net/dedicated.h"
#include "main/version.h"
#include "main/appProfile.h"
#include "main/settings.h"
#include "main/vromfs.h"
#include "main/main.h"
#include "main/gameLoad.h"

extern const char *default_game_statsd_url;
extern const char *default_game_statsd_key;
extern const char *default_game_eventlog_agent;
extern const char *default_game_eventlog_project;

void telemetry_shutdown()
{
  statsd::shutdown();
  event_log::shutdown();
}

static void init_statsd_client(const char *circuit_name)
{
  using namespace statsd;

  if (dgs_get_argv("nostatsd"))
    return;
  String os(framemem_ptr());
  os = get_platform_string_id();
  os.replaceAll(" ", "_");
  os.replaceAll(".", "_");
  os.toLower();

  const char *keys[] = {circuit_name, default_game_statsd_key, os, NULL};

  const char *metricsFormat = ::dgs_get_argv("statsd", MetricFormat::influx);
  const char *statsdProject = ::dgs_get_settings()->getStr("statsdGameName", nullptr);
  if (!statsdProject)
    statsdProject = ::dgs_get_settings()->getStr("matchingGameName", nullptr);
  if (!statsdProject)
    statsdProject = ::dgs_get_settings()->getStr("contactsGameId", get_game_name());

  const char *statsUrl = ::dgs_get_settings()->getStr("stats_url", default_game_statsd_url);
  if (!statsUrl)
    return;

  bool bInfluxProd = strcmp(metricsFormat, MetricFormat::influx) == 0;
  if (bInfluxProd || strcmp(metricsFormat, MetricFormat::influx_test) == 0)
  {
    statsd::init(statsd::get_dagor_logger(), keys, statsUrl, 20011, Env{bInfluxProd ? Env::production : Env::test},
      Circuit{circuit_name}, Application{Application::client}, statsd::Platform{os.c_str()}, Project{statsdProject},
      Host{"host_client"});
  }
  else if (strcmp(metricsFormat, MetricFormat::statsd) == 0)
  {
    statsd::init(statsd::get_dagor_logger(), keys, statsUrl, 20010);
  }
  else
    logerr("Unknown/unsupported statsd metrics format '%s'", metricsFormat);

  statsd::counter("app.start", 1); // put it as early as it can be
}

void init_statsd_common(const char *circuit_name)
{
  if (!dedicated::init_statsd(circuit_name))
    init_statsd_client(circuit_name);
}

static debug_log_callback_t orig_debug_log;
static int on_debug_log(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line);

void init_event_log(const char *circuit_name)
{
  const DataBlock *circuitCfg = circuit::get_conf();
  const DataBlock *cfg = circuitCfg ? circuitCfg->getBlockByName("eventLog") : nullptr;
  if (!cfg)
  {
    debug("Event log disabled: no configuration present");
    return;
  }

  const char *host = cfg->getStr("host", "");
  if (!host || !*host)
  {
    debug("Event log disabled: no host present");
    return;
  }

  event_log::EventLogInitParams eventInitParams;
  eventInitParams.host = host;
  eventInitParams.use_https = cfg->getBool("useHttps", true);
  eventInitParams.http_port = cfg->getInt("tcpPort", 0);
  eventInitParams.udp_port = cfg->getInt("udpPort", 20020);
  eventInitParams.user_agent = default_game_eventlog_agent;
  eventInitParams.origin = dedicated::is_dedicated() ? "dedicated" : "client";
  eventInitParams.circuit = circuit_name;
  eventInitParams.project = cfg->getStr("project", default_game_eventlog_project);
  // we probably don't need exe version but yup version. However - we don't have yup version
  eventInitParams.version = get_exe_version_str();

  if (event_log::init(eventInitParams))
    debug("Event log initialized as %s with '%s', udp %d, tcp %d", eventInitParams.origin, eventInitParams.host,
      eventInitParams.udp_port, eventInitParams.http_port);
  else
    logerr("Could not initialize remote event log.");

  event_log::ErrorLogInitParams eparams;
  eparams.collection = "events";
  eparams.game = get_game_name();

  if ((dedicated::is_dedicated() || DAGOR_DBGLEVEL == 0) && ::dgs_get_settings()->getBool("logerr_telemetry", true))
    orig_debug_log = debug_set_log_callback(on_debug_log);

  event_log::init_error_log(eparams);
}

static const char *find_nth_occurance(const String &where, char ch, int occurance_count)
{
  const char *end = where.find(ch);
  for (; occurance_count >= 1 && end; --occurance_count, ++end)
    end = where.find(ch, end);
  return end;
}

int on_debug_log(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  static const int D3DE_TAG = _MAKE4C('D3DE');

  if (orig_debug_log)
    orig_debug_log(lev_tag, fmt, arg, anum, ctx_file, ctx_line);

  if (lev_tag == LOGLEVEL_ERR || lev_tag == D3DE_TAG
#if _TARGET_PC_WIN | _TARGET_XBOX                                          // linux dumps to core on fatals anyway
      || (lev_tag == LOGLEVEL_FATAL && strstr(fmt, "FATAL ERROR") != NULL) // don't try to do IO on process exceptions
#endif
  )
  {
    const char chError = lev_tag == LOGLEVEL_ERR || lev_tag == D3DE_TAG ? 'E' : 'F';

    String s;
    if (ctx_file)
      s.printf(0, "[%c] %s,%d: ", chError, ctx_file, ctx_line);
    else
      s.printf(0, "[%c] ", chError);
    s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);

    event_log::ErrorLogSendParams params;
    params.attach_game_log = true;
    params.dump_call_stack = false;
    if (sceneload::is_user_game_mod())
      params.collection = dedicated::is_dedicated() ? "mods_dedicated_errorlog" : "mods_client_errorlog";
    else if (!app_profile::get().replay.playFile.empty())
      params.collection = "replay_client_errorlog"; // not possible to run replay in dedicated
    else
      params.collection = dedicated::is_dedicated() ? "dedicated_errorlog" : "client_errorlog";
    if (lev_tag == D3DE_TAG)
    {
      params.collection = "client_d3d_errorlog";
      params.meta["d3d_driver"] = d3d::get_driver_name();
      params.meta["gpu_vendor"] = d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor);
    }
    if (!app_profile::get().serverSessionId.empty())
    {
      params.meta["sessionId"] = app_profile::get().serverSessionId;
      if (dedicated::is_dedicated())
        params.attach_game_log = false;
    }

    const bool isNotQuirrelError = strstr(s.str(), "LOCALS") == nullptr && strstr(s.str(), "[SQ]") == nullptr;
    // Calculate hash for an event by format string.
    // In order to separate events on the server.
    // Skip squirrel errors.
    if (isNotQuirrelError)
    {
      // All fatals have the same format string.
      // So, the hash must be calculated in a different way
      // in order to separate on fatal of another.
      if (lev_tag == LOGLEVEL_FATAL)
      {
        // Calculate hash by LINES_COUNT_TO_HASH lines of message
        static const uint32_t LINES_COUNT_TO_HASH = 2;
        if (const char *end = find_nth_occurance(s, '\n', LINES_COUNT_TO_HASH))
          params.exMsgHash = mem_hash_fnv1(s.str(), end - s.str());
      }

      const uint32_t fmt_hash = __log_get_ctx_hash();
      if (!params.exMsgHash && fmt_hash)
        params.exMsgHash = fmt_hash;

      if (!params.exMsgHash)
        params.exMsgHash = str_hash_fnv1(fmt);
    }

    if (const updater::Version vromsVersion = get_updated_game_version())
      params.meta["gameVersion"] = (const char *)vromsVersion.to_string();

    auto &scnn = sceneload::get_current_game().sceneName;
    const char *pSlash = strrchr(scnn.c_str(), '/'), *pDot = strrchr(scnn.c_str(), '.');
    params.meta["scene"] =
      eastl::string(pSlash ? &pSlash[1] : scnn.c_str(), (pSlash && pDot > pSlash) ? pDot : (scnn.c_str() + scnn.length()));
    params.meta["uid"] = app_profile::get().userId.empty() ? "-1" : app_profile::get().userId;

    event_log::send_error_log(s.str(), params);
  }

  return 1;
}

void send_first_run_event()
{
  if (!dgs_get_settings()->getBool("launchCountTelemetry", true))
    return;
  int launchCnt = dgs_get_settings()->getInt("launchCnt", 0);
  get_settings_override_blk()->setInt("launchCnt", launchCnt + 1);
  save_settings(nullptr, false);

  if (launchCnt < 1)
  {
    event_log::ErrorLogSendParams params;
    params.attach_game_log = false;
    params.dump_call_stack = false;
    params.collection = "events";
    params.meta["launch_cnt"] = launchCnt;
    params.meta["event"] = "first_run";
    event_log::send_error_log("first_run", params);
  }
}
