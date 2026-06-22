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
    else if (vt_is_cbuf(vt))
    {
      stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GBUF, 0, varId));
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          stcode.push_back(shaderopcode::makeOpStageSlot(SHCOD_CONST_BUFFER, stage, e->slot[stage].value(), 0));
    }
    else if (vt_is_sampler(vt))
    {
      if (bindless && e->cbufOffset.has_value())
      {
        stcode.push_back(shaderopcode::makeOp2(SHCOD_REG_BINDLESS_SAMPLER, e->cbufOffset.value(), varId));
      }
      else
      {
        for (int stage = 0; stage < STAGE_MAX; ++stage)
          if (e->slot[stage].has_value())
            stcode.push_back(shaderopcode::makeOpStageSlot(SHCOD_GLOB_SAMPLER, stage, e->slot[stage].value(), varId));
      }
    }
    else if (vt_is_tlas(vt))
    {
      stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GTLAS, 0, varId));
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          stcode.push_back(shaderopcode::makeOpStageSlot(SHCOD_TLAS, stage, e->slot[stage].value(), 0));
    }
    else if (vt_is_uav(vt) && e->svType == SHVT_TEXTURE)
    {
      stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GTEX, 0, varId));
      G_STATIC_ASSERT(STAGE_CS == 0 && STAGE_PS == 1 && STAGE_VS == 2);
      const int cods[3] = {SHCOD_RWTEX_CS, SHCOD_RWTEX_PS, SHCOD_RWTEX_VS};
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          stcode.push_back(shaderopcode::makeOp2(cods[stage], e->slot[stage].value(), 0));
    }
    else if (vt_is_uav(vt) && e->svType == SHVT_BUFFER)
    {
      stcode.push_back(shaderopcode::makeOp2(SHCOD_GET_GBUF, 0, varId));
      G_STATIC_ASSERT(STAGE_CS == 0 && STAGE_PS == 1 && STAGE_VS == 2);
      const int cods[3] = {SHCOD_RWBUF_CS, SHCOD_RWBUF_PS, SHCOD_RWBUF_VS};
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          stcode.push_back(shaderopcode::makeOp2(cods[stage], e->slot[stage].value(), 0));
    }
  }
}

dag::ConstSpan<int> RefinedBlockLayout::getStcode() const { return dag::ConstSpan<int>(stcode.data(), stcode.size()); }

void RefinedBlockLayout::serializeToBindump(Tab<SerializedRefinedBlockVar> &out, Tab<int> &out_computed_stcode,
  Tab<char> &out_computed_cpp_exprs) const
{
  out.clear();
  out_computed_stcode.clear();
  out_computed_cpp_exprs.clear();
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
    if (!e.computedCppExpr.empty())
    {
      var.computedCppExprOffset = out_computed_cpp_exprs.size();
      var.computedCppExprLen = e.computedCppExpr.size();
      out_computed_cpp_exprs.insert(out_computed_cpp_exprs.end(), e.computedCppExpr.begin(), e.computedCppExpr.end());
    }
    out.push_back(var);
  }
}

RefinedBlockLayout RefinedBlockLayout::deserializeFromBindump(const VarNameMap &rbVarNameMap,
  const Tab<SerializedRefinedBlockVar> &entries, const Tab<int> &computed_stcode, const Tab<char> &computed_cpp_exprs)
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
    if (e.computedCppExprOffset >= 0 && e.computedCppExprLen > 0)
    {
      G_ASSERTF(e.computedCppExprOffset + e.computedCppExprLen <= computed_cpp_exprs.size(),
        "refined_block: computedCppExpr slice [%d, %d) out of range (array size %d)", e.computedCppExprOffset,
        e.computedCppExprOffset + e.computedCppExprLen, computed_cpp_exprs.size());
      var.computedCppExpr.assign(computed_cpp_exprs.data() + e.computedCppExprOffset, e.computedCppExprLen);
    }
    layout.vars.emplace(varId, eastl::move(var));
  }
  return layout;
}

