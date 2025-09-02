// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "binDumpUtils.h"

#include "loadShaders.h"
#include "shcode.h"
#include "namedConst.h"
#include "globVar.h"
#include "shaderVariant.h"
#include "shCompiler.h"
#include <generic/dag_sort.h>
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shFunc.h>
#include <libTools/util/makeBindump.h>
#include <generic/dag_enumerate.h>

#include "shLog.h"

#include <debug/dag_debug.h>

template <class T>
struct TabInd
{
  Tab<T> slice;
  int index;

  TabInd(dag::ConstSpan<T> s, int Index) : index(Index), slice(s, tmpmem) {}
  TabInd() : slice(tmpmem) {}
};

template <class T>
static int cmpTabInd(const TabInd<T> *a, const TabInd<T> *b)
{
  const Tab<T> *a_tab = &(a->slice), *b_tab = &(b->slice);
  int a_count = a_tab->size(), b_count = b_tab->size();
  if (a_count < b_count)
    return -1;
  else if (b_count < a_count)
    return 1;

  int pos = 0, min_count = a_count < b_count ? a_count : b_count;
  while (pos < min_count)
  {
    if ((*a_tab)[pos] < (*b_tab)[pos])
      return -1;
    else if ((*b_tab)[pos] < (*a_tab)[pos])
      return 1;
    pos++;
  }
  return 0;
}

//************************************************************
//  OpCodes operations
//************************************************************
static void addRefsFromVariants(Tab<int> &refsTable, dag::ConstSpan<ShaderClass *> shaderClasses, mkbindump::GatherNameMap &varMap,
  const shc::TargetContext &ctx)
{
  const Tab<ShaderGlobal::Var> &globvars = ctx.globVars().getVariableList();

  auto mark_used_var = [&](const char *name) {
    int id = varMap.getNameId(name);
    if (id == -1)
      return;
    for (auto [varId, var] : enumerate(globvars))
    {
      if (var.nameId == id)
      {
        G_ASSERT(varId >= 0 && varId < refsTable.size());
        refsTable[varId]++;
        break;
      }
    }
  };

  for (ShaderClass *sclass : shaderClasses)
  {
    for (ShaderCode *code : sclass->code)
    {
      if (code)
      {
        const ShaderVariant::TypeTable &dyn_types = code->dynVariants.getTypes();
        for (int type_ind = 0; type_ind < dyn_types.getCount(); type_ind++)
        {
          const ShaderVariant::VariantType &type = dyn_types.getType(type_ind);
          if (type.type == ShaderVariant::VARTYPE_INTERVAL)
            continue;

          const char *name = dyn_types.getIntervalName(type_ind);
          mark_used_var(name);
        }
      }
    }
  }
}

static void addRefsFromStCode(Tab<int> &refsTable, const shc::TargetContext &ctx)
{
  for (const auto &code : ctx.storage().shadersStcode)
  {
    for (int i = 0; i < code.size(); i++)
    {
      int op = shaderopcode::getOp(code[i]);

      switch (op)
      {
        case SHCOD_INVERSE:
        case SHCOD_LVIEW:
        case SHCOD_TMWORLD:
        case SHCOD_G_TM:
        case SHCOD_FSH_CONST:
        case SHCOD_VPR_CONST:
        case SHCOD_CS_CONST:
        case SHCOD_TEXTURE:
        case SHCOD_TEXTURE_VS:
        case SHCOD_SAMPLER:
        case SHCOD_REG_BINDLESS:
        case SHCOD_RWTEX:
        case SHCOD_RWBUF:
        case SHCOD_BUFFER:
        case SHCOD_CONST_BUFFER:
        case SHCOD_TLAS:
        case SHCOD_ADD_REAL:
        case SHCOD_SUB_REAL:
        case SHCOD_MUL_REAL:
        case SHCOD_DIV_REAL:
        case SHCOD_ADD_VEC:
        case SHCOD_SUB_VEC:
        case SHCOD_MUL_VEC:
        case SHCOD_DIV_VEC:
        case SHCOD_GET_INT:
        case SHCOD_GET_REAL:
        case SHCOD_GET_TEX:
        case SHCOD_GET_VEC:
        case SHCOD_GET_INT_TOREAL:
        case SHCOD_GET_IVEC_TOREAL:
        case SHCOD_IMM_REAL1:
        case SHCOD_IMM_SVEC1:
        case SHCOD_INT_TOREAL:
        case SHCOD_IVEC_TOREAL:
        case SHCOD_COPY_REAL:
        case SHCOD_COPY_VEC: break;

        case SHCOD_STATIC_BLOCK:
        case SHCOD_STATIC_MULTIDRAW_BLOCK:
        case SHCOD_IMM_REAL:
        case SHCOD_MAKE_VEC: i++; break;

        case SHCOD_IMM_VEC: i += 4; break;

        case SHCOD_CALL_FUNCTION:
          i += shaderopcode::getOp3p3(code[i]); // skip params
          break;

        case SHCOD_GET_GINT:
        case SHCOD_GET_GINT_TOREAL:
        case SHCOD_GET_GIVEC_TOREAL:
        case SHCOD_GET_GREAL:
        case SHCOD_GET_GTEX:
        case SHCOD_GET_GBUF:
        case SHCOD_GET_GTLAS:
        case SHCOD_GET_GVEC:
        case SHCOD_GET_GMAT44:
        case SHCOD_GLOB_SAMPLER:
        {
          int vi = shaderopcode::getOpStageSlot_Reg(code[i]);
          G_ASSERT(vi >= 0 && vi < refsTable.size());
          refsTable[vi]++;
        }
        break;

        default: G_ASSERTF(0, "code not processed: %s at %d!", ShUtils::shcod_tokname(op), i); G_ASSERT(0);
      }
    }
  }
}

