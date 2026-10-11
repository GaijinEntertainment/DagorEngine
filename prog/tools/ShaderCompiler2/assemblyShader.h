// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader assembler
/************************************************************************/

#include "shsem.h"
#include "subpassRead.h"
#include "shsyn.h"
#include "shlexterm.h"
#include "shcode.h"
#include "shSemCode.h"
#include "shVariantContext.h"
#include "hwSemantic.h"
#include "variantSemantic.h"
#include "preshaderCompilation.h"
#include "globalConfig.h"
#include "shExprParser.h"
#include "namedConst.h"
#include "nameMap.h"
#include "hlslStage.h"
#include "variablesMerger.h"
#include "shErrorReporting.h"
#include "fast_isalnum.h"
#include "gatherVariantLocalVars.h"
#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>
#include "defer.h"

/************************************************************************/
/* forwards
/************************************************************************/
struct ShHardwareOptions;
class CodeSourceBlocks;

constexpr bool useScarlettWave32 = false;

namespace ShaderParser
{
/*********************************
 *
 * class AssembleShaderEvalCB
 *
 *********************************/
class AssembleShaderEvalCB : public GatherVariantLocalVarsCB
{
public:
  struct HlslCompile
  {
    eastl::optional<semantic::HlslCompileDirective> compile{};
    hlsl_compile_class *symbol = nullptr;
    const char *stageName;
    String defaultTarget;

    void reset() { compile = {}; }
    bool hasCompilation() const { return compile.has_value(); }
  };

  // Cached refs from ctx
  ShaderSemCode::PassTab *curvariant;
  SemanticShaderPass *curpass;
  const ExpressionParser exprParser; // Isn't a ref, but is actually a pair of refs to ctx and parser with behaviour
  const ShaderVariant::TypeTable &allRefStaticVars;

  bool dont_render = false;
  Terminal *no_dynstcode = nullptr;
  const ShaderVariant::VariantInfo &variant;

  // RT pipeline config authored by the top-level max_recursion / allow_opacity_micromaps shader
  // statements; read by CompileShaderJob to fill the bindump so the driver builds the pipeline-config
  // subobject CPU-side. Defaults keep the flag clear (no explicit config) for shaders without them.
  bool rtPipelineConfigDeclared = false;
  bool rtPipelineAllowOmm = false;
  bool rtPipelineMaxRecursionSet = false;
  int rtPipelineMaxRecursion = 1;

  RenderPassOptIn activeOptIn;
  bool optInUnresolved = false;

  const ShaderTerminal::state_block_struct_decl *psOutputStructDecl = nullptr;

  PerHlslStage<HlslCompile> hlsls{};
  PreshaderCompilationInput preshaderSource{};

  Tab<eastl::pair<uintptr_t, ShVarBool>> boolElementsEvaluationResults{};

  Tab<eastl::pair<ShaderTerminal::state_block_stat *, ShaderStage>> refinedBlockStats{};

private:
  enum BlockPipelineType
  {
    BLOCK_COMPUTE = 0,
    BLOCK_GRAPHICS_PS = 1,
    BLOCK_GRAPHICS_VERTEX = 2,
    BLOCK_GRAPHICS_MESH = 3,

    BLOCK_MAX
  };
  bool declaredBlockTypes[BLOCK_MAX] = {};

public:
  /************************************************************************/
  /* AssemblyShader.cpp
  /************************************************************************/
  explicit AssembleShaderEvalCB(shc::VariantContext &ctx);

  void eval_channel_decl(channel_decl &s, int stream_id = 0) override;

  int get_blend_k(const Terminal &s);
  int get_blend_op_k(const Terminal &s);
  int get_stencil_cmpf_k(const Terminal &s);
  int get_stensil_op_k(const Terminal &s);
  int get_bool_const(const Terminal &s);
  void get_depth_cmpf_k(const Terminal &s, int &cmpf);

  void eval_state(state_stat &s) override;
  void eval_zbias_state(zbias_state_stat &s) override;
  void eval_external_block(external_state_block &) override;
  void eval(immediate_const_block &) override {}
  void eval_render_stage(render_stage_stat &s) override;
  void eval_raytrace_pipeline(raytrace_pipeline_stat &s) override;
  void eval_assume_stat(assume_stat &s) override {}
  void eval_command(shader_directive &s) override;
  enum class BlendValueType
  {
    Factor,
    BlendFunc
  };
  void eval_blend_value(const Terminal &blend_func_tok, const SHTOK_intnum *const index,
    SemanticShaderPass::BlendValues &blend_factors, const BlendValueType type);

  inline int eval_if(bool_expr &e) override;

  ShVarBool eval_expr(bool_expr &e) override
  {
    auto res = semantic::VariantBoolExprEvalCB::eval_expr(e);
    if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
      boolElementsEvaluationResults.emplace_back(uintptr_t(&e), res);
    return res;
  }
  ShVarBool eval_bool_value(bool_value &val) override
  {
    auto res = semantic::VariantBoolExprEvalCB::eval_bool_value(val);
    if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
      boolElementsEvaluationResults.emplace_back(uintptr_t(&val), res);
    return res;
  }

  bool end_pass();

  void end_eval(shader_decl &sh) override;

  void eval_hlsl_compile(hlsl_compile_class &hlsl_compile) override;
  void eval_hlsl_decl(hlsl_local_decl_class &hlsl_decl) override;

  void hlsl_compile(HlslCompilationStage stage);

  // Cache in main pass
  void eval_external_block_stat(state_block_stat &s, ShaderStage stage);
  void eval_refined_block_var(state_block_stat &s, ShaderStage stage);
  void processRefinedBlockVars();
  void eval_shader_locdecl(local_var_decl &s) override { preshaderSource.scalarStats.emplace_back(&s); }

  void eval_supports(supports_stat &s) override { preshaderSource.supportStats.emplace_back(&s); }
  void eval_render_pass(render_pass_use_stat &s) override;
  bool validateSubpassReads();
  void evalPsOutputStruct(ShaderTerminal::state_block_struct_decl &s, ShaderStage stage);
  void processPsOutputStruct();
  shc::PsOutputStructCacheEntry buildPsOutputStruct(const ShaderTerminal::state_block_struct_decl &s);
  eastl::string exportFormatPragmas(dag::ConstSpan<SemanticShaderPass::PsOutputField> field_by_slot, const char *src1_target);

  int is_debug_mode_enabled() override { return ctx.shCtx().isDebugModeEnabled(); }

private:
  void addBlockType(const char *name, const Terminal *t);
  bool hasDeclaredGraphicsBlocks();
  bool hasDeclaredMeshPipelineBlocks();

  bool isCompute() const { return hlsls.fields.cs.hasCompilation(); }
  bool isMesh() const { return hlsls.fields.ms.hasCompilation() || hlsls.fields.as.hasCompilation(); }
  bool isGraphics() const
  {
    return hlsls.fields.ps.hasCompilation() || hlsls.fields.vs.hasCompilation() || hlsls.fields.hs.hasCompilation() ||
           hlsls.fields.ds.hasCompilation() || hlsls.fields.gs.hasCompilation();
  }

  void evalHlslCompileClass(HlslCompile *comp);
}; // class AssembleShaderEvalCB

inline int AssembleShaderEvalCB::eval_if(bool_expr &e) { return eval_expr(e).value ? IF_TRUE : IF_FALSE; }

// clear caches
void clear_per_file_caches();

extern SCFastNameMap renderStageToIdxMap;

} // namespace ShaderParser
