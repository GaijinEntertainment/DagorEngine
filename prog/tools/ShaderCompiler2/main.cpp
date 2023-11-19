#include <sys/stat.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <stdio.h>
#include <signal.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/shader_ver.h>
#include "sha1_cache_version.h"
#include "shLog.h"
#include "shCompiler.h"
#include "shadervarGenerator.h"
#include <ioSys/dag_io.h>
#include "shCacheVer.h"
// #include <shaders/shOpcodeUnittest.h>
#include "sh_stat.h"
#include "loadShaders.h"
#include "assemblyShader.h"
#include <stdlib.h>
#include <time.h>
#include "nameMap.h"
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>

#include <libTools/util/hash.h>

#include "DebugLevel.h"
#include <libTools/util/atomicPrintf.h>

#if _TARGET_PC_WINDOWS
#include <windows.h>
#else

#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

#if _TARGET_PC_MACOSX
#include <AvailabilityMacros.h>
#endif

extern int __argc;
extern char **__argv;
char **environ;

typedef struct
{
  pid_t hProcess;
} PROCESS_INFORMATION;

#define DWORD        uint32_t
#define STILL_ACTIVE (~0u)
#define INFINITE     (~0u)
#define NOT_ACTIVE   0

void TerminateProcess(pid_t process, int ret_code) { kill(process, SIGINT); }

void WaitForMultipleObjects(uint32_t count, pid_t *pids, bool, uint32_t)
{
  for (uint32_t i = 0; i < count; ++i)
  {
    int status = 0;
    waitpid(pids[i], &status, 0);
  }
}

bool GetExitCodeProcess(pid_t process, DWORD *ret_code)
{
  int status = 0;
  int ret = waitpid(process, &status, WNOHANG);

  *ret_code = STILL_ACTIVE;
  if (ret >= 0 && WIFEXITED(status))
    *ret_code = WEXITSTATUS(status);
  if (ret == -1 && errno == ECHILD)
    *ret_code = NOT_ACTIVE, ret = 0;

  return ret >= 0;
}

void SleepEx(uint32_t ms, bool) { usleep(ms * 1000); }
#endif


#if _CROSS_TARGET_C1


#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
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

size_t dictionary_size_in_kb = 4096;
size_t sh_group_size_in_kb = 1024;
ShadervarGeneratorMode shadervar_generator_mode = ShadervarGeneratorMode::None;
std::string shadervars_code_template_filename;
GeneratedPathInfos generated_path_infos;
std::vector<std::string> exclude_from_generation;

extern int getShaderCacheVersion();
extern void reset_shaders_inc_base();
extern void add_shaders_inc_base(const char *base);
static Tab<PROCESS_INFORMATION> childCompilatioProcessInfo;
static Tab<SimpleString> childCompilatioTargetObj;
static WinCritSec *termCC = NULL;

#ifdef _CROSS_TARGET_METAL
bool use_ios_token = false;
bool use_binary_msl = false;
bool use_metal_glslang = false;
#endif
#if _CROSS_TARGET_METAL || _CROSS_TARGET_SPIRV
extern const char *debug_output_dir;
#endif

static void terminateChildProcesses()
{
  WinAutoLock lock(*termCC);
  if (!childCompilatioProcessInfo.size())
    return;

  debug_dump_stack("terminateChildProcesses");
  ATOMIC_PRINTF("--- Terminating child processes...\n");
  fflush(stdout);
  for (int j = 0; j < childCompilatioProcessInfo.size(); j++)
  {
    DWORD ret_code = -1;
    if (GetExitCodeProcess(childCompilatioProcessInfo[j].hProcess, &ret_code))
    {
      if (ret_code != STILL_ACTIVE)
      {
#if _TARGET_PC_WIN
        CloseHandle(childCompilatioProcessInfo[j].hThread);
        CloseHandle(childCompilatioProcessInfo[j].hProcess);
#endif
        mem_set_0(make_span(childCompilatioProcessInfo).subspan(j, 1));
      }
      if (ret_code == 0)
        childCompilatioTargetObj[j] = NULL;
    }
#if _TARGET_PC_WIN
    if (childCompilatioProcessInfo[j].dwProcessId)
      GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, childCompilatioProcessInfo[j].dwProcessId);
#else
    kill(childCompilatioProcessInfo[j].hProcess, SIGINT);
#endif
  }

  for (int i = 0; i < 100; i++)
  {
    int wait = 0;
    for (int j = 0; j < childCompilatioProcessInfo.size(); j++)
    {
      DWORD ret_code = -1;
      if (GetExitCodeProcess(childCompilatioProcessInfo[j].hProcess, &ret_code))
        if (ret_code == STILL_ACTIVE)
          wait++;
    }
    if (!wait)
    {
      ATOMIC_PRINTF("--- Wait done!\n");
      fflush(stdout);
      break;
    }
    SleepEx(100, false);
  }
  for (int j = 0; j < childCompilatioProcessInfo.size(); j++)
  {
    DWORD ret_code = 0;
    if (GetExitCodeProcess(childCompilatioProcessInfo[j].hProcess, &ret_code))
      if (ret_code == STILL_ACTIVE)
      {
        ATOMIC_PRINTF("terminating child[%d]\n", j);
        TerminateProcess(childCompilatioProcessInfo[j].hProcess, 13);
        mem_set_0(make_span(childCompilatioProcessInfo).subspan(j, 1));
      }
  }
  // childCompilatioProcessInfo.clear();

  for (int j = 0; j < childCompilatioTargetObj.size(); j++)
  {
    const char *nm = childCompilatioTargetObj[j];
    if (*nm && dd_file_exists(nm))
      ATOMIC_PRINTF("%s %s\n", unlink(nm) == 0 ? "removed" : "ERR: failed to remove", nm);
  }
  // childCompilatioTargetObj.clear();
}

