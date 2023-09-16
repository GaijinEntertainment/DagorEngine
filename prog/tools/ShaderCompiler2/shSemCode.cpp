// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "shSemCode.h"
#include <shaders/shInternalTypes.h>
#include "shaderVariant.h"
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_carray.h>
#include <debug/dag_debug.h>
#include <EASTL/numeric.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/random/dag_random.h>
#include "linkShaders.h"
#include "shCompiler.h"

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
extern dx12::dxil::Platform targetPlatform;
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

ShaderCode *ShaderSemCode::generateShaderCode(const ShaderVariant::VariantTableSrc &dynVariants) const
{
  ShaderCode *code = new (midmem) ShaderCode();

  dynVariants.fillVariantTable(code->dynVariants);
  code->flags = flags;

  //  debug("st='%s', dyn=%d", (char*)code->staticVariant.getVariantTypeStr(), code->dynVariants.getVariants().size());

  code->channel = channel;

  // compute var offsets
  Tab<int> cvar(tmpmem);
  cvar.resize(vars.size());
  int ofs = 0, i;

  for (i = 0; i < vars.size(); ++i)
  {
    int sz;
    switch (vars[i].type)
    {
      case SHVT_INT: sz = sizeof(int); break;
      case SHVT_INT4: sz = sizeof(int) * 4; break;
      case SHVT_REAL: sz = sizeof(real); break;
      case SHVT_COLOR4: sz = sizeof(Color4); break;
      case SHVT_TEXTURE: sz = sizeof(shaders_internal::Tex); break;
      default: G_ASSERT(0); sz = 0;
    }
    cvar[i] = ofs;
    ofs += sz;
  }

  code->varsize = ofs;
  code->regsize = regsize;

  // convert init code
  code->initcode = initcode;
  for (i = 0; i < code->initcode.size(); i += 2)
  {
    int vi = code->initcode[i];
    code->initcode[i] = cvar[vi];
  }

  // convert stvarmap
  code->stvarmap.resize(stvarmap.size());
  for (i = 0; i < code->stvarmap.size(); ++i)
  {
    code->stvarmap[i].v = cvar[stvarmap[i].v];
    code->stvarmap[i].sv = stvarmap[i].sv;
  }

  // convert passes
  tabutils::safeResize(code->passes, passes.size());
  Tab<ShaderCode::Pass> all_passid(tmpmem);

  for (i = 0; i < passes.size(); i++)
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
        convert_passes(*otherPasses->pass, p, cvar);

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
        currPasses->rpass = (ShaderCode::Pass *)found;
      }
    }
  }

  code->allPasses = all_passid;
  clear_and_shrink(all_passid);

  for (i = 0; i < code->passes.size(); i++)
    if (code->passes[i])
      code->passes[i]->toPtr(make_span(code->allPasses));

  // debug ( "passes=%d all_passes=%d",
  //   passes.size(), code->allPasses.size());

  // debug("minArraySize=%d", minArraySize);

  return code;
}

void ShaderSemCode::convert_stcode(dag::Span<int> cod, Tab<int> &cvar) const
{
  for (int i = 0; i < cod.size(); i++)
  {
    int op = shaderopcode::getOp(cod[i]);

    // debug("cod: % 9d %s",op,shcod_tokname(op));
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
      case SHCOD_REG_BINDLESS:
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF:
      case SHCOD_GET_GINT:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GBUF:
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
      {
        int vi = shaderopcode::getOp2p2s(cod[i]);
        G_ASSERT(vi >= 0);
        cod[i] = shaderopcode::patchOp2p2(cod[i], cvar[vi]); // replace var id with var offset
      }
      break;

      case SHCOD_GET_INT:
      case SHCOD_GET_REAL:
      case SHCOD_GET_VEC:
      case SHCOD_GET_INT_TOREAL:
      {
        int vi = shaderopcode::getOp2p2s(cod[i]);
        G_ASSERT(vi >= 0);
        cod[i] = shaderopcode::patchOp2p2(cod[i], cvar[vi]); // replace var id with var offset
      }
      break;

      default: debug("cod: %d '%s' not processed!", cod[i], ShUtils::shcod_tokname(op)); G_ASSERT(0);
    }
  }
}

void ShaderSemCode::convert_passes(ShaderSemCode::Pass &semP, ShaderCode::Pass &p, Tab<int> &cvar) const
{
  if (semP.cs.size())
  {
    p.fsh = add_fshader(semP.cs);
    p.vprog = -1;
  }
  else
  {
#if _CROSS_TARGET_DX12
    if (targetPlatform != dx12::dxil::Platform::PC)
    {
      auto idents = add_phase_one_progs(semP.vpr, semP.hs, semP.ds, semP.gs, semP.fsh);
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
  convert_stcode(make_span(stblkcode), cvar);
  p.stblkcodeNo = add_stcode(stblkcode);

  static Tab<int> stcode(tmpmem);
  stcode = semP.stcode;
  convert_stcode(make_span(stcode), cvar);
  p.stcodeNo = add_stcode(stcode);
  p.renderStateNo = add_render_state(semP);
  p.threadGroupSizes = semP.threadGroupSizes;

  //  if (isDump)
  //  {
  //    debug("stCode-------------");
  //    for (int i = 0; i < stcode.size(); i++)
  //    {
  //      debug("%d", stcode[i]);
  //    }
  //    debug("stCode end---------");
  //  }
}

void ShaderSemCode::Pass::reset_code()
{
  curStcode.clear();
  curStblkcode.clear();
}
void ShaderSemCode::Pass::finish_pass(bool accept)
{
  if (accept)
  {
    stcode = find_shared_stcode(curStcode, true);
    stblkcode = find_shared_stcode(curStblkcode, false);
  }
  reset_code();
}

static Tab<TabStcode> stcode_cache(tmpmem_ptr());
static Tab<TabStcode> stblkcode_cache(tmpmem_ptr());

Tab<int> ShaderSemCode::Pass::curStcode(tmpmem_ptr());
Tab<int> ShaderSemCode::Pass::curStblkcode(tmpmem_ptr());

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
