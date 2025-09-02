// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sh_stat.h"
#include <util/dag_string.h>
#include "shLog.h"
#include <stdio.h>
#include "shcode.h"
#include "linkShaders.h"

int ShaderCompilerStat::totalVariants = 0;
ShaderCompilerStat::DroppedVariants ShaderCompilerStat::droppedVariants = {0};
Tab<ShaderCompilerStat::ShaderStatistics> ShaderCompilerStat::shaderStatisticsList(stdmem_ptr());
int ShaderCompilerStat::hlslCompileCount = 0, ShaderCompilerStat::hlslCacheHitCount = 0, ShaderCompilerStat::hlslEqResultCount = 0;
dag::AtomicInteger<int> ShaderCompilerStat::hlslExternalCacheHitCount = 0;
unsigned lastFshSize, lastFshCount, lastVprSize, lastVprCount, lastStcodeSize; // @TODO: this shall be moved too, but for now we need
                                                                               // to cache stats

void ShaderCompilerStat::reset()
{
  totalVariants = 0;
  memset(&droppedVariants, 0, sizeof(droppedVariants));
  clear_and_shrink(shaderStatisticsList);
}

void ShaderCompilerStat::printReport(const char *dir)
{
  if (totalVariants != 0)
  {
    sh_debug(SHLOG_INFO, "Processed variants: %d, skipped: %d (assume)", totalVariants, droppedVariants.dueToAssumeVar);
  }
  if (hlslCompileCount)
    sh_debug(SHLOG_INFO,
      "HLSL compilation: %d total;   %d reused cached;   cache hit ratio=%.3f;"
      "   equ. result count=%d;   external cache hit count: %d;   unique compilations: %i",
      hlslCompileCount, hlslCacheHitCount, (float)hlslCacheHitCount / (float)hlslCompileCount, hlslEqResultCount,
      hlslExternalCacheHitCount.load(), getUniqueCompilationCount());

  if (lastFshSize + lastVprSize > 0)
  {
    sh_debug(SHLOG_INFO,
      "Total shader bytes: %d,\n"
      "FSH: %7d bytes in %4d shaders\n"
      "VPR: %7d bytes in %4d shaders\n"
      "stcode bytes: %d",
      lastFshSize + lastVprSize, lastFshSize, lastFshCount, lastVprSize, lastVprCount, lastStcodeSize);

    if (!dir)
      return;
    FILE *file = fopen(String(dir) + "\\Stat.txt", "w");
    fprintf(file, "                                                                           "
                  "total                     texture                  arithmetic                  flow\n");
    fprintf(file, "                   shader                               entry  average      "
                  "min      max  average      min      max  average      min      max  average      min      max\n");
    for (unsigned int shaderNo = 0; shaderNo < shaderStatisticsList.size(); shaderNo++)
    {
      ShaderStatistics &stat = shaderStatisticsList[shaderNo];
      fprintf(file, "%25s %35s %8.1f %8d %8d %8.1f %8d %8d %8.1f %8d %8d %8.1f %8d %8d\n", (const char *)stat.name,
        (const char *)stat.entry, stat.hlslVariants ? stat.totalInstructions / (float)stat.hlslVariants : 0, stat.minInstructions,
        stat.maxInstructions, stat.hlslVariants ? stat.totalTextureInstructions / (float)stat.hlslVariants : 0,
        stat.minTextureInstructions, stat.maxTextureInstructions,
        stat.hlslVariants ? stat.totalArithmeticInstructions / (float)stat.hlslVariants : 0, stat.minArithmeticInstructions,
        stat.maxArithmeticInstructions, stat.hlslVariants ? stat.totalFlowInstructions / (float)stat.hlslVariants : 0,
        stat.minFlowInstructions, stat.maxFlowInstructions);
    }
    fclose(file);
  }
}

void ShaderCompilerStat::collectTargetStats(const shc::TargetContext &ctx)
{
  count_shader_stats(lastFshSize, lastFshCount, lastVprSize, lastVprCount, lastStcodeSize, ctx);
}
