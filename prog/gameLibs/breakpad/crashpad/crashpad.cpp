// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <breakpad/binder.h>
#include <client/crashpad_client.h>
#include <util/dag_finally.h>
#include <generic/dag_initOnDemand.h>
#include <base/debug/alias.h>
#if _TARGET_PC_WIN
#include <client/simulate_crash_win.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_info.h>
#include <windows.h>
#include <io.h>
#endif
#include "context.h"
#include <string>
#include <algorithm>
#include <osApiWrappers/dag_unicode.h>
#include <startup/dag_globalSettings.h> // for dgs_argv
#include <osApiWrappers/dag_direct.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <folders/folders.h>

extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

namespace breakpad
{
using namespace crashpad;

Product::Product(bool with_defaults) // To consider: extract this to common with breakpad code?
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

static Context *context = nullptr;
static InitOnDemand<CrashpadClient> crashpad_client;
static SimpleStringDictionary simple_annotations_dict;

void init(Product &&prod, Configuration &&cfg_)
{
  G_ASSERT(!context);
  using namespace std;

  Finally shutdowner([] { shutdown(); });

  context = new Context(std::move(prod), std::move(cfg_));
  const Configuration &cfg = context->configuration;
  int iterator = 0;
  while (const char *path = ::dgs_get_argv("add_file_to_report", iterator))
    add_file_to_report(path);

#if _TARGET_PC_WIN
  wchar_t wtmp[DAGOR_MAX_PATH];
  char tmp[DAGOR_MAX_PATH];
  String exeDirStr = folders::get_exe_dir();
  const char *exeDirPtr = exeDirStr.c_str();
#ifdef _M_IX86 // win32 -> win64
  snprintf(tmp, sizeof(tmp), "%s../win64/", exeDirPtr);
  dd_simplify_fname_c(tmp);
  exeDirPtr = tmp;
#endif
  wstring exeDir = utf8_to_wcs(exeDirPtr, wtmp, countof(wtmp));
  replace(exeDir.begin(), exeDir.end(), L'/', L'\\');

  if (!dd_dir_exists(cfg.dumpPath.c_str()))
    dd_mkdir(cfg.dumpPath.c_str());
  wstring dumpDbPathStr = utf8_to_wcs(cfg.dumpPath.c_str(), wtmp, countof(wtmp));
  replace(dumpDbPathStr.begin(), dumpDbPathStr.end(), L'/', L'\\');
  base::FilePath dumpDbPath(dumpDbPathStr);

  if (auto dbPtr = CrashReportDatabase::Initialize(dumpDbPath))
  {
    bool disableUpload = dgs_get_argv("crashpad-disable-upload") != nullptr;
    // Not inited db or explicitly requested upload disable
    if (_waccess((dumpDbPathStr + L"settings.dat").c_str(), 0) != 0 || disableUpload)
    {
      auto dbSettings = dbPtr->GetSettings();
      G_ASSERT_RETURN(dbSettings, ); // Failed to create file?
      bool uploadsEnabled = false;
      if (!dbSettings->GetUploadsEnabled(&uploadsEnabled) || (!uploadsEnabled != disableUpload))
        dbSettings->SetUploadsEnabled(!disableUpload);
    }
  }
  else
    logerr("Failed to init crashreports db at '%s'", wcs_to_utf8(dumpDbPath.value().c_str(), tmp, sizeof(tmp)));

  map<string, string> annotations = {{"BuildID", context->product.build.c_str()}, {"ReleaseChannel", context->product.channel.c_str()},
    {"StartupTime", context->product.started.c_str()}};
  vector<base::FilePath> attachments = {base::FilePath(wstring(utf8_to_wcs(cfg.logFile.c_str(), wtmp, countof(wtmp))))};
  for (auto &fn : cfg.files)
    attachments.emplace_back(wstring(utf8_to_wcs(fn.c_str(), wtmp, countof(wtmp))));
  // Note: don't append ?guid=... to submit request as we use systemId instead (besides it could be zero if written by launcher)
  vector<string> aargs = {"--no-identify-client-via-url", "--no-periodic-tasks"};
  if (dgs_get_argv("crashpad-no-upload-gzip"))
    aargs.emplace_back("--no-upload-gzip");
  crashpad_client.demandInit();
  if (!crashpad_client->StartHandler(base::FilePath(exeDir + L"crashpad_handler.exe"), dumpDbPath, {},
        "http://palvella.gaijin.net/submit", annotations, aargs, true, false, attachments))
  {
    logerr("Failed to init crashpad. Missing crashpad_handler.exe?");
    return;
  }
  wstring modPath = exeDir + L"crashpad_wer.dll";
  DWORD dwZero = 0;
  if (RegSetKeyValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\Windows Error Reporting\\"
        L"RuntimeExceptionHelperModules",
        modPath.c_str(), REG_DWORD, &dwZero, sizeof(DWORD)) == ERROR_SUCCESS)
  {
    if (!crashpad_client->RegisterWerModule(modPath))
      logerr("RegisterWerModule('%s') failed", wcs_to_utf8(modPath.c_str(), tmp, sizeof(tmp)));
  }
  else
    logwarn("Failed to register 'RuntimeExceptionHelperModules' in Windows registry for '%s'",
      wcs_to_utf8(modPath.c_str(), tmp, sizeof(tmp)));

