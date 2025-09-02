// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/autoCleanup.h>
#include <sqModules/sqModules.h>


namespace ui
{

template <class S, unsigned MAX_VM = 2>
struct SqPrecachedStringsPerVM
{
  const S *resolveVm(HSQUIRRELVM vm) const
  {
    for (auto &rvm : regVm)
      if (rvm == vm)
        return &regObj[&rvm - regVm];
    return nullptr;
  }

  S *registerVm(HSQUIRRELVM vm)
  {
    if (auto s = resolveVm(vm))
      return const_cast<S *>(s);
    for (auto &rvm : regVm)
      if (!rvm)
      {
        rvm = vm;
        return &regObj[&rvm - regVm];
      }
    return nullptr;
  }
  S *unregisterVm(HSQUIRRELVM vm)
  {
    for (auto &rvm : regVm)
      if (rvm == vm)
      {
        rvm = nullptr;
        return &regObj[&rvm - regVm];
      }
    return nullptr;
  }

protected:
  HSQUIRRELVM regVm[MAX_VM] = {nullptr};
  S regObj[MAX_VM];
};

void preregister_strings(HSQUIRRELVM vm, Sqrat::Object *strings, unsigned strings_count, const char *comma_sep_string_names);

} // namespace ui

#define SQ_PRECACHED_STRINGS_DECLARE_EX(CLS, NM, STORAGE, ...)         \
  struct CLS                                                           \
  {                                                                    \
    Sqrat::Object __VA_ARGS__;                                         \
  };                                                                   \
  STORAGE ui::SqPrecachedStringsPerVM<CLS> NM;                         \
  STORAGE void NM##_reg_vm(SqModules *module_mgr)                      \
  {                                                                    \
    HSQUIRRELVM vm = module_mgr->getVM();                              \
    auto strings = NM.registerVm(vm);                                  \
    G_ASSERT_RETURN(strings, );                                        \
    auto str_count = sizeof(CLS) / sizeof(Sqrat::Object);              \
    auto str_objects = (Sqrat::Object *)(void *)strings;               \
    ui::preregister_strings(vm, str_objects, str_count, #__VA_ARGS__); \
  }                                                                    \
  STORAGE void NM##_unreg_vm(HSQUIRRELVM vm)                           \
  {                                                                    \
    if (auto strings = NM.unregisterVm(vm))                            \
      *strings = {};                                                   \
  }

#define SQ_PRECACHED_STRINGS_REGISTER(BHV_NM, PREFIX, NM)                             \
  static void strings_reg_vm_##BHV_NM(SqModules *m) { PREFIX NM##_reg_vm(m); }        \
  SQ_DEF_AUTO_BINDING_REGFUNC_EX(strings_reg_vm_##BHV_NM, sq::VM_UI_ALL);             \
  static void strings_unreg_vm_##BHV_NM(HSQUIRRELVM vm) { PREFIX NM##_unreg_vm(vm); } \
  SQ_DEF_AUTO_CLEANUP_UNREGFUNC(strings_unreg_vm_##BHV_NM)


// declare precached strings map as member of class (need to use SQ_PRECACHED_STRINGS_REGISTER in .CPP)
#define SQ_PRECACHED_STRINGS_DECLARE(CLS, NM, ...)        \
  SQ_PRECACHED_STRINGS_DECLARE_EX(CLS, NM, , __VA_ARGS__) \
  typedef ui::SqPrecachedStringsPerVM<CLS> CLS##Map;

// declare precached strings map as global static and register it
#define SQ_PRECACHED_STRINGS_IMPLEMENT(BHV_NM, ...)                              \
  SQ_PRECACHED_STRINGS_DECLARE_EX(PCS_##BHV_NM, ui_strings, static, __VA_ARGS__) \
  SQ_PRECACHED_STRINGS_REGISTER(BHV_NM, , ui_strings)

// define global behavior instance and register precached strings (that are member of bhv)
#define SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BHV_CLS, BHV_NM, CSTR_NM) \
  BHV_CLS BHV_NM;                                                        \
  SQ_PRECACHED_STRINGS_REGISTER(BHV_CLS, BHV_NM., CSTR_NM)
