// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader assembler
/************************************************************************/

#include "shsem.h"
#include "shsyn.h"
#include "shlexterm.h"
#include "shcode.h"
#include "shSemCode.h"
#include "shVariantContext.h"
#include "hwSemantic.h"
#include "variantSemantic.h"
#include "globalConfig.h"
#include "shExprParser.h"
#include "namedConst.h"
#include "nameMap.h"
#include "hlslStage.h"
#include "variablesMerger.h"
#include "shErrorReporting.h"
#include "fast_isalnum.h"
#include <EASTL/vector_map.h>
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
class AssembleShaderEvalCB : public ShaderEvalCB, public semantic::VariantBoolExprEvalCB
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

  struct PreshaderStat
  {
    state_block_stat *stat;
    ShaderStage stage;
    semantic::VariableType vt;
  };

  shc::VariantContext &ctx;

  // Cached refs from ctx
  ShaderClass &sclass; // @TODO: this should be const
  ShaderSemCode &code;
  ShaderSemCode::PassTab *curvariant;
  SemanticShaderPass *curpass;
  StcodeBytecodeAccumulator &stBytecodeAccum;
  StcodePass &stCppcodeAccum;
  NamedConstBlock &shConst;
  Parser &parser;
  const ExpressionParser exprParser; // Isn't a ref, but is actually a pair of refs to ctx and parser with behaviour
  const ShaderVariant::TypeTable &allRefStaticVars;

  bool dont_render = false;
  Terminal *no_dynstcode = nullptr;
  Tab<Terminal *> stcode_vars;
  const ShaderVariant::VariantInfo &variant;

  PerHlslStage<HlslCompile> hlsls{};

  Tab<eastl::variant<PreshaderStat, local_var_decl *>> preshaderScalarStats{};
  Tab<PreshaderStat> preshaderStaticTextureStats{};
  Tab<PreshaderStat> preshaderDynamicResourceStats{};
  Tab<PreshaderStat> preshaderHardcodedStats{};

  // For multidraw validation
  bool hasDynStcodeRelyingOnMaterialParams = false;
  Symbol *exprWithDynamicAndMaterialTerms = nullptr;

  eastl::vector_map<eastl::string_view, Symbol *> uavGlobalShadervarRefs{}, srvGlobalShadervarRefs{}, uavLocalShadervarRefs{},
    srvLocalShadervarRefs{};

  VariablesMerger varMerger;
  friend class VariablesMerger;

  Tab<uintptr_t> usedPreshaderStatements{};
  Tab<eastl::pair<uintptr_t, ShVarBool>> boolElementsEvaluationResults{};

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

  void eval_static(static_var_decl &s) override;
  void eval_interval_decl(interval &interv) override {}
  void eval_bool_decl(bool_decl &) override;
  void eval_init_stat(SHTOK_ident *var, shader_init_value &v);
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
  void eval_error_stat(error_stat &) override;
  void eval_render_stage(render_stage_stat &s) override;
  void eval_assume_stat(assume_stat &s) override {}
  void eval_assume_if_not_assumed_stat(assume_if_not_assumed_stat &s) override {}
  void eval_command(shader_directive &s) override;
  void eval_supports(supports_stat &) override;
  enum class BlendValueType
  {
    Factor,
    BlendFunc
  };
  void eval_blend_value(const Terminal &blend_func_tok, const SHTOK_intnum *const index,
    SemanticShaderPass::BlendValues &blend_factors, const BlendValueType type);

  inline int eval_if(bool_expr &e) override;
  void eval_else(bool_expr &) override {}
  void eval_endif(bool_expr &) override {}

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

  void compilePreshader();
  void end_pass();

  void end_eval(shader_decl &sh);

  void eval_hlsl_compile(hlsl_compile_class &hlsl_compile) override;
  void eval_hlsl_decl(hlsl_local_decl_class &hlsl_decl) override;

  void hlsl_compile(HlslCompilationStage stage);

  // Cache in main pass
  void eval_external_block_stat(state_block_stat &s, ShaderStage stage);
  void eval_shader_locdecl(local_var_decl &s) override { preshaderScalarStats.emplace_back(&s); }

  void process_external_block_stat(const PreshaderStat &stat);
  void process_shader_locdecl(local_var_decl &s);

  void decl_bool_alias(const char *name, bool_expr &expr) override;
  int is_debug_mode_enabled() override { return ctx.shCtx().isDebugModeEnabled(); }

  void compile_external_block_stat(const PreshaderStat &stat);

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

  void reserveSpecialCbufferAt(HlslSlotSemantic cbuffer_sem, int reg);

  bool validateDynamicConstsForMultidraw();
}; // class AssembleShaderEvalCB

inline int AssembleShaderEvalCB::eval_if(bool_expr &e) { return eval_expr(e).value ? IF_TRUE : IF_FALSE; }


// clear caches
void clear_per_file_caches();

inline CodeSourceBlocks *getSourceBlocks(const char *profile)
{
  extern PerHlslStage<CodeSourceBlocks *> curHlslBlks;
  return *curHlslBlks.validProfileSwitch(profile);
}

extern SCFastNameMap renderStageToIdxMap;

} // namespace ShaderParser