void RefinedBlockLayout::generateCppStcode(TargetContext &ctx)
{
  if (!shc::config().compileCppStcode() || vars.empty())
    return;

  eastl::vector<const RefinedBlockVar *> sorted;
  sorted.reserve(vars.size());
  for (const auto &[_, decl] : vars)
    sorted.push_back(&decl);

  // First goes vars with no computed expression to declare var with id, which will
  // be referenced by other computed expressions.
  eastl::sort(sorted.begin(), sorted.end(),
    [](const RefinedBlockVar *a, const RefinedBlockVar *b) { return a->computedCppExpr.empty() > b->computedCppExpr.empty(); });

  DynamicStcodeRoutine routine;

  for (const RefinedBlockVar *e : sorted)
  {
    const int varId = e->globVarId;
    const VariableType vt = e->varType;

    eastl::string typeStr;
    switch (vt)
    {
      case VariableType::f3: typeStr = "float3"; break;
      case VariableType::f2: typeStr = "float2"; break;
      case VariableType::f1: typeStr = "float"; break;
      case VariableType::i4:
      case VariableType::u4: typeStr = "int4"; break;
      case VariableType::i3:
      case VariableType::u3: typeStr = "int3"; break;
      case VariableType::i2:
      case VariableType::u2: typeStr = "int2"; break;
      case VariableType::i1:
      case VariableType::u1: typeStr = "int"; break;
      case VariableType::f4:
      default: typeStr = "float4"; break;
    };

    if (!e->computedCppExpr.empty())
    {
      const eastl::string locName = string_f("%s_vs", e->varName.c_str());
      routine.addSetConstStmt(typeStr.c_str(), 1, locName.c_str(), e->computedCppExpr.c_str(), "vsconst__", 0);
      routine.regsForStage(STAGE_VS).add(e->varName.c_str(), e->cbufOffset.value(), true, 1);
      continue;
    }

    if (vt_is_tex(vt))
    {
      if (shc::config().enableBindless && e->cbufOffset.has_value())
      {
        const eastl::string biVar = string_f("rb_bi_%s", e->varName.c_str());
        routine.addFormattedStatement("uint32_t %s = rb_alloc_bindless(rb_get_tex(%d));\n", biVar.c_str(), varId);
        routine.addFormattedStatement("rb_flush_bindless_tex(%s, rb_get_tex(%d));\n", biVar.c_str(), varId);
        const eastl::string locName = string_f("%s_vs", e->varName.c_str());
        routine.addSetConstStmt("uint", 1, locName.c_str(), biVar.c_str(), "vsconst__", 0);
        routine.regsForStage(STAGE_VS).add(e->varName.c_str(), e->cbufOffset.value(), true, 1);
      }
      else
      {
        G_STATIC_ASSERT(STAGE_CS == 0 && STAGE_PS == 1 && STAGE_VS == 2);
        for (int stage = 0; stage < STAGE_MAX; ++stage)
          if (e->slot[stage].has_value())
            routine.addFormattedStatement("rb_flush_tex(%d, %d, rb_get_tex(%d));\n", stage, e->slot[stage].value(), varId);
      }
    }
    else if (vt_is_buf(vt))
    {
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          routine.addFormattedStatement("rb_flush_buf(%d, %d, rb_get_buf(%d));\n", stage, e->slot[stage].value(), varId);
    }
    else if (vt_is_cbuf(vt))
    {
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          routine.addFormattedStatement("rb_flush_cbuf(%d, %d, rb_get_buf(%d));\n", stage, e->slot[stage].value(), varId);
    }
    else if (vt_is_sampled_texture(vt))
    {
      if (shc::config().enableBindless && e->cbufOffset.has_value())
      {
        const eastl::string smpVar = string_f("rb_smp_%s", e->varName.c_str());
        const eastl::string biVar = string_f("rb_bi_%s", e->varName.c_str());
        routine.addFormattedStatement("uint64_t %s = rb_get_sampler(%d);\n", smpVar.c_str(), varId);
        routine.addFormattedStatement("uint32_t %s = rb_alloc_bindless_sampler(%s);\n", biVar.c_str(), smpVar.c_str());
        routine.addFormattedStatement("rb_flush_bindless_sampler(%s, %s);\n", biVar.c_str(), smpVar.c_str());
        const eastl::string locName = string_f("%s_vs", e->varName.c_str());
        routine.addSetConstStmt("uint", 1, locName.c_str(), biVar.c_str(), "vsconst__", 0);
        routine.regsForStage(STAGE_VS).add(e->varName.c_str(), e->cbufOffset.value(), true, 1);
      }
      else
      {
        for (int stage = 0; stage < STAGE_MAX; ++stage)
          if (e->slot[stage].has_value())
            routine.addFormattedStatement("rb_flush_sampler(%d, %d, rb_get_sampler(%d));\n", stage, e->slot[stage].value(), varId);
      }
    }
    else if (vt_is_tlas(vt))
    {
      for (int stage = 0; stage < STAGE_MAX; ++stage)
        if (e->slot[stage].has_value())
          routine.addFormattedStatement("rb_flush_tlas(%d, %d, rb_get_tlas(%d));\n", stage, e->slot[stage].value(), varId);
    }
    else if (vt_is_uav(vt))
    {
      for (int stage = 0; stage < STAGE_MAX; ++stage)
      {
        if (e->slot[stage].has_value())
        {
          if (e->svType == SHVT_TEXTURE)
            routine.addFormattedStatement("rb_flush_rwtex(%d, %d, rb_get_tex(%d));\n", stage, e->slot[stage].value(), varId);
          else
            routine.addFormattedStatement("rb_flush_rwbuf(%d, %d, rb_get_buf(%d));\n", stage, e->slot[stage].value(), varId);
        }
      }
    }
    else if (vt == VariableType::f44 && e->cbufOffset.has_value())
    {
      routine.addFormattedStatement("float4x4 %s;\n", e->varName.c_str());
      routine.addFormattedStatement("rb_get_mat44(%d, &%s);\n", varId, e->varName.c_str());
      routine.addShaderConst(STAGE_VS, SHVT_FLOAT4X4, vt, e->varName.c_str(), e->cbufOffset.value(), e->varName.c_str(), 0);
    }
    else if (e->cbufOffset.has_value())
    {
      eastl::string getter;
      switch (vt)
      {
        case VariableType::f1: getter = string_f("rb_get_real(%d)", varId); break;
        case VariableType::i1:
        case VariableType::u1: getter = string_f("rb_get_int(%d)", varId); break;
        case VariableType::f2:
        case VariableType::f3:
        case VariableType::f4: getter = string_f("rb_get_f4(%d)", varId); break;
        case VariableType::i2:
        case VariableType::i3:
        case VariableType::i4:
        case VariableType::u2:
        case VariableType::u3:
        case VariableType::u4: getter = string_f("rb_get_ivec(%d)", varId); break;
        default:
          G_ASSERTF(false, "unsupported variable type %d for variable '%s'", eastl::to_underlying(vt), e->varName.c_str());
          break;
      }

      routine.addFormattedStatement("%s %s;\n", typeStr.c_str(), e->varName.c_str());
      routine.addFormattedStatement("%s = %s;\n", e->varName.c_str(), getter.c_str());

      const eastl::string locName = string_f("%s_vs", e->varName.c_str());
      routine.addSetConstStmt(typeStr.c_str(), 1, locName.c_str(), e->varName.c_str(), "vsconst__", 0);
      routine.regsForStage(STAGE_VS).add(e->varName.c_str(), e->cbufOffset.value(), true, 1);
    }
  }

  if (!routine.hasCode())
    return;

  HlslRegRange vsRange = routine.collectSetRegistersRange(STAGE_VS);
  vsRange.min = 0; // Must be always 0 since regs may be preallocated from previous compilations
  for (const RefinedBlockVar *e : sorted)
    if (e->varType == VariableType::f44 && e->cbufOffset.has_value())
      vsRange.cap = eastl::max(vsRange.cap, e->cbufOffset.value() + 16);

  const int id = ctx.cppStcode().addCode(eastl::move(routine), {}, vsRange);
  if (id < 0)
    return;

  cppStcodeId = id;
  if (id == (int)ctx.cppStcode().dynamicRoutineNames.size())
    ctx.cppStcode().dynamicRoutineNames.emplace_back(string_f("stcode::cpp::_refined_block::dynroutine%d", id).c_str());
}

} // namespace shc
