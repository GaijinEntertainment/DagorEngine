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
#include "shVarBool.h"
#include "shaderVariantSrc.h"
#include <shaders/slotTexturesRange.h>
#include "shaderBytecodeCache.h"
#include "preshaderCompilation.h"
#include <memory/dag_regionMemAlloc.h>
#include <EASTL/optional.h>

#include <osApiWrappers/dag_stackHlp.h>

namespace shc
{
class VariantContext;
}

struct SemanticShaderPass
{
  using BlendValues = eastl::array<int8_t, shaders::RenderState::NumIndependentBlendParameters>;

  struct EnableFp16State
  {
    bool vsOrMs : 1 = false;
    bool hs : 1 = false;
    bool ds : 1 = false;
    bool gsOrAs : 1 = false;
    bool psOrCs : 1 = false;
  };

  FSHTYPE fshver;
  ShaderStageData fsh;
  ShaderStageData vpr;
  ShaderStageData hs, ds, gs;
  ShaderStageData cs;

  eastl::optional<ShaderCacheLevelIds> cacheIds[HLSL_COUNT];

  CompiledPreshader *preshader = nullptr;

  HlslRegRange psOrCsConstRange, vsConstRange;
  HlslRegRange bufferedConstRange;
  SlotTexturesRangeInfo psTexSmpRange, vsTexSmpRange;
  Tab<eastl::pair<uintptr_t, ShVarBool>> boolAstNodesEvaluationResults{};

  bool scarlettWave32 = false;

  EnableFp16State enableFp16{};
  eastl::array<uint16_t, 3> threadGroupSizes = {};

  BlendValues blend_src, blend_dst, blend_asrc, blend_adst, blend_op, blend_aop;
  E3DCOLOR blend_factor = E3DCOLOR{0xFFFFFFFF};
  bool blend_factor_specified = false;

  int cull_mode, alpha_to_coverage, view_instances, z_write, atest_val, fog_color;
  int color_write, z_test, atest_func;
  int stencil, stencil_func, stencil_ref, stencil_pass, stencil_fail, stencil_zfail, stencil_mask;
  int z_func;

  int fog_mode;
  real fog_density;
  float z_bias_val = 0, slope_z_bias_val = 0;
  bool z_bias : 1;
  bool slope_z_bias : 1;
  bool force_noablend : 1;
  bool independent_blending : 1;
  bool dual_source_blending : 1;
  bool vs30 : 1;
  bool ps30 : 1;

  ShaderCode::Pass *target = nullptr;

  static constexpr eastl::array<ShaderStageData SemanticShaderPass::*, HLSL_COUNT> BYTECODE_MEMBERS_MAPPING = {
    &SemanticShaderPass::fsh, // HLSL_PS
    &SemanticShaderPass::vpr, // HLSL_VS
    &SemanticShaderPass::cs,  // HLSL_CS
    &SemanticShaderPass::ds,  // HLSL_DS
    &SemanticShaderPass::hs,  // HLSL_HS
    &SemanticShaderPass::gs,  // HLSL_GS
    &SemanticShaderPass::vpr, // HLSL_MS - aliased with vs
    &SemanticShaderPass::gs,  // HLSL_AS - aliased with gs
  };

  SemanticShaderPass() :
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
    independent_blending(false),
    dual_source_blending(false)
  {
    eastl::fill(blend_src.begin(), blend_src.end(), -1);
    eastl::fill(blend_dst.begin(), blend_dst.end(), -1);
    eastl::fill(blend_asrc.begin(), blend_asrc.end(), -1);
    eastl::fill(blend_adst.begin(), blend_adst.end(), -1);
    eastl::fill(blend_op.begin(), blend_op.end(), BLENDOP_ADD);
    eastl::fill(blend_aop.begin(), blend_aop.end(), BLENDOP_ADD);
  }

  void setCidx(HlslCompilationStage stage, ShaderCacheLevelIds entryIds) { cacheIds[stage] = entryIds; }

  eastl::optional<ShaderCacheLevelIds> getCidx(HlslCompilationStage stage) const { return cacheIds[stage]; }

  bool equal(SemanticShaderPass &p);
  void dump(ShaderSemCode &c);

  void clearIntermediateData() { clear_and_shrink(boolAstNodesEvaluationResults); }
};

class ShaderSemCode
{
public:
  struct Var
  {
    int nameId;
    ShaderVarType type;
    int slot = -1;
    ShaderVarTextureType texType = ShaderVarTextureType::SHVT_TEX_UNKNOWN;
    void *terminal;
    bool used, dynamic, noWarnings;

    Var() : nameId(-1), terminal(NULL), used(false), dynamic(false), noWarnings(false) {}
  };

  struct StVarMapping
  {
    int v, sv;
  };

  // for each variant create it's own passes
  struct PassTab
  {
    eastl::optional<SemanticShaderPass> pass;
    Tab<ShaderStateBlock *> suppBlk;

    Tab<int> varmap;
  };

  shc::TargetContext &ctx;

  Tab<ShaderChannelId> channel;

  int flags;
  int renderStageIdx;

  StcodeStaticVars staticStcodeVars;

  Tab<Var> vars;
  int regsize;

  Tab<int> initcode;
  Tab<StVarMapping> stvarmap;

  dag::Vector<eastl::unique_ptr<PassTab>> passes;

public:
  explicit ShaderSemCode(shc::TargetContext &a_ctx);

  int find_var(const int variable_id);

  const char *equal(ShaderSemCode &, bool compare_passes_and_vars);
  void dump();

  void initPassMap(int pass_id);
  void mergeVars(Tab<Var> &&other_vars, Tab<StVarMapping> &&other_stvarmap, int pass_id);

  ShaderCode *generateShaderCode(const ShaderVariant::VariantTableSrc &dynVariants);

private:
  void convert_passes(SemanticShaderPass &semP, ShaderCode::Pass &p, Tab<int> &cvar, const Tab<int> &var_map);
  static void convert_stcode(dag::Span<int> cod, Tab<int> &cvar, const Tab<int> &var_map);

  static int find_var_in_tab(const Tab<Var> &vars, const int variable_id);
};

const char *get_semcode_var_name(const ShaderSemCode::Var &var, const shc::TargetContext &ctx);
