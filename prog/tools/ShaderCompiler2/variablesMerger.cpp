// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "variablesMerger.h"
#include "assemblyShader.h"
#include "variantSemantic.h"
#include "variantAssembly.h"
#include "shsem.h"
#include "shExprParser.h"
#include "mdArray.h"

#include <memory/dag_regionMemAlloc.h>
#include <debug/dag_assert.h>

#include <EASTL/set.h>

namespace ShaderParser
{

static constexpr MdArray<semantic::VariableType, VariablesMerger::TYPE_COUNT, 5> VEC_TYPES_TABLE{
  {semantic::VariableType::Unknown, semantic::VariableType::f1, semantic::VariableType::f2, semantic::VariableType::f3,
    semantic::VariableType::f4},
  {semantic::VariableType::Unknown, semantic::VariableType::i1, semantic::VariableType::i2, semantic::VariableType::i3,
    semantic::VariableType::i4},
  {semantic::VariableType::Unknown, semantic::VariableType::u1, semantic::VariableType::u2, semantic::VariableType::u3,
    semantic::VariableType::u4}};

static constexpr MdArray<const char *, VariablesMerger::TYPE_COUNT, 5> VEC_TYPE_NAMES_TABLE{
  {"", "f1", "f2", "f3", "f4"},
  {"", "i1", "i2", "i3", "i4"},
  {"", "u1", "u2", "u3", "u4"},
};

void VariablesMerger::addBufferedStat(ShaderTerminal::state_block_stat &state_block, ShaderStage stage)
{
  int id = bufferedStatsNames.getNameId(state_block.var->var->name->text);
  if (id == -1)
  {
    id = bufferedStatsNames.addNameId(state_block.var->var->name->text);
    if (id >= bufferedBlocks.size())
      bufferedBlocks.resize(id + 1);
    bufferedBlocks[id] = &state_block;
  }
  auto &bits = stage == STAGE_VS ? bufferedDeclaredInVs : bufferedDeclaredInPsOrCs;
  if (bits.test(id, false))
    sh_debug(SHLOG_ERROR, "Packed const '%s' redeclaration at %s stage", state_block.var->var->name->text, SHADER_STAGE_NAMES[stage]);
  bits.set(id, true);
}

void VariablesMerger::mergeVars(AssembleShaderEvalCB *ascb, MergeableStateBlocks &blocks, MergedVarsMap &var_map, ShaderStage stage)
{
  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);

  auto tmpmemCalloc = [&rm_alloc]<class T>() {
    T *ptr = new (&rm_alloc) T;
    memset((void *)ptr, 0, sizeof(*ptr));
    return ptr;
  };
#define TMPMEM_CALLOC(type_) (tmpmemCalloc.operator()<type_>())

  const char *profiles[] = {"cs", "ps", "vs", nullptr};
  const char *comps[] = {nullptr, "x", "xy", "xyz", nullptr};

  if (blocks.empty())
    return;

