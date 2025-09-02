// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cppStcodeAssembly.h"
#include "processes.h"
#include "blkHash.h"
#include <generic/dag_expected.h>
#include <shaders/shader_layout.h>

class ShCompilationInfo;
namespace shc
{
class CompilerConfig;
}

enum class StcodeDynlibConfig
{
  DEV,
  REL,
  DBG
};

struct StcodeCompilationDirs
{
  eastl::string stcodeDir;
  eastl::string stcodeCompilationCacheDirUnmangled;
};

eastl::optional<shader_layout::ExternalStcodeMode> arg_str_to_cpp_stcode_mode(const char *str);

bool set_stcode_config_from_arg(const char *str, shc::CompilerConfig &config_rw);
bool set_stcode_arch_from_arg(const char *str, shc::CompilerConfig &config_rw);

#if _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
bool set_stcode_platform_from_arg(const char *str, shc::CompilerConfig &config_rw);
#endif

#define RET_IF_SHOULD_NOT_COMPILE(_ret)                                         \
  do                                                                            \
  {                                                                             \
    if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::NONE) \
      return _ret;                                                              \
  } while (0)

StcodeCompilationDirs init_stcode_compilation(const char *dest_dir, const char *shortened_dest_dir_for_caching);

void save_compiled_cpp_stcode(StcodeShader &&cpp_shader, const ShCompilationInfo &comp);
void save_stcode_global_vars(StcodeGlobalVars &&cpp_globvars, const ShCompilationInfo &comp);
void save_stcode_dll_main(StcodeShader &&combined_cppstcode, const CryptoHash &stcode_hash, const ShCompilationInfo &comp);

struct StcodeSourceFileStat
{
  int64_t mtime = -1;
  BlkHashBytes savedBlkHash{};
};

// Returns eastl::nullopt if files have mismatched blk hashes, mtime is the earliest of all
eastl::optional<StcodeSourceFileStat> get_gencpp_files_stat(const char *target_fn, const ShCompilationInfo &comp);
eastl::optional<StcodeSourceFileStat> get_main_cpp_files_stat(const ShCompilationInfo &comp);

enum class StcodeMakeTaskError
{
  FAILED,
  DISABLED
};

dag::Expected<proc::ProcessTask, StcodeMakeTaskError> make_stcode_compilation_task(const char *out_dir, const char *lib_name,
  const ShCompilationInfo &comp);
