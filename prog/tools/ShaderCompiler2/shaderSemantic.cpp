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

bool parse_hlsl_source_to_blocks(shc::ShaderContext &ctx, ShaderParser::ShaderBoolEvalCB &boolCb)
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
    void decl_bool_alias(const char *name, const char *base_name) override { base.decl_bool_alias(name, base_name); }
  } proxyBoolCb{boolCb, ctx};

  const bool pp_as_comments = shc::config().hlslSavePPAsComments && !ctx.isDebugModeEnabled();
  for (HlslCompilationStage stage : HLSL_ALL_LIST)
  {
    if (!ctx.hlslCodeBlocks().all[stage].parseSourceCode(HLSL_ALL_PROFILES[stage], ctx.localHlslSrc().all[stage], proxyBoolCb,
          pp_as_comments, ctx.tgtCtx()))
    {
      return false;
    }
  }

  if (ctx.isDebugModeEnabled())
  {
    auto &messages = ctx.messages();
    auto &stringTable = messages.strings;
    for (int i = 0; i < stringTable.nameCount(); i++)
    {
      if (messages.isFilenameMessage(i))
        ctx.compiledShader().messages.emplace_back(stringTable.getName(i));
      else
        ctx.compiledShader().messages.emplace_back(string_f("%s: %s", ctx.name(), stringTable.getName(i)));
    }
  }
  return true;
}

eastl::pair<ShaderVariant::ExtType, int> get_or_add_resource_interval(const char *var_name, shc::TargetContext &tgtCtx,
  IntervalList *localIntervals)
{
  // Try to find registered interval.
  bool isGlobal = tgtCtx.globVars().getVarInternalIndex(tgtCtx.varNameMap().getVarId(var_name)) >= 0;
  ShaderVariant::VarType type = isGlobal ? ShaderVariant::VARTYPE_GL_OVERRIDE_INTERVAL : ShaderVariant::VARTYPE_INTERVAL;

  int resourceName_id = tgtCtx.intervalNameMap().getNameId(var_name);
  ShaderVariant::ExtType intervalIndex = INTERVAL_NOT_INIT;

  // Lookup shader intervals, then global intervals, otherwise try to add a new interval to the current block's scope.
  if (localIntervals)
    intervalIndex = localIntervals->getIntervalIndex(resourceName_id);

  if (intervalIndex == INTERVAL_NOT_INIT)
    intervalIndex = tgtCtx.globVars().getIntervalList().getIntervalIndex(resourceName_id);

  if (intervalIndex != INTERVAL_NOT_INIT)
    return eastl::make_pair(intervalIndex, resourceName_id);

  // Interval not found - register new interval.
  int textureIntervalNameId = tgtCtx.intervalNameMap().addNameId(var_name);
  Interval newInterval(textureIntervalNameId, type);
  newInterval.addValue(tgtCtx.intervalNameMap().addNameId("NULL"), 1.0f);
  newInterval.addValue(tgtCtx.intervalNameMap().addNameId("EXISTS"), IntervalValue::VALUE_INFINITY);

  IntervalList &targetList = localIntervals ? *localIntervals : tgtCtx.globVars().getMutableIntervalList();
  if (!targetList.addInterval(newInterval))
    return eastl::make_pair(INTERVAL_NOT_INIT, resourceName_id);

  return eastl::make_pair(targetList.getIntervalIndex(newInterval.getNameId()), resourceName_id);
}

} // namespace semantic