  CrashpadInfo::GetCrashpadInfo()->set_simple_annotations(&simple_annotations_dict);
  configure(context->product);
  set_user_id(context->configuration.userId);

  debug("crashpad enabled");
#endif
  shutdowner.release();
}

void shutdown()
{
  crashpad_client.demandDestroy();
  del_it(context);
}

void dump(const CrashInfo &info)
{
  if (!context)
    return;

  context->crash = info;
  context->crash.isManual = true;

  if (auto prepcb = context->configuration.preprocess)
    prepcb(context->crash);
  if (auto hookcb = context->configuration.hook)
    hookcb(context->crash);

  if (context->crash.type == CrashInfo::BPAD_CRASHTYPE_FREEZE)
    simple_annotations_dict.SetKeyValue("Hang", "1");

  CRASHPAD_SIMULATE_CRASH();
}

Configuration *get_configuration() { return context ? &context->configuration : NULL; }

void configure(const Product &p)
{
  if (!context)
    return;
  Product &oldProd = context->product;
  if (!p.version.empty())
  {
    oldProd.version = p.version;
    simple_annotations_dict.SetKeyValue("Version", p.version.c_str());
  }
  if (!p.name.empty())
  {
    oldProd.name = p.name;
    simple_annotations_dict.SetKeyValue("ProductName", p.name.c_str());
  }
}

void set_environment(const char *) {}

void add_file_to_report(const char *path)
{
  if (!context || !path || !*path)
    return;

  context->configuration.files.emplace_back(path);
}

void remove_file_from_report(const char *path)
{
  if (!context || !path || !*path)
    return;

  context->configuration.files.remove(path);
}

void set_user_id(uint64_t uid)
{
  if (!context)
    return;
  context->configuration.userId = uid;

  std::string cvalue = std::string("System ID: ") + context->configuration.systemId.c_str();
  if (uid)
  {
    cvalue += " User ID: ";
    cvalue += std::to_string(uid);
  }
  simple_annotations_dict.SetKeyValue("Comments", cvalue);
}

void set_locale(const char *) {}

void set_product_title(const char *product_title)
{
  if (!context)
    return;
  context->configuration.productTitle = product_title;
  simple_annotations_dict.SetKeyValue("ProductTitle", product_title);
}

void set_d3d_driver_data(const char *driver, const char *vendor)
{
  if (!context)
    return;

  context->configuration.d3dDriver = driver;
  context->configuration.gpuVendor = vendor;

  simple_annotations_dict.SetKeyValue("D3DDriver", driver);
  simple_annotations_dict.SetKeyValue("GPUVendor", vendor);
}

bool is_enabled() { return context && crashpad_client; }

eastl::string Configuration::getSenderPath() { return {}; }

} // namespace breakpad
