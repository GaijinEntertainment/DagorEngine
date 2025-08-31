//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>

typedef struct SQVM *HSQUIRRELVM;

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

enum class ProcessingMode
{
  IMMEDIATE,
  MANUAL_PUMP
};

struct EventParams
{
  const char *sourceId = "native";
  int memoryThresholdBytes = 0; // zero means no value
};

void bind(SqModules *module_mgr, const char *vm_id, ProcessingMode mode);
void unbind(HSQUIRRELVM vm);
void clear_on_reload(HSQUIRRELVM vm);
void send_event(const char *event_name, const char *source_id = "native");
void send_event(const char *event_name, const Sqrat::Object &data, const char *source_id = "native");
void send_event(const char *event_name, const Json::Value &data, const char *source_id = "native");
bool has_listeners(const char *event_name, const char *source_id = "native");
void process_events(HSQUIRRELVM vm);
void do_with_vm(const eastl::function<void(HSQUIRRELVM)> &callback);

} // namespace sqeventbus
