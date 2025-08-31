// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_stcode.h>
#include <shaders/dag_shaders.h>
#include <shaders/stcode/scalarTypes.h>
#include <shaders/stcode/callbackTable.h>
#include <shaders/shInternalTypes.h>
#include <shaders/cppStcodeVer.h>

#include "stcode/stcodeCallbacksImpl.h"
#include "stcode/compareStcode.h"
#include "profileStcode.h"
#include "shadersBinaryData.h"

#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_assert.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <dag/dag_vector.h>
#include <util/dag_finally.h>
#include <osApiWrappers/dag_direct.h>

namespace stcode
{

using DynRoutinePtr = void (*)(const void *, uint32_t);
using StRoutinePtr = void (*)(void *);

static struct Context
{
  void *dllHandle = nullptr;

  dag::ConstSpan<DynRoutinePtr> dynRoutineStorageView{};
  dag::ConstSpan<StRoutinePtr> stRoutineStorageView{};
  ExecutionMode exMode = ExecutionMode::CPP;

  SpecialBlkTagInterpreter specialTagInterpreter = [](auto) { return eastl::nullopt; };

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  eastl::string tmpFileToDelete{};
#endif
} g_ctx;

ExecutionMode execution_mode() { return g_ctx.exMode; }

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
static bool file_cpy(const char *fn_to, const char *fn_from)
{
  G_ASSERT(fn_to);
  G_ASSERT(fn_from);

  file_ptr_t from = df_open(fn_from, DF_READ | DF_IGNORE_MISSING);
  if (!from)
    return false;
  FINALLY([from] { df_close(from); });

  file_ptr_t to = df_open(fn_to, DF_WRITE | DF_CREATE);
  if (!to)
    return false;
  FINALLY([to] { df_close(to); });

  size_t bytes = df_length(from);
  Tab<uint8_t> content(bytes);

  size_t readBytes = df_read(from, content.data(), content.size());
  if (readBytes != bytes)
    return false;

  size_t wroteBytes = df_write(to, content.data(), content.size());

  return wroteBytes == bytes;
}
#endif

static const char *get_dll_config_suffix(const DataBlock &settings)
{
  auto modeFromSetting = settings.getStr("config", nullptr);
  if (modeFromSetting)
  {
    if (strcmp(modeFromSetting, "dev") == 0)
      return "-dev";
    else if (strcmp(modeFromSetting, "rel") == 0)
      return "";
    else if (strcmp(modeFromSetting, "dbg") == 0)
      return "-dbg";
    else
    {
      logerr("Invalid stcode/config:t value (%s) in settings.blk", modeFromSetting);
      return nullptr;
    }
  }

#if DAGOR_DBGLEVEL > 1
  return "-dbg";
#elif DAGOR_DBGLEVEL > 0
  return "-dev";
#else
  return "";
#endif
}

static eastl::string get_dll_tag_suffix(const DataBlock &settings)
{
  auto tag = settings.getStr("tag", nullptr);
  if (tag)
  {
    if (tag[0] == '$')
    {
      auto maybeSpecialTag = g_ctx.specialTagInterpreter({&tag[1]});
      if (!maybeSpecialTag)
      {
        logerr("Unknown special cppstcode tag directive '%s' (specified in stcode/tag:t)", tag);
        return "";
      }

      return eastl::string{"-"} + (*maybeSpecialTag);
    }
    else
    {
      return eastl::string{"-"} + tag;
    }
  }
  else
  {
    return "";
  }
}

enum class FallbackLevel
{
  EXACT,
  NOTAG,
  REL_NOTAG,
  DEV_NOTAG,
};

static constexpr const char *FALLBACK_CONFIG_SUFFICES[] = {nullptr, nullptr, "", "-dev"};
static constexpr const char *FALLBACK_DEBUG_TEXTS[] = {nullptr, "no tag", "rel(no tag)", "dev(no tag)"};

// @NOTE: not using get_host_arch_string from miscApi here, because when running non-native exe (win32 on win64, x86_64 on arm, etc),
// we want the dll for the same arch as the exe, not as the actual processor (otherwise, we'll get ABI breaks, and so on)
static const char *get_exe_arch_string()
{
#if _TARGET_PC_LINUX
#if __e2k__
  return "e2k";
#elif defined(__x86_64__) || defined(_M_X64)
  return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
  return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
  return "arm";
#elif defined(__i386__) || defined(_M_IX86)
  return "x86";
#endif
#else
#if _TARGET_64BIT && _TARGET_SIMD_SSE
  return "x86_64";
#elif _TARGET_64BIT && _TARGET_SIMD_NEON
  return "arm64";
#elif !_TARGET_64BIT && _TARGET_SIMD_SSE
  return "x86";
#elif !_TARGET_64BIT && _TARGET_SIMD_NEON
  return "armv7";
#endif
#endif
  return "?";
}

static eastl::string build_dll_suffix(const DataBlock &settings, FallbackLevel fallback = FallbackLevel::NOTAG)
{
  const char *configSuffix = (fallback == FallbackLevel::EXACT || fallback == FallbackLevel::NOTAG)
                               ? get_dll_config_suffix(settings)
                               : FALLBACK_CONFIG_SUFFICES[eastl::to_underlying(fallback)];
  const auto tagSuffix = fallback == FallbackLevel::EXACT ? get_dll_tag_suffix(settings) : "";
#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX | _TARGET_C3
  return tagSuffix + eastl::string{configSuffix};
#else
  const eastl::string res{
    eastl::string::CtorSprintf{}, "-%s-%s%s%s", get_host_platform_string(), get_exe_arch_string(), tagSuffix.c_str(), configSuffix};
  G_ASSERT(!strchr(res.c_str(), '?'));
  return res;
#endif
}

static bool try_load_dll(const auto &path_base, const char *suffix, auto &out_dll_path)
{
  out_dll_path =
    eastl::string{eastl::string::CtorSprintf{}, "%.*s-stcode%s" DAGOR_OS_DLL_SUFFIX, path_base.length(), path_base.data(), suffix};

// If in development mode, copy to a tmp file and load that. This way, we don't lock up the file and can recompile it as the game is
// running. @TODO: check if this will work on PS & Xbox, or is actually needed there when running games mounted.
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  {
    eastl::string dllTmpPath(eastl::string::CtorSprintf{}, "%.*s-stcode%s.tmp" DAGOR_OS_DLL_SUFFIX, path_base.length(),
      path_base.data(), suffix);
    if (dd_file_exists(g_ctx.tmpFileToDelete.c_str()))
      dd_erase(g_ctx.tmpFileToDelete.c_str());
    if (dd_file_exists(dllTmpPath.c_str()))
      dd_erase(dllTmpPath.c_str());
    if (!file_cpy(dllTmpPath.c_str(), out_dll_path.c_str()))
    {
      debug("[SH] Failed to create temporary dynlib copy of '%s', either dll is missing, or file write failed", out_dll_path.c_str());
      return false;
    }
    g_ctx.tmpFileToDelete = dllTmpPath;
    out_dll_path = eastl::move(dllTmpPath);
  }
#endif

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_ANDROID | _TARGET_IOS
  auto it = out_dll_path.rfind('/');
  if (it != eastl::string::npos)
    out_dll_path.erase(0, it + 1);
  g_ctx.dllHandle = os_dll_load(out_dll_path.c_str());
#else
  g_ctx.dllHandle = os_dll_load(out_dll_path.c_str());
#endif

  if (!g_ctx.dllHandle)
  {
    debug("[SH] Dynlib could not be loaded from '%s' via os_dll_load, file is missing or corrupted", out_dll_path.c_str());
    return false;
  }

  return true;
}

static void unload_dll()
{
  os_dll_close(g_ctx.dllHandle);
  g_ctx.dllHandle = nullptr;
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  if (!g_ctx.tmpFileToDelete.empty())
    dd_erase(g_ctx.tmpFileToDelete.c_str());
#endif
  debug("[SH] Unloaded stcode dll");
}

static bool init_dll(const DataBlock &settings);

static bool try_load_dll_with_fallbacks(const auto &path_base, std::initializer_list<FallbackLevel> fallback_levels,
  const DataBlock &settings, auto &out_dll_path)
{
  eastl::string baseSuffix{};
  for (FallbackLevel level : fallback_levels)
  {
    eastl::string suffix = build_dll_suffix(settings, level);
    if (baseSuffix.empty() || suffix != baseSuffix)
    {
      if (!baseSuffix.empty())
      {
        debug("[SH] Failed to load stcode dll from '%s', trying %s version", out_dll_path.c_str(),
          FALLBACK_DEBUG_TEXTS[size_t(level)]);
      }
      if (try_load_dll(path_base, suffix.c_str(), out_dll_path))
      {
        if (!init_dll(settings))
          unload_dll();
        else
          return true;
      }
      if (baseSuffix.empty())
        baseSuffix = eastl::move(suffix);
    }
  }
  return false;
}

template <typename TFptr, typename... TArgs>
static auto fetch_and_run_dll_func(const char *func_name,
  TArgs &&...args) -> eastl::optional<decltype((*eastl::declval<TFptr>())(eastl::forward<TArgs>(args)...))>
{
  G_ASSERT(is_loaded());

  TFptr routine = (TFptr)os_dll_get_symbol(g_ctx.dllHandle, func_name);
  if (!routine)
    return eastl::nullopt;

  return (*routine)(eastl::forward<TArgs>(args)...);
}

#if STCODE_RUNTIME_CHOICE

#define FAIL_EXIT(fmt_, ...)                                 \
  do                                                         \
  {                                                          \
    debug(fmt_ ", falling back to bytecode", ##__VA_ARGS__); \
    g_ctx.exMode = ExecutionMode::BYTECODE;                  \
    return true;                                             \
  } while (0)

#else

#define FAIL_EXIT(fmt_, ...)     \
  do                             \
  {                              \
    logerr(fmt_, ##__VA_ARGS__); \
    return false;                \
  } while (0)

#endif

bool load(const char *shbindump_path_base)
{
  if (!shBinDumpOwner().getDumpV3())
    FAIL_EXIT("[SH] Failed to fetch external stcode data from bindump -- dump V3 header has incompatible format");

  const auto extractNameBase = [](const char *path) {
    const char *suffix = strstr(path, ".shdump.bin");
    return suffix ? eastl::string_view(path, suffix - path) : eastl::string_view(path);
  };

  const DataBlock &settings = *::dgs_get_settings()->getBlockByNameEx("stcode");
  const eastl::string_view pathBase = extractNameBase(shbindump_path_base);

#if STCODE_RUNTIME_CHOICE
  if (settings.getBool("useBytecode", false))
    FAIL_EXIT("[SH] useBytecode:b=yes is set in settings");
#endif

  eastl::string dllPath;

  if (!try_load_dll_with_fallbacks(pathBase,
        {FallbackLevel::EXACT, FallbackLevel::NOTAG, FallbackLevel::REL_NOTAG, FallbackLevel::DEV_NOTAG}, settings, dllPath))
  {
    FAIL_EXIT("[SH] Failed to load stcode dll from '%s'", dllPath.c_str());
  }

  debug("[SH] Successfully loaded stcode dll from '%s'", dllPath.c_str());

  return true;
}

#undef FAIL_EXIT

void unload()
{
  if (is_loaded())
  {
    g_ctx.dynRoutineStorageView = {};
    g_ctx.stRoutineStorageView = {};
    unload_dll();
  }
}

bool is_loaded() { return g_ctx.dllHandle != nullptr; }

void run_dyn_routine(size_t id, const void *vars, uint32_t dyn_offset, int req_tex_level, const char *new_name)
{
  // Prepare data for resource setters
  cpp::last_resource_name = new_name;
  cpp::req_tex_level = req_tex_level;

  DYNSTCODE_PROFILE_BEGIN();
  {
    g_ctx.dynRoutineStorageView[id](vars, dyn_offset);
  }
  DYNSTCODE_PROFILE_END();
}

void run_st_routine(size_t id, void *context)
{
  STBLKCODE_PROFILE_BEGIN();
  {
    g_ctx.stRoutineStorageView[id](context);
  }
  STBLKCODE_PROFILE_END();
}

void set_on_before_resource_used_cb(OnBeforeResourceUsedCb new_cb) { cpp::on_before_resource_used_cb = new_cb; }

static auto make_hash_log_string(dag::ConstSpan<uint8_t> hash)
{
  eastl::string res{};
  for (uint8_t byte : hash)
    res.append_sprintf("%02x", byte);
  return res;
}

static bool init_dll(const DataBlock &settings)
{
#if VALIDATE_CPP_STCODE
  const auto *stcodeDbgBlk = settings.getBlockByNameEx("debug");
  if (stcodeDbgBlk->getBool("validate", false))
  {
    g_ctx.exMode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;
    if (stcodeDbgBlk->getBool("requireStrictNumberEquality", false))
      dbg::require_exact_record_matches();
  }
#endif

  const auto *dump = shBinDumpOwner().getDumpV3();
  G_ASSERT(dump); // This is checked in stcode::load and should be non-null if this code is reached

  if (dump->externalStcodeVersion != CPP_STCODE_COMMON_VER)
  {
    debug("[SH] Stcode version from bindump (=%u) does not match the version from exe (=%u)", dump->externalStcodeVersion,
      CPP_STCODE_COMMON_VER);
    return false;
  }

  if (USE_BRANCHED_DYNAMIC_ROUTINES && dump->externalStcodeMode != shader_layout::ExternalStcodeMode::BRANCHED_CPP)
  {
    debug("[SH] loaded bindump does not support branched cpp stcode");
    return false;
  }
  else if (!USE_BRANCHED_DYNAMIC_ROUTINES && dump->externalStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
  {
    debug("[SH] loaded bindump does not support branchless cpp stcode");
    return false;
  }

  const auto verFromDllMaybe = fetch_and_run_dll_func<uint32_t (*)()>("get_version");
  if (!verFromDllMaybe.has_value())
  {
    debug("[SH] Failed to fetch version from stcode dll");
    return false;
  }
  else if (verFromDllMaybe.value() != dump->externalStcodeVersion)
  {
    debug("[SH] Stcode version from dll (=%u) does not match the version from bindump/exe (=%u)", verFromDllMaybe.value(),
      dump->externalStcodeVersion);
    return false;
  }
  else
    debug("[SH] Stcode dll version is %u, match", verFromDllMaybe.value());

  const auto hashFromDllMaybe = fetch_and_run_dll_func<const uint8_t *(*)()>("get_hash");
  if (!hashFromDllMaybe.has_value())
  {
    debug("[SH] Failed to fetch hash from stcode dll");
    return false;
  }
  else if (memcmp(hashFromDllMaybe.value(), dump->externalStcodeHash, sizeof(dump->externalStcodeHash)) != 0)
  {
    debug("[SH] Stcode hash from dll %s does not match the hash from bindump %s",
      make_hash_log_string({hashFromDllMaybe.value(), sizeof(dump->externalStcodeHash)}).c_str(),
      make_hash_log_string({dump->externalStcodeHash, sizeof(dump->externalStcodeHash)}).c_str());
    return false;
  }
  else
  {
    debug("[SH] Stcode dll contains hash %s, match",
      make_hash_log_string({hashFromDllMaybe.value(), sizeof(dump->externalStcodeHash)}).c_str());
  }

  void *dynRoutinesStart = nullptr, *stRoutinesStart = nullptr;
  uint32_t dynRoutinesCount = 0, stRoutinesCount = 0;

  const auto resMaybe = fetch_and_run_dll_func<bool (*)(void *, void **, uint32_t *, void **, uint32_t *)>("init_dll_and_get_routines",
    (void *)&cpp::cb_table_impl, &dynRoutinesStart, &dynRoutinesCount, &stRoutinesStart, &stRoutinesCount);

  if (!resMaybe.has_value() || !resMaybe.value())
  {
    debug("[SH] Failed to initialize stcode dll");
    return false;
  }

  g_ctx.dynRoutineStorageView = {(DynRoutinePtr *)dynRoutinesStart, intptr_t(dynRoutinesCount)};
  g_ctx.stRoutineStorageView = {(StRoutinePtr *)stRoutinesStart, intptr_t(stRoutinesCount)};

  return true;
}

void set_special_tag_interp(SpecialBlkTagInterpreter &&interp) { g_ctx.specialTagInterpreter = eastl::move(interp); }

void set_custom_const_setter(shaders_internal::ConstSetter *setter) { cpp::custom_const_setter = setter; }

ScopedCustomConstSetter::ScopedCustomConstSetter(shaders_internal::ConstSetter *setter) : prev(cpp::custom_const_setter)
{
  cpp::custom_const_setter = setter;
}

ScopedCustomConstSetter::~ScopedCustomConstSetter() { cpp::custom_const_setter = prev; }

} // namespace stcode
