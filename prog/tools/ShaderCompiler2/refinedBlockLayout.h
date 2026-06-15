// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <shaders/dag_shaderVarType.h>
#include <dag/dag_vectorMap.h>
#include <drv/3d/dag_consts.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/optional.h>
#include <EASTL/array.h>

#include "hlslRegisters.h"
#include "shaderSave.h"

namespace semantic
{
enum class VariableType;
}

namespace shc
{
class TargetContext;

struct RefinedBlockVar
{
  eastl::string varName{};
  int globVarId = -1;
  semantic::VariableType varType{};
  ShaderVarType svType = SHVT_UNKNOWN;
  eastl::optional<int> cbufOffset{};
  eastl::array<eastl::optional<int>, STAGE_MAX> slot{}; // t, b, s, u
  HlslRegisterSpace space = HlslRegisterSpace::HLSL_RSPACE_INVALID;
  eastl::string hlslDecl{};

  // Computed vars (RHS is an arithmetic expression rather than a simple global var ref).
  // computedStcode holds local var IDs until link(), global var IDs after.
  Tab<int> computedStcode{};
};

struct SerializedRefinedBlockVar
{
  NameId<RbVarMapAdapter> varNameId;
  int localVarId = -1;
  int varType = -1;
  int svType = -1;
  int cbufOffset = -1;
  int space = -1;
  int slot[STAGE_MAX]{-1, -1, -1}; // t, b, s, u
  int computedStcodeOffset = -1;
  int computedStcodeLen = 0;
};

class RefinedBlockLayout
{
public:
  eastl::pair<RefinedBlockVar &, bool> addVar(int varId, const RefinedBlockVar &decl);
  bool empty() const { return vars.empty(); }
  void link(dag::ConstSpan<int> global_var_link_table);
  void mergeFrom(const RefinedBlockLayout &other);

  void generateStcode();
  dag::ConstSpan<int> getStcode() const;

  void serializeToBindump(Tab<SerializedRefinedBlockVar> &out, Tab<int> &out_computed_stcode) const;
  static RefinedBlockLayout deserializeFromBindump(const VarNameMap &rbVarNameMap, const Tab<SerializedRefinedBlockVar> &entries,
    const Tab<int> &computed_stcode);

  template <typename Fn>
  void forEachVar(Fn &&fn) const
  {
    for (const auto &[id, decl] : vars)
      fn(id, decl);
  }

private:
  dag::VectorMap<int, RefinedBlockVar> vars;
  Tab<int> stcode;
};

} // namespace shc