#define QUOTE(x) #x
#define STR(x)   QUOTE(x)
static void show_header()
{
  printf("Dagor Shader Compiler 2.72 (dump format '%c%c%c%c', obj format '%c%c%c%c', shader cache ver 0x%x)\n"
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
    _DUMP4C(SHADER_BINDUMP_VER), _DUMP4C(SHADER_CACHE_VER), sha1_cache_version
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

String hlsl_defines;
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_METAL
const char *cross_compiler = "metal";
#elif _CROSS_TARGET_SPIRV
const char *cross_compiler = "spirv";
#elif _CROSS_TARGET_EMPTY
const char *cross_compiler = "stub";
#elif _CROSS_TARGET_DX12
const char *cross_compiler = "dx12";
#elif _CROSS_TARGET_DX11 //_TARGET_PC is also defined
const char *cross_compiler = "dx11";
#endif

static void showUsage()
{
  show_header();
  printf(
    "\nUsage:\n"
    "  dsc2-dev.exe <config blk-file|single *.sh file> <optional options>\n"
    "\n"
    "Common options:\n"
    "  -no_sha1_cache - do not use sha1 cache\n"
    "  -no_compression - do not use compression for shaders (default: false)\n"
    "  -purge_sha1_cache - delete sha1 cache (if no_sha1_cache is not enabled, it will be re-populated)\n"
    "  -commentPP - save PP as comments (for shader debugging)\n"
    "  -r - force rebuild all files (even if not modified)\n"
    "  -bones_start N - starting register for bones. If -1 (default) allocates as other named consts\n"
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
    "  -pdb - insert debug info during HLSL compile\n"
    "  -dfull | -debugfull - for Dx11 and Dx12, it produces unoptimized shaders with full debug info,\n"
    "   including shader source, so shader debugging will have the HLSL source. It implies -pdb.\n"
    "  -daftermath - for DX12, it produces shader binaries and pdb/cso files for detailed resolve of\n"
    "   Aftermath gpu dumps by Nsight Graphics\n"
    "  -w - show compilation warnings (hidden by default)\n"
    "  -ON - HLSL optimization level (N==0..4, default =4)\n"
    "  -skipvalidation - do not validate the generated code against known capabilities"
    " and constraints\n"
    "  -nodisassembly - no hlsl disassembly output\n"
    "  -sanitize_hash - sanitize hash of strings\n"
    "  -no_sanitize_hash - not sanitize hash\n"
    "  -codeDump  - always dump hlsl/Cg source code to be compiled to shaderlog\n"
    "  -codeDumpErr - dump hlsl/Cg source code to shaderlog when error occurs\n"
    "  -shaderOn - default shaders are on\n"
    "  -invalidAsNull - by default invalid variants are marked NULL (just as dont_render)\n"
    "  -o <intermediate dir> - path to store obj files.\n"
    "  -c <source.sh>  - compile only one shader file (using all settings in BLK as usual)\n"
    "  -logdir <dir>   - path to logs\n"
    "  -supressLogs    - do not make fileLogs\n"
    "  -enablefp16     - enable using 16 bit types in shaders\n"
    "  -HLSL2021       - use the HLSL 2021 version of the language\n"
#if _CROSS_TARGET_SPIRV
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
    "Options for !!SINGLE!! (not config BLK!!!) *.SH file rebuild:\n"
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
    "  -metal-use-glslang  - build with GLSLang (instead of DXC, which is default)\n"
#endif
#if _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL
    "  -debugdir DIR   - use DIR to store debuginfo for shader debugger\n"
#endif

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#else
    "  -fsh:<1.1|1.3|1.4|2.0|2.b|2.a|3.0> - set fsh version (<2.0> by default)\n"
#endif
  );
}

static SimpleString shaderSrcRoot;
const char *shc::getSrcRootFolder() { return shaderSrcRoot.empty() ? NULL : shaderSrcRoot.str(); }

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
#if _CROSS_TARGET_DX12
wchar_t *dx12_pdb_cache_dir = nullptr;
#endif
static bool forceRebuild = false;
static bool shortMessages = false;
static bool noSave = false;
bool hlslSavePPAsComments = false;
bool isDebugModeEnabled = false;
DebugLevel hlslDebugLevel = DebugLevel::NONE;
bool hlslSkipValidation = false;
bool hlslNoDisassembly = false;
bool hlslShowWarnings = false;
bool hlslWarningsAsErrors = false;
bool hlslDumpCodeAlways = false, hlslDumpCodeOnError = false;
bool hlsl2021 = false;
bool enableFp16 = false;
bool useCompression = true;
#if USE_BINDLESS_FOR_STATIC_TEX
bool enableBindless = true;
#else
bool enableBindless = false;
#endif
int hlslOptimizationLevel = 4;
static const char *intermediate_dir = NULL, *compile_only_sh = NULL;
static const char *log_dir = NULL;
bool supressLogs = false;
static int compilation_job_count = -1;
static bool wait_compilation_jobs = false;
int hlsl_maximum_vsf_allowed = 2048;
int hlsl_maximum_psf_allowed = 2048;
int hlsl_bones_base_reg = -1;
bool optionalIntervalsAsBranches = false;
#if _CROSS_TARGET_SPIRV
bool compilerHlslCc = false;
bool compilerDXC = false;
#endif
#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
dx12::dxil::Platform targetPlatform = dx12::dxil::Platform::PC;
#endif

static void doCompileModulesAsync(const ShVariantName &sv)
{
  Tab<PROCESS_INFORMATION> &pi = childCompilatioProcessInfo;
  String cmdname(__argv[0]);
#if _TARGET_PC_WIN
  Tab<HANDLE> piH;
  STARTUPINFO si;
  if (!dd_get_fname_ext(cmdname))
    cmdname += ".exe";
#else
  Tab<pid_t> piH;
#endif
  String cmdline(cmdname);
  for (int i = 1; i < __argc; i++)
    cmdline.aprintf(0, " %s", __argv[i]);
  cmdline.aprintf(0, " -r");

  if (compilation_job_count > 24) // very small benefit from 24+ processes, but noticeable memory consumption
    compilation_job_count = 24;

  childCompilatioTargetObj.resize(compilation_job_count);
  pi.resize(compilation_job_count);
  mem_set_0(pi);
  piH.reserve(pi.size());
  int compilations_in_flight = 0;
  bool some_compilations_failed = false;
  DWORD ret_code;

  if (sv.sourceFilesList.size() < 2)
    return;

#ifdef _CROSS_TARGET_METAL
  bool shouldTryAgain;
  int triesCount = 0;

restart:
  shouldTryAgain = false;
#endif

  int timeSec = time(NULL), modules_compiled = 0;
  for (int i = 0; i < sv.sourceFilesList.size(); i++)
  {
    PROCESS_INFORMATION *piNext = NULL;
    if (!shc::should_recompile_sh(sv, sv.sourceFilesList[i]) && !forceRebuild)
      continue;

  find_slot:
    for (int j = 0; j < pi.size(); j++)
      if (!pi[j].hProcess)
      {
        piNext = &pi[j];
        break;
      }

    if (!piNext)
    {
      piH.clear();
      for (int j = 0; j < pi.size(); j++)
        if (pi[j].hProcess)
          piH.push_back(pi[j].hProcess);
      WaitForMultipleObjects(piH.size(), piH.data(), false, INFINITE);

      for (int j = 0; j < pi.size(); j++)
        if (GetExitCodeProcess(pi[j].hProcess, &ret_code))
        {
          if (ret_code == STILL_ACTIVE)
            continue;
#if _TARGET_PC_WIN
          CloseHandle(pi[j].hThread);
          CloseHandle(pi[j].hProcess);
#endif
          compilations_in_flight--;
          mem_set_0(make_span(pi).subspan(j, 1));
          if (ret_code != 0)
          {
#ifdef _CROSS_TARGET_METAL
            shouldTryAgain = true;
#else
            if (wait_compilation_jobs)
              some_compilations_failed = true;
            else
            {
              terminateChildProcesses();
              sh_debug(SHLOG_FATAL, "compilation failed");
              compilation_job_count = 0;
              return;
            }
#endif
          }
          childCompilatioTargetObj[j] = NULL;
          if (!piNext)
            piNext = &pi[j];
        }
      if (!piNext)
      {
        SleepEx(100, true);
        goto find_slot;
      }
    }

#if _TARGET_PC_WIN
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    childCompilatioTargetObj[piNext - pi.data()] = shc::get_obj_file_name_from_source(sv.sourceFilesList[i], sv);

    if (!CreateProcessA(cmdname, cmdline + String(0, " -c %s", sv.sourceFilesList[i]), NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP,
          NULL, NULL, &si, piNext))
    {
      terminateChildProcesses();
      sh_debug(SHLOG_FATAL, "cannot start %s\nwith cmdline;: %s -c %s", cmdname, cmdline, sv.sourceFilesList[i]);
      compilation_job_count = 0;
      return;
    }
#else
    Tab<char *> arguments;
    arguments.push_back(cmdname);
    for (int arg = 1; arg < __argc; arg++)
      arguments.push_back(__argv[arg]);
    arguments.push_back("-r");
    arguments.push_back("-c");
    arguments.push_back((char *)sv.sourceFilesList[i].c_str());
    arguments.push_back(nullptr);
    int res = posix_spawn(&piNext->hProcess, cmdname, nullptr, nullptr, arguments.data(), environ);
    if (res)
    {
      terminateChildProcesses();
      sh_debug(SHLOG_FATAL, "cannot start %s\nwith cmdline;: %s -c %s", cmdname, cmdline, sv.sourceFilesList[i]);
      compilation_job_count = 0;
      return;
    }
#endif
    compilations_in_flight++;
    modules_compiled++;
  }

  while (compilations_in_flight > 0)
  {
    piH.clear();
    for (int j = 0; j < pi.size(); j++)
      if (pi[j].hProcess)
        piH.push_back(pi[j].hProcess);
    WaitForMultipleObjects(piH.size(), piH.data(), false, INFINITE);

    for (int j = 0; j < pi.size(); j++)
      if (GetExitCodeProcess(pi[j].hProcess, &ret_code))
      {
        if (ret_code == STILL_ACTIVE)
          continue;
#if _TARGET_PC_WIN
        CloseHandle(pi[j].hThread);
        CloseHandle(pi[j].hProcess);
#endif
        compilations_in_flight--;
        mem_set_0(make_span(pi).subspan(j, 1));
        if (ret_code != 0)
        {
#ifdef _CROSS_TARGET_METAL
          shouldTryAgain = true;
#else
          if (wait_compilation_jobs)
            some_compilations_failed = true;
          else
          {
            terminateChildProcesses();
            sh_debug(SHLOG_FATAL, "compilation failed");
            compilation_job_count = 0;
            return;
          }
#endif
        }
        childCompilatioTargetObj[j] = NULL;
      }
    if (compilations_in_flight)
      SleepEx(100, true);
  }

#ifdef _CROSS_TARGET_METAL
  if (shouldTryAgain)
  {
    triesCount++;

    if (triesCount < 2)
    {
      goto restart;
    }
  }
#endif

  timeSec = time(NULL) - timeSec;
  sh_debug(SHLOG_NORMAL, "[INFO] parallel compilation of %d modules using %d processes (x%d threads) done for %ds", modules_compiled,
    compilation_job_count, shc::compileJobsCount, timeSec);
  forceRebuild = false;
  if (wait_compilation_jobs && some_compilations_failed)
  {
    terminateChildProcesses();
    sh_debug(SHLOG_FATAL, "compilation failed");
    compilation_job_count = 0;
  }
}

static char sha1_cache_dir_buf[320] = {0};
static bool purge_sha1 = false;
char *sha1_cache_dir = sha1_cache_dir_buf;

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
  const char *blk_file_name, const char *cfg_dir, const char *binminidump_fnprefix, bool pack)
{
  ShVariantName sv(fn, opt);
  ShaderParser::AssembleShaderEvalCB::buildHwDefines(opt);
  sv.sourceFilesList = source_files;
  for (int i = 0; i < sv.sourceFilesList.size(); i++)
    simplify_fname(sv.sourceFilesList[i]);
  if (compile_only_sh)
  {
    for (int i = 0; i < sv.sourceFilesList.size(); i++)
      if (sv.sourceFilesList[i] == compile_only_sh)
      {
        clear_and_shrink(sv.sourceFilesList);
        sv.sourceFilesList.push_back() = compile_only_sh;
        break;
      }

    if (sv.sourceFilesList.size() != 1 || sv.sourceFilesList[0] != compile_only_sh)
    {
      sh_debug(SHLOG_ERROR, "unknown source '%s', available are:", compile_only_sh);
      for (int i = 0; i < sv.sourceFilesList.size(); i++)
        sh_debug(SHLOG_ERROR, "  %s", sv.sourceFilesList[i]);
      return;
    }
  }

  if (intermediate_dir)
  {
    sv.intermediateDir = intermediate_dir;
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
  additionalDirStr.aprintf(0, "-O%d", hlslOptimizationLevel);
  if (hlslDebugLevel == DebugLevel::BASIC)
    additionalDirStr += "D";
  else if (hlslDebugLevel == DebugLevel::FULL_DEBUG_INFO)
    additionalDirStr += "DFULL";
  else if (hlslDebugLevel == DebugLevel::AFTERMATH)
    additionalDirStr += "DAFTERMATH";
  if (isDebugModeEnabled)
    additionalDirStr += "-debug";
#if _CROSS_TARGET_SPIRV
  if (enableBindless)
    additionalDirStr += "-bindless";
#endif

  sv.intermediateDir += additionalDirStr;
  dd_mkdir(sv.intermediateDir);
  SNPRINTF(sha1_cache_dir_buf, sizeof(sha1_cache_dir_buf), "%s/../../shaders_sha~%s%s", sv.intermediateDir.c_str(), cross_compiler,
    additionalDirStr.c_str());
  sha1_cache_dir = sha1_cache_dir_buf;
  if (purge_sha1)
  {
    debug("purging cache %s: %s", sha1_cache_dir, remove_recursive(sha1_cache_dir) ? "success" : "error");
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
    blk.removeParam("outDumpName");
    blk.saveToTextStream(*hasher);
    get_computed_hash(hasher, blkHash.data(), blkHash.size());
    destory_hash_computer_cb(hasher);
    auto blkHashFile = df_open(blkHashFilename.c_str(), DF_READ);
    if (blkHashFile)
    {
      HashBytes previosBlkHash;
      df_read(blkHashFile, previosBlkHash.data(), previosBlkHash.size());
      df_close(blkHashFile);
      isBlkChanged = memcmp(previosBlkHash.data(), blkHash.data(), blkHash.size()) != 0;
      sh_debug(SHLOG_INFO, "'%s' hash=%16s, '%s' hash=%16s => isBlkChanged=%d", blkHashFilename.c_str(), previosBlkHash.data(),
        blk_file_name, blkHash.data(), isBlkChanged);
    }
    else
    {
      isBlkChanged = true;
      sh_debug(SHLOG_INFO, "Previous hash missed, '%s' hash=%16s => isBlkChanged=%d", blk_file_name, blkHash.data(), isBlkChanged);
    }
  }

  if (isBlkChanged)
  {
    sh_debug(SHLOG_INFO, "'%s' changed. '%s' should be deleted to force rebuild", blk_file_name, (const char *)sv.intermediateDir);
    forceRebuild = true;
  }

  // build binary dump
  auto verStr = d3d::as_ps_string(opt.fshVersion);

  char bindump_fn[260];
#if _CROSS_TARGET_SPIRV
  if (enableBindless)
  {
    _snprintf(bindump_fn, 260, "%s.bindless.%s.shdump.bin", bindump_fnprefix, verStr);
  }
  else
#endif
  {
    _snprintf(bindump_fn, 260, "%s.%s.shdump.bin", bindump_fnprefix, verStr);
  }

  {
    char destDir[260];
    dd_get_fname_location(destDir, bindump_fn);
    dd_append_slash_c(destDir);
    dd_mkdir(destDir);

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2













#elif _CROSS_TARGET_DX12
    {
      String pdb(260, "%s_pdb/", destDir[0] ? bindump_fnprefix : "./");
      dd_mkdir(pdb);
      static wchar_t dx12_pdb_cache_dir_storage[260];
      mbstowcs(dx12_pdb_cache_dir_storage, pdb, 260);
      dx12_pdb_cache_dir = dx12_pdb_cache_dir_storage;
    }
#endif
  }

  if (forceRebuild || shc::should_recompile(sv))
  {
    if (shortMessages && !compile_only_sh)
      ATOMIC_PRINTF("[INFO] Building '%s'...\n", (char *)sv.dest);
    if (compilation_job_count)
    {
      doCompileModulesAsync(sv);
      AtomicPrintfMutex::term();
    }
    shc::compileShader(sv, noSave, forceRebuild, compile_only_sh != NULL);
    if (!supressLogs)
      ShaderCompilerStat::printReport((compilation_job_count || compile_only_sh) ? NULL : (log_dir ? log_dir : cfg_dir));
  }
  else
  {
    sh_debug(SHLOG_INFO, "Skipping up-to-date '%s'", (char *)sv.dest);
  }
  shc::resetCompiler();
  if (compile_only_sh)
    return;

  if (strcmp(bindump_fnprefix, "*") != 0)
  {
    dd_mkpath(bindump_fn);
    shc::buildShaderBinDump(bindump_fn, sv.dest, forceRebuild, false, pack);
  }

  if (binminidump_fnprefix)
  {
    _snprintf(bindump_fn, 260, "%s.%s.shdump.bin", binminidump_fnprefix, verStr);
    dd_mkpath(bindump_fn);
    shc::buildShaderBinDump(bindump_fn, sv.dest, dd_stricmp(binminidump_fnprefix, bindump_fnprefix) == 0, true, pack);
  }

  if (blk_file_name && isBlkChanged)
  {
    ;
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
  if (dump_fn.length() > 3 && dd_stricmp(&dump_fn[dump_fn.length() - 3], ".sh") == 0)
    erase_items(dump_fn, dump_fn.length() - 3, 3);
  else if (dump_fn.length() > 4 && dd_stricmp(&dump_fn[dump_fn.length() - 4], ".blk") == 0)
    erase_items(dump_fn, dump_fn.length() - 4, 4);
  return dump_fn;
}

static eastl::vector<String> dsc_include_paths;
#define USE_FAST_INC_LOOKUP (__cplusplus >= 201703L)
#if _TARGET_PC_MACOSX && MAC_OS_X_VERSION_MIN_REQUIRED < 101500 // macOS SDK < 10.15
#undef USE_FAST_INC_LOOKUP
#endif
#if USE_FAST_INC_LOOKUP
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
static ska::flat_hash_map<eastl::string, eastl::string> file_to_full_path; // use power_of_two strategy, because we have good enough
                                                                           // hashes for strings
inline eastl::string convert_dir_separator(const char *s_)
{
  eastl::string s = s_;
  eastl::replace(s.begin(), s.end(), '\\', '/');
  return s;
}
#endif
void add_include_path(const char *d)
{
  dsc_include_paths.emplace_back(String(d));
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
    if (file_to_full_path.find(fn) == file_to_full_path.end())
    {
      file_to_full_path[fn] = (dir + "/") + fn;
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
    auto it = file_to_full_path.find(convert_dir_separator(fn));
    if (it != file_to_full_path.end())
      return String(it->second.c_str());
  }
  else
  {
    auto it = file_to_full_path.find_as(fn);
    if (it != file_to_full_path.end())
      return String(it->second.c_str());
  }
  return String(fn);
#else
  char buf[2048];
  for (int i = 0, e = (int)dsc_include_paths.size(); i < e; i++)
  {
    SNPRINTF(buf, sizeof(buf), "%s/%s", dsc_include_paths[i].c_str(), fn);
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
  ATOMIC_FPRINTF(stderr, "Fatal error: %s\n%s", msg, call_stack);
}

bool sanitize_hash_strings = false;

#if _TARGET_PC_WIN
static BOOL WINAPI ctrl_break_handler(_In_ DWORD dwCtrlType)
{
  if (dwCtrlType != CTRL_BREAK_EVENT && dwCtrlType != CTRL_C_EVENT)
    return FALSE;
#else
void ctrl_break_handler(int)
{
#endif

  setvbuf(stdout, NULL, _IOLBF, 4096);
  if (compile_only_sh)
    ATOMIC_PRINTF("Cancelling compilation (%s)...\n", compile_only_sh);
  else
    ATOMIC_PRINTF("Cancelling compilation (%d jobs)...\n", compilation_job_count);
  debug("SIGINT received!\n");
  if (compilation_job_count > 0)
  {
    terminateChildProcesses();
    for (int attempts = 10; compilation_job_count && attempts; attempts--)
      SleepEx(100, true);
    if (compilation_job_count)
      ATOMIC_PRINTF("(!) failed to wait compilation_job_count(%d) reset...\n", compilation_job_count);
  }
  if (cpujobs::is_inited())
  {
    debug("terminating jobs");
    cpujobs::term(true, 3000);
    debug("jobs termination done");
  }
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

  termCC = new WinCritSec("termChildren");
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

  // G_VERIFY(shaderopcode::formatUnittest());

  bool singleBuild = false;
  ShHardwareOptions singleOptions(4.0_sm);

  const char *filename = __argv[1];

  // check file extension
  if (strstr(filename, ".sh") || strstr(filename, ".SH"))
  {
    // this is a single shader file
    singleBuild = true;
  }
  const char *outDumpNameConfig = NULL;

  for (int i = 2; i < __argc; i++)
  {
    const char *s = __argv[i];
    if (dd_stricmp(s, "-r") == 0)
      forceRebuild = true;
    else if (dd_stricmp(s, "-depBlk") == 0)
      ; // no-op
    else if (dd_stricmp(s, "-q") == 0)
    {
      shortMessages = true;
      hlslNoDisassembly = true;
      sh_console_quet_output(true);
    }
    else if (dd_stricmp(s, "-out") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      outDumpNameConfig = __argv[i];
    }
    else if (strnicmp(s, "-j", 2) == 0)
      shc::compileJobsCount = atoi(s + 2);
    else if (dd_stricmp(s, "-nosave") == 0)
      noSave = true;
    else if (dd_stricmp(s, "-debug") == 0)
      isDebugModeEnabled = true;
    else if (dd_stricmp(s, "-pdb") == 0)
      hlslDebugLevel = DebugLevel::BASIC;
    else if (dd_stricmp(s, "-dfull") == 0 || dd_stricmp(s, "-debugfull") == 0)
      hlslDebugLevel = DebugLevel::FULL_DEBUG_INFO;
    else if (dd_stricmp(s, "-daftermath") == 0)
      hlslDebugLevel = DebugLevel::AFTERMATH;
    else if (dd_stricmp(s, "-commentPP") == 0)
      hlslSavePPAsComments = true;
    else if (dd_stricmp(s, "-codeDump") == 0)
      hlslDumpCodeAlways = true;
    else if (dd_stricmp(s, "-codeDumpErr") == 0)
      hlslDumpCodeOnError = true;
    else if (dd_stricmp(s, "-w") == 0)
    {
      hlslShowWarnings = true;
      sh_console_print_warnings(true);
    }
    else if (dd_stricmp(s, "-O0") == 0)
    {
      hlslOptimizationLevel = 0;
      sanitize_hash_strings = false;
    }
    else if (dd_stricmp(s, "-O1") == 0)
      hlslOptimizationLevel = 1;
    else if (dd_stricmp(s, "-O2") == 0)
      hlslOptimizationLevel = 2;
    else if (dd_stricmp(s, "-O3") == 0)
      hlslOptimizationLevel = 3;
    else if (dd_stricmp(s, "-O4") == 0)
    {
      hlslOptimizationLevel = 4;
      sanitize_hash_strings = true;
    }
    else if (dd_stricmp(s, "-no_sanitize_hash") == 0)
      sanitize_hash_strings = false;
    else if (dd_stricmp(s, "-sanitize_hash") == 0)
      sanitize_hash_strings = true;
    else if (dd_stricmp(s, "-bones_start") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      hlsl_bones_base_reg = atoi(__argv[i]);
    }
    else if (dd_stricmp(s, "-maxVSF") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      hlsl_maximum_vsf_allowed = atoi(__argv[i]);
    }
    else if (dd_stricmp(s, "-maxPSF") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      hlsl_maximum_psf_allowed = atoi(__argv[i]);
    }
    else if (dd_stricmp(s, "-skipvalidation") == 0)
    {
      hlslSkipValidation = true;
      sanitize_hash_strings = false;
    }
    else if (dd_stricmp(s, "-nodisassembly") == 0)
      hlslNoDisassembly = true;
    else if (dd_stricmp(s, "-shaderOn") == 0)
      shc::setRequiredShadersDef(true);
    else if (dd_stricmp(s, "-supressLogs") == 0)
      supressLogs = true;
    else if (dd_stricmp(s, "-shadervar-generator-mode") == 0)
    {
      using namespace std::string_view_literals;
      i++;
      if (i >= __argc)
        goto usage_err;
      if (__argv[i] == "none"sv)
        shadervar_generator_mode = ShadervarGeneratorMode::None;
      else if (__argv[i] == "remove"sv)
        shadervar_generator_mode = ShadervarGeneratorMode::Remove;
      else if (__argv[i] == "check"sv)
        shadervar_generator_mode = ShadervarGeneratorMode::Check;
      else if (__argv[i] == "generate"sv)
        shadervar_generator_mode = ShadervarGeneratorMode::Generate;
      else
        printf("\n[WARNING] Unknown shadervar generator mode '%s'\n", __argv[i]);
    }
    else if (dd_stricmp(s, "-invalidAsNull") == 0)
      shc::setInvalidAsNullDef(true);
    else if (dd_stricmp(s, "-no_sha1_cache") == 0)
    {
      extern bool useSha1Cache, writeSha1Cache;
      useSha1Cache = writeSha1Cache = false;
    }
    else if (dd_stricmp(s, "-no_compression") == 0)
    {
      useCompression = false;
    }
    else if (dd_stricmp(s, "-purge_sha1_cache") == 0)
    {
      purge_sha1 = true;
    }
#if _CROSS_TARGET_SPIRV
    else if (strnicmp(s, "-enableBindless:", 15) == 0)
    {
      if (strstr(s, "on"))
        enableBindless = true;
      else if (strstr(s, "off"))
        enableBindless = false;
    }
#endif
#if _CROSS_TARGET_DX12
    else if (0 == dd_stricmp(s, "-platform-pc"))
    {
      targetPlatform = dx12::dxil::Platform::PC;
      cross_compiler = "dx12";
    }
    else if (0 == dd_stricmp(s, "-platform-xbox-one"))
    {
      targetPlatform = dx12::dxil::Platform::XBOX_ONE;
      cross_compiler = "xbox_one_dx12";
    }
    else if (0 == dd_stricmp(s, "-platform-xbox-scarlett"))
    {
      targetPlatform = dx12::dxil::Platform::XBOX_SCARLETT;
      cross_compiler = "scarlett_dx12";
    }
    else if (0 == dd_stricmp(s, "-HLSL2021"))
    {
      hlsl2021 = true;
    }
    else if (0 == dd_stricmp(s, "-enablefp16"))
    {
      enableFp16 = true;
    }
#endif
#if _CROSS_TARGET_SPIRV
    else if (dd_stricmp(s, "-compiler-hlslcc") == 0)
      compilerHlslCc = true;
    else if (dd_stricmp(s, "-compiler-dxc") == 0)
      compilerDXC = true;
#endif
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
#ifdef _CROSS_TARGET_METAL
    else if (dd_stricmp(s, "-metalios") == 0)
    {
      use_ios_token = true;
      cross_compiler = "metal-ios";
    }
    else if (dd_stricmp(s, "-metal-glslang") == 0)
      ;
    else if (dd_stricmp(s, "-metal-use-glslang") == 0)
    {
      use_metal_glslang = true;
    }
    else if (dd_stricmp(s, "-metal-binary") == 0)
    {
      use_binary_msl = true;
      cross_compiler = "metal-binary";
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
      hlslWarningsAsErrors = true;
    }
    else if (dd_stricmp(s, "-wall") == 0)
    {
      sh_change_mode(SHLOG_SILENT_WARNING, SHLOG_WARNING);
      hlslShowWarnings = true;
      sh_console_print_warnings(true);
    }
    else if (strnicmp(s, "-fsh:", 5) == 0)
    {
      if (strstr(s, ":3.0"))
        singleOptions.fshVersion = 3.0_sm;
#if _CROSS_TARGET_DX11 || _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL | _CROSS_TARGET_EMPTY
      else if (strstr(s, ":4.0"))
        singleOptions.fshVersion = 4.0_sm;
      else if (strstr(s, ":4.1"))
        singleOptions.fshVersion = 4.1_sm;
      else if (strstr(s, ":5.0"))
        singleOptions.fshVersion = 5.0_sm;
      else if (strstr(s, ":6.0"))
        singleOptions.fshVersion = 6.0_sm;
      else if (strstr(s, ":6.6"))
        singleOptions.fshVersion = 6.6_sm;
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
      intermediate_dir = __argv[i];
    }
    else if (dd_stricmp(s, "-logdir") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      log_dir = __argv[i];
    }
    else if (dd_stricmp(s, "-c") == 0)
    {
      i++;
      if (i >= __argc)
        goto usage_err;
      compile_only_sh = __argv[i];
    }
    else if (dd_stricmp(s, "-cjWait") == 0)
      wait_compilation_jobs = true;
    else if (strnicmp(s, "-cj", 3) == 0)
      compilation_job_count = strlen(s) > 3 ? atoi(s + 3) : -1;
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
      hlsl_defines.aprintf(128, "#define %s %s\n", __argv[i - 1], __argv[i]);
    }
    else if (dd_stricmp(s, "-optionalAsBranches") == 0)
    {
      optionalIntervalsAsBranches = true;
    }
    else if (strnicmp("-addpath:", __argv[i], 9) == 0)
      ; // skip
    else
    {
    usage_err:
      show_header();
      printf("\n[FATAL ERROR] Unknown option '%s'\n", s);
      showUsage();
      return 13;
    }
  }
  if (compile_only_sh)
    compilation_job_count = 0;
  else
    printf("hlslOptimizationLevel is set to %d\n", hlslOptimizationLevel);

#if _CROSS_TARGET_EMPTY
  noSave = true;
#endif

  if (!shortMessages && !compile_only_sh)
    show_header();

  class LocalCpuJobs
  {
  public:
    LocalCpuJobs()
    {
      cpujobs::init();
      if (!shc::compileJobsCount)
        shc::compileJobsCount = cpujobs::get_physical_core_count();
      else if (shc::compileJobsCount > 32)
        shc::compileJobsCount = 32;

      if (shc::compileJobsCount > 1)
      {
        int logicalCores = cpujobs::get_logical_core_count();
        int physicalCores = cpujobs::get_physical_core_count();
        int coreRatio = max(1, logicalCores / physicalCores);

        G_ASSERTF(shc::compileJobsCount <= logicalCores, "There is not enough logic cores (%u) to run %u jobs", logicalCores,
          shc::compileJobsCount);

        if (coreRatio > 1)
        {
          // occupy cores properly, start with physical, then use logical HT ones
          // use virtual job manager, otherwise job indexing will be broken
          shc::compileJobsMgrBase = logicalCores;
          for (int i = 0; i < shc::compileJobsCount; i++)
          {
            uint64_t affinity = 1ull << ((i * coreRatio) % logicalCores + i / physicalCores);
            G_VERIFY(cpujobs::create_virtual_job_manager(1 << 20, affinity) == shc::compileJobsMgrBase + i);
          }
        }
        else
        {
          for (int i = 0; i < shc::compileJobsCount; i++)
            G_VERIFY(cpujobs::start_job_manager(i, 1 << 20));
        }
        debug("inited %d job managers", shc::compileJobsCount);
      }
      else
        debug("started in job-less mode");
    }
    ~LocalCpuJobs()
    {
      cpujobs::term(true, 1000);
      debug("terminated cpujobs");
    }
  };
  LocalCpuJobs lcj;
  if (compilation_job_count < 0)
    compilation_job_count = cpujobs::get_physical_core_count();
  if (compilation_job_count < 2)
    compilation_job_count = 0;
  if (compile_only_sh || compilation_job_count)
    setvbuf(stdout, NULL, _IOFBF, 4096);

  ErrorCounter::allCompilations().reset();
  int timeSec = time(NULL);

  ShaderParser::renderStageToIdxMap.reset();
  ShaderParser::renderStageToIdxMap.addNameId("opaque");
  ShaderParser::renderStageToIdxMap.addNameId("atest");
  ShaderParser::renderStageToIdxMap.addNameId("imm_decal");
  ShaderParser::renderStageToIdxMap.addNameId("decal");
  ShaderParser::renderStageToIdxMap.addNameId("trans");
  ShaderParser::renderStageToIdxMap.addNameId("distortion");
  if (singleBuild)
  {
    // compile single file
    ShaderCompilerStat::reset();
    Tab<String> sourceFiles(midmem);
    sourceFiles.push_back(String(filename));
    compile(sourceFiles, filename, makeShBinDumpName(filename), singleOptions, NULL, getDir(filename), NULL, false);
  }
  else
  {
    if (compile_only_sh || compilation_job_count > 1)
      AtomicPrintfMutex::init(dd_get_fname(__argv[0]), filename);

    // read data block with files
    DataBlock blk;
    const String cfgDir(getDir(filename));
    if (!blk.load(filename))
    {
      printf("\n[FATAL ERROR] Cannot open BLK file '%s' (file not found or syntax error, see log for details)\n", filename);
      showUsage();
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
          if (!compile_only_sh)
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
    for (int i = 0, nid = blk.getNameId("incDir"); i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
      {
        add_shaders_inc_base(blk.getStr(i));
        add_include_path(blk.getStr(i));
        if (!compile_only_sh)
          sh_debug(SHLOG_INFO, "Using include dir: %s", blk.getStr(i));
      }

    if (outDumpNameConfig)
      blk.setStr("outDumpName", outDumpNameConfig);
    String defShBindDumpPrefix(makeShBinDumpName(filename));
    if (blk.getStr("outDumpName", NULL))
      defShBindDumpPrefix = blk.getStr("outDumpName", NULL);

    Tab<String> sourceFiles(midmem);
    DataBlock *sourceBlk = blk.getBlockByName("source");
    const char *shader_root = blk.getStr("shader_root_dir", NULL);
    shaderSrcRoot = shader_root;
    if (!sourceBlk)
    {
      printf("Shader source files should be copied from shaders.sh\n"
             "to shaders.blk as a source block with file:t parameters\n"
             "Headers should NOT be copied.");
      return 13;
    }

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
          add_include_path(sourceBlk->getStr(paramNo));
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
      shadervars_code_template_filename = blk_svg->getStr("code_template");
      for (int b = blk_svg->findBlock("generated_path"); b != -1; b = blk_svg->findBlock("generated_path", b))
      {
        DataBlock *blk_gp = blk_svg->getBlock(b);
        if (!blk_gp)
        {
          printf("\n[ERROR] Couldn't get block 'generated_path[%i]'\n", b);
          continue;
        }
        auto &info = generated_path_infos.emplace_back();
        info.matcher = blk_gp->getStr("matcher");
        info.replacer = blk_gp->getStr("replacer");
      }

      for (int p = blk_svg->findParam("exclude"); p != -1; p = blk_svg->findParam("exclude", p))
        exclude_from_generation.emplace_back(blk_svg->getStr(p));

      if (shadervar_generator_mode == ShadervarGeneratorMode::None)
      {
        using namespace std::string_view_literals;
        const char *mode = blk_svg->getStr("mode", "none");
        if (mode == "none"sv)
          shadervar_generator_mode = ShadervarGeneratorMode::None;
        else if (mode == "remove"sv)
          shadervar_generator_mode = ShadervarGeneratorMode::Remove;
        else if (mode == "check"sv)
          shadervar_generator_mode = ShadervarGeneratorMode::Check;
        else if (mode == "generate"sv)
          shadervar_generator_mode = ShadervarGeneratorMode::Generate;
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

        dictionary_size_in_kb = comp->getInt("dict_size_in_kb", dictionary_size_in_kb);
        sh_group_size_in_kb = comp->getInt("group_size_in_kb", sh_group_size_in_kb);

        ShHardwareOptions opt(4.0_sm);

        const char *fsh = comp->getStr("fsh", NULL);
        if (fsh)
        {
          if (strstr(fsh, "3.0"))
            opt.fshVersion = 3.0_sm;
#if _CROSS_TARGET_DX11 || _CROSS_TARGET_C1 || _CROSS_TARGET_C2 || _CROSS_TARGET_SPIRV || _CROSS_TARGET_METAL || _CROSS_TARGET_EMPTY
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
#endif
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
        opt.enableHalfProfile = comp->getBool("enableHalfProfile", true);

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
        bool pack = useCompression && blk.getBool("packBin", false);
        compile(sourceFiles, fName, comp->getStr("outDumpName", defShBindDumpPrefix), opt, filename, getDir(filename),
          blk.getStr("outMiniDumpName", NULL), pack);
      }
    }
  }

  if (compile_only_sh)
  {
    timeSec = time(NULL) - timeSec;
    if (timeSec > 60)
      sh_debug(SHLOG_NORMAL, "[INFO]      done '%s' for %dm:%02ds", compile_only_sh, timeSec / 60, timeSec % 60);
    else if (timeSec > 5)
      sh_debug(SHLOG_NORMAL, "[INFO]      done '%s' for %ds", compile_only_sh, timeSec);
    fflush(stdout);
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#endif
    return 0;
  }

  printf("\n");
  timeSec = time(NULL) - timeSec;
  if (timeSec > 60)
    printf("took %dm:%02ds\n", timeSec / 60, timeSec % 60);
  else if (timeSec > 1)
    printf("took %ds\n", timeSec);

  return 0;
}
