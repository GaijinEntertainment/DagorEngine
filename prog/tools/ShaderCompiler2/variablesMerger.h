// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "shExpr.h"
#include "commonUtils.h"
#include "nameMap.h"

#include <drv/3d/dag_consts.h>
#include <generic/dag_functionRef.h>

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/map.h>
#include <EASTL/optional.h>
#include <EASTL/bitvector.h>

struct PreshaderStat;
struct LocalVar;

namespace ShaderParser
{

struct VariablesMerger
{
  enum
  {
    TYPE_FLOAT = 0,
    TYPE_INT,
    TYPE_UINT,

    TYPE_COUNT
  };

  struct MergedVarInfo
  {
    eastl::string name;
    int offset;
    int size;
    int type;

    eastl::string getSwizzle() const
    {
      const char swizzles[] = "xyzw";
      return eastl::string(swizzles + offset, size);
    }

    const char *getType() const
    {
      switch (size)
      {
        case 1: return type == TYPE_FLOAT ? "float" : (type == TYPE_UINT ? "uint" : "int");
        case 2: return type == TYPE_FLOAT ? "float2" : (type == TYPE_UINT ? "uint2" : "int2");
        case 3: return type == TYPE_FLOAT ? "float3" : (type == TYPE_UINT ? "uint3" : "int3");
        case 4: return type == TYPE_FLOAT ? "float4" : (type == TYPE_UINT ? "uint4" : "int4");
        default: G_ASSERT_RETURN(0, "");
      }
    }
  };

  using MergeableStateBlocks = Tab<ShaderTerminal::state_block_stat *>;
  using MergeableStateBlocksPerStage = eastl::array<MergeableStateBlocks, (int)ShaderStage::STAGE_MAX>;
  using MergedVars = eastl::fixed_vector<MergedVarInfo, 4>;
  using MergedVarsMap = ska::flat_hash_map<eastl::string, MergedVars>;
  using MergedVarsMapsPerStage = eastl::array<MergedVarsMap, (int)ShaderStage::STAGE_MAX>;

  struct Callbacks
  {
    dag::FunctionRef<void(const PreshaderStat &) const> compileStat;
    dag::FunctionRef<int(const ShaderTerminal::local_var_decl &) const> compileLocalVar;
    dag::FunctionRef<void(int) const> releaseRegister;
  };

  MergeableStateBlocksPerStage constBlocks;
  MergedVarsMapsPerStage constVarsMaps;
  MergeableStateBlocks bufferedBlocks;
  MergedVarsMap bufferedVarsMap;

  // @TODO: this is to prune duplicate ps/vs stat pairs for buffered stats. Should be replaced with actual syntax tree comparison
  // (currently, different decls for vs and ps race to be the one chosen for buffer)
  SCFastNameMap bufferedStatsNames;
  eastl::bitvector<> bufferedDeclaredInVs, bufferedDeclaredInPsOrCs;

  void addConstStat(ShaderTerminal::state_block_stat &state_block, ShaderStage stage)
  {
    constBlocks[stage].emplace_back(&state_block);
  }
  void addBufferedStat(ShaderTerminal::state_block_stat &state_block, ShaderStage stage);

  void mergeAllVars(const Callbacks &callbacks)
  {
    for (ShaderStage stage : {STAGE_CS, STAGE_PS, STAGE_VS})
      mergeVars(constBlocks[stage], constVarsMaps[stage], stage, callbacks);
    // Both STAGE_PS and STAGE_VS are ok here -- static cbuf is shared
    mergeVars(bufferedBlocks, bufferedVarsMap, STAGE_PS, callbacks);
  }

  void mergeVars(MergeableStateBlocks &blocks, MergedVarsMap &var_map, ShaderStage stage, const Callbacks &callbacks);
  static const MergedVars *findOriginalVarsInfo(const eastl::string merged_var_name, const MergedVarsMap &map)
  {
    auto found = map.find(merged_var_name);
    if (found == map.end())
      return nullptr;

    return &found->second;
  }
};

} // namespace ShaderParser
