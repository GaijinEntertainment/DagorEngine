//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <quirrel/sqEventBus/sqEventBus.h>
#include <rapidjson/fwd.h>
#include <EASTL/variant.h>

namespace sqeventbus
{

using JsonVariant = eastl::variant<eastl::monostate, Json::Value, rapidjson::Document>;
using NativeEventHandler = void (*)(const char *event_name, const JsonVariant &data, const char *source_id, bool immediate);

void set_native_event_handler(NativeEventHandler handler);

void send_event_ex(const char *event_name, rapidjson::Document &&data, const EventParams &eventParams);

void send_event(const char *event_name, const rapidjson::Document &data, const char *source_id = "native");
inline void send_event(const char *event_name, rapidjson::Document &&data, const char *source_id = "native")
{
  send_event_ex(event_name, eastl::move(data), EventParams{source_id, 0});
}

} // namespace sqeventbus
