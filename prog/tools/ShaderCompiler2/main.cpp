// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN && !defined(_M_ARM64)
#define HAVE_BREAKPAD_BINDER 1
#else
#define HAVE_BREAKPAD_BINDER 0
#endif
#include <sys/stat.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#if HAVE_BREAKPAD_BINDER
#include <binder.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/shader_ver.h>
#include "sha1_cache_version.h"
#include "shLog.h"
#include "shCompiler.h"
#include "globalConfig.h"
#include "shadervarGenerator.h"
#include <ioSys/dag_io.h>
#include "shCacheVer.h"
// #include <shaders/shOpcodeUnittest.h>
#include "sh_stat.h"
#include "loadShaders.h"
#include "assemblyShader.h"
#include <stdlib.h>
#include "nameMap.h"
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>

#include <libTools/util/hash.h>
#include <util/dag_strUtil.h>

#include <perfMon/dag_cpuFreq.h>

#include "DebugLevel.h"
#include <libTools/util/atomicPrintf.h>

#include "cppStcode.h"
#include <EASTL/string.h>

#include "defer.h"
#include "processes.h"

#if _TARGET_PC_WIN
#include <windows.h>
#else

#include <unistd.h>
#include <spawn.h>
#include <pthread.h>
#if _TARGET_PC_MACOSX
#include <AvailabilityMacros.h>
#elif _TARGET_PC_LINUX
#include <sys/wait.h>
#endif
extern int __argc;
extern char **__argv;

void SleepEx(uint32_t ms, bool) { usleep(ms * 1000); }
#endif


#if _CROSS_TARGET_C1


#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#define OUTPUT_DIR "_output-dx12/"
#elif _CROSS_TARGET_DX11
#define OUTPUT_DIR "_output-dx11/"
#include <d3dcompiler.h>
#else
#define OUTPUT_DIR "_output/"
#if _TARGET_PC_WIN
#include <d3dcompiler.h>
#endif
#endif

#if _TARGET_64BIT
size_t dagormem_first_pool_sz = size_t(2048) << 20;
size_t dagormem_next_pool_sz = 512 << 20;
#endif
// unsigned int dagormem_first_pool_sz = 512 << 20;
// unsigned int dagormem_next_pool_sz = 64 << 20;
size_t dagormem_max_crt_pool_sz = 64 << 20;

extern int getShaderCacheVersion();
extern void reset_shaders_inc_base();
extern void add_shaders_inc_base(const char *base);

#if _CROSS_TARGET_METAL || _CROSS_TARGET_SPIRV
extern const char *debug_output_dir;
#endif

#define QUOTE(x) #x
#define STR(x)   QUOTE(x)
static void show_header()
{
  printf("Dagor Shader Compiler %s (dump format '%c%c%c%c', obj format '%c%c%c%c', shader cache ver 0x%x)\n"
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_SPIRV
         "Target: Spir-V via HLSLcc (dxbc->glsl), DX SDK %s"
#elif _CROSS_TARGET_METAL
         "Target: Metal via HLSL2GLS"
#elif _CROSS_TARGET_DX12
         "Target: XboxOne or PC DirectX 12, DXC"
#elif _CROSS_TARGET_DX11
         "Target: PC DirectX 11, SDK.%s"
#elif _CROSS_TARGET_EMPTY
         "Target: stub.%s"
#else
#error "DX9 support dropped"
#endif
         "\n\nCopyright (C) Gaijin Games KFT, 2023\n",
    shc::config().version, _DUMP4C(SHADER_BINDUMP_VER), _DUMP4C(SHADER_CACHE_VER), sha1_cache_version
#if _CROSS_TARGET_C1


#elif _CROSS_TARGET_DX12
  // no version info
#elif _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV
    ,
    STR(D3D_COMPILER_VERSION)
#elif _CROSS_TARGET_EMPTY
    ,
    "none"
#endif
  );
}
#undef QUOTE
#undef STR


// @TODO: update usage (-optionalAsBranches, maybe there is something else)
static void showUsage()
{
  show_header();
  printf(
    "\nUsage:\n"
    "  dsc2-dev.exe <config blk-file|single *.dshl file> <optional options>\n"
    "\n"
    "Common options:\n"
    "  -no_sha1_cache - do not use sha1 cache\n"
    "  -no_compression - do not use compression for shaders (default: false)\n"
    "  -purge_sha1_cache - delete sha1 cache (if no_sha1_cache is not enabled, it will be re-populated)\n"
    "  -commentPP - save PP as comments (for shader debugging)\n"
    "  -r - force rebuild all files (even if not modified)\n"
    "  -relinkOnly - forbid compilation so build will fail if any OBJ is out-of-data. If passed with -r, reduces -r to -forceRelink\n"
    "  -forceRelink - enforce linking even if bindump is up to date (may be useful for debugging)\n"
    "  -maxVSF N - maximum allowed VSF register no. Default 4096\n"
    "  -maxPSF N - maximum allowed PSF register no. Default 4096\n"
    "  -cjN - per-source parallel compilation using N processes (N=CpuCount by default)\n"
    "  -cjWait - don't terminate compilation jobs on first error (when -cjN used), off by default\n"
    "  -jN - use N job threads to compile shaders in parallel (N=CpuCount by default)\n"
    "  -belownormal - lower compiler process priority\n"
    "  -q - minimum messages\n"
    "  -define NAME VAL - add HLSL #define NAME VAL\n"
    "  -wx - treat all warnings as errors\n"
    "  -wall - print really all warning, including strange ones\n"
    "  -nosave - no save output code (for debugs)\n"
    "  -debug - enable debugging mode in shaders. It assumes the DEBUG = yes interval, and enables assert\n"
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    "  -pdb"
#endif
    " - insert debug info during HLSL compile\n"
    "  -dfull | -debugfull - for Dx11 and Dx12, it produces unoptimized shaders with full debug info,\n"
    "  -embed_source - for Dx11 and Dx12, it embeds hlsl source in the binary without turning off optimization,\n"
    "   including shader source, so shader debugging will have the HLSL source. It implies -pdb.\n"
    "  -daftermath - for DX12, it produces shader binaries and pdb/cso files for detailed resolve of\n"
    "   Aftermath gpu dumps by Nsight Graphics\n"
    "  -w - show compilation warnings (hidden by default)\n"
    "  -ON - HLSL optimization level (N==0..4, default =4)\n"
    "  -skipvalidation - do not validate the generated code against known capabilities"
    " and constraints\n"
    "  -nodisassembly - no hlsl disassembly output\n"
    "  -codeDump    - always dump hlsl/Cg source code to be compiled to shaderlog\n"
    "  -codeDumpSep - always dump hlsl/Cg source code to be compiled to separate files\n"
    "  -codeDumpErr - dump hlsl/Cg source code to shaderlog when error occurs\n"
    "  -validateIdenticalBytecode - dumps variants that were compiled but had identical bytecode\n"
    "  -shaderOn - default shaders are on\n"
    "  -invalidAsNull - by default invalid variants are marked NULL (just as dont_render)\n"
    "  -o <intermediate dir> - path to store obj files.\n"
    "  -c <source.dshl>  - compile only one shader file (using all settings in BLK as usual)\n"
    "  -logdir <dir>   - path to logs\n"
    "  -suppressLogs   - do not make fileLogs\n"
    "  -enablefp16     - enable using 16 bit types in shaders\n"
    "  -HLSL2021       - use the HLSL 2021 version of the language\n"
    "  -addTextureType - save static texture types to shaderdump (need for texture type validation in daBuild)\n"
    "  -logExactTiming - enable logging of compilation times with 0.001s (1ms) precision\n"
    "  -perFileAllLogs - enable logging of compilation times for all files with additional info\n"
    "  -saveDumpOnCrash - save a dump for the proccess if there is a critical issue during compilation (could cause a process hung)\n"
    "  -cppStcode       - compile cpp stcode along with bytecode (overrides compileCppStcode:b from blk).\n"
    "  -noCppStcode     - complie only bytecode stcode (overrides compileCppStcode:b from blk).\n"
    "  -useCpujobsBackend - Use old cpujobs as the mt backend. Shows worse cpu utilizaiton, kept as a fallback for now.\n"
    "  -cppUnityBuild   - use unity build for cpp stcode.\n"
    "  -cppStcodeArch   - Arch for stcode compilation. Default is chosen when it is not specified. Opts: "
    "x86|x86_64|arm64|arm64e|armv7|armv7s|armeabi-v7a|arm64-v8a|i386|e2k.\n"
    "  -cppStcodePlatform - Platform for stcode compilation. Win is chosen when it is not specified. Current opts: pc|android.\n"
#if _CROSS_TARGET_DX12
    "  -localTimestamp - use filesystem timestamps for shader classes instead of git commit timestamps\n"
    "  -autotestMode - disables some features for test speed. Currently disables phase 2 of xbox compilation\n"
    "compilation tests\n"
#endif
#if _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL
    "  -enableBindless:<on|off> - enables utilizing bindless features (default: "
#if USE_BINDLESS_FOR_STATIC_TEX
    "on)\n"
#else
    "off)\n"
#endif
#endif
    "  -shadervar-generator-mode mode - specifies in which mode the shadervar generator will work. Possible modes:\n"
    "                                 - none - the generator does nothing (by default)\n"
    "                                 - remove - all generated files and folders are deleted\n"
    "                                 - check - generates code, but does not save it to files\n"
    "                                           checks that all generated code fully corresponds to the one already saved to files\n"
    "                                           if this is not true, the compilation is failed. This mode is used in bs\n"
    "                                 - generate - generates code and saves it in files\n"
    "\n"
    "Options for !!SINGLE!! (not config BLK!!!) *.DSHL file rebuild:\n"
    "  -gi:<on|off> - enable/disable variants for GI (<on> by default)\n"
#if _CROSS_TARGET_SPIRV
    "  -compiler-hlslcc - always use hlslcc compiler path\n"
    "  -compiler-dxc - always use dxc compiler path\n"
#endif
#if _CROSS_TARGET_DX12
    "  -platform-pc - target PC (default)\n"
    "  -platform-xbox-one - target Xbox One\n"
    "  -platform-xbox-scarlett - target Xbox Scarlett\n"
#endif
#ifdef _CROSS_TARGET_METAL
    "  -metalios       - build shaders for iOS device\n"
#endif
#if _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL
    "  -debugdir DIR   - use DIR to store debuginfo for shader debugger\n"
#endif
#if !_CROSS_TARGET_C1 && !_CROSS_TARGET_C2
    "  -fsh:<1.1|1.3|1.4|2.0|2.b|2.a|3.0> - set fsh version (<2.0> by default)\n"
#endif
  );
}

