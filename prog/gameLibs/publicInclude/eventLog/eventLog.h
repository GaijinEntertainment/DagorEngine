//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_simpleString.h>

// Forward declarations
namespace cpujobs
{
class IJob;
}

namespace Json
{
class Value;
}

namespace event_log
{
struct EventLogInitParams
{
  const char *host = nullptr;
  uint16_t http_port = 0;
  uint16_t udp_port = 20020;
  const char *user_agent = nullptr;
  const char *origin = nullptr;
  const char *circuit = nullptr;
  const char *project = nullptr;
  const char *version = nullptr;
  bool use_https = true;
};

// The following 2 functions are not threadsafe
bool init(EventLogInitParams const &init_params);
void shutdown();

bool is_enabled();

void send_http(const char *type, const void *data, uint32_t size, Json::Value *meta = NULL);
void send_http_instant(const char *type, const void *data, uint32_t size, Json::Value *meta = NULL);
void send_udp(const char *type, const void *data, uint32_t size, Json::Value *meta = NULL);
} // namespace event_log
