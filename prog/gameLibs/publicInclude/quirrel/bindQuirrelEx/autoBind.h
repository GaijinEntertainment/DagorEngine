//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>
#include <stdint.h>

class SqModules;

namespace sq
{

enum VmType
{
  VM_GAME = 0x01,
  VM_INTERNAL_UI = 0x02,
  VM_USER_UI = 0x04,
  VM_UI_ALL = VM_INTERNAL_UI | VM_USER_UI,

  VM_ALL = 0xFFFF,
  VM_FLG_REGFUNC = 0x10000,
};

typedef Sqrat::Table (*binding_module_cb_t)(HSQUIRRELVM vm); // Shall return Table that will be registered as native module (with name
                                                             // speicifed on registration)
typedef void (*binding_regfunc_cb_t)(SqModules *module_mgr);

class BindingRegRec // record of auto-binding registry
{
  union
  {
    binding_module_cb_t bindModuleCb;
    binding_regfunc_cb_t bindRegfuncCb;
  };
  const uint32_t vmt;
  BindingRegRec *next;
  const char *moduleName = nullptr;
  friend void auto_bind_native_api(SqModules *, uint32_t);

public:
  BindingRegRec(binding_module_cb_t cb, uint32_t vmt_, const char *module_name);
  BindingRegRec(binding_regfunc_cb_t cb, uint32_t vmt_);
};

// Actually perform binding of native API
void auto_bind_native_api(SqModules *modules_mgr, uint32_t vm_mask);

#define SQ_DEF_AUTO_BINDING_MODULE_EX(Func, module_name, vmt_)                   \
  static Sqrat::Table Func(HSQUIRRELVM);                                         \
  static sq::BindingRegRec Func##_auto_bind_var(&Func, vmt_, module_name);       \
  extern const size_t sq_autobind_pull_##Func = (size_t)(&Func##_auto_bind_var); \
  static Sqrat::Table Func(HSQUIRRELVM vm)

#define SQ_DEF_AUTO_BINDING_MODULE(Func, module_name) SQ_DEF_AUTO_BINDING_MODULE_EX(Func, module_name, sq::VM_ALL)

#define SQ_DEF_AUTO_BINDING_REGFUNC_EX(Func, vmt_)            \
  static sq::BindingRegRec Func##_auto_bind_var(&Func, vmt_); \
  extern const size_t sq_autobind_pull_##Func = (size_t)(&Func##_auto_bind_var);

#define SQ_DEF_AUTO_BINDING_REGFUNC(Func) SQ_DEF_AUTO_BINDING_REGFUNC_EX(Func, sq::VM_ALL)

}; // namespace sq
