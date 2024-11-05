// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <breakpad/binder.h>
#include "callbacks.h"
#include "context.h"
#include <EASTL/utility.h>

#if _TARGET_PC_WIN
#include <client/windows/handler/exception_handler.h>
#include <windows.h>
#include <memory/dag_framemem.h>
#if !_M_ARM64
#include <MinHook.h>
#endif
#elif _TARGET_PC_LINUX
#include <client/linux/handler/exception_handler.h>
#elif _TARGET_PC_MACOSX
#include <client/mac/handler/exception_handler.h>
#endif


#include <string.h>

#include <osApiWrappers/dag_unicode.h>
#include <startup/dag_globalSettings.h> // for dgs_argv
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <folders/folders.h>

extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;


namespace breakpad
{

Product::Product(bool with_defaults)
{
  if (!with_defaults)
    return;

  version = "0.0.0.0";
  name = "Unknown";

  const int n = 6;
  char signParseBuf[8][n];
  sscanf(dagor_exe_build_date, "%s %s %s", signParseBuf[1], signParseBuf[2], signParseBuf[0]);

  sscanf(dagor_exe_build_time, "%2s:%2s:%2s", signParseBuf[3], signParseBuf[4], signParseBuf[5]);

  for (int i = 0; i < n; i++)
    build.append(signParseBuf[i]);

  started.printf(32, "%llu", time(NULL));

#if (DAGOR_DBGLEVEL == 1)
  channel = "dev";
#elif (DAGOR_DBGLEVEL == 0)
  channel = "rel";
#elif (DAGOR_DBGLEVEL == -1)
  channel = "irel";
#elif (DAGOR_DBGLEVEL == 2)
  channel = "dbg";
#else
  channel = "Unknown";
#endif

  if (::dgs_argv && *::dgs_argv) // Otherwise we have no defaults to use
  {
    String &exe = commandLine.push_back();
    exe = ::dgs_argv[0];
#if _TARGET_PC_WIN
    char utf8buf[MAX_PATH * 3] = {'\0'};
    wchar_t exePathW[MAX_PATH] = {L'\0'};
    if (::GetModuleFileNameW(NULL, exePathW, countof(exePathW)))
      exe = wcs_to_utf8(exePathW, utf8buf, countof(utf8buf));
#elif _TARGET_PC_LINUX
    char buf[MAX_PATH] = {'\0'};
    int sz = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (sz > 0 && sz < sizeof(buf))
      exe = buf;
#endif


    for (int i = 1; i < ::dgs_argc; ++i)
    {
      String &arg = commandLine.push_back();
      char *space = strchr(::dgs_argv[i], ' ');
      if (space)
        arg += "\\\"";
#if _TARGET_PC_WIN
      Tab<char> buf;
      arg += acp_to_utf8(::dgs_argv[i], buf);
#else
      arg += ::dgs_argv[i];
#endif
      if (space)
        arg += "\\\"";
    }
  }
}


typedef google_breakpad::ExceptionHandler ExHndl;

static ExHndl *handler = NULL;
static Context *context = NULL;

#if _TARGET_PC_WIN
// Some microsoft CRT functions reset our exception filter by calling SetUnhandledExceptionFilter(NULL)
// Prevent this behaviour by disabling SetUnhandledExceptionFilter() and making sure that no fastfail supported in order
// to force exception reported
static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI noop_SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER)
{
  return NULL; // Do nothing
}
static decltype(&IsProcessorFeaturePresent) fpIsProcessorFeaturePresent = nullptr;
static BOOL WINAPI no_fastfail_IsProcessorFeaturePresent(DWORD pf)
{
  return pf != PF_FASTFAIL_AVAILABLE && fpIsProcessorFeaturePresent(pf);
}
static void setup_crt_hooks()
{
#if !_M_ARM64
  G_VERIFY(MH_Initialize() == MH_OK);
  if (MH_CreateHookApi(L"kernelbase", "SetUnhandledExceptionFilter", &noop_SetUnhandledExceptionFilter, NULL) ==
      MH_ERROR_MODULE_NOT_FOUND)
    MH_CreateHookApi(L"kernel32", "SetUnhandledExceptionFilter", &noop_SetUnhandledExceptionFilter, NULL);
  if (MH_CreateHookApi(L"kernelbase", "IsProcessorFeaturePresent", &no_fastfail_IsProcessorFeaturePresent,
        (void **)&fpIsProcessorFeaturePresent) == MH_ERROR_MODULE_NOT_FOUND)
    MH_CreateHookApi(L"kernel32", "IsProcessorFeaturePresent", &no_fastfail_IsProcessorFeaturePresent,
      (void **)&fpIsProcessorFeaturePresent);
  MH_EnableHook(MH_ALL_HOOKS);
#else
  G_UNUSED(&noop_SetUnhandledExceptionFilter);
  G_UNUSED(&no_fastfail_IsProcessorFeaturePresent);
#endif
}

