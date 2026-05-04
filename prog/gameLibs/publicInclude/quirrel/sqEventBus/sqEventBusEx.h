//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <quirrel/sqEventBus/sqEventBus.h>
#include <rapidjson/fwd.h>
#include <EASTL/variant.h>
#include <sqrat.h>

namespace sqeventbus
{

using JsonVariant = eastl::variant<eastl::monostate, Json::Value, rapidjson::Document,
  /*rel ofs to JsonVariant*/ intptr_t, Sqrat::Object>;
using NativeEventHandler = void (*)(const char *event_name, const JsonVariant &data, const char *source_id, bool immediate);

void set_native_event_handler(NativeEventHandler handler);

void send_event_ex(const char *event_name, rapidjson::Document &&data, const EventParams &eventParams);

// Note: intentonally no `const rapidjson::Document &` overload, make explicit copy if really meant to do it
inline void send_event(const char *event_name, rapidjson::Document &&data, const char *source_id = nullptr)
{
  send_event_ex(event_name, eastl::move(data), EventParams{source_id, 0});
}

void send_event(const char *, const rapidjson::Document &&, const char * = nullptr) = delete; // Lost `mutable` kw to lambda?

} // namespace sqeventbus
