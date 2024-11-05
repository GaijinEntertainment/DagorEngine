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


class DasScriptsData
{
public:
  DasScriptsData();
  ~DasScriptsData() = default;

  static bool is_das_inited();

public:
  das::ModuleGroup moduleGroup;
  das::smart_ptr<das::ModuleFileAccess> fAccess;
  DasLogWriter logWriter;
  das::DebugInfoHelper dbgInfoHelper;
  das::daScriptEnvironment *dasEnv = nullptr;

  das::TypeInfo *typeGuiContextRef = nullptr, *typeConstElemRenderDataRef = nullptr, *typeConstRenderStateRef = nullptr,
                *typeConstPropsRef = nullptr;

  AotMode aotMode = AotMode::AOT;
  LogAotErrors needAotErrorLog = LogAotErrors::YES;
};


void bind_das(Sqrat::Table &exports);

} // namespace darg
