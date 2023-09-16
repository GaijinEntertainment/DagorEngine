#pragma once

#include <daScript/daScript.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Sqrat
{
class Table;
}


namespace darg
{

class DasLogWriter : public das::TextWriter
{
  virtual void output() override;
  int pos = 0;
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

  das::TypeInfo *typeGuiContextRef = nullptr, *typeConstElemRenderDataRef = nullptr, *typeConstRenderStateRef = nullptr,
                *typeConstPropsRef = nullptr;
};


void bind_das(Sqrat::Table &exports);

} // namespace darg
