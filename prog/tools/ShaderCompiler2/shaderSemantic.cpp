// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderSemantic.h"

namespace semantic
{

void initialize_debug_mode(shc::ShaderContext &ctx)
{
  bool debugModeEnabled = shc::config().isDebugModeEnabled;
  if (!debugModeEnabled)
  {
    if (auto valueMaybe = ctx.assumes().getAssumedVal("debug_mode_enabled", true))
      debugModeEnabled = (*valueMaybe > 0.0f);
  }
  else
    ctx.assumes().addAssume("debug_mode_enabled", 1.0f, false, ctx.tgtCtx().sourceParseState().parser);

  if (!debugModeEnabled)
  {
    ctx.localHlslSrc().forEach([](String &src) {
      constexpr char PATTERN_TO_REMOVE[] = "#line 1 \"precompiled\"\n#undef _FILE_\n#define _FILE_ ";
      constexpr char STUBHDR_TO_INSERT[] = "#line 1 \"precompiled\"\n#define _FILE_ -1\n";

      src.replaceAll(PATTERN_TO_REMOVE, "//");
      src.insert(0, STUBHDR_TO_INSERT, LITSTR_LEN(STUBHDR_TO_INSERT));
    });
  }

  ctx.setDebugModeEnabled(debugModeEnabled);
}

bool parse_hlsl_source_to_blocks(const PerHlslStage<CodeSourceBlocks *> &dst, ShaderParser::ShaderBoolEvalCB &boolCb,
  shc::ShaderContext &ctx)
{
  // @TODO: If possible, add filenames from the hlsl in shader into the local messages table, and not global. This would be one step
  // towards parallelism.
  if (ctx.isDebugModeEnabled())
    ctx.messages().strings = ctx.tgtCtx().globMessages();

  // Adds the message functionality impl to a basic bool callback
  // @TODO: what about separating this functionality into two interfaces instead? This would save an indirection.
  struct ProxyBoolEvalCB : public ShaderParser::ShaderBoolEvalCB
  {
    ShaderParser::ShaderBoolEvalCB &base;
    shc::ShaderContext &ctx;

    ProxyBoolEvalCB(ShaderParser::ShaderBoolEvalCB &a_base, shc::ShaderContext &a_ctx) : base{a_base}, ctx{a_ctx} {}

    int add_message(const char *message, bool file_name) override
    {
      if (!ctx.isDebugModeEnabled())
        return -1;
      return ctx.messages().addMessage(message, file_name);
    }
    int is_debug_mode_enabled() override { return ctx.isDebugModeEnabled(); }

    ShVarBool eval_expr(bool_expr &e) override { return base.eval_expr(e); }
    ShVarBool eval_bool_value(bool_value &v) override { return base.eval_bool_value(v); }
    int eval_interval_value(const char *ival_name) override { return base.eval_interval_value(ival_name); }
    void decl_bool_alias(const char *name, bool_expr &expr) override { base.decl_bool_alias(name, expr); }
  } proxyBoolCb{boolCb, ctx};

  const bool pp_as_comments = shc::config().hlslSavePPAsComments && !ctx.isDebugModeEnabled();
  for (HlslCompilationStage stage : HLSL_ALL_LIST)
  {
    if (!dst.all[stage]->parseSourceCode(HLSL_ALL_PROFILES[stage], ctx.localHlslSrc().all[stage], proxyBoolCb, pp_as_comments,
          ctx.tgtCtx()))
      return false;
  }
  return true;
}

} // namespace semantic
