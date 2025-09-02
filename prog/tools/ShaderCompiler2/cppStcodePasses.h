// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsem.h"
#include "shVariantContext.h"
#include "variantSemantic.h"
#include "commonUtils.h"

class StcodeBranchedBuildEvalCB : public ShaderParser::ShaderEvalCB
{
  shc::VariantContext &ctx;

  // Cached from ctx
  Parser &parser;

  ska::flat_hash_map<uintptr_t, ShVarBool> boolEvalResults{};
  ska::flat_hash_set<const state_block_stat *> preshaderStatementsUsedInAnyVariant{};

public:
  explicit StcodeBranchedBuildEvalCB(shc::VariantContext &a_ctx);

  PINNED_TYPE(StcodeBranchedBuildEvalCB)

  void eval_static(static_var_decl &d) override {}
  void eval_interval_decl(interval &) override {}
  void eval_error_stat(error_stat &) override {}
  void eval_channel_decl(channel_decl &, int) override {}
  void eval_state(state_stat &) override {}
  void eval_zbias_state(zbias_state_stat &) override {}
  void eval(immediate_const_block &) override {}
  void eval_supports(supports_stat &) override {}
  void eval_render_stage(render_stage_stat &) override {}
  void eval_assume_stat(assume_stat &) override {}
  void eval_assume_if_not_assumed_stat(assume_if_not_assumed_stat &) override {}
  void eval_command(shader_directive &) override {}
  void eval_hlsl_compile(hlsl_compile_class &) override {}
  void eval_hlsl_decl(hlsl_local_decl_class &) override {}

  void eval_bool_decl(bool_decl &d) override;

  void eval_external_block(external_state_block &state_block) override;
  void eval_shader_locdecl(local_var_decl &s) override;

  int eval_if(bool_expr &e) override;
  void eval_else(bool_expr &e) override;
  void eval_endif(bool_expr &e) override;

private:
  void evalExternalBlockStat(const state_block_stat &, ShaderStage stage);

  eastl::optional<ShVarBool> evalBoolStatIfExists(const auto &stat)
  {
    auto it = boolEvalResults.find(uintptr_t(&stat));
    if (it == boolEvalResults.end())
      return eastl::nullopt;
    return it->second;
  }

  ShVarBool evalBoolStat(const auto &stat)
  {
    auto maybe = evalBoolStatIfExists(stat);
    G_ASSERT(maybe);
    return *maybe;
  }

  // @TODO: abstract out, should not be cpp spesific
  void buildConditionString(const bool_expr &e, StcodeBuilder &out);
  void buildConditionBoolAnd(const and_expr &e, StcodeBuilder &out);
  void buildConditionBoolNot(const not_expr &e, StcodeBuilder &out);
  void buildConditionBoolValue(const bool_value &val, StcodeBuilder &out);

  static const char *boolToStr(bool b) { return b ? "true" : "false"; }
};