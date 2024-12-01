// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader semantic code
/************************************************************************/

#include <util/dag_globDef.h>
#include <generic/dag_tab.h>
#include "const3d.h"
#include <drv/3d/dag_renderStates.h>

#include <shaders/dag_shaderCommon.h>
#include "shaderVariant.h"
#include "shcode.h"
#include "cppStcodeAssembly.h"
#include "shLocVar.h"
#include "shaderVariantSrc.h"
#include "shaderTab.h"
#include "varMap.h"
#include <memory/dag_regionMemAlloc.h>
#include <EASTL/optional.h>

#include <osApiWrappers/dag_stackHlp.h>

#define codemem tmpmem

class ShaderSemCode
{
public:
  DAG_DECLARE_NEW(codemem)

  RegionMemAlloc rm_alloc;
  Tab<ShaderChannelId> channel;

  int flags;
  int renderStageIdx;

  StcodeRegisters staticVarRegs;

  struct Var
  {
    int nameId;
    ShaderVarType type;
    int slot = -1;
    ShaderVarTextureType texType = ShaderVarTextureType::SHVT_TEX_UNKNOWN;
    void *terminal;
    bool used, dynamic, noWarnings;

    Var() : nameId(-1), terminal(NULL), used(false), dynamic(false), noWarnings(false) {}

    inline const char *getName() const { return VarMap::getName(nameId); };
  };
  Tab<Var> vars;
  int regsize;

  LocalVarTable locVars;

  int find_var(const int variable_id);

  Tab<int> initcode;
  struct StVarMap
  {
    int v, sv;
  };
  Tab<StVarMap> stvarmap;

  struct Pass
  {
    FSHTYPE fshver;
    dag::ConstSpan<unsigned> fsh;
    dag::ConstSpan<unsigned> vpr;
    dag::ConstSpan<unsigned> hs, ds, gs;
    dag::ConstSpan<unsigned> cs;
    dag::ConstSpan<int> stcode, stblkcode;
    bool enableFp16 = false;
    bool scarlettWave32 = false;
    eastl::array<uint16_t, 3> threadGroupSizes = {};

    StcodeRoutine cppStcode = StcodeRoutine(StcodeRoutine::Type::DYNSTCODE);
    StcodeRoutine cppStblkcode = StcodeRoutine(StcodeRoutine::Type::STBLKCODE);

    void setCidx(int sh_profile, int c1, int c2, int c3)
    {
      c1 = (c1 << 4) | 3;
      int cnt = (c2 << 16) + c3;
      switch (sh_profile)
      {
        case 'v': vpr.set((unsigned *)(void *)intptr_t(c1), cnt); break;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
        case 'h': hs.set((unsigned *)(void *)intptr_t(c1), cnt); break;
        case 'd': ds.set((unsigned *)(void *)intptr_t(c1), cnt); break;
        case 'g': gs.set((unsigned *)(void *)intptr_t(c1), cnt); break;
        case 'p': fsh.set((unsigned *)(void *)intptr_t(c1), cnt); break;
        case 'c': cs.set((unsigned *)(void *)intptr_t(c1), cnt); break;
        case 'm': vpr.set((unsigned *)(void *)intptr_t(c1), cnt); break;
        case 'a': gs.set((unsigned *)(void *)intptr_t(c1), cnt); break;
      }
    }

    bool getCidx(const void *p, int cnt, int &c1, int &c2, int &c3, int sh_profile, bool verify) const
    {
      c1 = intptr_t(p) & 0xffffffff;
      if ((void *)intptr_t(c1) != p || (c1 & 0xF) != 3)
      {
        if (verify)
          sh_debug(SHLOG_ERROR, "Packed indices are corrupted: p=%p, c1=%i, cnt=%i, profile=%c", p, c1, cnt, sh_profile);
        return false;
      }
      c1 = c1 >> 4;
      c2 = cnt >> 16;
      c3 = cnt & 0xFFFF;
      return true;
    }
    bool getCidx(int sh_profile, int &c1, int &c2, int &c3, bool verify = false) const
    {
      switch (sh_profile)
      {
        case 'v': return getCidx(vpr.data(), vpr.size(), c1, c2, c3, sh_profile, verify);
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif
        case 'h': return getCidx(hs.data(), hs.size(), c1, c2, c3, sh_profile, verify);
        case 'd': return getCidx(ds.data(), ds.size(), c1, c2, c3, sh_profile, verify);
        case 'g': return getCidx(gs.data(), gs.size(), c1, c2, c3, sh_profile, verify);
        case 'p': return getCidx(fsh.data(), fsh.size(), c1, c2, c3, sh_profile, verify);
        case 'c': return getCidx(cs.data(), cs.size(), c1, c2, c3, sh_profile, verify);
        case 'm': return getCidx(vpr.data(), vpr.size(), c1, c2, c3, sh_profile, verify);
        case 'a': return getCidx(gs.data(), gs.size(), c1, c2, c3, sh_profile, verify);
        default: sh_debug(SHLOG_ERROR, "Unknown profile %c", (char)sh_profile); return false;
      }
    }

