// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texgen_dshl_compile.h"

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <EditorCore/ec_wndGlobal.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_unicode.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>
#include <util/dag_finally.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_driverDesc.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <string.h>

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX // msvc-sets.jam / clang-sets.jam already pass -DNOMINMAX globally
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
extern char **environ;
#endif

static String s_toolsPath, s_rootPath;

// Spawn dsc2 (argv[0] = exe path, argv[1..] = args) with stdout+stderr redirected into log_path, wait for
// it to exit, and return the exit code. Returns -1 with launch_err filled if the process could not be
// launched at all (so the caller can tell a launch failure apart from a dsc2 nonzero exit and avoid
// reporting a stale/empty log as dsc2 output). Shell-free on both platforms: no cmd.exe console flash on
// Windows, no /bin/sh dependency on Linux. dsc2's [ERROR]/timing lines land in log_path either way.
#if _TARGET_PC_WIN
static int run_dsc2_process(const eastl::vector<eastl::string> &argv, const char *log_path, String &launch_err)
{
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE; // the child inherits the log handle it writes stdout/stderr to

  eastl::vector<wchar_t> wlog(strlen(log_path) + 1);
  utf8_to_wcs(log_path, wlog.data(), static_cast<int>(wlog.size()));
  HANDLE hLog = CreateFileW(wlog.data(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hLog == INVALID_HANDLE_VALUE)
  {
    launch_err.printf(0, "cannot create dsc2 log '%s' (err %u)", log_path, static_cast<unsigned>(GetLastError()));
    return -1;
  }

  // Command line = space-joined argv, quoting any token containing a space (paths under "Program Files").
  eastl::string cmdline;
  for (const eastl::string &a : argv)
  {
    if (!cmdline.empty())
    {
      cmdline.push_back(' ');
    }
    const bool quote = a.find(' ') != eastl::string::npos;
    if (quote)
    {
      cmdline.push_back('"');
    }
    cmdline.append(a);
    if (quote)
    {
      cmdline.push_back('"');
    }
  }
  eastl::vector<wchar_t> wapp(argv[0].size() + 1); // wchar count <= utf8 byte count, so these fit
  utf8_to_wcs(argv[0].c_str(), wapp.data(), static_cast<int>(wapp.size()));
  eastl::vector<wchar_t> wcmd(cmdline.size() + 1);
  utf8_to_wcs(cmdline.c_str(), wcmd.data(), static_cast<int>(wcmd.size()));

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = hLog;
  si.hStdError = hLog;

  // dsc2 spins a full-core threadpool even with -cj0, and the caller runs on the texgen worker thread;
  // below normal priority keeps the editor render thread responsive during a compile.
  PROCESS_INFORMATION pi{};
  const BOOL launched = CreateProcessW(wapp.data(), wcmd.data(), nullptr, nullptr, /*bInheritHandles*/ TRUE,
    CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
  if (!launched)
  {
    launch_err.printf(0, "CreateProcess('%s') failed (err %u)", argv[0].c_str(), static_cast<unsigned>(GetLastError()));
    CloseHandle(hLog);
    return -1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 0;
  if (!GetExitCodeProcess(pi.hProcess, &exitCode))
  {
    exitCode = static_cast<DWORD>(-1); // fail closed: an unreadable exit code is treated as a compile failure
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(hLog);
  return static_cast<int>(exitCode);
}
#else
static int run_dsc2_process(const eastl::vector<eastl::string> &argv, const char *log_path, String &launch_err)
{
  eastl::vector<char *> cargv;
  cargv.reserve(argv.size() + 1);
  for (const eastl::string &a : argv)
  {
    cargv.push_back(const_cast<char *>(a.c_str()));
  }
  cargv.push_back(nullptr); // execv-style NULL terminator

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  // stdout+stderr -> log file (truncated), matching the Windows redirect so dsc2's output is captured.
  posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);

  pid_t pid = 0;
  const int spawnRc = posix_spawn(&pid, argv[0].c_str(), &actions, nullptr, cargv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  if (spawnRc != 0)
  {
    launch_err.printf(0, "posix_spawn('%s') failed: %s", argv[0].c_str(), strerror(spawnRc));
    return -1;
  }

  // Best-effort below-normal priority (mirror of the Windows BELOW_NORMAL_PRIORITY_CLASS); races harmlessly
  // with a fast child exit, so its result is ignored.
  setpriority(PRIO_PROCESS, pid, 10);

  int status = 0;
  int w = 0;
  // Retry on EINTR: a signal (waitpid is not covered by SA_RESTART on every handler) must not be mistaken
  // for dsc2 exiting, which would falsely fail the compile and leak a zombie.
  while ((w = waitpid(pid, &status, 0)) < 0 && errno == EINTR) {}
  if (w < 0)
  {
    launch_err.printf(0, "waitpid on dsc2 failed: %s", strerror(errno));
    return -1;
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif

bool TexgenDshlCompiler::initPaths(String &out_err)
{
  s_toolsPath = ::dgs_get_settings()->getBlockByNameEx("debug")->getStr("toolsPathWin", "");
  s_rootPath = ::dgs_get_settings()->getBlockByNameEx("debug")->getStr("rootPathWin", "");
  // Fallback: dsc2-<platform>-dev ships in the editor's own bin dir (tools/dagor_cdk/<platform>),
  // so the toolchain resolves even when the project doesn't set debug{toolsPathWin} (which NBS uses).
  if (s_toolsPath.empty())
  {
    s_toolsPath = sgg::get_exe_path_full();
  }
  if (s_rootPath.empty())
  {
    s_rootPath = s_toolsPath + "/../../..";
  }
  if (s_toolsPath.empty())
  {
    out_err = "texgen dshl: cannot resolve dsc2 toolchain path";
    return false;
  }
  // Normalize away the ".." from the get_exe_path_full fallback ("<exe>/../../..") so every path derived
  // from the root (shader_root_dir, engineRootDir, includePath, dump/obj dirs) is a clean absolute path.
  simplify_fname(s_rootPath);
  return true;
}

bool TexgenDshlCompiler::compileToDump(const char *dshl_text, const char *dump_name_base, String &out_dump_base, String &out_err)
{
  if (s_toolsPath.empty())
  {
    out_err = "texgen dshl: TexgenDshlCompiler::initPaths was not called";
    return false;
  }

#if _TARGET_PC_WIN
  const char *dscSuff = "hlsl11";
  const char *platform = "dx11";
  const char *dumpSuff = ""; // runtime dx11 bindump loader appends no platform suffix
  const char *exeSuffix = ".exe";
#elif _TARGET_PC_LINUX
  const char *dscSuff = "spirv";
  const char *platform = "spirv";
  const char *dumpSuff = "SpirV"; // loader prefix only; the ".bindless" infix is added by dsc2 (see below)
  const char *exeSuffix = "";
  // The vulkan loader (build_shaderdump_filename) resolves "<base>SpirV.bindless.<ver>.shdump.bin" when the
  // device reports caps.hasBindless, else "<base>SpirV.<ver>.shdump.bin". dsc2 emits that same infix from
  // its -enableBindless config, so it must be driven by the runtime cap or the produced dump is unloadable.
  const bool enableBindless = d3d::get_driver_desc().caps.hasBindless;
#else
  out_err = "texgen dshl: unsupported editor host platform";
  return false;
#endif

  // Temporary profiling aid: keep the dsc2 log on success and ask dsc2 for its per-phase timing lines
  // (Parsed preshader / linked / Compressed shaders / Saved bindump in Xms). Off by default.
  const bool profileCompile = ::dgs_get_settings()->getBlockByNameEx("texgen")->getBool("dshlProfileCompile", false);

  // Artifacts under <root>/_output/texgen (same drive as the engine root) so the repo tree stays clean.
  // base is the loadOrReload base (the loader appends the platform+version suffix). dd_mkpath creates
  // _output/texgen (it treats the last path element as a file).
  const String base(0, "%s/_output/texgen/%s", s_rootPath.str(), dump_name_base);
  dd_mkpath(base.str());
  const String outputDshl(0, "%s.dshl.tmp", base.str());
  // dsc2 builds the source path as shader_root_dir + "/" + file (main.cpp). Pass shader_root_dir = the
  // absolute root and this file RELATIVE to it: an absolute file with shader_root_dir="." makes dsc2 form
  // ".//<abs>", which simplify_fname reduces to a leading "./" it strips -> the path loses its '/' and
  // won't open on Linux.
  const String sourceRel(0, "_output/texgen/%s.dshl.tmp", dump_name_base);
  const String outputDscBlk(0, "%s.blk.tmp", base.str());
  const String tmpLog(0, "%s.log", base.str());

  bool ok = false;
  FINALLY([&] {
    if (!ok)
    {
      return; // keep .dshl / config / log on failure so the failing shader can be diagnosed
    }
    // NOTE: the produced .shdump.bin is intentionally NOT erased here -- loadOrReload reads it (and the
    // next compileToDump overwrites it).
    dd_erase(outputDshl.str());
    dd_erase(outputDscBlk.str());
    if (!profileCompile)
    {
      dd_erase(tmpLog.str());
    }
  });

  {
    FullFileSaveCB f(outputDshl.str());
    if (!f.fileHandle)
    {
      out_err.aprintf(0, "texgen dshl: cannot write '%s'", outputDshl.str());
      return false;
    }
    f.write(dshl_text, static_cast<int>(strlen(dshl_text)));
  }

  // dsc config: self-contained source, only the render/shaders include dir (dsc2 auto-includes assert.dshl).
  // additional_dump => an editor-loadable non-primary dump (matches NodeBasedShaderManagerCompile).
  // packShader:b=no drops the zstd dictionary training + per-group compression: a fixed multi-hundred-ms
  // cost that is pure waste for an editor-local temp dump (the loader decompresses-or-copies transparently).
  // outDumpName carries the platform dumpSuff so the produced file name matches the runtime bindump loader.
  // outDumpName/packShader/compileCppStcode are excluded from dsc2's config hash, so none of this
  // invalidates the shared obj/sha1 bytecode cache.
  const String dscBlk(0,
    R"(
      shader_root_dir:t="%s"
      outDumpName:t="%s%s"
      engineRootDir:t="%s"
      compileCppStcode:b=no
      packShader:b=no
      source {
        file:t="%s"
        includePath:t="%s/prog/gameLibs/render/shaders"
      }
      Compile {
        fsh:t = 5.0
        additional_dump:b = yes
      }
    )",
    s_rootPath.str(), base.str(), dumpSuff, s_rootPath.str(), sourceRel.str(), s_rootPath.str());

  {
    FullFileSaveCB f(outputDscBlk.str());
    if (!f.fileHandle)
    {
      out_err.aprintf(0, "texgen dshl: cannot write '%s'", outputDscBlk.str());
      return false;
    }
    f.write(dscBlk.str(), dscBlk.length());
  }

  // Flags:
  //   -cj0          single-process; skips the redundant master "preshader parse" pass that multi-process
  //                 mode (numProcesses=-1 default) runs before bailing to in-process for a 1-file config.
  //   -suppressLogs no ShaderLog-*/Stat.txt file IO. [ERROR] lines still go to stdout (our redirected log).
  //   -r            force obj rebuild: the .dshl is rewritten every compile, so obj incrementality can't
  //                 trigger; the backend sha1 bytecode cache still applies, so this is not a full recompile.
  //   -logExactTiming (profiling only) dsc2's per-phase millisecond lines, read from the kept log.
  // No -q: dsc2 prints [ERROR] lines (with the offending shader/HLSL) to stdout, which we surface;
  //   -q would suppress them. -codeDumpErr additionally dumps the failing HLSL on error.
  //
  // Paths are built with forward slashes; converted to backslashes on Windows (both dsc2 and the OS accept
  // either). Argv is passed to the OS spawn directly, so no shell quoting is needed beyond the
  // space-quoting run_dsc2_process does for CreateProcessW.
  String exe(0, "%s/dsc2-%s-dev%s", s_toolsPath.str(), dscSuff, exeSuffix);
  String objDir(0, "%s/_output/lshader/shaders~%s", s_rootPath.str(), platform);
  String blkArg(outputDscBlk);
#if _TARGET_PC_WIN
  exe.replaceAll("/", "\\");
  objDir.replaceAll("/", "\\");
  blkArg.replaceAll("/", "\\");
#endif

  eastl::vector<eastl::string> argv;
  argv.push_back(exe.str());
  argv.push_back(blkArg.str());
  argv.push_back("-shaderOn");
  argv.push_back("-nodisassembly");
  argv.push_back("-commentPP");
  argv.push_back("-codeDumpErr");
  argv.push_back("-r");
  argv.push_back("-cj0");
  argv.push_back("-suppressLogs");
#if _TARGET_PC_LINUX
  // Match the produced dump name's bindless infix to what the runtime vulkan loader will look for.
  argv.push_back(enableBindless ? "-enableBindless:on" : "-enableBindless:off");
  // Accept FXC-tolerated implicit vector truncation (shader_editor nodes read float3 tc as float2) that
  // DXC->SPIR-V otherwise rejects. Editor-only; the engine's shader build never passes this.
  argv.push_back("-noConversionWarnings");
  // Validate the DX-scalar-packed ParticleInstance StructuredBuffer (float3 members at DX offsets) under
  // Vulkan scalar block layout; the runtime enables the scalarBlockLayout device feature. Editor-only.
  argv.push_back("-useScalarLayout");
#endif
  if (profileCompile)
  {
    argv.push_back("-logExactTiming");
  }
  argv.push_back("-o");
  argv.push_back(objDir.str());

  String launchErr;
  if (run_dsc2_process(argv, tmpLog.str(), launchErr) != 0)
  {
    if (!launchErr.empty())
    {
      // dsc2 never ran, so tmpLog holds no fresh output (possibly stale or empty): report why it failed.
      logerr("texgen dshl: dsc2 launch failed: %s", launchErr.str());
      out_err.aprintf(0, "texgen dshl: dsc2 launch failed: %s", launchErr.str());
      return false;
    }
    String log;
    FullFileLoadCB f(tmpLog.str());
    if (f.fileHandle)
    {
      const int sz = f.getTargetDataSize();
      log.resize(sz + 1);
      f.read(log.str(), sz);
      log.str()[sz] = 0;
    }
    // Full output to the debug log (the editor console truncates); the .dshl is kept at outputDshl.
    logerr("texgen dshl: dsc2 failed. Generated .dshl kept at '%s'. dsc2 output:\n%s", outputDshl.str(), log.str());
    out_err.aprintf(0, "texgen dshl: dsc2 failed (full output in debug log; .dshl at %s)", outputDshl.str());
    return false;
  }

  out_dump_base = base;
  ok = true;
  return true;
}

bool TexgenDshlCompiler::loadOrReload(const char *dump_base, ShaderBindumpHandle &handle, String &out_err)
{
  if (handle == INVALID_BINDUMP_HANDLE)
  {
    handle = load_additional_shaders_bindump(dump_base, d3d::sm50);
    if (handle == INVALID_BINDUMP_HANDLE)
    {
      out_err.aprintf(0, "texgen dshl: load_additional_shaders_bindump('%s') failed", dump_base);
      return false;
    }
  }
  else if (!reload_shaders_bindump(handle, dump_base, d3d::sm50))
  {
    out_err = "texgen dshl: reload_shaders_bindump failed";
    return false;
  }
  return true;
}
