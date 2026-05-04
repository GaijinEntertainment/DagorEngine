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
#include <dag/dag_vectorSet.h>
#include <util/dag_finally.h>
#include <osApiWrappers/dag_direct.h>

namespace stcode
{

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
static bool file_cpy(const char *fn_to, const char *fn_from, auto &&cb)
{
  G_ASSERT(fn_to);
  G_ASSERT(fn_from);

  file_ptr_t from = df_open(fn_from, DF_READ | DF_IGNORE_MISSING);
  if (!from)
    return false;
  FINALLY([from] { df_close(from); });

  file_ptr_t to = df_open(fn_to, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING);
  if (!to)
    return false;
  FINALLY([to] { df_close(to); });

  size_t bytes = df_length(from);
  Tab<uint8_t> content(bytes);

  size_t readBytes = df_read(from, content.data(), content.size());
  if (readBytes != bytes)
    return false;

  cb(content);

  size_t wroteBytes = df_write(to, content.data(), content.size());

  return wroteBytes == content.size();
}

static bool file_cpy(const char *fn_to, const char *fn_from)
{
  return file_cpy(fn_to, fn_from, [](auto &) {});
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

static eastl::string get_dll_tag_suffix(Context &ctx, const DataBlock &settings)
{
  auto tag = settings.getStr("tag", nullptr);
  if (tag)
  {
    if (tag[0] == '$')
    {
      auto maybeSpecialTag = ctx.specialTagInterpreter({&tag[1]});
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
  REL,
  UNIVERSAL_BINARY,
  UNIVERSAL_BINARY_REL,
  NOTAG,
  REL_NOTAG,
  DEV_NOTAG,
};

static constexpr const char *FALLBACK_CONFIG_SUFFICES[] = {nullptr, "", nullptr, "", nullptr, "", "-dev"};
static constexpr const char *FALLBACK_DEBUG_TEXTS[] = {
  nullptr, "rel", "universal binary", "universal binary rel", "no tag", "rel(no tag)", "dev(no tag)"};

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

static eastl::string build_dll_suffix(Context &ctx, const DataBlock &settings, FallbackLevel fallback = FallbackLevel::NOTAG)
{
  bool autoConfig =
    fallback == FallbackLevel::EXACT || fallback == FallbackLevel::UNIVERSAL_BINARY || fallback == FallbackLevel::NOTAG;
  const char *configSuffix = autoConfig ? get_dll_config_suffix(settings) : FALLBACK_CONFIG_SUFFICES[eastl::to_underlying(fallback)];
  const auto tagSuffix = autoConfig ? get_dll_tag_suffix(ctx, settings) : "";
#if _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX | _TARGET_C3
  return tagSuffix + eastl::string{configSuffix};
#else
  const char *archSuffix = (fallback == FallbackLevel::UNIVERSAL_BINARY || fallback == FallbackLevel::UNIVERSAL_BINARY_REL)
                             ? "universal"
                             : get_exe_arch_string();
  const eastl::string res{
    eastl::string::CtorSprintf{}, "-%s-%s%s%s", get_host_platform_string(), archSuffix, tagSuffix.c_str(), configSuffix};
  G_ASSERT(!strchr(res.c_str(), '?'));
  return res;
#endif
}

static eastl::string make_tmp_file_name(const auto &main_file_path, uint16_t gen)
{
  eastl::string tmpPath{main_file_path};
  // @NOTE: we have to replace chars instead of extending the name for the following reason:
  // * For hot reaload on windows we need to copy over both dll and pdb to temp files.
  // * However, in the new file all references still point to the old pbd.
  // * We solve it by find-and-replacing the names in both files on copying to the new one
  // * However, if we extend/contract the strings, the files' layout and possibly referential integrity breaks
  // * But if we overwrite in place preserving offsets and lenghths, all works fine
  // *
  // * We also need to make a generations of tmp files, because windows debuggers don't release pdbs much
  const eastl::string replHeader{eastl::string::CtorSprintf{}, "tm%04X", gen};
  constexpr char PART_TO_REPLACE[] = "stcode";
  G_ASSERT(replHeader.length() == countof(PART_TO_REPLACE) - 1);
  char *dst = strstr(tmpPath.data(), PART_TO_REPLACE);
  if (!dst)
  {
    G_ASSERTF(0, "Invalid stcode dl path '%s' -- must contain 'stcode'!", tmpPath.c_str());
    return {};
  }
  strncpy(dst, replHeader.c_str(), replHeader.length());
  return tmpPath;
}

static void clean_tmp_files_from_first_generations(std::initializer_list<const eastl::string *> base_names, uint32_t gen_count)
{
  G_ASSERT(gen_count <= UINT16_MAX);
  for (uint32_t gen = 0; gen < gen_count; ++gen)
    for (const auto *name : base_names)
    {
      const auto fn = make_tmp_file_name(*name, uint16_t(gen));
      if (dd_file_exists(fn.c_str()))
        dd_erase(fn.c_str());
    }
}

static bool try_load_dll(Context &ctx, const auto &path_base, const char *suffix, auto &out_dll_path)
{
  out_dll_path =
    eastl::string{eastl::string::CtorSprintf{}, "%.*s-stcode%s" DAGOR_OS_DLL_SUFFIX, path_base.length(), path_base.data(), suffix};

// If in dev mode, copy to a tmp file and load that. This way, we don't lock up the file and can recompile it as the game is running.
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  {
    if (ctx.generationsWrappedAround) // we wrapped around and may potentially fail to write the pdb
    {
      G_ASSERTF(0,
        "Trying to reload cppstcode for the %u-th time in a development session, while limit is %u. Please restart the app!",
        int(UINT16_MAX) + 1, UINT16_MAX);
      return false;
    }

    eastl::string dllTmpPath = make_tmp_file_name(out_dll_path, ctx.tmpFilesGen);

#if _TARGET_PC_WIN
#define DEBINFO_EXT ".pdb"
#elif _TARGET_PC_LINUX
#define DEBINFO_EXT ".so.debuginfo"
#else // _TARGET_PC_MACOSX
#define DEBINFO_EXT ".dSYM"
#endif

    eastl::string pdbPath{eastl::string::CtorSprintf{}, "%.*s-stcode%s" DEBINFO_EXT, path_base.length(), path_base.data(), suffix};
    eastl::string pdbTmpPath = make_tmp_file_name(pdbPath, ctx.tmpFilesGen);

#undef DEBINFO_EXT

    ctx.tmpFilesToDelete.insert(dllTmpPath);
    ctx.tmpFilesToDelete.insert(pdbTmpPath);

    if (!ctx.attemptedInitialCleanup)
    {
      // A compromise between speed and throughness -- in theory we can have up to 65536 tmp files, but running that loop takes
      // seconds, so only do first 64, this should be more than enough for realistic development cases.
      clean_tmp_files_from_first_generations({&out_dll_path, &pdbPath}, 64);
      ctx.attemptedInitialCleanup = true;
    }
    else
    {
      for (const auto &file : ctx.tmpFilesToDelete)
        dd_erase(file.c_str());
    }

    const bool pdbPresent = dd_file_exists(pdbPath.c_str());

    // @NOTE: if there is a pdb present, we have to copy it over too. Furthermore, we need
    // to fix up the references betweend the pdb and the dll to point to the new files.
    auto permanentToTempFilenameReplacer = [&](Tab<uint8_t> &file) {
      if (!pdbPresent)
        return;

      const auto getFilenameBase = [](const auto &path) {
        size_t base = eastl::string_view{path}.rfind('/');
        size_t ext = eastl::string_view{path}.rfind('.');
        size_t first = (base == eastl::string::npos ? 0 : base + 1);
        size_t len = (ext == eastl::string::npos ? path.size() : ext) - first;
        return eastl::string_view{path.data() + first, len};
      };

      // @NOTE: abusing the fact that base names are the same for dl and debuginfo
      eastl::string_view oldRef = getFilenameBase(out_dll_path);
      eastl::string_view newRef = getFilenameBase(dllTmpPath);

      G_ASSERTF(oldRef.size() == newRef.size(), "Tmp stcode dl filename '%s' must be the same len as the main filename '%s'",
        dllTmpPath.c_str(), out_dll_path.c_str());

      eastl::string_view origSource{(const char *)file.data(), file.size()};
      for (size_t pos = origSource.find(oldRef, 0); pos != eastl::string::npos; pos = origSource.find(oldRef, pos + oldRef.size()))
        memcpy(&file[pos], newRef.data(), newRef.size());
    };

    if (!file_cpy(dllTmpPath.c_str(), out_dll_path.c_str(), permanentToTempFilenameReplacer))
    {
      debug("[SH] Failed to create temporary dynlib copy of '%s', either dll is missing, or file write failed", out_dll_path.c_str());
      return false;
    }

    if (pdbPresent && !file_cpy(pdbTmpPath.c_str(), pdbPath.c_str(), permanentToTempFilenameReplacer))
    {
      debug("[SH] Failed to create temporary copy of debug info '%s', file write failed", pdbPath.c_str());
      return false;
    }

    out_dll_path = eastl::move(dllTmpPath);
  }
#endif

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_ANDROID | _TARGET_IOS
  auto it = out_dll_path.rfind('/');
  if (it != eastl::string::npos)
    out_dll_path.erase(0, it + 1);
  ctx.dllHandle = os_dll_load(out_dll_path.c_str());
#else
  ctx.dllHandle = os_dll_load(out_dll_path.c_str());
#endif

  if (!ctx.dllHandle)
  {
    debug("[SH] Dynlib could not be loaded from '%s' via os_dll_load, file is missing or corrupted", out_dll_path.c_str());
    return false;
  }

  return true;
}

static void unload_dll(Context &ctx)
{
  os_dll_close(ctx.dllHandle);
  ctx.dllHandle = nullptr;
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  for (const auto &file : ctx.tmpFilesToDelete)
    dd_erase(file.c_str());
#endif
  debug("[SH] Unloaded stcode dll");
}

static bool init_dll(Context &ctx, const DataBlock &settings);

static bool try_load_dll_with_fallbacks(Context &ctx, const auto &path_base, std::initializer_list<FallbackLevel> fallback_levels,
  const DataBlock &settings, auto &out_dll_path)
{
  eastl::string baseSuffix{};
  for (FallbackLevel level : fallback_levels)
  {
    eastl::string suffix = build_dll_suffix(ctx, settings, level);
    if (baseSuffix.empty() || suffix != baseSuffix)
    {
      if (!baseSuffix.empty())
      {
        debug("[SH] Failed to load stcode dll from '%s', trying %s version", out_dll_path.c_str(),
          FALLBACK_DEBUG_TEXTS[size_t(level)]);
      }
      if (try_load_dll(ctx, path_base, suffix.c_str(), out_dll_path))
      {
        if (!init_dll(ctx, settings))
          unload_dll(ctx);
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
static auto fetch_and_run_dll_func(Context &ctx, const char *func_name,
  TArgs &&...args) -> eastl::optional<decltype((*eastl::declval<TFptr>())(eastl::forward<TArgs>(args)...))>
{
  G_ASSERT(is_loaded(ctx));

  TFptr routine = (TFptr)os_dll_get_symbol(ctx.dllHandle, func_name);
  if (!routine)
    return eastl::nullopt;

  return (*routine)(eastl::forward<TArgs>(args)...);
}

#define FAIL_EXIT(fmt_, ...)                                 \
  do                                                         \
  {                                                          \
    debug(fmt_ ", falling back to bytecode", ##__VA_ARGS__); \
    ctx.dynstcodeExMode = ExecutionMode::BYTECODE;           \
    ctx.stblkcodeExMode = ExecutionMode::BYTECODE;           \
    return true;                                             \
  } while (0)

bool load(Context &ctx, const char *shbindump_path_base)
{
  if (!shBinDumpOwner().getDumpV3())
    FAIL_EXIT("[SH] Failed to fetch external stcode data from bindump -- dump V3 header has incompatible format");

  // Reset from possible previous failures
  ctx.dynstcodeExMode = ExecutionMode::CPP;
  ctx.stblkcodeExMode = ExecutionMode::CPP;

  const auto extractNameBase = [](const char *path) {
    const char *suffix = strstr(path, ".shdump.bin");
    return suffix ? eastl::string_view(path, suffix - path) : eastl::string_view(path);
  };

  const DataBlock &settings = *::dgs_get_settings()->getBlockByNameEx("stcode");
  const eastl::string_view pathBase = extractNameBase(shbindump_path_base);

  if (settings.getBool("useBytecode", false))
    FAIL_EXIT("[SH] useBytecode:b=yes is set in settings");
  else
  {
    if (settings.getBool("useBytecodeForDynstcode", false))
    {
      debug("[SH] useBytecodeForDynstcode:b=yes is set in settings, selecting bytecode for dynstcode");
      ctx.dynstcodeExMode = ExecutionMode::BYTECODE;
    }
    if (settings.getBool("useBytecodeForStblkcode", false))
    {
      debug("[SH] useBytecodeForStblkcode:b=yes is set in settings, selecting bytecode for stblkcode");
      ctx.stblkcodeExMode = ExecutionMode::BYTECODE;
    }

    if (ctx.dynstcodeExMode == ExecutionMode::BYTECODE && ctx.stblkcodeExMode == ExecutionMode::BYTECODE)
    {
      debug("[SH] All stcode will be using bytecode, dll will not be loaded");
      return true;
    }
  }

  eastl::string dllPath;

#if _TARGET_PC_MACOSX
  if (!try_load_dll_with_fallbacks(ctx, pathBase,
        {FallbackLevel::EXACT, FallbackLevel::REL, FallbackLevel::UNIVERSAL_BINARY, FallbackLevel::UNIVERSAL_BINARY_REL,
          FallbackLevel::REL_NOTAG, FallbackLevel::DEV_NOTAG},
        settings, dllPath))
#else
  if (!try_load_dll_with_fallbacks(ctx, pathBase,
        {FallbackLevel::EXACT, FallbackLevel::REL, FallbackLevel::NOTAG, FallbackLevel::REL_NOTAG, FallbackLevel::DEV_NOTAG}, settings,
        dllPath))
#endif
  {
    FAIL_EXIT("[SH] Failed to load stcode dll from '%s'", dllPath.c_str());
  }

  debug("[SH] Successfully loaded stcode dll from '%s'", dllPath.c_str());

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  if (ctx.tmpFilesGen == UINT16_MAX)
    ctx.generationsWrappedAround = true;

  ++ctx.tmpFilesGen;
#endif

  return true;
}

#undef FAIL_EXIT

void unload(Context &ctx)
{
  if (is_loaded(ctx))
  {
    ctx.dynRoutineStorageView = {};
    ctx.stRoutineStorageView = {};
    unload_dll(ctx);
  }
}

bool is_loaded(const Context &ctx) { return ctx.dllHandle != nullptr; }

void disable(Context &ctx)
{
  ctx.dynstcodeExMode = ExecutionMode::BYTECODE;
  ctx.stblkcodeExMode = ExecutionMode::BYTECODE;
}

void run_dyn_routine(const Context &ctx, size_t id, const void *vars, uint32_t dyn_offset, int req_tex_level, const char *new_name)
{
  // Prepare data for resource setters
  cpp::last_resource_name = new_name;
  cpp::req_tex_level = req_tex_level;

  DYNSTCODE_PROFILE_BEGIN();
  {
    ctx.dynRoutineStorageView[id](vars, dyn_offset);
  }
  DYNSTCODE_PROFILE_END();
}

void run_st_routine(const Context &ctx, size_t id, void *execution_context)
{
  STBLKCODE_PROFILE_BEGIN();
  {
    ctx.stRoutineStorageView[id](execution_context);
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

static bool init_dll(Context &ctx, const DataBlock &settings)
{
#if VALIDATE_CPP_STCODE
  const auto *stcodeDbgBlk = settings.getBlockByNameEx("debug");
  if (stcodeDbgBlk->getBool("validate", false))
  {
    ctx.dynstcodeExMode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;
    ctx.stblkcodeExMode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;
  }
  else
  {
    if (stcodeDbgBlk->getBool("validateDynstcode", false))
      ctx.dynstcodeExMode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;
    if (stcodeDbgBlk->getBool("validateStblkcode", false))
      ctx.stblkcodeExMode = ExecutionMode::TEST_CPP_AGAINST_BYTECODE;
  }
  if (stcodeDbgBlk->getBool("requireStrictNumberEquality", false))
    dbg::require_exact_record_matches();
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

  const auto verFromDllMaybe = fetch_and_run_dll_func<uint32_t (*)()>(ctx, "get_version");
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

  const auto hashFromDllMaybe = fetch_and_run_dll_func<const uint8_t *(*)()>(ctx, "get_hash");
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

  const auto resMaybe =
    fetch_and_run_dll_func<bool (*)(void *, void **, uint32_t *, void **, uint32_t *)>(ctx, "init_dll_and_get_routines",
      (void *)&cpp::cb_table_impl, &dynRoutinesStart, &dynRoutinesCount, &stRoutinesStart, &stRoutinesCount);

  if (!resMaybe.has_value() || !resMaybe.value())
  {
    debug("[SH] Failed to initialize stcode dll");
    return false;
  }

  ctx.dynRoutineStorageView = {(Context::DynRoutinePtr *)dynRoutinesStart, intptr_t(dynRoutinesCount)};
  ctx.stRoutineStorageView = {(Context::StRoutinePtr *)stRoutinesStart, intptr_t(stRoutinesCount)};

  return true;
}

void set_special_tag_interp(Context &ctx, SpecialBlkTagInterpreter &&interp) { ctx.specialTagInterpreter = eastl::move(interp); }

void set_custom_const_setter(shaders_internal::ConstSetter *setter) { cpp::custom_const_setter = setter; }

ScopedCustomConstSetter::ScopedCustomConstSetter(shaders_internal::ConstSetter *setter) : prev(cpp::custom_const_setter)
{
  cpp::custom_const_setter = setter;
}

ScopedCustomConstSetter::~ScopedCustomConstSetter() { cpp::custom_const_setter = prev; }

} // namespace stcode
