//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <time.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/list.h>

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_stdint.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>

namespace systeminfo
{
eastl::string get_system_id();
}


namespace breakpad
{

struct Product
{
  String version;
  String name;
  String build;
  String started;
  String channel;
  String comment;
  Tab<String> commandLine;

  Product(bool with_defaults = true);
}; // struct Product


struct Configuration;

// Does not free any memory passed to it:
// it is unsafe in a compromised context
struct CrashInfo
{
  enum Type
  {
    BPAD_CRASHTYPE_UNDEFINED = 0,
    BPAD_CRASHTYPE_FREEZE,
  };
  Type type;

  time_t timestamp;
  const char *name;
  const char *expression;
  const char *callstack;
  const char *file;
  int line;
  int64_t errorCode;
  bool isManual;
  bool dumped;

  const char *overrideProduct;

  CrashInfo(Type t = BPAD_CRASHTYPE_UNDEFINED)
  {
    reset();
    type = t;
  }

  void reset() { ::memset(this, 0, sizeof(CrashInfo)); }
  const char *getName(const Configuration &cfg) const;
}; // struct CrashInfo


struct Configuration
{
  typedef void (*hook_type)(const CrashInfo &ci);
  typedef void (*preprocessor_type)(CrashInfo &ci);

  bool silent = ::dgs_execute_quiet;
  bool doubleDump = ::dgs_get_argv("doublecrashdump");
  bool catchDebug = true; // Meaningless anywhere but Windows
  bool treatDebugBreakAsFreeze = false;

  eastl::vector<eastl::string> urls;
  eastl::string userAgent = "Breakpad/1.1";
  eastl::string dumpPath = ::get_log_directory();
  eastl::string logFile = ::get_log_filename();
  eastl::list<eastl::string> files;

  eastl::string senderCmd = getSenderPath();

  eastl::string systemId = systeminfo::get_system_id();
  uint64_t userId = 0;
  eastl::string defaultCrashName = "Exception";
  eastl::string locale = "en-US";
  eastl::string environment;
  eastl::string productTitle;

  // XXX: hooks might be called in a compromised context.
  hook_type hook = nullptr;               // Called after dump generation
  hook_type notify = nullptr;             // Called before dump generation
                                          // when silent-mode is not enabled
  preprocessor_type preprocess = nullptr; // Called before dump generation always

private:
  eastl::string getSenderPath();
}; // struct Configuration


inline const char *CrashInfo::getName(const Configuration &cfg) const
{
  return name ? name : (isManual ? "Fatal" : cfg.defaultCrashName.c_str());
}

void init(Product &&prod, Configuration &&cfg);
void shutdown();
void dump(const CrashInfo &info);

Configuration *get_configuration();
void configure(const Product &product);
void add_file_to_report(const char *path);
void remove_file_from_report(const char *path);
void set_user_id(uint64_t uid);
void set_locale(const char *locale_code);
void set_product_title(const char *product_name);
void set_environment(const char *env);

bool is_enabled();

} // namespace breakpad