#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
static pthread_t sigint_handler_thread;
#endif

static bool doCompileModulesAsync(const ShVariantName &sv)
{
  eastl::string cmdname(__argv[0]);
#if _TARGET_PC_WIN
  if (!dd_get_fname_ext(cmdname.c_str()))
    cmdname += ".exe";
#endif

  dag::Vector<eastl::string> commonArgv{cmdname};
  for (int i = 1; i < __argc; i++)
    commonArgv.emplace_back(__argv[i]);
  commonArgv.emplace_back("-r");
  if (shc::config().compileCppStcode)
    commonArgv.emplace_back("-cppStcode");

  if (sv.sourceFilesList.size() < 2)
    return false;

  int timeMsec = get_time_msec();
  int modules_cnt = 0;
  for (int i = 0; i < sv.sourceFilesList.size(); i++)
  {
    if (!shc::should_recompile_sh(sv, sv.sourceFilesList[i]) && !shc::config().forceRebuild)
      continue;

    auto argv = commonArgv;
    argv.emplace_back("-c");
    argv.emplace_back(sv.sourceFilesList[i].c_str());

    // clang-format off
    proc::enqueue(proc::ProcessTask{
      eastl::move(argv),
      eastl::nullopt,
      [fname = eastl::string(shc::get_obj_file_name_from_source(sv.sourceFilesList[i], sv))] {
        if (dd_file_exist(fname.c_str()))
          dd_erase(fname.c_str());
      }});
    // clang-format on

    modules_cnt++;
  }

  bool compiled = proc::execute();
  if (!compiled)
  {
    sh_debug(SHLOG_FATAL, "compilation failed");
    return false;
  }

  float timeSec = float(get_time_msec() - timeMsec) * 1e-3;
  if (shc::config().logExactCompilationTimes)
  {
    sh_debug(SHLOG_NORMAL, "[INFO] parallel compilation of %d modules using %d processes (x%d threads) done for %.4gs", modules_cnt,
      proc::max_proc_count(), shc::worker_count(), timeSec);
  }
  else
  {
    sh_debug(SHLOG_NORMAL, "[INFO] parallel compilation of %d modules using %d processes (x%d threads) done for %ds", modules_cnt,
      proc::max_proc_count(), shc::worker_count(), int(timeSec));
  }

  return true;
}

static char sha1_cache_dir_buf[320] = {0};

static bool remove_recursive(const char *dirname)
{
  if (!dd_dir_exist(dirname))
    return false;
  alefind_t ff;
  char dirmask[320] = {0};
  SNPRINTF(dirmask, sizeof(dirmask), "%s/*.*", dirname);
  bool ret = true;
  if (dd_find_first(dirmask, DA_SUBDIR, &ff))
  {
    do
    {
      SNPRINTF(dirmask, sizeof(dirmask), "%s/%s", dirname, ff.name);
      ret &= (ff.attr & DA_SUBDIR) ? remove_recursive(dirmask) : dd_erase(dirmask);
    } while (dd_find_next(&ff));
    dd_find_close(&ff);
  }
  ret &= dd_rmdir(dirname);
  return ret;
}