static void getRemapTable(Tab<int> &remapTable, Tab<int> &refTable)
{
  int gvar_ind = 0;
  remapTable.resize(refTable.size());
  for (int ind = 0; ind < refTable.size(); ind++)
  {
    if (refTable[ind])
      remapTable[ind] = gvar_ind++;
    else
      remapTable[ind] = -1;
  }
}

void bindumphlp::patchStCode(dag::Span<int32_t> code, dag::ConstSpan<int> remapTable, dag::ConstSpan<int> smpTable)
{
  for (int i = 0; i < code.size(); i++)
  {
    int op = shaderopcode::getOp(code[i]);

    switch (op)
    {
      case SHCOD_INVERSE:
      case SHCOD_LVIEW:
      case SHCOD_TMWORLD:
      case SHCOD_G_TM:
      case SHCOD_FSH_CONST:
      case SHCOD_VPR_CONST:
      case SHCOD_REG_BINDLESS:
      case SHCOD_CS_CONST:
      case SHCOD_TEXTURE:
      case SHCOD_TEXTURE_VS:
      case SHCOD_SAMPLER:
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      case SHCOD_TLAS:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF:
      case SHCOD_ADD_REAL:
      case SHCOD_SUB_REAL:
      case SHCOD_MUL_REAL:
      case SHCOD_DIV_REAL:
      case SHCOD_ADD_VEC:
      case SHCOD_SUB_VEC:
      case SHCOD_MUL_VEC:
      case SHCOD_DIV_VEC:
      case SHCOD_GET_INT:
      case SHCOD_GET_REAL:
      case SHCOD_GET_TEX:
      case SHCOD_GET_VEC:
      case SHCOD_GET_INT_TOREAL:
      case SHCOD_GET_IVEC_TOREAL:
      case SHCOD_IMM_REAL1:
      case SHCOD_IMM_SVEC1:
      case SHCOD_INT_TOREAL:
      case SHCOD_IVEC_TOREAL:
      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC: break;

      case SHCOD_STATIC_BLOCK:
      case SHCOD_STATIC_MULTIDRAW_BLOCK:
      case SHCOD_IMM_REAL:
      case SHCOD_MAKE_VEC: i++; break;

      case SHCOD_IMM_VEC: i += 4; break;

      case SHCOD_CALL_FUNCTION:
      {
        int fun = shaderopcode::getOp3p1(code[i]);
        if (fun == functional::BF_REQUEST_SAMPLER)
        {
          if (!smpTable.empty())
          {
            int id = shaderopcode::getOp3p2(code[i]);
            G_ASSERT(id >= 0 && id < smpTable.size());
            code[i] = shaderopcode::makeOp3(op, fun, smpTable[id], shaderopcode::getOp3p3(code[i]));
          }

          int vi = code[i + 1];
          G_ASSERT(vi >= 0 && vi < remapTable.size());
          code[i + 1] = remapTable[vi];
        }
        i += shaderopcode::getOp3p3(code[i]); // skip params
      }
      break;

      case SHCOD_GET_GINT:
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GIVEC_TOREAL:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GBUF:
      case SHCOD_GET_GTLAS:
      case SHCOD_GET_GVEC:
      case SHCOD_GET_GMAT44:
      case SHCOD_GLOB_SAMPLER:
      {
        int vi = shaderopcode::getOpStageSlot_Reg(code[i]);
        G_ASSERT(vi >= 0 && vi < remapTable.size());
        code[i] = shaderopcode::makeOpStageSlot(op, shaderopcode::getOpStageSlot_Stage(code[i]),
          shaderopcode::getOpStageSlot_Slot(code[i]), remapTable[vi]);
      }
      break;

      default: G_ASSERTF(0, "code not processed: %s at %d!", ShUtils::shcod_tokname(op), i);
    }
  }
}

