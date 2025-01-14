// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shSemCode.h"
#include "globalConfig.h"
#include <shaders/shInternalTypes.h>
#include "shaderVariant.h"
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_carray.h>
#include <debug/dag_debug.h>
#include <EASTL/numeric.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/random/dag_random.h>
#include "linkShaders.h"
#include "shCompiler.h"
#include <dag/dag_vectorMap.h>

#include <osApiWrappers/dag_stackHlp.h>

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#endif

extern int opcode_usage[2][256];

// create if needed passes for dynamic variant #n
ShaderSemCode::PassTab *ShaderSemCode::createPasses(int variant)
{
  curVariant = variant;

  if (!tabutils::isCorrectIndex(passes, curVariant))
    return NULL;

  if (!passes[curVariant])
    passes[curVariant] = new PassTab();
  return passes[curVariant];
}

ShaderSemCode::ShaderSemCode() :
  channel(midmem),
  vars(midmem),
  initcode(midmem),
  passes(&rm_alloc),
  stvarmap(midmem),
  curVariant(-1),
  flags(0),
  renderStageIdx(-1),
  rm_alloc(4 << 20, 4 << 20)
{}

void ShaderSemCode::initPassMap(int pass_id)
{
  if (vars.empty() || !passes[pass_id])
    return;

  passes[pass_id]->varmap.resize(vars.size());
  for (auto it = vars.begin(); it != vars.end(); ++it)
  {
    auto id = eastl::distance(vars.begin(), it);
    passes[pass_id]->varmap[id] = id;
  }
}

void ShaderSemCode::mergeVars(Tab<Var> &&other_vars, Tab<StVarMap> &&other_stvarmap, int pass_id)
{
  dag::VectorMap<int, Var *> ownedVarsByNameId{};
  eastl::transform(vars.begin(), vars.end(), eastl::inserter(ownedVarsByNameId, ownedVarsByNameId.end()),
    [this](Var &var) { return eastl::make_pair(var.nameId, &var); });

  Tab<int> otherStvarmapDirect{};
  otherStvarmapDirect.resize(other_stvarmap.size(), -1);
  for (const StVarMap &mapping : other_stvarmap)
  {
    if (mapping.v >= otherStvarmapDirect.size())
      otherStvarmapDirect.resize(mapping.v + 1, -1);
    otherStvarmapDirect[mapping.v] = mapping.sv;
  }

  passes[pass_id]->varmap.reserve(passes[pass_id]->varmap.size() + other_vars.size());

  for (auto it = other_vars.begin(); it != other_vars.end(); ++it)
  {
    auto stIt = ownedVarsByNameId.find(it->nameId);
    Var *staticVar = nullptr;
    if (stIt == ownedVarsByNameId.end())
    {
      vars.push_back(eastl::move(*it));
      const int lastId = (int)(vars.size() - 1);

      staticVarRegs.add(vars.back().getName(), lastId);

      staticVar = vars.begin() + lastId;
      int sv = otherStvarmapDirect[(int)eastl::distance(other_vars.begin(), it)];
      if (sv >= 0)
        stvarmap.push_back({lastId, sv});
    }
    else
    {
      staticVar = stIt->second;
      staticVar->used = it->used ? true : staticVar->used;
    }

    passes[pass_id]->varmap.push_back(eastl::distance(vars.data(), staticVar));
  }
}