void *memory_reserve = NULL;
static const size_t memory_reserve_size = 50 * 1024 * 1024;
#endif

void init(Product &&prod, Configuration &&cfg_)
{
  G_ASSERT(!context && !handler);

  context = new Context(eastl::move(prod), eastl::move(cfg_));
  context->crash.reset();
  const Configuration &cfg = context->configuration;
  int iterator = 0;
  while (const char *path = ::dgs_get_argv("add_file_to_report", iterator))
    add_file_to_report(path);

#if _TARGET_PC_LINUX
  google_breakpad::MinidumpDescriptor md(cfg.dumpPath.c_str());
  handler = new ExHndl(md, get_filter_cb(), get_upload_cb(), context,
    true, // install_handler
    -1);  // For now, we use in-process generation.
#elif _TARGET_PC_MACOSX
  handler = new ExHndl(cfg.dumpPath.c_str(), get_filter_cb(), get_upload_cb(), context,
    true,  // install_handler
    NULL); // port_name
#elif _TARGET_PC_WIN
  if (!memory_reserve)
  {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (!GlobalMemoryStatusEx(&statex) || statex.ullTotalVirtual > (2048ull << 20)) // don't reserve memory on 32-bit OS
      memory_reserve = VirtualAlloc(NULL, memory_reserve_size, MEM_RESERVE, 0);
  }

  Tab<wchar_t> tmp(framemem_ptr());
  wide_dump_path = cfg.dumpPath.length() ? convert_utf8_to_u16_buf(tmp, cfg.dumpPath.c_str(), cfg.dumpPath.length()) : L"";
  wide_sender_cmd = cfg.senderCmd.length() ? convert_utf8_to_u16_buf(tmp, cfg.senderCmd.c_str(), cfg.senderCmd.length()) : L"";
  handler = new ExHndl(wide_dump_path, get_filter_cb(), get_upload_cb(), context, ExHndl::HANDLER_ALL, MiniDumpNormal,
    (const wchar_t *)NULL, // pipe_name, resolves ambiguity
    NULL);                 // custom_info
  handler->set_handle_debug_exceptions(cfg.catchDebug);
  setup_crt_hooks();
  prepare_args(context, handler->dump_path().c_str(), handler->next_minidump_id().c_str()); // pre-init
  // Dump can't be written unless there is directory to write it to exist
  // (logs directory created on first log write, which might not be case at the time of this call)
  if (!wide_dump_path.empty())
    CreateDirectoryW(wide_dump_path.c_str(), NULL);
#endif // _TARGET_PC_LINUX|_TARGET_PC_MACOSX|_TARGET_PC_WIN
  G_ASSERT(handler);
}


void shutdown()
{
  del_it(handler);
  del_it(context);
#if _TARGET_PC_WIN && !_M_ARM64
  MH_Uninitialize();
#endif
}

