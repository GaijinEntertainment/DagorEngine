// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "variablesMerger.h"
#include "assemblyShader.h"
#include "shsem.h"
#include "shExprParser.h"

#include <memory/dag_regionMemAlloc.h>
#include <debug/dag_assert.h>

#include <EASTL/set.h>

namespace ShaderParser
{

void VariablesMerger::mergeVars(AssembleShaderEvalCB *ascb, MergeableStateBlocks &blocks, MergedVarsMap &varMap)
{
  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);
  ExpressionParser::getStatic().setOwner(ascb);

  for (int stage = STAGE_CS; stage < STAGE_MAX; ++stage)
  {
    auto &stats = blocks[stage];
    if (stats.empty())
      continue;

    struct Var
    {
      eastl::fixed_vector<MergedVarInfo *, 4> origVars;
      ShaderTerminal::state_block_stat *stat;
    };
    using VarsByDimension = eastl::array<eastl::vector<Var>, 4 + 1>;

    auto addMergedVarToMap = [&varMap](const Var &var) {
      eastl::fixed_vector<MergedVarInfo, 4> origVarInfos;
      for (MergedVarInfo *info : var.origVars)
        origVarInfos.push_back(*info);

      varMap.emplace(var.stat->var->var->name->text, eastl::move(origVarInfos));
    };

    VarsByDimension float_vars;
    VarsByDimension int_vars;
    VarsByDimension uint_vars;

    eastl::vector<MergedVarInfo> sourceVarInfos;
    sourceVarInfos.reserve(stats.size());

    for (auto &stat : stats)
    {
      const eastl::set<eastl::string> allowedNamespaces = {"@f1", "@f2", "@f3", "@i1", "@i2", "@i3", "@u1", "@u2", "@u3"};
      auto found = allowedNamespaces.find(stat.var->var->nameSpace->text);
      G_ASSERT(found != allowedNamespaces.end());

      MergedVarInfo varInfo{
        .name = stat.var->var->name->text,
        .offset = 0,
        .size = (int)((*found)[2] - '0'),
        .type = ((*found)[1] == 'f'     ? TYPE_FLOAT
                 : ((*found)[1] == 'u') ? TYPE_UINT
                                        : TYPE_INT),
      };

      sourceVarInfos.push_back(varInfo);
      (varInfo.type == TYPE_FLOAT ? float_vars : (varInfo.type == TYPE_UINT ? uint_vars : int_vars))[varInfo.size].emplace_back(
        Var{.origVars = {&sourceVarInfos.back()}, .stat = &stat});
    }

    auto merge = [&, this](VarsByDimension &vars, int dim1, int dim2, auto &merge) {
      if (dim1 != dim2)
      {
        if (vars[dim1].empty() || vars[dim2].empty())
          return;
      }
      else if (vars[dim1].size() < 2)
        return;

      Var varA = vars[dim1].back();
      vars[dim1].pop_back();
      Var varB = vars[dim2].back();
      vars[dim2].pop_back();

      ShaderTerminal::external_variable *var_a = varA.stat->var;
      ShaderTerminal::external_variable *var_b = varB.stat->var;

      String merged_name(32, "%s__%s", var_a->var->name->text, var_b->var->name->text);
      String merged_name_space(8, "@%c%i", var_a->var->nameSpace->text[1], dim1 + dim2);

      const char *comps[] = {"", "x", "xy", "xyz", "xyzw"};

      auto nameSpaceLetter = merged_name_space[1];
      auto mergedTypeName = (nameSpaceLetter == 'f' ? "float4" : "int4");

      String local_a(32, "local %s __%s%i = 0;", mergedTypeName, var_a->var->name->text, stage);
      auto *local_a_stat = parse_shader_stat(local_a, local_a.length(), &rm_alloc);
      if (local_a_stat == nullptr || local_a_stat->local_decl == nullptr || var_a->val->expr == nullptr)
      {
        sh_debug(SHLOG_ERROR, "shader parser failed on generated code in variable merger");
        return;
      }
      local_a_stat->local_decl->expr = var_a->val->expr;

      String local_b(32, "local %s __%s%i = 0;", mergedTypeName, var_b->var->name->text, stage);
      auto *local_b_stat = parse_shader_stat(local_b, local_b.length(), &rm_alloc);
      if (local_b_stat == nullptr || local_b_stat->local_decl == nullptr || var_b->val->expr == nullptr)
      {
        sh_debug(SHLOG_ERROR, "shader parser failed on generated code in variable merger");
        return;
      }
      local_b_stat->local_decl->expr = var_b->val->expr;

      auto reg1_var = ExpressionParser::getStatic().parseLocalVarDecl(*local_a_stat->local_decl, true);
      if (reg1_var == nullptr)
      {
        sh_debug(SHLOG_ERROR, "shader parser failed on generated code in variable merger");
        return;
      }
      int reg1 = reg1_var->reg;

      auto reg2_var = ExpressionParser::getStatic().parseLocalVarDecl(*local_b_stat->local_decl, true);
      if (reg2_var == nullptr)
      {
        sh_debug(SHLOG_ERROR, "shader parser failed on generated code in variable merger");
        return;
      }
      int reg2 = reg2_var->reg;

      const char *profiles[] = {"cs", "ps", "vs", nullptr};

      // (ms)-tagged blocks are processed as STAGE_VS, but it is illegal to have blocks for both mesh (ms/as) and
      // regular graphics pre-ps (vs/gs/hs/ds) blocks in the same shader, so we have to choose which block to
      // declare (vs or ms) if stage==STAGE_VS
      const char *profile = (stage == STAGE_VS && ascb->hasDeclaredMeshPipelineBlocks()) ? "ms" : profiles[stage];

      // clang-format off
      String merged_var(32, "(%s) { %s%s = (%s.%s, %s.%s); }",
                        profile, merged_name, merged_name_space,
                        local_a_stat->local_decl->name->text, comps[dim1],
                        local_b_stat->local_decl->name->text, comps[dim2]);
      // clang-format on
      auto *merged_var_stat = parse_shader_stat(merged_var, merged_var.length(), &rm_alloc);

      eastl::fixed_vector<MergedVarInfo *, 4> combinedSourceVars = eastl::move(varA.origVars);
      for (MergedVarInfo *appendedVar : eastl::move(varB.origVars))
      {
        appendedVar->offset += dim1;
        combinedSourceVars.push_back(appendedVar);
      }

      {
        Var var{.origVars = eastl::move(combinedSourceVars), .stat = merged_var_stat->external_block->stblock_stat[0]};

        if (dim1 + dim2 == 4)
        {
          addMergedVarToMap(var);
          ascb->handle_external_block_stat(*var.stat, (ShaderStage)stage);
          ascb->manuallyReleaseRegister(reg1);
          ascb->manuallyReleaseRegister(reg2);
        }
        else
        {
          vars[dim1 + dim2].emplace_back(eastl::move(var));
        }
      }

      merge(vars, dim1, dim2, merge);
    };

    for (auto *vars : {&float_vars, &int_vars, &uint_vars})
    {
      merge(*vars, 3, 1, merge);
      merge(*vars, 2, 2, merge);
      merge(*vars, 2, 1, merge);
      merge(*vars, 3, 1, merge);
      merge(*vars, 1, 1, merge);
      merge(*vars, 2, 2, merge);
      merge(*vars, 2, 1, merge);

      for (int dim = 1; dim <= 4; dim++)
        for (Var &var : (*vars)[dim])
        {
          if (var.origVars.size() > 1)
            addMergedVarToMap(var);
          ascb->handle_external_block_stat(*var.stat, (ShaderStage)stage);
        }
    }
  }
}

} // namespace ShaderParser