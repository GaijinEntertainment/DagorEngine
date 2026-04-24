// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClientInstance/globalSepClientInstance.h>

#include <sepClientInstance/blkConfigReader.h>
#include <sepClientInstance/sepClientInstance.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>


namespace sepclientinstance::global_instance
{

static bool global_init_called = false;
static InitOnDemand<SepClientInstance, false> global_sep_client_instance;


// ========================================================


void global_init_once(get_config_cb_t config_cb, AuthTokenProvider &&token_provider,
  const sepclient::SepClientStatsCallbackPtr &stats_callback, eastl::string_view default_project_id)
{
  sepclientinstance::SepClientInstanceConfig sepConfig = sepclientinstance::configreader::from_circuit_blk_config(config_cb);

  sepConfig.defaultProjectId = default_project_id;

  global_init_once(sepConfig, eastl::move(token_provider), stats_callback);
}


void global_init_once(const SepClientInstanceConfig &config, AuthTokenProvider &&token_provider,
  const sepclient::SepClientStatsCallbackPtr &stats_callback)
{
  logdbg("[sep] SepClientInstance: global SEP client instance init");

  G_ASSERT_RETURN(!global_init_called, );
  global_init_called = true;

  if (!config.isSepEnabled())
  {
    logdbg("[sep] SepClientInstance: SEP is disabled");
    return;
  }

  G_ASSERT(!global_sep_client_instance.get());
  global_sep_client_instance.demandInit(config, eastl::move(token_provider), stats_callback);
  G_ASSERT(global_sep_client_instance.get());

  logdbg("[sep] SepClientInstance: SEP is enabled");
}


bool is_global_initialized() { return global_init_called; }


SepClientInstance *get_global_instance() { return global_sep_client_instance.get(); }


void global_initiate_shutdown()
{
  logdbg("[sep] SepClientInstance: global *initiate* SEP client instance shutdown");
  SepClientInstance *sepClientInstance = global_sep_client_instance.get();
  if (sepClientInstance)
  {
    sepClientInstance->initiateClose();

    // Existing games may not call poll() any more after `global_shutdown()` call.
    // Make at least one call to poll() to provide a chance for WebSocket to send close message to the server.
    sepClientInstance->poll();
  }
}


bool is_global_shutdown_completed()
{
  SepClientInstance *sepClientInstance = global_sep_client_instance.get();
  return sepClientInstance ? sepClientInstance->isCloseCompleted() : true;
}


void global_shutdown(const int max_milliseconds_to_wait)
{
  logdbg("[sep] SepClientInstance: global SEP client instance shutdown: start");
  const auto beginTick = ref_time_ticks();
  const auto timeoutTick = rel_ref_time_ticks(beginTick, max_milliseconds_to_wait * 1000);

  SepClientInstance *sepClientInstance = global_sep_client_instance.get();
  if (sepClientInstance)
  {
    sepClientInstance->initiateClose();

    // Existing games may not call poll() any more after `global_shutdown()` call.
    // Make at least one call to poll() to provide a chance for WebSocket to send close message to the server.
    sepClientInstance->poll();

    while (!sepClientInstance->isCloseCompleted())
    {
      const auto nowTick = ref_time_ticks();
      if (nowTick >= timeoutTick)
      {
        logwarn("WARN! SepClientInstance: global SEP client instance shutdown: timeout after %d ms",
          (int)(ref_time_delta_to_usec(nowTick - beginTick) / 1000));
        break;
      }

      sleep_msec(1);
      sepClientInstance->poll();
    }

    sepClientInstance->destroy();
  }
  logdbg("[sep] SepClientInstance: global SEP client instance shutdown: finish");
}


void global_poll()
{
  if (global_sep_client_instance.get())
  {
    global_sep_client_instance->poll();
  }
}


} // namespace sepclientinstance::global_instance
