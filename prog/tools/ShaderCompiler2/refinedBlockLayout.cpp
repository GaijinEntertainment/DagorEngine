// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>

#include "variantSemantic.h"
#include "refinedBlockLayout.h"
#include "globalConfig.h"
#include "shLog.h"

using namespace semantic;

namespace shc
{

eastl::pair<RefinedBlockVar &, bool> RefinedBlockLayout::addVar(int varId, const RefinedBlockVar &decl)
{
  auto [it, inserted] = vars.emplace(varId, decl);
  return {it->second, inserted};
}

void RefinedBlockLayout::link(dag::ConstSpan<int> global_var_link_table)
{
  for (auto &[_, decl] : vars)
  {
    if (decl.globVarId >= 0)
    {
      G_ASSERTF(decl.globVarId < (int)global_var_link_table.size(), "refined_block layout: localVarId %d out of range [0, %d)",
        decl.globVarId, global_var_link_table.size());
      decl.globVarId = global_var_link_table[decl.globVarId];
    }

    // Remap global var ID operands embedded in computed stcode.
    for (int i = 0; i < (int)decl.computedStcode.size(); i++)
    {
      int &word = decl.computedStcode[i];
      int op = shaderopcode::getOp(word);
      if (op == SHCOD_GET_GREAL || op == SHCOD_GET_GVEC || op == SHCOD_GET_GMAT44 || op == SHCOD_GET_GINT ||
          op == SHCOD_GET_GINT_TOREAL || op == SHCOD_GET_GIVEC_TOREAL || op == SHCOD_GET_GTLAS || op == SHCOD_GET_GBUF ||
          op == SHCOD_GET_GTEX)
      {
        int reg = shaderopcode::getOp2p1(word);
        int localId = shaderopcode::getOp2p2(word);
        G_ASSERTF(localId >= 0 && localId < (int)global_var_link_table.size(),
          "refined_block computed stcode: localVarId %d out of range [0, %d)", localId, global_var_link_table.size());
        word = shaderopcode::makeOp2(op, reg, global_var_link_table[localId]);
      }
      else if (op == SHCOD_IMM_REAL || op == SHCOD_MAKE_VEC)
        i++;
      else if (op == SHCOD_IMM_VEC)
        i += 4;
    }
  }
}

void RefinedBlockLayout::mergeFrom(const RefinedBlockLayout &other)
{
  for (const auto &[id, src] : other.vars)
  {
    auto [it, inserted] = vars.emplace(id, src);
    if (!inserted)
    {
      const RefinedBlockVar &dst = it->second;
#define CHECK_CONFLICT(field) \
  if (dst.field != src.field) \
    sh_debug(SHLOG_FATAL, "refined_block layout conflict: '%s' has different " #field, src.varName.c_str());

      CHECK_CONFLICT(cbufOffset);
      CHECK_CONFLICT(svType);
      CHECK_CONFLICT(varType);
      CHECK_CONFLICT(globVarId);
      CHECK_CONFLICT(varName);
      CHECK_CONFLICT(hlslDecl);
      CHECK_CONFLICT(slot[STAGE_VS]);
      CHECK_CONFLICT(slot[STAGE_PS]);
      CHECK_CONFLICT(slot[STAGE_CS]);
      CHECK_CONFLICT(space);
      CHECK_CONFLICT(computedStcode);

#undef CHECK_CONFLICT
    }
  }
}

void RefinedBlockLayout::generateStcode()
{
  G_ASSERT(stcode.empty());

  if (vars.empty())
    return;

  const bool bindless = shc::config().enableBindless;

  dag::Vector<const RefinedBlockVar *> sorted;
  sorted.reserve(vars.size());
  for (const auto &[_, decl] : vars)
    sorted.push_back(&decl);

  // Numeric vars sorted by cbufOffset first, then tex/buf vars sorted by tSlot.
  fast_sort(sorted, [](const RefinedBlockVar *a, const RefinedBlockVar *b) {
    const bool aNum = vt_is_numeric(a->varType);
    const bool bNum = vt_is_numeric(b->varType);
    if (aNum != bNum)
      return int(aNum) > int(bNum);
    return aNum ? (a->cbufOffset.value_or(-1) < b->cbufOffset.value_or(-1)) : false;
  });

  for (const auto *e : sorted)
  {
    if (!e->computedStcode.empty())
    {
      stcode.insert(stcode.end(), e->computedStcode.begin(), e->computedStcode.end());
      continue;
    }

    const int varId = e->globVarId;
    const VariableType vt = e->varType;

    if (vt_is_tex(vt))
    {
      stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GTEX, 0, varId));
      if (bindless && e->cbufOffset.has_value())
      {
        stcode.push_back(shaderopcode::makeOp2(SHCOD_REG_BINDLESS, e->cbufOffset.value(), 0));
      }
      else
      {
        const int cods[3] = {SHCOD_TEXTURE_CS, SHCOD_TEXTURE, SHCOD_TEXTURE_VS};
        G_STATIC_ASSERT(STAGE_CS == 0 && STAGE_PS == 1 && STAGE_VS == 2);
        for (int stage = 0; stage < STAGE_MAX; ++stage)
        {
          if (e->slot[stage].has_value())
            stcode.push_back(shaderopcode::makeOp2(cods[stage], e->slot[stage].value(), 0));
        }
      }
    }
    else if (vt_is_buf(vt))
    {
      stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GBUF, 0, varId));
      for (int stage = 0; stage < STAGE_MAX; ++stage)
      {
        if (e->slot[stage].has_value())
          stcode.push_back(shaderopcode::makeOpStageSlot(SHCOD_BUFFER, static_cast<ShaderStage>(stage), e->slot[stage].value(), 0));
      }
    }
    else if (vt_is_numeric(vt) && e->cbufOffset.has_value())
    {
      if (vt == VariableType::f44)
        stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GMAT44, 0, varId));
      else if (vt_is_integer(vt))
      {
        if (vt_float_size(vt) == 1)
          stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GINT, 0, varId));
        else
          stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GIVEC, 0, varId));
      }
      else
      {
        if (vt_float_size(vt) == 1)
          stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GREAL, 0, varId));
        else
          stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GVEC, 0, varId));
      }

      stcode.push_back(shaderopcode::makeOpStageSlot(SHCOD_SET_CONST_PACKED, 0, vt_float_size(vt), e->cbufOffset.value()));
    }
  }
}

