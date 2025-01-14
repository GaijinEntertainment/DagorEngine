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

#include "shLog.h"

#include <debug/dag_debug.h>

template <class T>
struct TabInd
{
  Tab<T> slice;
  int index;

  TabInd(dag::ConstSpan<T> s, int Index) : index(Index), slice(s, tmpmem) {}
  TabInd() : slice(tmpmem){};
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
static void addRefsFromVariants(Tab<int> &refsTable, dag::ConstSpan<ShaderClass *> shaderClasses, mkbindump::GatherNameMap &varMap)
{
  auto mark_used_var = [&](const char *name) {
    int id = varMap.getNameId(name);
    if (id == -1)
      return;
    for (int v_ind = 0; v_ind < ShaderGlobal::get_var_count(); v_ind++)
    {
      ShaderGlobal::Var &var = ShaderGlobal::get_var(v_ind);
      if (var.nameId == id)
      {
        G_ASSERT(v_ind >= 0 && v_ind < refsTable.size());
        refsTable[v_ind]++;
        break;
      }
    }
  };

  for (int i = 0; i < shaderClasses.size(); i++)
  {
    for (int codeNo = 0; codeNo < shaderClasses[i]->code.size(); codeNo++)
    {
      ShaderCode *code = shaderClasses[i]->code[codeNo];
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

static void addRefsFromStCode(Tab<int> &refsTable)
{
  using loadedshaders::stCode;
  for (int codeInd = 0; codeInd < stCode.size(); codeInd++)
  {
    Tab<int> &code = stCode[codeInd];
    // ShUtils::shcod_dump(code);
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
        case SHCOD_BLK_ICODE_LEN:
        case SHCOD_IMM_REAL1:
        case SHCOD_IMM_SVEC1:
        case SHCOD_COPY_REAL:
        case SHCOD_COPY_VEC:
        case SHCOD_PACK_MATERIALS: break;

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

        default: DAG_FATAL("code not processed: %s at %d!", ShUtils::shcod_tokname(op), i); G_ASSERT(0);
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

void bindumphlp::patchStCode(dag::Span<int> code, dag::ConstSpan<int> remapTable, dag::ConstSpan<int> smpTable)
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
      case SHCOD_BLK_ICODE_LEN:
      case SHCOD_IMM_REAL1:
      case SHCOD_IMM_SVEC1:
      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC:
      case SHCOD_PACK_MATERIALS: break;

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

      default: DAG_FATAL("code not processed: %s at %d!", ShUtils::shcod_tokname(op), i); G_ASSERT(0);
    }
  }
}

static void patchStCode(dag::ConstSpan<int> remapTable)
{
  using loadedshaders::stCode;
  for (int codeInd = 0; codeInd < stCode.size(); codeInd++)
  {
    Tab<int> &code = stCode[codeInd];
    bindumphlp::patchStCode(make_span(code), remapTable, {});
  }
}

void bindumphlp::sortShaders(dag::ConstSpan<ShaderStateBlock *> blocks, StcodeInterface *stcode_interface)
{
  using loadedshaders::fsh;
  using loadedshaders::shClass;
  using loadedshaders::stCode;
  using loadedshaders::vpr;

  Tab<TabInd<unsigned>> fshIndexes(tmpmem);
  Tab<TabInd<unsigned>> vprIndexes(tmpmem);
  Tab<TabInd<int>> stIndexes(tmpmem);

  for (int i = 0; i < fsh.size(); i++)
    fshIndexes.push_back(TabInd<unsigned>(fsh[i], i));
  sort(fshIndexes, &cmpTabInd);
  for (int i = 0; i < fshIndexes.size(); i++)
    fsh[i] = fshIndexes[i].slice;

  for (int i = 0; i < vpr.size(); i++)
    vprIndexes.push_back(TabInd<unsigned>(vpr[i], i));
  sort(vprIndexes, &cmpTabInd);
  for (int i = 0; i < vprIndexes.size(); i++)
    vpr[i] = vprIndexes[i].slice;

  for (int i = 0; i < stCode.size(); i++)
    stIndexes.push_back(TabInd<int>(stCode[i], i));

  sort(stIndexes, &cmpTabInd);

  for (int i = 0; i < stIndexes.size(); i++)
    stCode[i] = stIndexes[i].slice;

  for (int i = 0; i < shClass.size(); i++)
  {
    ShaderClass &sc = *shClass[i];
    for (int j = 0; j < sc.code.size(); j++)
    {
      if (sc.code[j])
        for (int k = 0; k < sc.code[j]->allPasses.size(); k++)
        {
          ShaderCode::Pass &p = sc.code[j]->allPasses[k];
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

  if (stcode_interface)
  {
    for (int i = 0; i < stIndexes.size(); i++)
      stcode_interface->patchRoutineGlobalId(stIndexes[i].index, i);
  }
}

void bindumphlp::countRefAndRemapGlobalVars(Tab<int> &remapTable, dag::ConstSpan<ShaderClass *> shaderClasses,
  mkbindump::GatherNameMap &varMap)
{
  Tab<int> refsTable(tmpmem);
  refsTable.resize(ShaderGlobal::get_var_count());
  mem_set_0(refsTable);
  addRefsFromVariants(refsTable, shaderClasses, varMap);
  addRefsFromStCode(refsTable);
  for (int i = 0; i < refsTable.size(); i++)
    if (!refsTable[i] && (ShaderGlobal::get_var(i).isAlwaysReferenced || ShaderGlobal::get_var(i).isImplicitlyReferenced ||
                           shc::isGlobVarRequired(ShaderGlobal::get_var(i).getName())))
    {
      debug("explicitly referenced <%s>", ShaderGlobal::get_var(i).getName());
      refsTable[i]++;
    }

  getRemapTable(remapTable, refsTable);
  ::patchStCode(remapTable);
}