static const char *cut_file_path(const char *expression)
{
  const char *comma = strchr(expression, ',');
  if (!comma)
    return expression;
  const char *c = comma;
  for (; c != expression && *c != '\\' && *c != '/'; --c)
    ;
  return (c == expression) ? c : c + 1;
}

// NOTE: dump() is not intended to be called from compromised context
void dump(const CrashInfo &info)
{
  if (!context || !handler)
    return;

  context->crash = info;
  context->crash.isManual = true;

#if _TARGET_PC_WIN || _TARGET_PC_LINUX
  // TODO: Patch breakpad to make the same work on Mac OS X
  //       to provide brief info about fatal() context. If possible.
  if (info.expression || info.file)
  {
    MDRawAssertionInfo assertion;
    memset(&assertion, 0, sizeof(assertion));
    if (info.expression)
    {
      const char *cutExpr = cut_file_path(info.expression);
      ::utf8_to_utf16(cutExpr, (int)strlen(cutExpr), assertion.expression, countof(assertion.expression));
    }
    if (info.file)
      ::utf8_to_utf16(info.file, (int)strlen(info.file), assertion.file, countof(assertion.file));

    assertion.line = info.line;
    handler->WriteMinidump(&assertion);
  }
  else
    handler->WriteMinidump();
#else
  handler->WriteMinidump(true);
#endif
}

Configuration *get_configuration() { return context ? &context->configuration : NULL; }

void reload_args()
{
  // Clear pre-built command line string (we need it at least on Windows)
  reset_args();

#if _TARGET_PC_WIN
  // We need to fill pre-built command line string on Windows as we do in init()
  // Pre-built string must be already filled at the moment of bpreport.exe call
  if (handler != nullptr)
    prepare_args(context, handler->dump_path().c_str(), handler->next_minidump_id().c_str());
#endif
}

void configure(const Product &p)
{
  if (!context)
    return;

  Product &oldProd = context->product;

  if (!p.version.empty())
    oldProd.version = p.version;

  if (!p.name.empty())
    oldProd.name = p.name;

  if (!p.build.empty())
    oldProd.build = p.build;

  if (!p.started.empty())
    oldProd.started = p.started;

  if (!p.channel.empty())
    oldProd.channel = p.channel;

  if (!p.comment.empty())
    oldProd.comment = p.comment;

  reload_args();
}

void set_environment(const char *env)
{
  if (context)
  {
    context->configuration.environment = env;
    reload_args();
  }
}

void add_file_to_report(const char *path)
{
  if (!context || !path || !*path)
    return;

  context->configuration.files.emplace_back(path);
  reload_args();
}

void remove_file_from_report(const char *path)
{
  if (!context || !path || !*path)
    return;

  context->configuration.files.remove(path);
  reload_args();
}

void set_user_id(uint64_t uid)
{
  if (!context)
    return;

  context->configuration.userId = uid;
  reload_args();
}

void set_locale(const char *locale_code)
{
  if (!context)
    return;

  context->configuration.locale = locale_code;
  reload_args();
}

void set_product_title(const char *product_title)
{
  if (!context)
    return;

  context->configuration.productTitle = product_title;
  reload_args();
}

void set_d3d_driver_data(const char *driver, const char *vendor)
{
  if (!context)
    return;

  context->configuration.d3dDriver = driver;
  context->configuration.gpuVendor = vendor;
  reload_args();
}

bool is_enabled() { return context && handler; }

eastl::string Configuration::getSenderPath()
{
  if (::dgs_get_argv("disable_breakpad"))
    return {};

  String fullpath(folders::get_exe_dir());
  G_ASSERT(!fullpath.empty());
#if _TARGET_PC_WIN && _TARGET_64BIT
  fullpath += "../win32/";
#endif
  fullpath += "bpreport";
#if _TARGET_PC_WIN
  fullpath += ".exe";
#endif
  return fullpath.str();
}

} // namespace breakpad