static void compile(Tab<String> &source_files, const char *fn, const char *bindump_fnprefix, const ShHardwareOptions &opt,
  const char *blk_file_name, const char *cfg_dir, const char *binminidump_fnprefix, BindumpPackingFlags packing_flags,
  shc::CompilerConfig &config_rw)
{
  ShVariantName sv(dd_get_fname(fn), opt);
  ShaderParser::AssembleShaderEvalCB::buildHwDefines(opt);
  sv.sourceFilesList = source_files;
  for (int i = 0; i < sv.sourceFilesList.size(); i++)
    simplify_fname(sv.sourceFilesList[i]);
  if (shc::config().singleCompilationShName)
  {
    for (int i = 0; i < sv.sourceFilesList.size(); i++)
      if (sv.sourceFilesList[i] == shc::config().singleCompilationShName)
      {
        clear_and_shrink(sv.sourceFilesList);
        sv.sourceFilesList.push_back() = shc::config().singleCompilationShName;
        break;
      }

    if (sv.sourceFilesList.size() != 1 || sv.sourceFilesList[0] != shc::config().singleCompilationShName)
    {
      sh_debug(SHLOG_ERROR, "unknown source '%s', available are:", shc::config().singleCompilationShName);

      int linesForStdout = min<size_t>(100ul, sv.sourceFilesList.size());
      for (int i = 0; i < linesForStdout; i++)
        sh_debug(SHLOG_NORMAL, "    %s", sv.sourceFilesList[i]);
      if (linesForStdout > sv.sourceFilesList.size())
      {
        sh_debug(SHLOG_NORMAL, "\nToo many available files, see full list in log");
        for (int i = linesForStdout; i < sv.sourceFilesList.size(); i++)
          debug("    %s", sv.sourceFilesList[i]);
      }

      sh_process_errors();
      return;
    }
  }

  if (shc::config().intermediateDir)
  {
    sv.intermediateDir = shc::config().intermediateDir;
  }
  else
  {
    char intermediateDir[260];
    dd_get_fname_location(intermediateDir, fn);
    dd_append_slash_c(intermediateDir);
    strcat(intermediateDir, OUTPUT_DIR);
    sv.intermediateDir = intermediateDir;
  }
  String additionalDirStr;
  additionalDirStr.aprintf(0, "-O%d", shc::config().hlslOptimizationLevel);
  if (shc::config().hlslDebugLevel == DebugLevel::BASIC)
    additionalDirStr += "D";
  else if (shc::config().hlslDebugLevel == DebugLevel::FULL_DEBUG_INFO)
    additionalDirStr += "DFULL";
  else if (shc::config().hlslDebugLevel == DebugLevel::AFTERMATH)
    additionalDirStr += "DAFTERMATH";
  if (shc::config().isDebugModeEnabled)
    additionalDirStr += "-debug";
  if (shc::config().hlslEmbedSource)
    additionalDirStr += "-embed_src";
#if _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL
  if (shc::config().enableBindless)
    additionalDirStr += "-bindless";
#endif
  if (shc::config().autotestMode)
    additionalDirStr += "-autotest";

  sv.intermediateDir += additionalDirStr;
  dd_mkdir(sv.intermediateDir);
  SNPRINTF(sha1_cache_dir_buf, sizeof(sha1_cache_dir_buf), "%s/../../shaders_sha~%s%s", sv.intermediateDir.c_str(),
    shc::config().crossCompiler, additionalDirStr.c_str());
  config_rw.sha1CacheDir = sha1_cache_dir_buf;
  if (shc::config().purgeSha1)
  {
    debug("purging cache %s: %s", shc::config().sha1CacheDir, remove_recursive(shc::config().sha1CacheDir) ? "success" : "error");
  }
  sv.dest = String(0, "%s/%s", sv.intermediateDir, sv.dest);
  debug("dest: %s", sv.dest);

  bool isBlkChanged = false;
  eastl::string blkHashFilename;
  using HashBytes = eastl::array<uint8_t, 20>;
  HashBytes blkHash;
  if (blk_file_name)
  {
    blkHashFilename = eastl::string(eastl::string::CtorSprintf{}, "%s/blk_hash.txt", sv.intermediateDir.c_str());
    IGenSave *hasher = create_hash_computer_cb(HASH_SAVECB_SHA1);
    DataBlock blk(blk_file_name);
    blk.removeBlock("source");
    blk.removeBlock("explicit_var_ref");
    blk.removeParam("outDumpName");
    blk.removeParam("outMiniDumpName");
    blk.removeParam("packBin");
    blk.removeParam("packShGroups");
    blk.removeParam("compileCppStcode"); // This will be added back in with possible override
    if (shc::config().addTextureType)
      blk.addBool("addTextureType", true);
    if (shc::config().compileCppStcode)
      blk.addBool("compileCppStcode", true);
    if (shc::config().cppStcodeUnityBuild)
      blk.addBool("cppStcodeUnityBuild", true);
    if (shc::config().cppStcodeArch != StcodeTargetArch::DEFAULT)
      blk.addInt("cppStcodeArch", (int)shc::config().cppStcodeArch);
    blk.saveToTextStream(*hasher);
    get_computed_hash(hasher, blkHash.data(), blkHash.size());
    destory_hash_computer_cb(hasher);
    String new_hash_hex;
    data_to_str_hex(new_hash_hex, blkHash.data(), blkHash.size());
    // blk.saveToTextFile(String(0, "%s-stripped-%s.blk", blk_file_name, new_hash_hex));
    auto blkHashFile = df_open(blkHashFilename.c_str(), DF_READ);
    if (blkHashFile)
    {
      HashBytes previosBlkHash;
      df_read(blkHashFile, previosBlkHash.data(), previosBlkHash.size());
      df_close(blkHashFile);
      isBlkChanged = memcmp(previosBlkHash.data(), blkHash.data(), blkHash.size()) != 0;
      String old_hash_hex;
      data_to_str_hex(old_hash_hex, previosBlkHash.data(), previosBlkHash.size());
      sh_debug(SHLOG_INFO, "'%s' hash=%s, '%s' hash=%s => isBlkChanged=%d", blkHashFilename.c_str(), old_hash_hex, blk_file_name,
        new_hash_hex, isBlkChanged);
    }
    else
    {
      isBlkChanged = true;
      sh_debug(SHLOG_INFO, "Previous hash missed (%s), '%s' hash=%s => isBlkChanged=%d", blkHashFilename.c_str(), blk_file_name,
        new_hash_hex, isBlkChanged);
    }
  }

  if (isBlkChanged)
  {
    sh_debug(SHLOG_INFO, "'%s' changed. '%s' should be deleted to force rebuild", blk_file_name, (const char *)sv.intermediateDir);
    config_rw.forceRebuild = true;
  }
  if (shc::config().forceRebuild && shc::config().relinkOnly)
  {
    if (isBlkChanged)
    {
      sh_debug(SHLOG_FATAL, "need to recompile %s but compilation is denied by -relinkOnly",
        shc::config().singleCompilationShName ? shc::config().singleCompilationShName : "");
      return;
    }
    else // caused by -r flag: -relinkOnly reduces -r to -forceRelink
    {
      config_rw.forceRebuild = false;
      config_rw.forceRelink = true;
      sh_debug(SHLOG_NORMAL, "[WARNING] -r flag was reduced to -forceRelink because -relinkOnly was specified");
    }
  }
  if (shc::config().forceRelink && shc::config().singleCompilationShName)
  {
    sh_debug(SHLOG_FATAL,
      "trying to compile single shader %s but relinking is enforced with -forceRelink or -r and -relinkOnly combination",
      shc::config().singleCompilationShName);
    return;
  }
  if (shc::config().forceRelink && shc::config().noSave)
  {
    sh_debug(SHLOG_FATAL, "need to relink shaders but -nosave flag enforces no save or linkage",
      shc::config().singleCompilationShName);
    return;
  }

  // build binary dump
  auto verStr = d3d::as_ps_string(opt.fshVersion);

  char bindump_fn[260];
  char stcode_lib_path[260];
#if _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL
  if (shc::config().enableBindless)
  {
    _snprintf(bindump_fn, 260, "%s.bindless.%s%s.shdump.bin", bindump_fnprefix, verStr, shc::config().autotestMode ? ".autotest" : "");
    _snprintf(stcode_lib_path, 260, "%s.bindless.%s-stcode", bindump_fnprefix, verStr);
  }
  else
#endif
  {
    _snprintf(bindump_fn, 260, "%s.%s%s.shdump.bin", bindump_fnprefix, verStr, shc::config().autotestMode ? ".autotest" : "");
    _snprintf(stcode_lib_path, 260, "%s.%s-stcode", bindump_fnprefix, verStr);
  }

  const char *stcode_lib_fn = strrchr(stcode_lib_path, '/');
  if (!stcode_lib_fn)
    stcode_lib_fn = stcode_lib_path;
  else
    ++stcode_lib_fn;

  {
    char destDir[260];
    dd_get_fname_location(destDir, bindump_fn);
    dd_append_slash_c(destDir);
    dd_mkdir(destDir);

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2













#elif _CROSS_TARGET_DX12
    if (!shc::config().noSave)
    {
      String pdb(260, "%s_pdb/", destDir[0] ? bindump_fnprefix : "./");
      static wchar_t dx12_pdb_cache_dir_storage[260];
      mbstowcs(dx12_pdb_cache_dir_storage, pdb, 260);
      config_rw.dx12PdbCacheDir = dx12_pdb_cache_dir_storage;
    }
#endif
  }

  prepare_stcode_directory(sv.intermediateDir);
  eastl::optional<StcodeInterface> stcodeMainInterface{}; // Only gets inited if linkage takes place

  CompilerAction compAction = shc::config().forceRebuild ? CompilerAction::COMPILE_AND_LINK : shc::should_recompile(sv);
  if (shc::config().singleCompilationShName)
  {
    if (compAction == CompilerAction::COMPILE_AND_LINK)
      compAction = CompilerAction::COMPILE_ONLY;
    else if (compAction == CompilerAction::LINK_ONLY)
      compAction = CompilerAction::NOTHING;
  }
  else if (shc::config().forceRelink)
  {
    if (compAction == CompilerAction::NOTHING)
      compAction = CompilerAction::LINK_ONLY;
    else if (compAction == CompilerAction::COMPILE_ONLY)
      compAction = CompilerAction::COMPILE_AND_LINK;
  }

  if (compAction != CompilerAction::NOTHING)
  {
    if (shc::config().shortMessages && !shc::config().singleCompilationShName)
      ATOMIC_PRINTF_IMM("[INFO] Building '%s'...\n", (char *)sv.dest);
    if (proc::is_multiproc())
    {
      const bool compileResult = doCompileModulesAsync(sv);
      AtomicPrintfMutex::term();

      if (compileResult)
      {
        config_rw.forceRebuild = false;
        if (compAction == CompilerAction::COMPILE_AND_LINK)
          compAction = CompilerAction::LINK_ONLY;
        else if (compAction == CompilerAction::COMPILE_ONLY)
          compAction = CompilerAction::NOTHING;
      }
    }
    shc::compileShader(sv, stcodeMainInterface, shc::config().noSave, shc::config().forceRebuild, compAction);
    if (!shc::config().suppressLogs)
      ShaderCompilerStat::printReport((proc::is_multiproc() || shc::config().singleCompilationShName)
                                        ? nullptr
                                        : (shc::config().logDir ? shc::config().logDir : cfg_dir));
  }
  else
  {
    sh_debug(SHLOG_INFO, "Skipping up-to-date '%s'", (char *)sv.dest);
  }
  shc::resetCompiler();
  if (shc::config().singleCompilationShName)
    return;

  if (strcmp(bindump_fnprefix, "*") != 0)
  {
    dd_mkpath(bindump_fn);
    if (!shc::buildShaderBinDump(bindump_fn, sv.dest, shc::config().forceRebuild, false, packing_flags,
          stcodeMainInterface.has_value() ? &stcodeMainInterface.value() : nullptr))
    {
      sh_debug(SHLOG_FATAL, "Failed to build bindump at '%s' from combined obj at '%s'", bindump_fn, sv.dest.c_str());
      return;
    }
  }

  if (binminidump_fnprefix)
  {
    _snprintf(bindump_fn, 260, "%s.%s%s.shdump.bin", binminidump_fnprefix, verStr, shc::config().autotestMode ? ".autotest" : "");
    dd_mkpath(bindump_fn);
    if (!shc::buildShaderBinDump(bindump_fn, sv.dest, dd_stricmp(binminidump_fnprefix, bindump_fnprefix) == 0, true, packing_flags,
          nullptr))
    {
      sh_debug(SHLOG_FATAL, "Failed to build minidump %s at '%s' from combined obj at '%s'", binminidump_fnprefix, bindump_fn,
        sv.dest.c_str());
      return;
    }
  }

  eastl::string bindump_path;
  const char *end = strrchr(bindump_fnprefix, '/'); // remove last part, the prefix
  if (end)
    bindump_path = eastl::string(bindump_fnprefix, end);
  else
    bindump_path = eastl::string("./");

  auto stcodeCompTaskMaybe = make_stcode_compilation_task(bindump_path.c_str(), stcode_lib_fn, sv);
  if (stcodeCompTaskMaybe)
  {
    proc::enqueue(eastl::move(stcodeCompTaskMaybe).value());
    bool compiled = proc::execute();
    if (!compiled)
    {
      sh_debug(SHLOG_FATAL, "Failed to compile stcode");
      return;
    }

    cleanup_after_stcode_compilation(bindump_path.c_str(), stcode_lib_fn);
  }
  else if (stcodeCompTaskMaybe.error() == StcodeMakeTaskError::FAILED)
  {
    sh_debug(SHLOG_FATAL, "Failed to create stcode task");
    return;
  }

  if (blk_file_name && isBlkChanged)
  {
    if (auto blkHashFile = df_open(blkHashFilename.c_str(), DF_WRITE | DF_CREATE))
    {
      df_write(blkHashFile, blkHash.data(), blkHash.size());
      df_close(blkHashFile);
      sh_debug(SHLOG_INFO, "Write a new blk hash into %s.", blkHashFilename.c_str());
    }
    else
    {
      sh_debug(SHLOG_ERROR, "Failed to write a new blk hash into %s. Next compilation could lead to the whole recompilation.",
        blkHashFilename.c_str());
    }
  }
}

