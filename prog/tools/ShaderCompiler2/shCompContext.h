// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompilationInfo.h"
#include "commonUtils.h"
#include "hwAssembly.h"
#include "globalConfig.h"
#include "refinedBlockRegisterAllocator.h"
#include "varMap.h"
#include <ioSys/dag_dataBlock.h>
#include <dag/dag_vectorMap.h>


namespace shc
{

class TargetContext;

class CompilationContext
{
  const ShCompilationInfo *mInfo = nullptr;
  eastl::string commonHlslDefinesCached;
  FILE *dependencyDumpFile = nullptr;
  RefinedBlockRegisterAllocator mRbAllocator;
  VarNameMap mRbVarNameMap;
  dag::VectorMap<int, RefinedBlockRegister> mRbVarIdToRegInfo;

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

  auto &rbAllocator() { return mRbAllocator; }
  const auto &rbAllocator() const { return mRbAllocator; }

  auto &rbVarNameMap() { return mRbVarNameMap; }
  const auto &rbVarNameMap() const { return mRbVarNameMap; }

  auto &rbVars() { return mRbVarIdToRegInfo; }
  const auto &rbVars() const { return mRbVarIdToRegInfo; }

  TargetContext makeTargetContext(const char *fname, bool preshader_scan = false);

  bool isOnlyCollectingDependencies() const { return dependencyDumpFile != nullptr; }
  void reportDepFile(char const *name) const
  {
    if (dependencyDumpFile)
      fprintf(dependencyDumpFile, "%s\n", name);
  }
};

} // namespace shc