dag::ConstSpan<int> RefinedBlockLayout::getStcode() const { return dag::ConstSpan<int>(stcode.data(), stcode.size()); }

void RefinedBlockLayout::serializeToBindump(Tab<SerializedRefinedBlockVar> &out, Tab<int> &out_computed_stcode) const
{
  out.clear();
  out_computed_stcode.clear();
  out.reserve(vars.size());
  for (const auto &[id, e] : vars)
  {
    SerializedRefinedBlockVar var{
      .localVarId = e.globVarId,
      .varType = static_cast<int>(eastl::to_underlying(e.varType)),
      .svType = static_cast<int>(eastl::to_underlying(e.svType)),
      .cbufOffset = e.cbufOffset.value_or(-1),
      .space = static_cast<int>(e.space),
    };
    var.varNameId = id;
    for (int stage = 0; stage < STAGE_MAX; ++stage)
      var.slot[stage] = e.slot[stage].value_or(-1);

    if (!e.computedStcode.empty())
    {
      var.computedStcodeOffset = out_computed_stcode.size();
      var.computedStcodeLen = e.computedStcode.size();
      for (int word : e.computedStcode)
        out_computed_stcode.push_back(word);
    }
    out.push_back(var);
  }
}

RefinedBlockLayout RefinedBlockLayout::deserializeFromBindump(const VarNameMap &rbVarNameMap,
  const Tab<SerializedRefinedBlockVar> &entries, const Tab<int> &computed_stcode)
{
  RefinedBlockLayout layout;
  for (const SerializedRefinedBlockVar &e : entries)
  {
    const int varId = e.varNameId;
    const char *varName = rbVarNameMap.getName(varId);
    G_ASSERT(varName);
    RefinedBlockVar var{
      .varName = varName ? varName : "",
      .globVarId = e.localVarId,
      .varType = static_cast<VariableType>(e.varType),
      .svType = e.svType >= 0 ? static_cast<ShaderVarType>(e.svType) : SHVT_UNKNOWN,
      .cbufOffset = e.cbufOffset != -1 ? eastl::optional<int>(e.cbufOffset) : eastl::nullopt,
      .space = e.space != -1 ? static_cast<HlslRegisterSpace>(e.space) : HlslRegisterSpace::HLSL_RSPACE_INVALID,
    };
    for (int stage = 0; stage < STAGE_MAX; ++stage)
      var.slot[stage] = e.slot[stage] != -1 ? eastl::optional<int>(e.slot[stage]) : eastl::nullopt;

    if (e.computedStcodeOffset >= 0 && e.computedStcodeLen > 0)
    {
      G_ASSERTF(e.computedStcodeOffset + e.computedStcodeLen <= computed_stcode.size(),
        "refined_block: computedStcode slice [%d, %d) out of range (array size %d)", e.computedStcodeOffset,
        e.computedStcodeOffset + e.computedStcodeLen, computed_stcode.size());
      var.computedStcode.insert(var.computedStcode.end(), computed_stcode.data() + e.computedStcodeOffset,
        computed_stcode.data() + e.computedStcodeOffset + e.computedStcodeLen);
    }
    layout.vars.emplace(varId, eastl::move(var));
  }
  return layout;
}

} // namespace shc