ShaderCode *ShaderSemCode::generateShaderCode(const ShaderVariant::VariantTableSrc &dynVariants, StcodeShader &cppcode)
{
  ShaderCode *code = new (midmem) ShaderCode();

  dynVariants.fillVariantTable(code->dynVariants);
  code->flags = flags;

  code->channel = channel;

  // compute var offsets
  Tab<int> cvar(tmpmem);
  cvar.resize(vars.size());
  int ofs = 0;

  for (int i = 0; i < vars.size(); ++i)
  {
    int sz;
    switch (vars[i].type)
    {
      case SHVT_INT: sz = sizeof(int); break;
      case SHVT_INT4: sz = sizeof(int) * 4; break;
      case SHVT_REAL: sz = sizeof(real); break;
      case SHVT_COLOR4: sz = sizeof(Color4); break;
      case SHVT_TEXTURE: sz = sizeof(shaders_internal::Tex); break;
      case SHVT_SAMPLER: sz = sizeof(d3d::SamplerHandle); break;
      default: G_ASSERT(0); sz = 0;
    }
    cvar[i] = ofs;
    ofs += sz;
    if (shc::config().addTextureType && vars[i].slot >= 0)
    {
      if (vars[i].slot >= code->staticTextureTypes.size())
        for (int j = code->staticTextureTypes.size(); j <= vars[i].slot; j++)
          code->staticTextureTypes.push_back(ShaderVarTextureType::SHVT_TEX_UNKNOWN);
      code->staticTextureTypes[vars[i].slot] = vars[i].texType;
    }
  }

  code->varsize = ofs;
  code->regsize = regsize;

  // convert init code
  code->initcode = initcode;
  for (int i = 0; i < code->initcode.size(); i += 2)
  {
    int vi = code->initcode[i];
    code->initcode[i] = cvar[vi];
  }

  // convert stvarmap
  code->stvarmap.reserve(stvarmap.size());
  for (const StVarMap &mapping : stvarmap)
    code->stvarmap.push_back({cvar[mapping.v], mapping.sv});

  // convert passes
  tabutils::safeResize(code->passes, passes.size());
  Tab<ShaderCode::Pass> all_passid(tmpmem);

  for (int i = 0; i < passes.size(); i++)
  {
    PassTab *otherPasses = passes[i];
    if (otherPasses)
    {
      ShaderCode::PassTab *currPasses = code->createPasses(i);

      clear_and_shrink(currPasses->suppBlk);
      currPasses->suppBlk.resize(otherPasses->suppBlk.size());
      for (size_t i = 0; i < otherPasses->suppBlk.size(); i++)
        currPasses->suppBlk[i] = otherPasses->suppBlk[i];
      clear_and_shrink(otherPasses->suppBlk);

      if (otherPasses->pass)
      {
        ShaderCode::Pass p;
        convert_passes(*otherPasses->pass, p, cvar, cppcode, otherPasses->varmap);

        int found = -1;
        for (int j = 0; j < all_passid.size(); j++)
          if (memcmp(&p, &all_passid[j], sizeof(p)) == 0)
          {
            found = j;
            break;
          }

        if (found == -1)
        {
          found = all_passid.size();
          all_passid.push_back(p);
        }
        currPasses->rpass = (ShaderCode::Pass *)intptr_t(found);
      }
    }
  }

  cppcode.addStaticVarLocations(eastl::move(staticVarRegs));

  code->allPasses = all_passid;
  clear_and_shrink(all_passid);

  for (int i = 0; i < code->passes.size(); i++)
    if (code->passes[i])
      code->passes[i]->toPtr(make_span(code->allPasses));

  return code;
}

void ShaderSemCode::convert_stcode(dag::Span<int> cod, Tab<int> &cvar, StcodeRegisters &static_regs, const Tab<int> &var_map)
{
  for (int i = 0; i < cod.size(); i++)
  {
    int op = shaderopcode::getOp(cod[i]);

    switch (op)
    {
      case SHCOD_INVERSE:
      case SHCOD_LVIEW:
      case SHCOD_TMWORLD:
      case SHCOD_G_TM:
      case SHCOD_FSH_CONST:
      case SHCOD_VPR_CONST:
      case SHCOD_CS_CONST:
      case SHCOD_GLOB_SAMPLER:
      case SHCOD_TEXTURE:
      case SHCOD_TEXTURE_VS:
      case SHCOD_REG_BINDLESS:
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      case SHCOD_TLAS:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF:
      case SHCOD_GET_GINT:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GBUF:
      case SHCOD_GET_GTLAS:
      case SHCOD_GET_GVEC:
      case SHCOD_GET_GMAT44:
      case SHCOD_ADD_REAL:
      case SHCOD_SUB_REAL:
      case SHCOD_MUL_REAL:
      case SHCOD_DIV_REAL:
      case SHCOD_ADD_VEC:
      case SHCOD_SUB_VEC:
      case SHCOD_MUL_VEC:
      case SHCOD_DIV_VEC:
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GIVEC_TOREAL:
      case SHCOD_IMM_REAL1:
      case SHCOD_IMM_SVEC1:
      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC:
      case SHCOD_PACK_MATERIALS: break;

      case SHCOD_IMM_REAL:
      case SHCOD_MAKE_VEC: i++; break;

      case SHCOD_IMM_VEC: i += 4; break;

      case SHCOD_CALL_FUNCTION:
        i += shaderopcode::getOp3p3(cod[i]); // skip params
        break;

      case SHCOD_GET_TEX:
      case SHCOD_GET_INT:
      case SHCOD_GET_REAL:
      case SHCOD_GET_VEC:
      case SHCOD_GET_INT_TOREAL:
      case SHCOD_GET_IVEC_TOREAL:
      {
        int vi = var_map[shaderopcode::getOp2p2s(cod[i])];
        G_ASSERT(vi >= 0);
        cod[i] = shaderopcode::patchOp2p2(cod[i], cvar[vi]); // replace var id with var offset
        static_regs.patch(vi, cvar[vi]);
      }
      break;

      case SHCOD_SAMPLER:
      {
        int vi = var_map[shaderopcode::getOpStageSlot_Reg(cod[i])];
        static_regs.patch(vi, cvar[vi]);
        cod[i] = shaderopcode::makeOpStageSlot(op, shaderopcode::getOpStageSlot_Stage(cod[i]),
          shaderopcode::getOpStageSlot_Slot(cod[i]), cvar[vi]);
      }
      break;

      default: debug("cod: %d '%s' not processed!", cod[i], ShUtils::shcod_tokname(op)); G_ASSERT(0);
    }
  }
}