static String getDir(const char *filename)
{
  int len = (int)strlen(filename);

  String res("./");
  while (len > 0)
  {
    len--;
    if (filename[len] == '\\' || filename[len] == '/')
      break;
  }

  if (len > 0)
  {
    res.resize(len + 1);
    res[len] = 0;
    if (len > 0)
      memcpy(&res[0], filename, len);
  }

  return res;
}

static String makeShBinDumpName(const char *filename)
{
  String dump_fn(filename);
  if (dump_fn.length() > 5 && dd_stricmp(&dump_fn[dump_fn.length() - 5], ".dshl") == 0)
    erase_items(dump_fn, dump_fn.length() - 3, 3);
  else if (dump_fn.length() > 3 && dd_stricmp(&dump_fn[dump_fn.length() - 3], ".sh") == 0)
    erase_items(dump_fn, dump_fn.length() - 3, 3);
  else if (dump_fn.length() > 4 && dd_stricmp(&dump_fn[dump_fn.length() - 4], ".blk") == 0)
    erase_items(dump_fn, dump_fn.length() - 4, 4);
  return dump_fn;
}

#define USE_FAST_INC_LOOKUP (__cplusplus >= 201703L)
#if _TARGET_PC_MACOSX && MAC_OS_X_VERSION_MIN_REQUIRED < 101500 // macOS SDK < 10.15
#undef USE_FAST_INC_LOOKUP
#elif _TARGET_PC_LINUX && defined(__clang_major__) && __clang_major__ < 12 // older compilers lacking filesystem (e.g. on AstraLinux)
#undef USE_FAST_INC_LOOKUP
#endif
#if USE_FAST_INC_LOOKUP
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

inline eastl::string convert_dir_separator(const char *s_)
{
  eastl::string s = s_;
  eastl::replace(s.begin(), s.end(), '\\', '/');
  return s;
}
#endif
void add_include_path(const char *d, shc::CompilerConfig &config_rw)
{
  config_rw.dscIncludePaths.emplace_back(String(d));
#if USE_FAST_INC_LOOKUP
  G_ASSERT(d);
  fs::path dirPath(d);
  eastl::string dir = convert_dir_separator(d);
  std::error_code ec;
  // printf("dir %s\n", d);
  for (const auto &p : fs::recursive_directory_iterator(d, ec))
  {
    if (bool(ec))
    {
      printf("error %s iterating for %s\n", ec.message().c_str(), d);
      continue;
    }
    // fs::path absPath = fs::absolute(p.path(), ec);
    if (p.is_directory(ec) || bool(ec))
      continue;
    eastl::string fn = convert_dir_separator(p.path().lexically_relative(dirPath).string().c_str());
    // printf("%s: %s\n", d, fn.c_str());
    if (config_rw.fileToFullPath.find(fn) == config_rw.fileToFullPath.end())
    {
      config_rw.fileToFullPath[fn] = (dir + "/") + fn;
    }
  }
#endif
}

String shc::search_include_with_pathes(const char *fn) // we can save mem allocation, by return of const char*
{
#if USE_FAST_INC_LOOKUP
  // String s;
  if (strstr(fn, "\\"))
  {
    auto it = shc::config().fileToFullPath.find(convert_dir_separator(fn));
    if (it != shc::config().fileToFullPath.end())
      return String(it->second.c_str());
  }
  else
  {
    auto it = shc::config().fileToFullPath.find_as(fn);
    if (it != shc::config().fileToFullPath.end())
      return String(it->second.c_str());
  }
  return String(fn);
#else
  char buf[2048];
  for (int i = 0, e = (int)shc::config().dscIncludePaths.size(); i < e; i++)
  {
    SNPRINTF(buf, sizeof(buf), "%s/%s", shc::config().dscIncludePaths[i].c_str(), fn);
    if (dd_file_exist(buf))
      return String(buf);
    // dd_simplify_fname_c(s);
    // if (dd_file_exists(s))
    //   return s;
  }
  return String(fn);
#endif
}

static class CompilerRestartProc : public SRestartProc
{
public:
  const char *procname() { return "shaderCompiler"; }

  CompilerRestartProc() : SRestartProc(RESTART_GAME | RESTART_VIDEO) {}

  void startup() { shc::startup(); }

  void shutdown() { shc::shutdown(); }
} compiler_rproc;

static void mergeDataBlock(DataBlock &dest, const DataBlock &src, const SCFastNameMap *ex_nm = NULL)
{
  int num = src.paramCount();
  for (int i = 0; i < num; ++i)
  {
    const char *name = src.getParamName(i);

    switch (src.getParamType(i))
    {
      case DataBlock::TYPE_INT: dest.setInt(name, src.getInt(i)); break;
      case DataBlock::TYPE_REAL: dest.setReal(name, src.getReal(i)); break;
      case DataBlock::TYPE_BOOL: dest.setBool(name, src.getBool(i)); break;

      default: sh_debug(SHLOG_WARNING, "unsupported type=%d", src.getParamType(i));
    }
  }

  num = src.blockCount();
  for (int i = 0; i < num; ++i)
  {
    const char *name = src.getBlock(i)->getBlockName();
    DataBlock *d = dest.getBlockByName(name);
    if (!d || (ex_nm && ex_nm->getNameId(name) != -1))
      d = dest.addNewBlock(src.getBlock(i), name);
    else
      mergeDataBlock(*d, *src.getBlock(i));
  }
}

static void stderr_report_fatal_error(const char *, const char *msg, const char *call_stack)
{
  ATOMIC_FPRINTF_IMM(stderr, "Fatal error: %s\n%s", msg, call_stack);
}

#if HAVE_BREAKPAD_BINDER
static void enable_breakpad()
{
  breakpad::Product p;
  p.name = "ShaderCompiler";
  p.version = shc::config().version;

  breakpad::Configuration cfg;
  cfg.userAgent = "SC";
  if (shc::config().suppressLogs)
    cfg.dumpPath = "ShaderCrashDumps";

  breakpad::init(eastl::move(p), eastl::move(cfg));
}

static bool fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  if (!breakpad::is_enabled())
    return true;

  breakpad::CrashInfo info;
  info.expression = msg;
  info.callstack = call_stack;
  info.errorCode = 0;
  info.file = file;
  info.line = line;

  breakpad::dump(info);
  return true;
}
#endif

