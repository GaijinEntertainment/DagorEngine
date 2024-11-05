//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <json/value.h>

namespace event_log
{
struct ErrorLogInitParams
{
  const char *collection = nullptr;
  const char *game = nullptr;
  float fatal_probability = 0.0f;
};

void init_error_log(const ErrorLogInitParams &params);
void set_error_log_meta_val(const char *key, const char *value);

enum class LogSeverity
{
  Low,
  Normal,
  Critical
};

struct ErrorLogSendParams
{
  const char *collection = nullptr;
  bool dump_call_stack = true;
  bool attach_game_log = false;
  bool instant_send = false;
  LogSeverity severity = LogSeverity::Normal;
  uint32_t exMsgHash = 0;

  Json::Value meta;
};

void send_error_log(const char *error_message, const ErrorLogSendParams &params = ErrorLogSendParams());

} // namespace event_log
