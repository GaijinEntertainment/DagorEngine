//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <EASTL/memory.h>
#include <sqrat.h>
#include <debug/dag_assert.h>
#include <osApiWrappers/dag_miscApi.h> // is_main_thread

class DataBlock;
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

enum class ProcessingMode : uint8_t
{
  IMMEDIATE,
  MANUAL_PUMP
};

struct EventParams
{
  const char *sourceId = nullptr; // nullptr intepreted as "native"
  int memoryThresholdBytes = 0;   // zero means no value
};

struct Value // Json::Value -> Sqrat::Object adapter
{
  Sqrat::Object obj;

  Value() = default;
  Value(HSQUIRRELVM vm) { obj = Sqrat::Table(vm); }
  Value(HSQUIRRELVM vm, unsigned size) { obj = Sqrat::Array(vm, size); }

  Value &operator=(const Value &) = default;
  Value &operator=(Value &&) = default;
  Value &operator=(const Sqrat::Object &o)
  {
    obj = o;
    return *this;
  }
  Value &operator=(Sqrat::Object &&o)
  {
    obj = eastl::move(o);
    return *this;
  }
  Value &operator=(const Json::Value &) = delete; // Missed `jsoncpp_to_quirrel`?

  HSQUIRRELVM getVM() const { return obj.GetVM(); }
  void setNull() { obj = Sqrat::Object(getVM()); }
  void setObject()
  {
    G_ASSERT(getVM());
    obj = Sqrat::Table(getVM());
  }
  void setArray()
  {
    G_ASSERT(getVM());
    obj = Sqrat::Array(getVM());
  }
  void resize(unsigned sz)
  {
    G_ASSERT(getVM());
    if (obj.GetType() != OT_ARRAY)
      obj = Sqrat::Array(getVM(), sz);
    else
      Sqrat::Array(obj).Resize(sz);
  }

  template <typename T, int t, typename S>
  class SlotRef
  {
    Value &val;
    S slot;

  public:
    SlotRef(Value &v, S s) : val(v), slot(s) { G_ASSERT((int)val.obj.GetType() == t); }
    Value &operator=(const Value &v)
    {
      static_cast<T *>(&val.obj)->SetValue(slot, v.obj);
      return val;
    }
    Value &operator=(const Json::Value &) = delete; // Mismatched send_event/write_event code?
    template <typename TT>
    Value &operator=(const TT &v)
    {
      static_cast<T *>(&val.obj)->SetValue(slot, v);
      return val;
    }
  };

  auto operator[](const char *s) { return SlotRef<Sqrat::Table, int(OT_TABLE), const char *>(*this, s); }
  auto operator[](int i) { return SlotRef<Sqrat::Array, int(OT_ARRAY), int>(*this, i); }
};

void bind(SqModules *module_mgr, const char *vm_id, ProcessingMode mode, bool freeze_wevt_tables = false,
  bool freeze_sq_tables = false);
ProcessingMode bind_ex(SqModules *module_mgr, const char *vm_id, bool freeze_wevt_tables = false, bool immediate = false);
void unbind(HSQUIRRELVM vm);
// If true all sent events converted to json before saving to queue (even in main thread).
// Relevant only for MANUAL_PUMP processing mode.
void set_mtsafe_mode(bool enable);
void clear_on_reload(HSQUIRRELVM vm);
// Note: nullptr `source_id` interpreted as "native"
void send_event(const char *event_name, const char *source_id = nullptr);
void send_event(const char *event_name, const Json::Value &data, const char *source_id = nullptr);
void send_event(const char *event_name, Json::Value &&data, const char *source_id = nullptr);
// Warn: `wcb` can be called 0, 1 or more times depending on existence of listeners
void write_event_main_thread(const char *event_name, const eastl::fixed_function<sizeof(void *) * 8, void(Value &)> &wcb,
  const char *source_id = nullptr);
template <typename F, typename T = Json::Value>
void write_event(const char *event_name, F wcb, const char *source_id = nullptr)
{
  if (is_main_thread())
    write_event_main_thread(event_name, wcb, source_id);
  else
  {
    T data;
    wcb(data);
    send_event(event_name, eastl::move(data), source_id);
  }
}
bool has_listeners(const char *event_name, const char *source_id = nullptr);
void process_events(HSQUIRRELVM vm);
void flush_events(); // process queued events for all VMs

} // namespace sqeventbus
