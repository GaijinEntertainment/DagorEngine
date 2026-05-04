// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompilationInfo.h"
#include "commonUtils.h"
#include "hwAssembly.h"
#include "globalConfig.h"
#include <ioSys/dag_dataBlock.h>


namespace shc
{

class TargetContext;

class CompilationContext
{
  const ShCompilationInfo *mInfo = nullptr;
  eastl::string commonHlslDefinesCached;
  FILE *dependencyDumpFile = nullptr;

public:
  CompilationContext(const ShCompilationInfo &info) : mInfo{&info}
  {
    commonHlslDefinesCached = assembly::build_common_hardware_defines_hlsl(*this);
    if (shc::config().dependencyDumpMode)
    {
      if (shc::config().dependencyDumpFile.empty())
      {
        dependencyDumpFile = stdout;
      }
      else
      {
        dependencyDumpFile = fopen(shc::config().dependencyDumpFile.str(), "w+");
        if (!dependencyDumpFile)
          sh_debug(SHLOG_FATAL, "Failed to open dep-dump file '%s'", shc::config().dependencyDumpFile.str());
      }
    }
  }

  ~CompilationContext()
  {
    if (dependencyDumpFile && dependencyDumpFile != stdout)
      fclose(eastl::exchange(dependencyDumpFile, nullptr));
  }

  PINNED_TYPE(CompilationContext)

  const ShCompilationInfo &compInfo() const
  {
    G_ASSERT(mInfo);
    return *mInfo;
  }

  const eastl::string &commonHlslDefines() const { return commonHlslDefinesCached; }

  TargetContext makeTargetContext(const char *fname) const;

  bool isOnlyCollectingDependencies() const { return dependencyDumpFile != nullptr; }
  void reportDepFile(char const *name) const
  {
    if (dependencyDumpFile)
      fprintf(dependencyDumpFile, "%s\n", name);
  }
};

} // namespace shc
