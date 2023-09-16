//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/internal/typesAndLimits.h>

typedef struct SQVM *HSQUIRRELVM;
class SqModules;

extern void ecs_register_sq_binding(SqModules *module_mgr, bool create_systems, bool create_factories);
extern void shutdown_ecs_sq_script(HSQUIRRELVM vm);
extern void update_ecs_sq_timers(float dt, float rt_dt);
extern void start_es_loading(); // prevents calls to es_reset_order during es registrations
extern void end_es_loading();   // calls es_reset_order

typedef void (*SqPushCB)(HSQUIRRELVM vm, const void *raw_data);
extern void sq_register_native_component(ecs::component_type_t type, SqPushCB cb);

#define ECS_REGISTER_SQ_COMPONENT(T)              \
  sq_register_native_component(ECS_HASH(#T).hash, \
    [](HSQUIRRELVM vm, const void *p) { Sqrat::PushVar(vm, ecs::ComponentTypeInfo<T>::is_boxed ? *(const T **)p : (const T *)p); })

ECS_BROADCAST_EVENT_TYPE(EventScriptReloaded);
ECS_BROADCAST_EVENT_TYPE(CmdRequireUIScripts);

class WinCritSec;

extern void sqvm_enter_critical_section(HSQUIRRELVM vm, WinCritSec *crit_sect);
extern void sqvm_leave_critical_section(HSQUIRRELVM vm);

struct VMScopedCriticalSection
{
  HSQUIRRELVM vm;
  VMScopedCriticalSection(HSQUIRRELVM set_vm, WinCritSec *crit_sect) : vm(set_vm) { sqvm_enter_critical_section(vm, crit_sect); }
  ~VMScopedCriticalSection() { sqvm_leave_critical_section(vm); }
};
