//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClientInstance/sepClientInstanceTypes.h>

#include <sepClient/sepClientStats.h>


class DataBlock;


namespace sepclientinstance::global_instance
{

using get_config_cb_t = const DataBlock *();
using ConfigProvider = eastl::function<SepClientInstanceConfig()>;

void global_init_once(get_config_cb_t config_cb, AuthTokenProvider &&token_provider,
  const sepclient::SepClientStatsCallbackPtr &stats_callback, eastl::string_view default_project_id);

void global_init_once(const SepClientInstanceConfig &config, AuthTokenProvider &&token_provider,
  const sepclient::SepClientStatsCallbackPtr &stats_callback);

bool is_global_initialized();

SepClientInstance *get_global_instance();

void global_initiate_shutdown();

bool is_global_shutdown_completed();

void global_shutdown(int max_milliseconds_to_wait);

void global_poll();

} // namespace sepclientinstance::global_instance
