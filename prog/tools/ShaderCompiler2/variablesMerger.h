// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"

#include <drv/3d/dag_consts.h>

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/map.h>
#include <EASTL/optional.h>

namespace ShaderParser
{

class AssembleShaderEvalCB;

class VariablesMerger
{
public:
  enum
  {
    TYPE_FLOAT,
    TYPE_INT,
    TYPE_UINT,
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

  using MergeableStateBlocks = eastl::array<eastl::vector<ShaderTerminal::state_block_stat>, (int)ShaderStage::STAGE_MAX>;
  using MergedVars = eastl::fixed_vector<MergedVarInfo, 4>;
  using MergedVarsMap = eastl::map<eastl::string, MergedVars>;

  void addStat(ShaderTerminal::state_block_stat &state_block, ShaderStage stage, bool is_dynamic)
  {
    (is_dynamic ? dynamicBlocks : staticBlocks)[stage].emplace_back(state_block);
  }

  void mergeAllVars(AssembleShaderEvalCB *ascb)
  {
    mergeVars(ascb, dynamicBlocks, dynamicVarsMap);
    mergeVars(ascb, staticBlocks, staticVarsMap);
  }

  const MergedVars *findOriginalVarsInfo(const eastl::string merged_var_name, bool is_dynamic) const
  {
    const MergedVarsMap &varMap = (is_dynamic ? dynamicVarsMap : staticVarsMap);
    auto found = varMap.find(merged_var_name);
    if (found == varMap.end())
      return nullptr;

    return &found->second;
  }

private:
  MergeableStateBlocks dynamicBlocks, staticBlocks;
  MergedVarsMap dynamicVarsMap, staticVarsMap;

  void mergeVars(AssembleShaderEvalCB *ascb, MergeableStateBlocks &blocks, MergedVarsMap &var_map);
};

} // namespace ShaderParser
