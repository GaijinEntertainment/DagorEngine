// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Sqrat
{
class Table;
}

struct SQVM;
typedef struct SQVM *HSQUIRRELVM;

namespace darg
{
typedef void (*TInitDasEnv)();

enum class AotMode
{
  NO_AOT,
  AOT,
};

enum class LogAotErrors
{
  NO,
  YES,
};

void set_das_loading_settings(HSQUIRRELVM vm, AotMode aot_mode, LogAotErrors need_aot_error_log);

class DasLogWriter : public das::TextWriter
{
  virtual void output() override;
  uint64_t pos = 0;
};


struct DasScript
{
  eastl::string filename;
  das::smart_ptr<das::Context> ctx;
  das::ProgramPtr program;
};


struct DasEnvironmentGuard
{
  das::daScriptEnvironment *initialOwned;
  das::daScriptEnvironment *initialBound;
  DasEnvironmentGuard(das::daScriptEnvironment *environment = nullptr);
  ~DasEnvironmentGuard();
};


class DasScriptsData
{
public:
  DasScriptsData();
  ~DasScriptsData() = default;

  void initDasEnvironment(TInitDasEnv init_callback);
  void shutdownDasEnvironment();
  void initModuleGroup();
  void resetBeforeReload();

public:
  das::daScriptEnvironment *dasEnv = nullptr;
  bool isOwnedDasEnv = false;
  TInitDasEnv initCallback = nullptr;

  das::unique_ptr<das::ModuleGroup> moduleGroup;
  DasLogWriter logWriter;
  das::unique_ptr<das::DebugInfoHelper> dbgInfoHelper;
  das::smart_ptr<das::ModuleFileAccess> fAccess;
  AotMode aotMode = AotMode::AOT;
  LogAotErrors needAotErrorLog = LogAotErrors::YES;

  das::TypeInfo *typeGuiContextRef = nullptr;
  das::TypeInfo *typeConstElemRenderDataRef = nullptr;
  das::TypeInfo *typeConstRenderStateRef = nullptr;
  das::TypeInfo *typeConstPropsRef = nullptr;
};


void bind_das(Sqrat::Table &exports);

} // namespace darg