  {
    struct VarData
    {
      eastl::fixed_vector<MergedVarInfo *, 4> origVars;
      ShaderTerminal::state_block_stat *stat;
    };

    struct Var
    {
      VarData data;
      String combinedName;
      String lhsName;
      String rhsName;

      Var(VarData &&vdata, IMemAlloc *alloc) : data{eastl::move(vdata)}, combinedName{alloc}, lhsName{alloc}, rhsName{alloc} {}
      Var(VarData &&vdata, String &&combined_name, String &&lhs_name, String &&rhs_name) :
        data{eastl::move(vdata)},
        combinedName{eastl::move(combined_name)},
        lhsName{eastl::move(lhs_name)},
        rhsName{eastl::move(rhs_name)}
      {}
    };
    auto addMergedVarToMap = [&](const Var &var) {
      eastl::fixed_vector<MergedVarInfo, 4> origVarInfos;
      for (MergedVarInfo *info : var.data.origVars)
        origVarInfos.push_back(*info);

      var_map.emplace(var.data.stat->var->var->name->text, eastl::move(origVarInfos));
    };

    auto allVars = mdarray_make<TYPE_COUNT, 4 + 1>([&] { return Tab<Var>(&rm_alloc); });

    Tab<MergedVarInfo> sourceVarInfos{&rm_alloc};
    sourceVarInfos.reserve(blocks.size());

    MdArray<int, TYPE_COUNT, 5> statCountsByTypeBySize{};

    const auto nsToSizeType = [](const char *ns) {
      // @NOTE: ns format is validated on inital parsing in AssembleShaderEvalCB
      return eastl::make_pair((int)(ns[2] - '0'), (ns[1] == 'f' ? TYPE_FLOAT : (ns[1] == 'u') ? TYPE_UINT : TYPE_INT));
    };

    for (auto &stat : blocks)
    {
      auto [size, type] = nsToSizeType(stat->var->var->nameSpace->text);
      ++statCountsByTypeBySize[type][size];
    }

    for (const auto &[type, tcounts] : enumerate(statCountsByTypeBySize.rowView()))
      for (const auto [size, count] : enumerate(tcounts))
      {
        allVars[type][size].reserve(count);
      }

    for (auto &stat : blocks)
    {
      auto [size, type] = nsToSizeType(stat->var->var->nameSpace->text);
      MergedVarInfo varInfo{.name = stat->var->var->name->text, .offset = 0, .size = size, .type = type};

      sourceVarInfos.push_back(varInfo);
      allVars[varInfo.type][varInfo.size].emplace_back(Var{VarData{.origVars = {&sourceVarInfos.back()}, .stat = stat}, &rm_alloc});
    }

    auto merge = [&, this](auto type, int dim1, int dim2) {
      auto &vars = allVars[type];

      auto done = [&] {
        if (dim1 != dim2)
          return vars[dim1].empty() || vars[dim2].empty();
        else
          return vars[dim1].size() < 2;
      };

      while (!done())
      {
        Var varA = vars[dim1].back();
        vars[dim1].pop_back();
        Var varB = vars[dim2].back();
        vars[dim2].pop_back();

        ShaderTerminal::external_variable *var_a = varA.data.stat->var;
        ShaderTerminal::external_variable *var_b = varB.data.stat->var;

        if (var_a->val->expr == nullptr || var_b->val->expr == nullptr)
        {
          sh_debug(SHLOG_ERROR, "shader parser failed on generated code in variable merger");
          return;
        }

        String merged_var_name_str{&rm_alloc};
        merged_var_name_str.printf(8, "%s__%s", var_a->var->name->text, var_b->var->name->text);

        auto mergedTypeTokNum = (type == TYPE_FLOAT ? SHADER_TOKENS::SHTOK_float4 : SHADER_TOKENS::SHTOK_int4);

        String lhsName{&rm_alloc};
        lhsName.printf(16, "__%s%i", var_a->var->name->text, stage);

        local_var_decl local_a_decl = {};
        local_var_type local_a_type = {};
        SHTOK_ident local_a_type_id = {};
        SHTOK_ident local_a_name_id = {};
        local_a_name_id.text = lhsName.c_str();
        local_a_type_id.num = mergedTypeTokNum;
        local_a_type.type = &local_a_type_id;
        local_a_decl.type = &local_a_type;
        local_a_decl.name = &local_a_name_id;
        local_a_decl.expr = var_a->val->expr;

        auto [reg1_var, reg1_expr] = unwrap(semantic::parse_local_var_decl(local_a_decl, ascb->ctx, true));
        if (!reg1_var->isConst)
          assembly::assemble_local_var<assembly::StcodeBuildFlagsBits::ALL>(reg1_var, reg1_expr.get(), local_a_decl.name, ascb->ctx);
        int reg1 = reg1_var->reg;

        String rhsName{&rm_alloc};
        rhsName.printf(16, "__%s%i", var_b->var->name->text, stage);

        local_var_decl local_b_decl = {};
        local_var_type local_b_type = {};
        SHTOK_ident local_b_type_id = {};
        SHTOK_ident local_b_name_id = {};
        local_b_name_id.text = rhsName.c_str();
        local_b_type_id.num = mergedTypeTokNum;
        local_b_type.type = &local_b_type_id;
        local_b_decl.type = &local_b_type;
        local_b_decl.name = &local_b_name_id;
        local_b_decl.expr = var_b->val->expr;

        auto [reg2_var, reg2_expr] = unwrap(semantic::parse_local_var_decl(local_b_decl, ascb->ctx, true));
        if (!reg2_var->isConst)
          assembly::assemble_local_var<assembly::StcodeBuildFlagsBits::ALL>(reg2_var, reg2_expr.get(), local_b_decl.name, ascb->ctx);
        int reg2 = reg2_var->reg;

        auto *merged_var_stat = TMPMEM_CALLOC(state_block_stat);
        auto *merged_var_var = TMPMEM_CALLOC(external_variable);

        auto *merged_var_name = TMPMEM_CALLOC(external_var_name);
        auto *merged_var_name_id = TMPMEM_CALLOC(SHTOK_ident);
        auto *merged_var_namespace = TMPMEM_CALLOC(SHTOK_type_ident);
        merged_var_name_id->text = merged_var_name_str.c_str();
        merged_var_namespace->text = VEC_TYPE_NAMES_TABLE[type][dim1 + dim2];
        merged_var_name->name = merged_var_name_id;
        merged_var_name->nameSpace = merged_var_namespace;

        auto *merged_var_val = TMPMEM_CALLOC(external_var_value_single);
        auto *merged_var_expr = TMPMEM_CALLOC(arithmetic_expr);
        auto *merged_var_expr_md = TMPMEM_CALLOC(arithmetic_expr_md);
        auto *merged_var_operand = TMPMEM_CALLOC(arithmetic_operand);
        auto *merged_var_color = TMPMEM_CALLOC(arithmetic_color);

        auto *merged_var_lhs = TMPMEM_CALLOC(arithmetic_expr);
        auto *merged_var_lhs_md = TMPMEM_CALLOC(arithmetic_expr_md);
        auto *merged_var_lhs_operand = TMPMEM_CALLOC(arithmetic_operand);
        auto *merged_var_lhs_cmask = TMPMEM_CALLOC(arithmetic_color_mask);
        auto *merged_var_lhs_channel = TMPMEM_CALLOC(SHTOK_ident);
        auto *merged_var_lhs_var = TMPMEM_CALLOC(SHTOK_ident);
        merged_var_lhs_var->text = lhsName.c_str();
        merged_var_lhs_channel->text = comps[dim1];
        merged_var_lhs_cmask->channel = merged_var_lhs_channel;
        merged_var_lhs_operand->var_name = merged_var_lhs_var;
        merged_var_lhs_operand->cmask = merged_var_lhs_cmask;
        merged_var_lhs_md->lhs = merged_var_lhs_operand;
        merged_var_lhs->lhs = merged_var_lhs_md;
        auto *merged_var_rhs = TMPMEM_CALLOC(arithmetic_expr);
        auto *merged_var_rhs_md = TMPMEM_CALLOC(arithmetic_expr_md);
        auto *merged_var_rhs_operand = TMPMEM_CALLOC(arithmetic_operand);
        auto *merged_var_rhs_cmask = TMPMEM_CALLOC(arithmetic_color_mask);
        auto *merged_var_rhs_channel = TMPMEM_CALLOC(SHTOK_ident);
        auto *merged_var_rhs_var = TMPMEM_CALLOC(SHTOK_ident);
        merged_var_rhs_var->text = rhsName.c_str();
        merged_var_rhs_channel->text = comps[dim2];
        merged_var_rhs_cmask->channel = merged_var_rhs_channel;
        merged_var_rhs_operand->var_name = merged_var_rhs_var;
        merged_var_rhs_operand->cmask = merged_var_rhs_cmask;
        merged_var_rhs_md->lhs = merged_var_rhs_operand;
        merged_var_rhs->lhs = merged_var_rhs_md;

        merged_var_color->expr0 = merged_var_lhs;
        merged_var_color->expr1 = merged_var_rhs;

        merged_var_operand->color_value = merged_var_color;
        merged_var_expr_md->lhs = merged_var_operand;
        merged_var_expr->lhs = merged_var_expr_md;
        merged_var_val->expr = merged_var_expr;

        merged_var_var->var = merged_var_name;
        merged_var_var->val = merged_var_val;
        merged_var_stat->var = merged_var_var;

        merged_var_stat->var->var->name->file_start = varA.data.stat->var->var->name->file_start;
        merged_var_stat->var->var->name->line_start = varA.data.stat->var->var->name->line_start;

        eastl::fixed_vector<MergedVarInfo *, 4> combinedSourceVars = eastl::move(varA.data.origVars);
        for (MergedVarInfo *appendedVar : eastl::move(varB.data.origVars))
        {
          appendedVar->offset += dim1;
          combinedSourceVars.push_back(appendedVar);
        }

        {
          Var var{VarData{.origVars = eastl::move(combinedSourceVars), .stat = merged_var_stat}, eastl::move(merged_var_name_str),
            eastl::move(lhsName), eastl::move(rhsName)};

          if (dim1 + dim2 == 4)
          {
            addMergedVarToMap(var);
            ascb->compile_external_block_stat({var.data.stat, ShaderStage(stage), VEC_TYPES_TABLE[type][4]});
            if (reg1 > 0)
              ascb->ctx.stBytecode().regAllocator->manuallyReleaseRegister(reg1);
            if (reg2 > 0)
              ascb->ctx.stBytecode().regAllocator->manuallyReleaseRegister(reg2);
          }
          else
          {
            vars[dim1 + dim2].emplace_back(eastl::move(var));
          }
        }
      }
    };

    for (int type = 0; type < TYPE_COUNT; ++type)
    {
      merge(type, 3, 1);
      merge(type, 2, 2);
      merge(type, 2, 1);
      merge(type, 3, 1);
      merge(type, 1, 1);
      merge(type, 2, 2);
      merge(type, 2, 1);

      for (int dim = 1; dim <= 4; dim++)
        for (Var &var : allVars[type][dim])
        {
          if (var.data.origVars.size() > 1)
            addMergedVarToMap(var);
          ascb->compile_external_block_stat({var.data.stat, ShaderStage(stage), VEC_TYPES_TABLE[type][dim]});
        }
    }
  }
#undef TMPMEM_CALLOC
}

} // namespace ShaderParser