void ShaderSemCode::convert_passes(ShaderSemCode::Pass &semP, ShaderCode::Pass &p, Tab<int> &cvar, StcodeShader &cppcode,
  const Tab<int> &var_map)
{
  if (semP.cs.size())
  {
    p.fsh = add_fshader(semP.cs);
    p.vprog = -1;
  }
  else
  {
#if _CROSS_TARGET_DX12
    if (use_two_phase_compilation(shc::config().targetPlatform) && !shc::config().autotestMode)
    {
      auto idents = add_phase_one_progs(semP.vpr, semP.hs, semP.ds, semP.gs, semP.fsh, semP.enableFp16);
      p.vprog = idents.vprog;
      p.fsh = idents.fsh;
    }
    else
#endif
    {
      // add fragment shader
      p.fsh = add_fshader(semP.fsh);

      // add vertex program
      p.vprog = add_vprog(semP.vpr, semP.hs, semP.ds, semP.gs);
    }
  }

  // convert state code
  static Tab<int> stblkcode(tmpmem);
  stblkcode = semP.stblkcode;
  convert_stcode(make_span(stblkcode), cvar, staticVarRegs, var_map);

  auto [blkcodeId, blkcodeIsNew] = add_stcode(stblkcode);
  p.stblkcodeNo = blkcodeId;
  if (blkcodeIsNew)
    cppcode.addCode(eastl::move(semP.cppStblkcode), p.stblkcodeNo);

  static Tab<int> stcode(tmpmem);
  stcode = semP.stcode;
  convert_stcode(make_span(stcode), cvar, staticVarRegs, var_map);
  auto [stcodeId, stcodeIsNew] = add_stcode(stcode);
  p.stcodeNo = stcodeId;
  if (stcodeIsNew)
    cppcode.addCode(eastl::move(semP.cppStcode), p.stcodeNo);
  p.renderStateNo = add_render_state(semP);
  p.threadGroupSizes = semP.threadGroupSizes;
  p.scarlettWave32 = semP.scarlettWave32;
}

void ShaderSemCode::Pass::reset_code()
{
  curStcode.clear();
  curStblkcode.clear();

  curCppStcode.reset();
  curCppStblkcode.reset();
}
void ShaderSemCode::Pass::finish_pass(bool accept)
{
  if (accept)
  {
    stcode = find_shared_stcode(curStcode, true);
    stblkcode = find_shared_stcode(curStblkcode, false);

    cppStcode = eastl::move(curCppStcode);
    cppStblkcode = eastl::move(curCppStblkcode);
  }
  reset_code();
}

static Tab<TabStcode> stcode_cache(tmpmem_ptr());
static Tab<TabStcode> stblkcode_cache(tmpmem_ptr());

Tab<int> ShaderSemCode::Pass::curStcode(tmpmem_ptr());
Tab<int> ShaderSemCode::Pass::curStblkcode(tmpmem_ptr());

StcodeRoutine ShaderSemCode::Pass::curCppStcode(StcodeRoutine::Type::DYNSTCODE);
StcodeRoutine ShaderSemCode::Pass::curCppStblkcode(StcodeRoutine::Type::STBLKCODE);

dag::ConstSpan<int> ShaderSemCode::Pass::find_shared_stcode(Tab<int> &code, bool dyn)
{
  if (!code.size())
    return dag::ConstSpan<int>(0, 0);

  Tab<TabStcode> &cache = dyn ? stcode_cache : stblkcode_cache;
  for (int i = cache.size() - 1; i >= 0; i--)
    if (cache[i].size() == code.size() && mem_eq(code, cache[i].data()))
      return cache[i];

  int l = append_items(cache, 1);
  cache[l] = code;

#ifdef PROFILE_OPCODE_USAGE
  debug_("%s code - ", dyn ? "dynamic" : "stateblock");
  ShUtils::shcod_dump(cache[l]);

  {
    for (int i = 0; i < code.size(); i++)
    {
      int opcode = shaderopcode::getOp(code[i]);
      if (dyn)
        opcode_usage[1][opcode]++;
      else
        opcode_usage[0][opcode]++;

      if (opcode == SHCOD_IMM_REAL || opcode == SHCOD_MAKE_VEC)
        i++;
      else if (opcode == SHCOD_IMM_VEC)
        i += 4;
      else if (opcode == SHCOD_CALL_FUNCTION)
        i += shaderopcode::getOp3p3(code[i]);
    }
  }
#endif

  return cache[l];
}

void clear_shared_stcode_cache()
{
  debug("[stat] stcode.count=%d  stblk.size()=%d", stcode_cache.size(), stblkcode_cache.size());
  stcode_cache.clear();
  stblkcode_cache.clear();
}
