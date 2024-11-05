// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "callbacks.h"
#include <breakpad/binder.h>
#include "context.h"

#include <stdio.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <wchar.h>
#include <string>
#include <common/windows/string_utils-inl.h>
#include <client/windows/handler/exception_handler.h>
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#endif // _TARGET_PC_WIN|_TARGET_PC_LINUX|_TARGET_PC_MACOSX

#include <debug/dag_logSys.h>

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/winHwExceptUtils.h>
#include <osApiWrappers/dag_unicode.h>
#include <malloc.h>
#endif

namespace breakpad
{
#if DAGOR_DBGLEVEL > 0
static char breakpad_stk[8192] = {0};
#endif


void preprocess(Context *ctx)
{
  if (!ctx->crash.expression)
    ctx->crash.expression = "Exception";

  if (ctx->configuration.preprocess)
    ctx->configuration.preprocess(ctx->crash);
}

void postprocess(Context *ctx)
{
  if (ctx->configuration.notify && !ctx->configuration.silent)
    ctx->configuration.notify(ctx->crash);

  if (ctx->configuration.hook)
    ctx->configuration.hook(ctx->crash);

  ctx->crash.reset();
}


#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX


static char user_id[32] = {'\0'};
#define MAX_CLI_ARGUMENTS 255
static char *uploader_cmd[MAX_CLI_ARGUMENTS] = {(char *)NULL};

bool filter(void *context)
{
  interlocked_exchange(g_debug_is_in_fatal, 1);

  preprocess((Context *)context);

  return true;
}


static void run_uploader(const Context *ctx, const char *dump_path, const char *dump_id)
{
  int pid = fork();
  if (!pid)
  {
    // Below may not be absolutely safe, but we need a char*.
    size_t i = 0;
    uploader_cmd[i] = (char *)ctx->configuration.senderCmd.c_str();
#define PUSH_CLI_ARG(pos, arg, val)      \
  {                                      \
    uploader_cmd[++pos] = (char *)(arg); \
    uploader_cmd[++pos] = (char *)(val); \
  }
    PUSH_CLI_ARG(i, "--Version", ctx->product.version.str())
    if (ctx->crash.overrideProduct)
    {
      PUSH_CLI_ARG(i, "--ProductName", ctx->crash.overrideProduct)
    }
    else
    {
      PUSH_CLI_ARG(i, "--ProductName", ctx->product.name.str())
    }
    PUSH_CLI_ARG(i, "--D3DDriver", ctx->configuration.d3dDriver.c_str());
    PUSH_CLI_ARG(i, "--GPUVendor", ctx->configuration.gpuVendor.c_str());
    PUSH_CLI_ARG(i, "--BuildID", ctx->product.build.str())
    PUSH_CLI_ARG(i, "--StartupTime", ctx->product.started.str())
    PUSH_CLI_ARG(i, "--ReleaseChannel", ctx->product.channel.str())
    if (!ctx->product.comment.empty())
      PUSH_CLI_ARG(i, "--Comments", ctx->product.comment.str())
    PUSH_CLI_ARG(i, "--Hang", (ctx->crash.type == CrashInfo::BPAD_CRASHTYPE_FREEZE) ? "1" : "0")
    PUSH_CLI_ARG(i, "--lang", ctx->configuration.locale.c_str())
    PUSH_CLI_ARG(i, "--log", ctx->configuration.logFile.c_str())
    PUSH_CLI_ARG(i, "--silent", ctx->configuration.silent ? "1" : "0")
#if _TARGET_PC_MACOSX
    // TODO: On Linux new breakpad provides a full path to dump file.
    //       Consider porting the same to Mac (this way we can rename
    //       the dump even if uploader is disabled).
    PUSH_CLI_ARG(i, "--dump_id", dump_id)
    PUSH_CLI_ARG(i, "--dump_dir", dump_path)
#else  // _TARGET_PC_MACOSX
    G_UNREFERENCED(dump_id);
    PUSH_CLI_ARG(i, "--dump_path", dump_path)
#endif // _TARGET_PC_MACOSX
    for (const auto &url : ctx->configuration.urls)
      PUSH_CLI_ARG(i, "--url", url.c_str())
    PUSH_CLI_ARG(i, "--useragent", ctx->configuration.userAgent.c_str())

    const char *ct = ctx->crash.name ? ctx->crash.name : ctx->configuration.defaultCrashName.c_str();
    PUSH_CLI_ARG(i, "--crashtype", ct)
    if (!ctx->configuration.systemId.empty())
      PUSH_CLI_ARG(i, "--systemid", ctx->configuration.systemId.c_str())
    if (!ctx->configuration.environment.empty())
      PUSH_CLI_ARG(i, "--env", ctx->configuration.environment.c_str())
    snprintf(user_id, sizeof(user_id), "%llu", (unsigned long long)ctx->configuration.userId);
    PUSH_CLI_ARG(i, "--uid", user_id)
    for (const auto &f : ctx->configuration.files)
    {
      PUSH_CLI_ARG(i, "--file", f.c_str())
    }
    if (!ctx->product.commandLine.empty())
    {
      uploader_cmd[++i] = (char *)"--";
      for (const auto &arg : ctx->product.commandLine)
        uploader_cmd[++i] = (char *)arg.str();
    }
    G_ASSERT(i < MAX_CLI_ARGUMENTS);
#undef PUSH_CLI_ARG

    errno = 0;
    if (execv(ctx->configuration.senderCmd.c_str(), uploader_cmd))
    {
      // We have totally crashed and can do nothing about it but try to notify user
      fprintf(stderr, "%s: Could not execute %s: (%d) %s", ctx->product.name.str(), ctx->configuration.senderCmd.c_str(), errno,
        strerror(errno));
      // Kill the procces since it failed to exec uploader
      _exit(1);
    }
  }
}

void reset_args() {} // stub

#if _TARGET_PC_MACOSX
bool upload(const char *dump_path, const char *dump_id, void *context, bool succeeded)
#else  //_TARGET_PC_MACOSX
bool upload(const google_breakpad::MinidumpDescriptor &md, void *context, bool succeeded)
#endif //_TARGET_PC_MACOSX
{
  Context *ctx = (Context *)context;
  if (!ctx->configuration.senderCmd.empty())
#if _TARGET_PC_MACOSX
    run_uploader(ctx, dump_path, dump_id);
#else
    run_uploader(ctx, md.path(), NULL);
#endif //_TARGET_PC_MACOSX

  ctx->crash.dumped = succeeded;
  postprocess(ctx);

  return succeeded;
}


#elif _TARGET_PC_WIN
extern void *memory_reserve;

// XXX: Somehow MiniDumpIgnoreInaccessibleMemory is not accepted at runtime by
//      dbghelp 6.5 from cvs but it works with system-wide dbghelp.dll v6.1.
const int dump_mode_full = MiniDumpWithFullMemory |
                           // MiniDumpIgnoreInaccessibleMemory |
                           MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules;
std::wstring wide_dump_path;
std::wstring wide_sender_cmd;

static wchar_t args_buffer[4096] = {L'\0'};

void reset_args() { args_buffer[0] = L'\0'; }

wchar_t *prepare_args(const Context *ctx, const wchar_t *path, const wchar_t *id)
{
  wchar_t val[512];

#define ADVANCEPOS(n)                                        \
  if ((unsigned)(n) >= space_left) /* if buffer exhausted */ \
  {                                                          \
    G_FAST_ASSERT(0);                                        \
    *pos = L'\0';                                            \
    return args_buffer;                                      \
  }                                                          \
  pos += n;                                                  \
  space_left -= n

#define CONCAT(A, B) A##B
#define CONVWRITE(a, v)                                                                                             \
  do                                                                                                                \
  {                                                                                                                 \
    int written = _snwprintf(pos, space_left, L" --" CONCAT(L, a) L" \"%s\"", utf8_to_wcs((v), val, countof(val))); \
    ADVANCEPOS(written);                                                                                            \
  } while (0)

#define FMTWRITE(f, a, v)                                                                        \
  do                                                                                             \
  {                                                                                              \
    int written = _snwprintf(pos, space_left, L" --" CONCAT(L, a) L" \"" CONCAT(L, f) L"\"", v); \
    ADVANCEPOS(written);                                                                         \
  } while (0)

  wchar_t *pos = args_buffer;
  int space_left = countof(args_buffer);

  if (*args_buffer)
  {
    pos = args_buffer + wcslen(args_buffer);
    space_left = args_buffer + countof(args_buffer) - pos;
    CONVWRITE("Hang", (ctx->crash.type == CrashInfo::BPAD_CRASHTYPE_FREEZE) ? "1" : "0");

    if (!wcsstr(args_buffer, L"--crashtype"))
      CONVWRITE("crashtype", ctx->crash.getName(ctx->configuration)); // To consider: pass CrashInfo::[w]name as wide string and remove
                                                                      // this utf8_to_wcs() call

    if (!ctx->product.commandLine.empty())
    {
      int written = _snwprintf(pos, space_left, L" --");
      ADVANCEPOS(written);
      for (const auto &a : ctx->product.commandLine)
      {
        written = _snwprintf(pos, space_left, L" \"%s\"", utf8_to_wcs(a.c_str(), val, countof(val)));
        ADVANCEPOS(written);
      }
    }

    return args_buffer; // already filled up
  }

  {
    int written = _snwprintf(pos, space_left, L"%s", utf8_to_wcs(ctx->configuration.senderCmd.c_str(), val, countof(val)));
    ADVANCEPOS(written);
  }

  CONVWRITE("Version", ctx->product.version.str());
  if (ctx->crash.overrideProduct)
    CONVWRITE("ProductName", ctx->crash.overrideProduct);
  else
    CONVWRITE("ProductName", ctx->product.name.str());
  if (!ctx->configuration.productTitle.empty())
    CONVWRITE("ProductTitle", ctx->configuration.productTitle.c_str());
  CONVWRITE("D3DDriver", ctx->configuration.d3dDriver.c_str());
  CONVWRITE("GPUVendor", ctx->configuration.gpuVendor.c_str());
  CONVWRITE("BuildID", ctx->product.build.str());
  CONVWRITE("StartupTime", ctx->product.started.str());
  CONVWRITE("ReleaseChannel", ctx->product.channel.str());
  if (!ctx->product.comment.empty())
    CONVWRITE("Comments", ctx->product.comment.str());

  CONVWRITE("lang", ctx->configuration.locale.c_str());
  CONVWRITE("log", ctx->configuration.logFile.c_str());
  for (const auto &url : ctx->configuration.urls)
    CONVWRITE("url", url.c_str());
  CONVWRITE("useragent", ctx->configuration.userAgent.c_str());
  CONVWRITE("silent", ctx->configuration.silent ? "1" : "0");
  if (!ctx->configuration.systemId.empty())
    CONVWRITE("systemid", ctx->configuration.systemId.c_str());
  if (!ctx->configuration.environment.empty())
    CONVWRITE("env", ctx->configuration.environment.c_str());
  FMTWRITE("%llu", "uid", ctx->configuration.userId);
  if (ctx->crash.name || !ctx->product.commandLine.empty())
    CONVWRITE("crashtype", ctx->crash.getName(ctx->configuration));
  FMTWRITE("%s", "dump_id", id);
  FMTWRITE("%s", "dump_dir", path);
  for (const auto &f : ctx->configuration.files)
    CONVWRITE("file", f.c_str());

#undef FMTWRITE
#undef CONVWRITE
#undef ADVANCEPOS

  return args_buffer;
}


// Used as ExceptionHandler::MinidumpCallback
bool renameFullDump(const wchar_t *dump_path, const wchar_t *minidump_id, void *context, EXCEPTION_POINTERS * /*exinfo*/,
  MDRawAssertionInfo * /*assertion*/, bool succeeded)
{
  if (!succeeded)
    return succeeded; // Nothing to do.

  static char full_dump_name[DAGOR_MAX_PATH] = {'\0'};
  static char path[DAGOR_MAX_PATH] = {'\0'};
  static char id[128] = {'\0'};

  _snprintf(full_dump_name, sizeof(full_dump_name), "%s/%s.dmp", wcs_to_utf8(dump_path, path, sizeof(path)),
    wcs_to_utf8(minidump_id, id, sizeof(id)));

  Context *ctx = (Context *)context;
  if (!ctx->crash.timestamp)
    ctx->crash.timestamp = time(NULL);
  struct tm *t = localtime(&ctx->crash.timestamp);
  _snprintf(path, sizeof(path), "%s/crashDump-%.02d.%.02d.%.02d.full.dmp", ctx->configuration.dumpPath.c_str(), t->tm_hour, t->tm_min,
    t->tm_sec);

  rename(full_dump_name, path);

  return succeeded;
}


bool filter(void *context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo * /*assertion*/)
{
  interlocked_exchange(g_debug_is_in_fatal, 1);

  if (exinfo && !WinHwExceptionUtils::checkException(exinfo->ExceptionRecord->ExceptionCode))
  {
    ((Context *)context)->crash.reset();
    return false;
  }

  Context *ctx = (Context *)context;

#if DAGOR_DBGLEVEL > 0
  // Fill callstack to report to user
  if (!ctx->crash.isManual && exinfo)
  {
    const size_t stackSize = 32;
    void *stack[stackSize];
    ::stackhlp_fill_stack_exact(stack, stackSize, exinfo->ContextRecord);
    ctx->crash.callstack = ::stackhlp_get_call_stack(breakpad_stk, sizeof(breakpad_stk), stack, stackSize);
  }
#endif // DAGOR_DBGLEVEL > 0

  if (ctx->configuration.treatDebugBreakAsFreeze && exinfo && exinfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
    ctx->crash.type = CrashInfo::BPAD_CRASHTYPE_FREEZE;

  preprocess(ctx);

  if (memory_reserve)
    VirtualFree(memory_reserve, 0, MEM_RELEASE);
  return true;
}


bool upload(const wchar_t *dump_path, const wchar_t *minidump_id, void *context, EXCEPTION_POINTERS * /*exinfo*/,
  MDRawAssertionInfo * /*assertion*/, bool succeeded)
{
  Context *ctx = (Context *)context;

  if (!ctx->configuration.senderCmd.empty())
  {
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    wchar_t *args = prepare_args(ctx, dump_path, minidump_id);
    BOOL ok = CreateProcessW(wide_sender_cmd.c_str(), args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    constexpr size_t bufsz = 8192;
    char *buf = (char *)alloca(bufsz);
    memset(buf, 0, bufsz);
    if (ok)
      debug("SPAWN: ok: '%s'", wcs_to_utf8(args, buf, bufsz));
    else
      debug("SPAWN: err %d: '%s'", ::GetLastError(), wcs_to_utf8(args, buf, bufsz));
  }

  // Renaming might not be really safe, so upload first, doubledump next.
  if (ctx->configuration.doubleDump)
  {
    typedef google_breakpad::ExceptionHandler ExHndl;
    ExHndl::WriteMinidump(wide_dump_path, (MINIDUMP_TYPE)dump_mode_full, renameFullDump, context);
  }

  ctx->crash.dumped = succeeded;
  postprocess(ctx);

  return succeeded;
}

#endif

filter_cb_t get_filter_cb() { return filter; }
minidump_cb_t get_upload_cb() { return upload; }

} // namespace breakpad
