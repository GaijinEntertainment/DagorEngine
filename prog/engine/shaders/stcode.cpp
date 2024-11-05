// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_stcode.h>
#include <shaders/dag_shaders.h>
#include <shaders/stcode/scalarTypes.h>
#include <shaders/stcode/callbackTable.h>
#include <shaders/shInternalTypes.h>

#include "stcode/stcodeCallbacksImpl.h"
#include "profileStcode.h"
#include "shadersBinaryData.h"

#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_assert.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <dag/dag_vector.h>

namespace stcode
{

static ExecutionMode ex_mode = ExecutionMode::CPP;
ExecutionMode execution_mode() { return ex_mode; }

using RoutinePtr = void (*)(const void *, bool);

static dag::ConstSpan<RoutinePtr> routine_storage_ref{};

static void *dll_handle = nullptr;

static bool load_dll(const char *shbindump_path_base)
{
  auto extract_name_base = [](const char *path) {
    const char *suffix = strstr(path, ".shdump.bin");
    return suffix ? eastl::string_view(path, suffix - path) : eastl::string_view(path);
  };

  // @TODO: fetch config for dev/rel
  eastl::string_view pathBase = extract_name_base(shbindump_path_base);
  eastl::string dllPath(eastl::string::CtorSprintf{}, "%.*s-stcode-dev" DAGOR_OS_DLL_SUFFIX, pathBase.length(), pathBase.data());

#if _TARGET_C1 | _TARGET_C2

#else
  dll_handle = os_dll_load(dllPath.c_str());
#endif

  if (!dll_handle)
  {
    logerr("Failed to load stcode dll from %s", dllPath.c_str());
    return false;
  }

  return true;
}

static void unload_dll()
{
  os_dll_close(dll_handle);
  dll_handle = nullptr;
}

template <typename TFptr, typename... TArgs>
static auto fetch_and_run_dll_func(const char *func_name, TArgs &&...args)
  -> decltype((*eastl::declval<TFptr>())(eastl::forward<TArgs>(args)...))
{
  G_ASSERT(is_loaded());

  TFptr routine = (TFptr)os_dll_get_symbol(dll_handle, func_name);
  // @TODO: tighter validation for runtime
  G_VERIFY(routine);

  return (*routine)(eastl::forward<TArgs>(args)...);
}

// For use in INVOKE macro to have fptr type
using init_dll_and_get_routines_t = const RoutinePtr *(*)(void *);

#define INVOKE_STCODE_FUNC(funcname, ...) fetch_and_run_dll_func<funcname##_t>(#funcname, ##__VA_ARGS__)

static void init();
bool load(const char *shbindump_path_base)
{
  if (!load_dll(shbindump_path_base))
    return false;

  init();
  return true;
}

void unload()
{
  if (is_loaded())
  {
    routine_storage_ref = {};
    unload_dll();
  }
}

bool is_loaded() { return dll_handle != nullptr; }

void run_routine(size_t id, const void *vars, bool is_compute, int req_tex_level, const char *new_name)
{
  // Prepare data for resource setters
  cpp::last_resource_name = new_name;
  cpp::req_tex_level = req_tex_level;

  STCODE_PROFILE_BEGIN();
  {
    routine_storage_ref[id](vars, is_compute);
  }
  STCODE_PROFILE_END();
}

void set_on_before_resource_used_cb(OnBeforeResourceUsedCb new_cb) { cpp::on_before_resource_used_cb = new_cb; }

void init()
{
  if (::dgs_get_settings()->getBlockByNameEx("stcode")->getBlockByNameEx("debug")->getBool("validate", false))
    ex_mode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;

  const RoutinePtr *routineArray = INVOKE_STCODE_FUNC(init_dll_and_get_routines, (void *)&cpp::cb_table_impl);
  G_VERIFYF(routineArray, "Failed to init stcode dll");

  routine_storage_ref = {routineArray, (intptr_t)shBinDump().stcode.size()};
}

void set_custom_const_setter(shaders_internal::ConstSetter *setter) { cpp::custom_const_setter = setter; }

ScopedCustomConstSetter::ScopedCustomConstSetter(shaders_internal::ConstSetter *setter) : prev(cpp::custom_const_setter)
{
  cpp::custom_const_setter = setter;
}

ScopedCustomConstSetter::~ScopedCustomConstSetter() { cpp::custom_const_setter = prev; }

} // namespace stcode
