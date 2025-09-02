// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************

/************************************************************************/
#ifndef __SH_STAT_H
#define __SH_STAT_H

#include "shTargetContext.h"
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_atomic_types.h>

namespace ShaderCompilerStat
{
extern int totalVariants;
struct DroppedVariants
{
  int dueToAssumeVar;
};
extern DroppedVariants droppedVariants;
extern int hlslCompileCount, hlslCacheHitCount, hlslEqResultCount;
extern dag::AtomicInteger<int> hlslExternalCacheHitCount;

struct ShaderStatistics
{
  String name;
  String entry;
  unsigned int hlslVariants;
  int minInstructions;
  int maxInstructions;
  int totalInstructions;
  int minTextureInstructions;
  int maxTextureInstructions;
  int totalTextureInstructions;
  int minArithmeticInstructions;
  int maxArithmeticInstructions;
  int totalArithmeticInstructions;
  int minFlowInstructions;
  int maxFlowInstructions;
  int totalFlowInstructions;

  ShaderStatistics()
  {
    hlslVariants = 0;
    minInstructions = 0;
    maxInstructions = 0;
    totalInstructions = 0;
    minTextureInstructions = 0;
    maxTextureInstructions = 0;
    totalTextureInstructions = 0;
    minArithmeticInstructions = 0;
    maxArithmeticInstructions = 0;
    totalArithmeticInstructions = 0;
    minFlowInstructions = 0;
    maxFlowInstructions = 0;
    totalFlowInstructions = 0;
  }
};

extern Tab<ShaderStatistics> shaderStatisticsList;


void reset();
void printReport(const char *dir);
void collectTargetStats(const shc::TargetContext &ctx);
inline int getUniqueCompilationCount() { return hlslCompileCount - hlslCacheHitCount - hlslExternalCacheHitCount.load(); }
} // namespace ShaderCompilerStat

#endif