static void patchStCode(dag::ConstSpan<int> remapTable, shc::TargetContext &ctx)
{
  for (auto &code : ctx.storage().shadersStcode)
    bindumphlp::patchStCode(make_span(code), remapTable, {});
}

void bindumphlp::sortShaders(dag::ConstSpan<ShaderStateBlock *> blocks, shc::TargetContext &ctx)
{
  ShaderTargetStorage &stor = ctx.storage();

  Tab<TabInd<uint32_t>> fshIndexes(tmpmem);
  Tab<TabInd<uint32_t>> vprIndexes(tmpmem);
  Tab<TabInd<int32_t>> stIndexes(tmpmem);

  for (int i = 0; i < stor.ldShFsh.size(); i++)
    fshIndexes.push_back(TabInd<uint32_t>(stor.ldShFsh[i], i));
  sort(fshIndexes, &cmpTabInd);
  for (int i = 0; i < fshIndexes.size(); i++)
    stor.ldShFsh[i] = fshIndexes[i].slice;

  for (int i = 0; i < stor.ldShVpr.size(); i++)
    vprIndexes.push_back(TabInd<uint32_t>(stor.ldShVpr[i], i));
  sort(vprIndexes, &cmpTabInd);
  for (int i = 0; i < vprIndexes.size(); i++)
    stor.ldShVpr[i] = vprIndexes[i].slice;

  for (int i = 0; i < stor.shadersStcode.size(); i++)
    stIndexes.push_back(TabInd<int32_t>(stor.shadersStcode[i], i));

  sort(stIndexes, &cmpTabInd);

  for (int i = 0; i < stIndexes.size(); i++)
    stor.shadersStcode[i] = stIndexes[i].slice;

  if (shc::config().generateCppStcodeValidationData)
  {
    Tab<shader_layout::StcodeConstValidationMask *> masks{};
    G_ASSERT(stor.stcodeConstValidationMasks.size() == stIndexes.size());
    masks.resize(stor.stcodeConstValidationMasks.size());
    for (int i = 0; i < stIndexes.size(); i++)
      masks[i] = stor.stcodeConstValidationMasks[stIndexes[i].index];
    stor.stcodeConstValidationMasks = eastl::move(masks);
  }

  for (ShaderClass *sc : stor.shaderClass)
  {
    for (int j = 0; j < sc->code.size(); j++)
    {
      if (sc->code[j])
        for (int k = 0; k < sc->code[j]->allPasses.size(); k++)
        {
          ShaderCode::Pass &p = sc->code[j]->allPasses[k];
          for (int vpr_ind = 0; vpr_ind < vprIndexes.size(); vpr_ind++)
          {
            if (p.vprog == vprIndexes[vpr_ind].index)
            {
              p.vprog = vpr_ind;
              break;
            }
          }
          for (int fsh_ind = 0; fsh_ind < fshIndexes.size(); fsh_ind++)
          {
            if (p.fsh == fshIndexes[fsh_ind].index)
            {
              p.fsh = fsh_ind;
              break;
            }
          }
          for (int st_ind = 0; st_ind < stIndexes.size(); st_ind++)
          {
            if (p.stcodeNo == stIndexes[st_ind].index)
            {
              p.stcodeNo = st_ind;
              break;
            }
          }

          for (int st_ind = 0; st_ind < stIndexes.size(); st_ind++)
          {
            if (p.stblkcodeNo == stIndexes[st_ind].index)
            {
              p.stblkcodeNo = st_ind;
              break;
            }
          }
        }
    }
  }

  for (int i = 0; i < blocks.size(); i++)
  {
    for (int j = 0; j < stIndexes.size(); j++)
      if (blocks[i]->stcodeId == stIndexes[j].index)
      {
        blocks[i]->stcodeId = j;
        break;
      }
  }
}

void bindumphlp::countRefAndRemapGlobalVars(Tab<int> &remapTable, dag::ConstSpan<ShaderClass *> shaderClasses,
  mkbindump::GatherNameMap &varMap, shc::TargetContext &ctx)
{
  const auto &globvars = ctx.globVars();

  Tab<int> refsTable(tmpmem);
  refsTable.resize(globvars.getVarCount());
  mem_set_0(refsTable);
  addRefsFromVariants(refsTable, shaderClasses, varMap, ctx);
  addRefsFromStCode(refsTable, ctx);
  for (auto [varId, var] : enumerate(globvars.getVariableList()))
    if (
      !refsTable[varId] && (var.isAlwaysReferenced || var.isImplicitlyReferenced || shc::isGlobVarRequired(globvars.getVarName(var))))
    {
      debug("explicitly referenced <%s>", ShaderGlobal::get_var_name(var, ctx));
      refsTable[varId]++;
    }

  getRemapTable(remapTable, refsTable);
  ::patchStCode(remapTable, ctx);
}
