//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>

namespace Json
{
class Value;
}

namespace Sqrat
{
class Object;
}

class SqModules;

namespace sqeventbus
{

using NativeEventHandler = void (*)(const char *event_name, const Json::Value &data, const char *source_id, bool immediate);

enum class ProcessingMode
{
  IMMEDIATE,
  MANUAL_PUMP
};

void bind(SqModules *module_mgr, const char *vm_id, ProcessingMode mode);
void unbind(HSQUIRRELVM vm);
void clear_on_reload(HSQUIRRELVM vm);
void send_event(const char *event_name, const Sqrat::Object &data, const char *source_id = "native");
void send_event(const char *event_name, const Json::Value &data, const char *source_id = "native");
void send_immediate_event(const char *event_name, const Json::Value &data, const char *source_id = "native");
bool has_listeners(const char *event_name, const char *source_id = "native");
void process_events(HSQUIRRELVM vm);
HSQUIRRELVM get_vm();

void set_native_event_handler(NativeEventHandler handler);

} // namespace sqeventbus