struct StdoutBlkErrReporter : public DataBlock::IErrorReporterPipe
{
  void reportError(const char *err, bool fat) override { fputs(String(0, "[%c] %s\n", fat ? 'F' : 'E', err).str(), stdout); }
};
static bool load_blk_with_report(DataBlock &blk, const char *fn)
{
  StdoutBlkErrReporter rep;
  DataBlock::InstallReporterRAII irep(&rep);
  return dblk::load(blk, fn);
}

#if _TARGET_PC_WIN
static BOOL WINAPI ctrl_break_handler(_In_ DWORD dwCtrlType)
{
  if (dwCtrlType != CTRL_BREAK_EVENT && dwCtrlType != CTRL_C_EVENT)
    return FALSE;
#else
void ctrl_break_handler(int)
{
#endif

#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
  // We only process sigints in a dedicated thread to avoid deadlocks in shutdown.
  // (See comment below where we create sigint_handler_thread)
  // We should not be doing shutdown logic in handlers anyway, but that requires more substantial refac.

  if (!pthread_equal(sigint_handler_thread, pthread_self()))
  {
    signal(SIGINT, &ctrl_break_handler);
    pthread_kill(sigint_handler_thread, SIGINT);
    return;
  }
#endif

  // When in process loop, shutdown is locked, so instead we signal it to cancel
  if (proc::is_multiproc())
    proc::cancel();

  if (!shc::try_enter_shutdown())
  {
#if _TARGET_PC_WIN
    return TRUE;
#else
    signal(SIGINT, &ctrl_break_handler);
    return;
#endif
  }

  setvbuf(stdout, NULL, _IOLBF, 4096);
  if (shc::config().singleCompilationShName)
    ATOMIC_PRINTF_IMM("Cancelling compilation (%s)...\n", shc::config().singleCompilationShName);
  else
    ATOMIC_PRINTF_IMM("Cancelling compilation (%d processes)...\n", proc::max_proc_count());
  debug("SIGINT received!\n");

  shc::deinit_jobs();
  fflush(stdout);
  AtomicPrintfMutex::term();

  quit_game(1);

#if _TARGET_PC_WIN
  return TRUE;
#endif
}

int DagorWinMain(bool debugmode)
{
  if (dgs_execute_quiet)
    dgs_report_fatal_error = stderr_report_fatal_error;

  debug_enable_timestamps(false);
  add_restart_proc(&compiler_rproc);

  startup_game(RESTART_ALL);

#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
  // On unix signal can be handled by any thread, on windows a separate thread is spun up.
  // The unix behaviour is unacceptable in the current implementation: as we are still shutting
  // down in the handler, it a worker catches it we deadlock in jobs deinit. If the main thread
  // catches it while awaiting jobs, we get a deadlock. So, we spin up a thread that just
  // listens to signals, and SIGINTS are forwarded to it (see ctrl_break_handler).
  {
    int res = pthread_create(
      &sigint_handler_thread, nullptr,
      +[](void *) -> void * {
        for (;;)
          pause();
        return nullptr;
      },
      nullptr);
    if (res != 0)
    {
      fprintf(stderr, "Failed to create sighandler thread\n");
      return 1;
    }
  }
#endif

#if _TARGET_PC_WIN
  SetConsoleCtrlHandler(&ctrl_break_handler, TRUE);
#else
  signal(SIGINT, ctrl_break_handler);
#endif

  // get options
  if (__argc < 2)
  {
    showUsage();
    return 1;
  }

  if (dd_stricmp(__argv[1], "-h") == 0 || dd_stricmp(__argv[1], "-H") == 0 || dd_stricmp(__argv[1], "/?") == 0)
  {
    showUsage();
    return 0;
  }

  shc::CompilerConfig &globalConfigRW = shc::acquire_rw_config();

  const char *filename = __argv[1];

  // check file extension
  if (strstr(filename, ".dshl") || strstr(filename, ".sh") || strstr(filename, ".SH"))
  {
    // this is a single shader file
    globalConfigRW.singleBuild = true;
  }
  bool shouldCancelRunningProcsOnFail = true;
  eastl::optional<bool> useCppStcodeOverride{};
  for (int i = 2; i < __argc; i++)
  {
    const char *s = __argv[i];
    if (dd_stricmp(s, "-r") == 0)
      globalConfigRW.forceRebuild = true;
    else if (dd_stricmp(s, "-depBlk") == 0)
      ; // no-op
    else if (dd_stricmp(s, "-q") == 0)
    {
      globalConfigRW.shortMessages = true;
      globalConfigRW.hlslNoDisassembly = true;
      sh_console_quet_output(true);
    }
    else if (dd_stricmp(s, "-out") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.outDumpNameConfig = __argv[i];
    }
    else if (strnicmp(s, "-j", 2) == 0)
      globalConfigRW.numWorkers = atoi(s + 2);
    else if (stricmp(s, "-relinkOnly") == 0)
      globalConfigRW.relinkOnly = true;
    else if (stricmp(s, "-forceRelink") == 0)
      globalConfigRW.forceRelink = true;
    else if (dd_stricmp(s, "-nosave") == 0)
      globalConfigRW.noSave = true;
    else if (dd_stricmp(s, "-debug") == 0)
      globalConfigRW.isDebugModeEnabled = true;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    else if (dd_stricmp(s, "-pdb") == 0)
#endif
      globalConfigRW.hlslDebugLevel = DebugLevel::BASIC;
    else if (dd_stricmp(s, "-dfull") == 0 || dd_stricmp(s, "-debugfull") == 0)
      globalConfigRW.hlslDebugLevel = DebugLevel::FULL_DEBUG_INFO;
    else if (dd_stricmp(s, "-embed_source") == 0)
      globalConfigRW.hlslEmbedSource = true;
    else if (dd_stricmp(s, "-daftermath") == 0)
      globalConfigRW.hlslDebugLevel = DebugLevel::AFTERMATH;
    else if (dd_stricmp(s, "-commentPP") == 0)
      globalConfigRW.hlslSavePPAsComments = true;
    else if (dd_stricmp(s, "-codeDump") == 0)
      globalConfigRW.hlslDumpCodeAlways = true;
    else if (dd_stricmp(s, "-codeDumpSep") == 0)
      globalConfigRW.hlslDumpCodeAlways = globalConfigRW.hlslDumpCodeSeparate = true;
    else if (dd_stricmp(s, "-codeDumpErr") == 0)
      globalConfigRW.hlslDumpCodeOnError = true;
    else if (dd_stricmp(s, "-validateIdenticalBytecode") == 0)
      globalConfigRW.validateIdenticalBytecode = true;
    else if (dd_stricmp(s, "-w") == 0)
    {
      globalConfigRW.hlslShowWarnings = true;
      sh_console_print_warnings(true);
    }
    else if (dd_stricmp(s, "-O0") == 0)
      globalConfigRW.hlslOptimizationLevel = 0;
    else if (dd_stricmp(s, "-O1") == 0)
      globalConfigRW.hlslOptimizationLevel = 1;
    else if (dd_stricmp(s, "-O2") == 0)
      globalConfigRW.hlslOptimizationLevel = 2;
    else if (dd_stricmp(s, "-O3") == 0)
      globalConfigRW.hlslOptimizationLevel = 3;
    else if (dd_stricmp(s, "-O4") == 0)
      globalConfigRW.hlslOptimizationLevel = 4;
    else if (dd_stricmp(s, "-maxVSF") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.hlslMaximumVsfAllowed = atoi(__argv[i]);
    }
    else if (dd_stricmp(s, "-maxPSF") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.hlslMaximumPsfAllowed = atoi(__argv[i]);
    }
    else if (dd_stricmp(s, "-skipvalidation") == 0)
    {
      globalConfigRW.hlslSkipValidation = true;
    }
    else if (dd_stricmp(s, "-nodisassembly") == 0)
      globalConfigRW.hlslNoDisassembly = true;
    else if (dd_stricmp(s, "-shaderOn") == 0)
      shc::setRequiredShadersDef(true);
    else if (dd_stricmp(s, "-supressLogs") == 0 || dd_stricmp(s, "-suppressLogs") == 0)
      globalConfigRW.suppressLogs = true;
    else if (dd_stricmp(s, "-shadervar-generator-mode") == 0)
    {
      using namespace std::string_view_literals;
      i++;
      if (i >= __argc)
        goto usage_err;
      if (__argv[i] == "none"sv)
        globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::None;
      else if (__argv[i] == "remove"sv)
        globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::Remove;
      else if (__argv[i] == "check"sv)
        globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::Check;
      else if (__argv[i] == "generate"sv)
        globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::Generate;
      else
        printf("\n[WARNING] Unknown shadervar generator mode '%s'\n", __argv[i]);
    }
    else if (dd_stricmp(s, "-invalidAsNull") == 0)
      shc::setInvalidAsNullDef(true);
    else if (dd_stricmp(s, "-no_sha1_cache") == 0)
    {
      globalConfigRW.useSha1Cache = globalConfigRW.writeSha1Cache = false;
    }
    else if (dd_stricmp(s, "-no_compression") == 0)
    {
      globalConfigRW.useCompression = false;
    }
    else if (dd_stricmp(s, "-addTextureType") == 0)
    {
      globalConfigRW.addTextureType = true;
    }
    else if (dd_stricmp(s, "-purge_sha1_cache") == 0)
    {
      globalConfigRW.purgeSha1 = true;
    }
#if _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL
    else if (strnicmp(s, "-enableBindless:", 15) == 0)
    {
      if (strstr(s, "on"))
        globalConfigRW.enableBindless = true;
      else if (strstr(s, "off"))
        globalConfigRW.enableBindless = false;
    }
#endif
#if _CROSS_TARGET_DX12
    else if (0 == dd_stricmp(s, "-platform-pc"))
    {
      globalConfigRW.targetPlatform = dx12::dxil::Platform::PC;
      globalConfigRW.crossCompiler = "dx12";
    }
    else if (0 == dd_stricmp(s, "-platform-xbox-one"))
    {
      globalConfigRW.targetPlatform = dx12::dxil::Platform::XBOX_ONE;
      globalConfigRW.crossCompiler = "xbox_one_dx12";
    }
    else if (0 == dd_stricmp(s, "-platform-xbox-scarlett"))
    {
      globalConfigRW.targetPlatform = dx12::dxil::Platform::XBOX_SCARLETT;
      globalConfigRW.crossCompiler = "scarlett_dx12";
    }
    else if (0 == dd_stricmp(s, "-HLSL2021"))
    {
      globalConfigRW.hlsl2021 = true;
    }
    else if (0 == dd_stricmp(s, "-enablefp16"))
    {
      globalConfigRW.enableFp16 = true;
    }
    else if (0 == dd_stricmp(s, "-autotestMode"))
    {
      globalConfigRW.autotestMode = true;
    }
#endif
#if _CROSS_TARGET_SPIRV
    else if (dd_stricmp(s, "-compiler-hlslcc") == 0)
      globalConfigRW.compilerHlslCc = true;
    else if (dd_stricmp(s, "-compiler-dxc") == 0)
      globalConfigRW.compilerDXC = true;
#endif
#ifdef _CROSS_TARGET_METAL
    else if (dd_stricmp(s, "-metalios") == 0)
    {
      globalConfigRW.useIosToken = true;
      globalConfigRW.crossCompiler = "metal-ios";
    }
    else if (dd_stricmp(s, "-metal-binary") == 0)
    {
      globalConfigRW.useBinaryMsl = true;
      globalConfigRW.crossCompiler = "metal-binary";
    }
#endif
#if _CROSS_TARGET_METAL || _CROSS_TARGET_SPIRV
    else if (dd_stricmp(s, "-debugdir") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      debug_output_dir = __argv[i];
      dd_mkdir(debug_output_dir);
    }
#endif
    else if (dd_stricmp(s, "-wx") == 0)
    {
      sh_change_mode(SHLOG_WARNING, SHLOG_ERROR);
      globalConfigRW.hlslWarningsAsErrors = true;
    }
    else if (dd_stricmp(s, "-wall") == 0)
    {
      sh_change_mode(SHLOG_SILENT_WARNING, SHLOG_WARNING);
      globalConfigRW.hlslShowWarnings = true;
      sh_console_print_warnings(true);
    }
    else if (strnicmp(s, "-fsh:", 5) == 0)
    {
      if (strstr(s, ":3.0"))
        globalConfigRW.singleOptions.fshVersion = 3.0_sm;
#if _CROSS_TARGET_DX11 || _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL | _CROSS_TARGET_EMPTY
      else if (strstr(s, ":4.0"))
        globalConfigRW.singleOptions.fshVersion = 4.0_sm;
      else if (strstr(s, ":4.1"))
        globalConfigRW.singleOptions.fshVersion = 4.1_sm;
      else if (strstr(s, ":5.0"))
        globalConfigRW.singleOptions.fshVersion = 5.0_sm;
      else if (strstr(s, ":6.0"))
        globalConfigRW.singleOptions.fshVersion = 6.0_sm;
      else if (strstr(s, ":6.6"))
        globalConfigRW.singleOptions.fshVersion = 6.6_sm;
#endif
      else
      {
        show_header();
        printf("\n[FATAL ERROR] Unknown fsh version '%s'\n", s);
        showUsage();
        return 13;
      }
    }
    else if (dd_stricmp(s, "-noeh") == 0)
      ; // no-op
    else if (dd_stricmp(s, "-quiet") == 0)
      ; // no-op
    else if (dd_stricmp(s, "-pdb") == 0)
      ; // no-op
    else if (dd_stricmp(s, "-o") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.intermediateDir = __argv[i];
    }
    else if (dd_stricmp(s, "-logdir") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.logDir = __argv[i];
    }
    else if (dd_stricmp(s, "-c") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.singleCompilationShName = __argv[i];
    }
    else if (stricmp(s, "-cjWait") == 0)
      shouldCancelRunningProcsOnFail = false;
    else if (strnicmp(s, "-cj", 3) == 0)
      globalConfigRW.numProcesses = strlen(s) > 3 ? atoi(s + 3) : -1;
    else if (dd_stricmp(s, "-belownormal") == 0)
    {
#if _TARGET_PC_WIN
      SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#endif
    }
    else if (dd_stricmp(s, "-define") == 0)
    {
      i += 2;
      if (i >= __argc)
        goto usage_err;
      globalConfigRW.hlslDefines.aprintf(128, "#define %s %s\n", __argv[i - 1], __argv[i]);

// @TODO: replace w/ dedicated arg, hardware. token, and make this define in hardware_defines.dshl
#if _CROSS_TARGET_SPIRV
      if (dd_stricmp(__argv[i - 1], "MOBILE_DEVICE") == 0 && dd_stricmp(__argv[i], "1") == 0)
        globalConfigRW.usePcToken = false;
#endif
    }
    else if (dd_stricmp(s, "-optionalAsBranches") == 0)
    {
      globalConfigRW.optionalIntervalsAsBranches = true;
    }
    else if (dd_stricmp(s, "-logExactTiming") == 0)
    {
      globalConfigRW.logExactCompilationTimes = true;
    }
    else if (dd_stricmp(s, "-perFileAllLogs") == 0)
    {
      globalConfigRW.logFullPerFileCompilationStats = true;
    }
    else if (strnicmp("-addpath:", __argv[i], 9) == 0)
      ; // skip
    else if (dd_stricmp(s, "-saveDumpOnCrash") == 0)
    {
      globalConfigRW.saveDumpOnCrash = true;
    }
    else if (strnicmp("-useCpujobsBackend", __argv[i], 18) == 0)
    {
      globalConfigRW.useThreadpool = false;
    }
    else if (strnicmp("-cppStcode", __argv[i], 11) == 0)
    {
      if (useCppStcodeOverride.has_value() && !useCppStcodeOverride.value())
      {
        printf("Can't combine -cppStcode and -noCppStcode flags\n");
        goto usage_err;
      }
      useCppStcodeOverride.emplace(true);
    }
    else if (strnicmp("-noCppStcode", __argv[i], 13) == 0)
    {
      if (useCppStcodeOverride.has_value() && useCppStcodeOverride.value())
      {
        printf("Can't combine -cppStcode and -noCppStcode flags\n");
        goto usage_err;
      }
      useCppStcodeOverride.emplace(false);
    }
    else if (strnicmp("-cppUnityBuild", __argv[i], 15) == 0)
    {
      globalConfigRW.cppStcodeUnityBuild = true;
    }
    else if (strnicmp("-cppStcodePdb", __argv[i], 14) == 0)
    {
      globalConfigRW.cppStcodeDeleteDebugInfo = false;
    }
    else if (strnicmp("-cppStcodeArch", s, 15) == 0)
    {
      ++i;
      if (i >= __argc)
        goto usage_err;
      if (!set_stcode_arch_from_arg(__argv[i], globalConfigRW))
        goto usage_err;
    }
#if _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
    else if (strnicmp("-cppStcodePlatform", s, 20) == 0)
    {
      ++i;
      if (i >= __argc)
        goto usage_err;
      if (!set_stcode_platform_from_arg(__argv[i], globalConfigRW))
        goto usage_err;
    }
#endif
#if _CROSS_TARGET_DX12
    else if (dd_stricmp(s, "-localTimestamp") == 0)
    {
      globalConfigRW.useGitTimestamps = false;
    }
#endif
    else if (dd_stricmp(s, "-noHlslHardcodedRegs") == 0)
      globalConfigRW.disallowHlslHardcodedRegs = true;
    else
    {
    usage_err:
      show_header();
      printf("\n[FATAL ERROR] Unknown option '%s'\n", s);
      showUsage();
      return 13;
    }
  }

#if HAVE_BREAKPAD_BINDER
  if (shc::config().saveDumpOnCrash)
  {
    dgs_fatal_handler = fatal_handler;
    enable_breakpad();
  }
#endif

  globalConfigRW.useSha1Cache &= (shc::config().hlslDebugLevel == DebugLevel::NONE) && shc::config().hlslNoDisassembly;
  if (shc::config().singleCompilationShName)
    globalConfigRW.numProcesses = 0;
  else
    printf("hlslOptimizationLevel is set to %d\n", shc::config().hlslOptimizationLevel);

#if _CROSS_TARGET_EMPTY
  noSave = true;
#endif

  if (!shc::config().shortMessages && !shc::config().singleCompilationShName)
    show_header();

  shc::init_jobs(shc::config().numWorkers);
  DEFER(shc::deinit_jobs());

  proc::init(shc::config().numProcesses, shouldCancelRunningProcsOnFail);
  DEFER(proc::deinit());

  if (shc::config().singleCompilationShName || proc::is_multiproc())
    setvbuf(stdout, NULL, _IOFBF, 4096);

  ErrorCounter::allCompilations().reset();
  int timeMsec = get_time_msec();

  ShaderParser::renderStageToIdxMap.reset();
  ShaderParser::renderStageToIdxMap.addNameId("opaque");
  ShaderParser::renderStageToIdxMap.addNameId("atest");
  ShaderParser::renderStageToIdxMap.addNameId("imm_decal");
  ShaderParser::renderStageToIdxMap.addNameId("decal");
  ShaderParser::renderStageToIdxMap.addNameId("trans");
  ShaderParser::renderStageToIdxMap.addNameId("distortion");
  if (shc::config().singleBuild)
  {
    // compile single file
    ShaderCompilerStat::reset();
    Tab<String> sourceFiles(midmem);
    sourceFiles.push_back(String(filename));
    compile(sourceFiles, filename, makeShBinDumpName(filename), shc::config().singleOptions, NULL, getDir(filename), NULL,
      BindumpPackingFlagsBits::NONE, globalConfigRW);
  }
  else
  {
    if (shc::config().singleCompilationShName || proc::is_multiproc())
      AtomicPrintfMutex::init(dd_get_fname(__argv[0]), filename);

    // read data block with files
    const String cfgDir(getDir(filename));
    DataBlock blk;
    if (!load_blk_with_report(blk, filename))
    {
      printf("\n[FATAL ERROR] Cannot open BLK file '%s' (file not found or syntax error, or unresolved includes)\n", filename);
      return 13;
    }
    if (const DataBlock *b = blk.getBlockByName("renderStages"))
    {
      ShaderParser::renderStageToIdxMap.reset();
      for (int i = 0, nid = b->getNameId("stage"); i < b->paramCount(); i++)
        if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
        {
          int nc = ShaderParser::renderStageToIdxMap.nameCount();
          ShaderParser::renderStageToIdxMap.addNameId(b->getStr(i));
          if (nc == ShaderParser::renderStageToIdxMap.nameCount())
          {
            printf("\n[FATAL ERROR] duplicate stage <%s> in renderStages{} in <%s> ?\n", b->getStr(i), filename);
            return 13;
          }
          if (!shc::config().singleCompilationShName)
            sh_debug(SHLOG_NORMAL, "[info] render_stage \"%s\"=%d", b->getStr(i),
              ShaderParser::renderStageToIdxMap.getNameId(b->getStr(i)));
        }
    }
    if (ShaderParser::renderStageToIdxMap.nameCount() > SC_STAGE_IDX_MASK + 1)
    {
      printf("\n[FATAL ERROR] too many render stages (%d) > %d", ShaderParser::renderStageToIdxMap.nameCount(), SC_STAGE_IDX_MASK + 1);
      return 13;
    }

    reset_shaders_inc_base();

    Tab<eastl::string> mount_points;
    mount_points.push_back(".");
    for (int i = 0, nid = blk.getNameId("mountPoint"); i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
        mount_points.emplace_back(blk.getStr(i));

    for (int i = 0, nid = blk.getNameId("incDir"); i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
      {
        auto param = blk.getStr(i);
        eastl::string folder;
        for (const auto &mount_point : mount_points)
        {
          folder = mount_point + "/" + param;
          if (dd_dir_exists(folder.c_str()))
          {
            add_shaders_inc_base(folder.c_str());
            add_include_path(folder.c_str(), globalConfigRW);
            if (!shc::config().singleCompilationShName)
              sh_debug(SHLOG_INFO, "Using include dir: %s", folder.c_str());
          }
        }
      }

    if (shc::config().outDumpNameConfig)
      blk.setStr("outDumpName", shc::config().outDumpNameConfig);
    String defShBindDumpPrefix(makeShBinDumpName(filename));
    if (blk.getStr("outDumpName", NULL))
      defShBindDumpPrefix = blk.getStr("outDumpName", NULL);

    globalConfigRW.compileCppStcode =
      useCppStcodeOverride.has_value() ? useCppStcodeOverride.value() : blk.getBool("compileCppStcode", false);

    Tab<String> sourceFiles(midmem);
    DataBlock *sourceBlk = blk.getBlockByName("source");
    const char *shader_root = blk.getStr("shader_root_dir", NULL);
    globalConfigRW.shaderSrcRoot = shader_root;
    if (!sourceBlk)
    {
      printf("Shader source files should be copied from shaders.sh\n"
             "to shaders.blk as a source block with file:t parameters\n"
             "Headers should NOT be copied.");
      return 13;
    }

    globalConfigRW.engineRootDir = blk.getStr("engineRootDir", nullptr);

    int fileParamId = sourceBlk->getNameId("file");
    int includePathId = sourceBlk->getNameId("includePath");
    for (unsigned int paramNo = 0; paramNo < sourceBlk->paramCount(); paramNo++)
    {
      if (sourceBlk->getParamType(paramNo) == DataBlock::TYPE_STRING)
      {
        if (sourceBlk->getParamNameId(paramNo) == fileParamId)
        {
          if (shader_root)
            sourceFiles.push_back(String(260, "%s/%s", shader_root, sourceBlk->getStr(paramNo)));
          else
            sourceFiles.push_back(String(sourceBlk->getStr(paramNo)));
        }
        else if (sourceBlk->getParamNameId(paramNo) == includePathId)
          add_include_path(sourceBlk->getStr(paramNo), globalConfigRW);
      }
    }
    fflush(stdout);

    shc::clearFlobVarRefList();
    DataBlock *blk_gvr = blk.getBlockByName("explicit_var_ref");
    if (blk_gvr)
    {
      int nid = blk.getNameId("ref");
      for (int i = 0; i < blk_gvr->paramCount(); i++)
        if (blk_gvr->getParamNameId(i) == nid && blk_gvr->getParamType(i) == DataBlock::TYPE_STRING)
          shc::addExplicitGlobVarRef(blk_gvr->getStr(i));
    }

    DataBlock *blk_cav = blk.getBlockByName("common_assume_vars");
    DataBlock *blk_crs = blk.getBlockByName("common_required_shaders");
    DataBlock *blk_cvv = blk.getBlockByName("common_valid_variants");
    DataBlock *blk_svg = blk.getBlockByName("shadervar_generator");
    if (!blk_cav && blk.getBlockByName("common_assume_static"))
    {
      sh_debug(SHLOG_WARNING, "use '%s' block name instead of obsolete '%s'!", "common_assume_vars", "common_assume_static");
      blk_cav = blk.getBlockByName("common_assume_static");
    }
    if (!blk_crs && blk.getBlockByName("common_assume_no_shaders"))
    {
      sh_debug(SHLOG_WARNING, "use '%s' block name instead of obsolete '%s'!", "common_required_shaders", "common_assume_no_shaders");
      blk_crs = blk.getBlockByName("common_assume_no_shaders");
    }
    if (blk_cav && blk_cav->paramCount() == 0 && blk_cav->blockCount() == 0)
      blk_cav = NULL;
    if (blk_crs && blk_crs->paramCount() == 0)
      blk_crs = NULL;
    if (blk_cvv && blk_cvv->blockCount() == 0)
      blk_cvv = NULL;
    if (blk_svg)
    {
      globalConfigRW.shadervarsCodeTemplateFilename = blk_svg->getStr("code_template");
      for (int b = blk_svg->findBlock("generated_path"); b != -1; b = blk_svg->findBlock("generated_path", b))
      {
        DataBlock *blk_gp = blk_svg->getBlock(b);
        if (!blk_gp)
        {
          printf("\n[ERROR] Couldn't get block 'generated_path[%i]'\n", b);
          continue;
        }
        auto &info = globalConfigRW.generatedPathInfos.emplace_back();
        info.matcher = blk_gp->getStr("matcher");
        info.replacer = blk_gp->getStr("replacer");
      }

      for (int p = blk_svg->findParam("exclude"); p != -1; p = blk_svg->findParam("exclude", p))
        globalConfigRW.excludeFromGeneration.emplace_back(blk_svg->getStr(p));

      if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::None)
      {
        using namespace std::string_view_literals;
        const char *mode = blk_svg->getStr("mode", "none");
        if (mode == "none"sv)
          globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::None;
        else if (mode == "remove"sv)
          globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::Remove;
        else if (mode == "check"sv)
          globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::Check;
        else if (mode == "generate"sv)
          globalConfigRW.shadervarGeneratorMode = ShadervarGeneratorMode::Generate;
        else
          printf("\n[WARNING] Unknown shadervar generator mode '%s'\n", mode);
      }
    }

    const int compNameId = blk.getNameId("Compile");
    for (int i = 0; i < blk.blockCount(); i++)
    {
      DataBlock *comp = blk.getBlock(i);
      if (comp->getBlockNameId() == compNameId)
      {
        // compile file
        String fName(comp->getStr("output", NULL));
        if (!fName.length())
        {
          fName = makeShBinDumpName(filename);
          fName += ".sh";
        }

        if (fName[0] == '\\' || fName[0] == '/')
          fName = cfgDir + fName;

        globalConfigRW.dictionarySizeInKb = comp->getInt("dict_size_in_kb", shc::config().dictionarySizeInKb);
        globalConfigRW.shGroupSizeInKb = comp->getInt("group_size_in_kb", shc::config().shGroupSizeInKb);

        ShHardwareOptions opt(4.0_sm);

        const char *fsh = comp->getStr("fsh", NULL);
        if (fsh)
        {
          if (strstr(fsh, "3.0"))
            opt.fshVersion = 3.0_sm;
          else if (strstr(fsh, "4.0"))
            opt.fshVersion = 4.0_sm;
          else if (strstr(fsh, "4.1"))
            opt.fshVersion = 4.1_sm;
          else if (strstr(fsh, "5.0"))
            opt.fshVersion = 5.0_sm;
          else if (strstr(fsh, "6.0"))
            opt.fshVersion = 6.0_sm;
          else if (strstr(fsh, "6.6"))
            opt.fshVersion = 6.6_sm;
#if _CROSS_TARGET_SPIRV
          else if (strstr(fsh, "SpirV"))
            opt.fshVersion = 5.0_sm;
#endif
#if _CROSS_TARGET_METAL
          else if (strstr(fsh, "Metal"))
            opt.fshVersion = 5.0_sm;
#endif
          else
          {
            sh_debug(SHLOG_ERROR, "Invalid FSH version '%s' in config.blk '%s' section %d", fsh, filename, i);
            return 13;
          }
        }
        opt.enableHalfProfile = comp->getBool("enableHalfProfile", true) || shc::config().enableFp16;

        DataBlock *blk_av = comp->getBlockByName("assume_vars");
        DataBlock *blk_rs = comp->getBlockByName("required_shaders");
        DataBlock *blk_vv = comp->getBlockByName("valid_variants");
        if (!blk_av && comp->getBlockByName("assume_static"))
        {
          sh_debug(SHLOG_WARNING, "use '%s' block name instead of obsolete '%s'!", "assume_vars", "assume_static");
          blk_av = comp->getBlockByName("assume_static");
        }
        if (!blk_rs && comp->getBlockByName("assume_no_shaders"))
        {
          sh_debug(SHLOG_WARNING, "use '%s' block name instead of obsolete '%s'!", "required_shaders", "assume_no_shaders");
          blk_rs = comp->getBlockByName("assume_no_shaders");
        }

        if (!blk_av && blk_cav)
          blk_av = comp->addNewBlock("assume_vars");
        if (!blk_rs && blk_crs)
          blk_rs = comp->addNewBlock("required_shaders");
        if (!blk_vv && blk_cvv)
          blk_vv = comp->addNewBlock("valid_variants");

        if (blk_av && blk_cav)
        {
          mergeDataBlock(*blk_av, *blk_cav);
          // blk_av->saveToTextFile(String(260,"%d_%s_assume_vars", i, fsh));
        }
        if (blk_rs && blk_crs)
        {
          mergeDataBlock(*blk_rs, *blk_crs);
          // blk_rs->saveToTextFile(String(260,"%d_%s_required_shaders", i, fsh));
        }
        if (blk_vv && blk_cvv)
        {
          SCFastNameMap names;
          names.addNameId("invalid");
          mergeDataBlock(*blk_vv, *blk_cvv, &names);
          // blk_av->saveToTextFile(String(260,"%d_%s_valid_variants", i, fsh));
        }

        if (blk_av && !blk_av->paramCount() && !blk_av->blockCount())
          blk_av = NULL;
        if (blk_rs && !blk_rs->paramCount())
          blk_rs = NULL;
        if (blk_vv && !blk_vv->blockCount())
          blk_vv = NULL;
        shc::setAssumedVarsBlock(blk_av);
        shc::setValidVariantsBlock(blk_vv);
        shc::setRequiredShadersBlock(blk_rs);

        ShaderCompilerStat::reset();

        BindumpPackingFlags packingFlags = 0;
        if (shc::config().useCompression)
        {
          if (blk.getBool("packShGroups", true)) // Pack groups by default
            packingFlags |= BindumpPackingFlagsBits::SHADER_GROUPS;
          if (blk.getBool("packBin", false)) // Don't further compress bindump by default
            packingFlags |= BindumpPackingFlagsBits::WHOLE_BINARY;
        }
        compile(sourceFiles, fName, comp->getStr("outDumpName", defShBindDumpPrefix), opt, filename, getDir(filename),
          blk.getStr("outMiniDumpName", NULL), packingFlags, globalConfigRW);
      }
    }
  }

  if (shc::config().singleCompilationShName)
  {
    float timeSec = float(get_time_msec() - timeMsec) * 1e-3;
    int wholeSeconds = int(timeSec);
    String compCountMsg("");
    if (shc::config().logFullPerFileCompilationStats)
    {
      int uniqueCompilationCount = ShaderCompilerStat::getUniqueCompilationCount();
      compCountMsg.aprintf(64, " (%u compilation%s)", uniqueCompilationCount, uniqueCompilationCount == 1 ? "" : "s");
    }
    if (shc::config().logFullPerFileCompilationStats || wholeSeconds > 5)
    {
      if (shc::config().logExactCompilationTimes)
        sh_debug(SHLOG_NORMAL, "[INFO]      done '%s' for %.4gs%s", shc::config().singleCompilationShName, timeSec, compCountMsg);
      else
      {
        if (wholeSeconds > 60)
          sh_debug(SHLOG_NORMAL, "[INFO]      done '%s' for %dm:%02ds%s", shc::config().singleCompilationShName, wholeSeconds / 60,
            wholeSeconds % 60, compCountMsg);
        else
          sh_debug(SHLOG_NORMAL, "[INFO]      done '%s' for %ds%s", shc::config().singleCompilationShName, wholeSeconds, compCountMsg);
      }
    }
    fflush(stdout);
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#endif
    return 0;
  }

  printf("\n");
  float timeSec = float(get_time_msec() - timeMsec) * 1e-3;
  int wholeSeconds = int(timeSec);
  if (shc::config().logExactCompilationTimes && wholeSeconds > 1)
    printf("took %.4gs\n", timeSec);
  else
  {
    if (wholeSeconds > 60)
      printf("took %dm:%02ds\n", wholeSeconds / 60, wholeSeconds % 60);
    else if (wholeSeconds > 1)
      printf("took %ds\n", wholeSeconds);
  }

  if (!::dgs_execute_quiet && ::getenv("DAGOR_BUILD_BELL") != nullptr)
  {
    if (void *console = get_console_window_handle())
      flash_window(console, true);

#if _TARGET_PC_WIN
    // Sends the BEL signal to the Windows Terminal app, which is default in latest versions of Windows.
    // The Windows Terminal app is not reacting to FlashWindowEx() API, so we need to send the BEL signal
    // to get the user's attention.
    printf("\a");
#endif
  }

  return 0;
}
