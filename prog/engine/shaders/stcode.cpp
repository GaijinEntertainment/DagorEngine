// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_stcode.h>
#include <shaders/dag_shaders.h>
#include <shaders/stcode/scalarTypes.h>
#include <shaders/stcode/callbackTable.h>
#include <shaders/shInternalTypes.h>
#include <shaders/stcodeHash.h>

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

#elif _TARGET_ANDROID | _TARGET_IOS
  auto it = dllPath.rfind('/');
  if (it != eastl::string::npos)
    dllPath.erase(0, it + 1);
  dll_handle = os_dll_load(dllPath.c_str());
#else
  dll_handle = os_dll_load(dllPath.c_str());
#endif

  if (!dll_handle)
  {
#if STCODE_RUNTIME_CHOICE
    debug("[SH] Failed to load stcode dll from '%s', falling back to bytecode", dllPath.c_str());
#else
    logerr("[SH] Failed to load stcode dll from '%s'", dllPath.c_str());
#endif
    return false;
  }

  debug("[SH] Successfully loaded stcode dll from '%s'", dllPath.c_str());
  return true;
}

static void unload_dll()
{
  os_dll_close(dll_handle);
  dll_handle = nullptr;
  debug("[SH] Unloaded stcode dll");
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

static bool init();
bool load(const char *shbindump_path_base)
{
#if STCODE_RUNTIME_CHOICE
  if (::dgs_get_settings()->getBlockByNameEx("stcode")->getBool("useBytecode", false))
  {
    ex_mode = ExecutionMode::BYTECODE;
    return true;
  }
#endif

  if (!load_dll(shbindump_path_base) || !init())
  {
#if STCODE_RUNTIME_CHOICE
    ex_mode = ExecutionMode::BYTECODE;
    return true;
#else
    return false;
#endif
  }

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

static bool init()
{
#if VALIDATE_CPP_STCODE
  if (::dgs_get_settings()->getBlockByNameEx("stcode")->getBlockByNameEx("debug")->getBool("validate", false))
    ex_mode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;
#endif

  // @TODO: replace live calculation with bindump field (once perf is needed enough to break dump format)
  // @TODO: for "enable" mode replace this hash with stored-in-bindump hash of cpp sources, so that it does not require bytecode.
  uint64_t stcodeHash = calc_stcode_hash(shBinDump().stcode);

  const RoutinePtr *routineArray = fetch_and_run_dll_func<const RoutinePtr *(*)(void *, uint64_t)>("init_dll_and_get_routines",
    (void *)&cpp::cb_table_impl, stcodeHash);
  if (!routineArray)
  {
#if STCODE_RUNTIME_CHOICE
    debug("[SH] Stcode hash from dll (see sources) does not match the has from bindump (=%lu), falling back to bytecode", stcodeHash);
#else
    logerr("[SH] Stcode hash from dll (see sources) does not match the hash from bindump (=%lu)", stcodeHash);
#endif
    return false;
  }

  routine_storage_ref = {routineArray, (intptr_t)shBinDump().stcode.size()};
  return true;
}

void set_custom_const_setter(shaders_internal::ConstSetter *setter) { cpp::custom_const_setter = setter; }

ScopedCustomConstSetter::ScopedCustomConstSetter(shaders_internal::ConstSetter *setter) : prev(cpp::custom_const_setter)
{
  cpp::custom_const_setter = setter;
}

ScopedCustomConstSetter::~ScopedCustomConstSetter() { cpp::custom_const_setter = prev; }

} // namespace stcode
