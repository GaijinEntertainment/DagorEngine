// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompilationInfo.h"
#include "commonUtils.h"
#include "hwAssembly.h"
#include <ioSys/dag_dataBlock.h>


namespace shc
{

class TargetContext;

class CompilationContext
{
  const ShCompilationInfo *mInfo = nullptr;
  eastl::string commonHlslDefinesCached;

public:
  CompilationContext(const ShCompilationInfo &info) : mInfo{&info}
  {
    commonHlslDefinesCached = assembly::build_common_hardware_defines_hlsl(*this);
  }

  PINNED_TYPE(CompilationContext)

  const ShCompilationInfo &compInfo() const
  {
    G_ASSERT(mInfo);
    return *mInfo;
  }

  const eastl::string &commonHlslDefines() const { return commonHlslDefinesCached; }

  TargetContext makeTargetContext(const char *fname) const;
};

} // namespace shc