    using BlendValues = eastl::array<int8_t, shaders::RenderState::NumIndependentBlendParameters>;
    BlendValues blend_src, blend_dst, blend_asrc, blend_adst, blend_op, blend_aop;

    int cull_mode, alpha_to_coverage, view_instances, z_write, atest_val, fog_color;
    int color_write, z_test, atest_func;
    int stencil, stencil_func, stencil_ref, stencil_pass, stencil_fail, stencil_zfail, stencil_mask;
    int z_func;

    int fog_mode;
    real fog_density;
    bool z_bias, slope_z_bias;
    float z_bias_val = 0, slope_z_bias_val = 0;
    bool force_noablend;
    bool independent_blending;
    bool vs30, ps30;

    Pass() :
      vs30(false),
      ps30(false),
      cull_mode(-1),
      alpha_to_coverage(-1),
      view_instances(-1),
      z_write(-1),
      atest_val(-1),
      fog_color(-1),
      fog_mode(-1),
      fog_density(-1),
      color_write(-1),
      z_func(DRV3DC::CMPF_GREATEREQUAL),
      z_test(-1),
      atest_func(-1),
      stencil(0),
      stencil_func(DRV3DC::CMPF_ALWAYS),
      stencil_ref(0),
      stencil_pass(DRV3DC::STNCLOP_KEEP),
      stencil_fail(DRV3DC::STNCLOP_KEEP),
      stencil_zfail(DRV3DC::STNCLOP_KEEP),
      stencil_mask(255),
      z_bias(false),
      slope_z_bias(false),
      force_noablend(false),
      independent_blending(false)
    {
      eastl::fill(blend_src.begin(), blend_src.end(), -1);
      eastl::fill(blend_dst.begin(), blend_dst.end(), -1);
      eastl::fill(blend_asrc.begin(), blend_asrc.end(), -1);
      eastl::fill(blend_adst.begin(), blend_adst.end(), -1);
      eastl::fill(blend_op.begin(), blend_op.end(), BLENDOP_ADD);
      eastl::fill(blend_aop.begin(), blend_aop.end(), BLENDOP_ADD);
    }

    void push_stcode(int a) { curStcode.push_back(a); }
    void push_stblkcode(int a) { curStblkcode.push_back(a); }
    void append_stcode(dag::ConstSpan<int> c) { append_items(curStcode, c.size(), c.data()); }
    void append_stblkcode(dag::ConstSpan<int> c) { append_items(curStblkcode, c.size(), c.data()); }

    void push_alt_stcode(bool dyn, int a)
    {
      if (dyn)
        push_stcode(a);
      else
        push_stblkcode(a);
    }
    void append_alt_stcode(bool dyn, dag::ConstSpan<int> c)
    {
      if (dyn)
        append_stcode(c);
      else
        append_stblkcode(c);
    }
    Tab<int> &get_alt_curstcode(bool dyn) { return dyn ? curStcode : curStblkcode; }
    StcodeRoutine &get_alt_curcppcode(bool dyn) { return dyn ? curCppStcode : curCppStblkcode; }

    void finish_pass(bool accept);
    void reset_code();

    bool equal(Pass &p);
    void dump(ShaderSemCode &c);

  private:
    static Tab<int> curStcode, curStblkcode;
    static dag::ConstSpan<int> find_shared_stcode(Tab<int> &code, bool dyn);

    static StcodeRoutine curCppStcode, curCppStblkcode;
  };

  // for each variant create it's own passes
  struct PassTab
  {
    eastl::optional<Pass> pass;
    Tab<ShaderStateBlock *> suppBlk;

    Tab<int> varmap;

    PassTab() : suppBlk(codemem) {}
  };

  Tab<PassTab *> passes;
  int curVariant;

  // return current passes
  inline PassTab *getCurPasses() const { return ((curVariant >= 0) && (curVariant < passes.size())) ? passes[curVariant] : NULL; }


  // create if needed passes for dynamic variant #n
  PassTab *createPasses(int variant);

  // delete if needed passes for dynamic variant #n
  inline void deletePasses(int variant)
  {
    if (curVariant == variant)
      curVariant = -1;
    passes[variant] = NULL;
  }

  ShaderSemCode();
  ~ShaderSemCode() = default;

  const char *equal(ShaderSemCode &, bool compare_passes_and_vars);
  void dump();

  void initPassMap(int pass_id);
  void mergeVars(Tab<Var> &&other_vars, Tab<StVarMap> &&other_stvarmap, int pass_id);

  ShaderCode *generateShaderCode(const ShaderVariant::VariantTableSrc &dynVariants, StcodeShader &cppcode);

private:
  void convert_passes(ShaderSemCode::Pass &semP, ShaderCode::Pass &p, Tab<int> &cvar, StcodeShader &cppcode, const Tab<int> &var_map);
  static void convert_stcode(dag::Span<int> cod, Tab<int> &cvar, StcodeRegisters &static_regs, const Tab<int> &var_map);

  static int find_var_in_tab(const Tab<Var> &vars, const int variable_id);
};

#undef codemem

void clear_shared_stcode_cache();
