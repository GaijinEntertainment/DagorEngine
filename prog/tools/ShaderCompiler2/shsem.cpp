// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shCompiler.h"
#include "shTargetContext.h"
#include "shShaderContext.h"
#include "shVariantContext.h"
#include "shaderSemantic.h"
#include "variantAssembly.h"
#include "cppStcodePasses.h"
#include "globalConfig.h"
#include "shsem.h"
#include "shLog.h"
#include "assemblyShader.h"
#include "gatherVar.h"
#include "globVar.h"
#include "boolVar.h"
#include "codeBlocks.h"
#include "cppStcodeAssembly.h"
#include "cppStcode.h"
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <generic/dag_tabUtils.h>
#include "linkShaders.h"
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/slotTexturesRange.h>

using namespace ShaderParser;

// if defined, partial boolean expressions evaluation used
#define FAST_BOOLEAN_EVAL

void ShaderParser::addSourceCode(String &text, Symbol *term, const char *src, Parser &p, SCFastNameMap &messages_table,
  bool need_file_idx)
{
  if (!*src)
    return;
  const char *filename = p.get_lexer().get_input_stream()->get_filename(term->file_start);
  if (need_file_idx)
    text.aprintf(32, "#line 1 \"precompiled\"\n#undef _FILE_\n#define _FILE_ %d\n", messages_table.addNameId(filename));
  text.aprintf(32, "#line %d \"%s\"\n%s", term->line_start, filename, src);
}

void ShaderParser::addSourceCode(String &text, SHTOK_hlsl_text *src, Parser &p, SCFastNameMap &messages_table, bool need_file_idx)
{
  addSourceCode(text, src, src->text, p, messages_table, need_file_idx);
}


void ShaderSemCode::dump()
{
  debug("regsize = %d", regsize);
  debug("%d channels:", channel.size());
  for (int i = 0; i < channel.size(); ++i)
  {
    ShaderChannelId &c = channel[i];
    debug("  %s %s[%d] <- %s[%d]", ShUtils::channel_type_name(c.t), ShUtils::channel_usage_name(c.vbu), c.vbui,
      ShUtils::channel_usage_name(c.u), c.ui);
  }

  debug("%d vars:", vars.size());
  for (int i = 0; i < vars.size(); ++i)
  {
    Var &v = vars[i];
    debug("  %s (%s)", get_semcode_var_name(v, ctx), ShUtils::shader_var_type_name(v.type));
  }

  debug("init code: %d tokens", initcode.size());
  for (int i = 0; i < initcode.size(); i += 2)
  {
    int vi = initcode[i];
    int c = initcode[i + 1];
    if (shaderopcode::getOp(c) == SHCOD_TEXTURE)
    {
      debug("  %s <- texture.%d", get_semcode_var_name(vars[vi], ctx), shaderopcode::getOp1p1(c));
    }
    else
      debug("  %s <- %s", get_semcode_var_name(vars[vi], ctx), ShUtils::shcod_tokname(shaderopcode::getOp(c)));
  }

  for (int i = 0; i < passes.size(); ++i)
  {
    if (passes[i])
    {
      debug("dyn variant %d:", i);
      if (passes[i]->pass)
      {
        passes[i]->pass->dump(*this);
      }
    }
  }
}

void SemanticShaderPass::dump(ShaderSemCode &code)
{
  debug("  state code: %d tokens", stcode.size());
  for (int i = 0; i < stcode.size(); ++i)
    debug("    %10d %s", stcode[i], ShUtils::shcod_tokname(shaderopcode::getOp(stcode[i])));
  debug("  stateblk code: %d tokens", stblkcode.size());
  for (int i = 0; i < stblkcode.size(); ++i)
    debug("    %10d %s", stblkcode[i], ShUtils::shcod_tokname(shaderopcode::getOp(stblkcode[i])));
}

int ShaderSemCode::find_var(const int variable_id)
{
  for (int i = 0; i < vars.size(); i++)
  {
    if (vars[i].nameId == variable_id)
      return i;
  }

  return -1;
}


bool SemanticShaderPass::equal(SemanticShaderPass &p)
{
  if (fsh.size() != p.fsh.size())
    return false;
  if (fsh.data() != p.fsh.data() && !mem_eq(fsh, p.fsh.data()))
    return false;

  if (vpr.size() != p.vpr.size())
    return false;
  if (vpr.data() != p.vpr.data() && !mem_eq(vpr, p.vpr.data()))
    return false;

  if (hs.size() != p.hs.size())
    return false;
  if (hs.data() != p.hs.data() && !mem_eq(hs, p.hs.data()))
    return false;

  if (ds.size() != p.ds.size())
    return false;
  if (ds.data() != p.ds.data() && !mem_eq(ds, p.ds.data()))
    return false;

  if (gs.size() != p.gs.size())
    return false;
  if (gs.data() != p.gs.data() && !mem_eq(gs, p.gs.data()))
    return false;

  if (stcode.size() != p.stcode.size())
    return false;
  if (stcode.data() != p.stcode.data() && !mem_eq(stcode, p.stcode.data()))
    return false;

  if (stblkcode.size() != p.stblkcode.size())
    return false;
  if (stblkcode.data() != p.stblkcode.data() && !mem_eq(stblkcode, p.stblkcode.data()))
    return false;

#define COMPARE_V(v) \
  if (v != p.v)      \
    return false;
  COMPARE_V(blend_src);
  COMPARE_V(blend_dst);
  COMPARE_V(blend_asrc);
  COMPARE_V(blend_adst);
  COMPARE_V(blend_factor);
  COMPARE_V(cull_mode);
  COMPARE_V(alpha_to_coverage);
  COMPARE_V(z_write);
  COMPARE_V(atest_val);
  COMPARE_V(fog_color);
  COMPARE_V(color_write);
  COMPARE_V(z_test);
  COMPARE_V(atest_func);
  COMPARE_V(stencil);
  COMPARE_V(stencil_func);
  COMPARE_V(stencil_ref);
  COMPARE_V(stencil_pass);
  COMPARE_V(stencil_fail);
  COMPARE_V(stencil_zfail);
  COMPARE_V(stencil_mask);
  COMPARE_V(z_func);

  COMPARE_V(fog_mode);
  COMPARE_V(fog_density);
  COMPARE_V(z_bias);
  COMPARE_V(slope_z_bias);
  COMPARE_V(z_bias_val);
  COMPARE_V(slope_z_bias_val);
  COMPARE_V(force_noablend);
  COMPARE_V(vs30);
  COMPARE_V(ps30);
#undef COMPARE_V

  return true;
}

const char *ShaderSemCode::equal(ShaderSemCode &c, bool compare_passes_and_vars)
{
  if (channel.size() != c.channel.size())
    return "channel count";
  if (!mem_eq(channel, c.channel.data()))
    return "channel data";

  if (regsize != c.regsize)
  {
    return "regsize";
  }

  if (initcode.size() != c.initcode.size())
    return "initcode count";
  if (!mem_eq(initcode, c.initcode.data()))
    return "initcode data";

  if (!compare_passes_and_vars)
    return NULL;

  if (vars.size() != c.vars.size())
    return "variable count";

  int i;
  for (i = 0; i < vars.size(); ++i)
  {
    if (vars[i].type != c.vars[i].type)
      return "variable type";
    if (vars[i].nameId != c.vars[i].nameId)
      return "variable name";
  }

  if (stvarmap.size() != c.stvarmap.size())
    return "stvarmap count";
  if (!mem_eq(stvarmap, c.stvarmap.data()))
    return "stvarmap data";

  if (passes.size() != c.passes.size())
    return "different number of dynamic variants";

  for (int di = 0; di < passes.size(); ++di)
  {
    ShaderSemCode::PassTab *thisTab = passes[di].get();
    ShaderSemCode::PassTab *otherTab = c.passes[di].get();

    if (!thisTab && !otherTab)
      continue;

    if (!thisTab || !otherTab)
      return "no passes";

    if (thisTab->pass.has_value() != otherTab->pass.has_value())
      return "diffirent passes count";

    if (thisTab->pass)
      if (!thisTab->pass->equal(*otherTab->pass))
        return "pass not equal";
  }

  return NULL;
}

// ³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³//

namespace ShaderParser
{
PerHlslStage<CodeSourceBlocks *> curHlslBlks{};
SCFastNameMap renderStageToIdxMap;

static void eval_shader_stat(shader_stat &s, ShaderEvalCB &cb);
static void eval_shader_stat(shader_stat *s, ShaderEvalCB &cb);

static void eval_shader_if(shader_if &s, ShaderEvalCB &cb)
{
  G_ASSERT(s.expr);
  int res = cb.eval_if(*s.expr);
  if (res == ShaderEvalCB::IF_TRUE || res == ShaderEvalCB::IF_BOTH)
  {
    for (int i = 0; i < s.true_stat.size(); ++i)
      eval_shader_stat(s.true_stat[i], cb);
  }
  if (s.false_stat.size() || s.else_if)
    cb.eval_else(*s.expr);
  if (res == ShaderEvalCB::IF_FALSE || res == ShaderEvalCB::IF_BOTH)
  {
    for (int i = 0; i < s.false_stat.size(); ++i)
      eval_shader_stat(s.false_stat[i], cb);
    if (s.else_if)
      eval_shader_if(*s.else_if, cb);
  }
  cb.eval_endif(*s.expr);
}

static void eval_shader_stat(shader_stat &s, ShaderEvalCB &cb)
{
  if (s.static_decl)
    cb.eval_static(*s.static_decl);
  else if (s.interval_decl)
    cb.eval_interval_decl(*s.interval_decl);
  else if (s.state)
    cb.eval_state(*s.state);
  else if (s.zbias_state)
    cb.eval_zbias_state(*s.zbias_state);
  else if (s.channel)
    cb.eval_channel_decl(*s.channel);
  else if (s.render_stage)
    cb.eval_render_stage(*s.render_stage);
  else if (s.assume)
    cb.eval_assume_stat(*s.assume);
  else if (s.assume_if_not_assumed)
    cb.eval_assume_if_not_assumed_stat(*s.assume_if_not_assumed);
  else if (s.dir)
    cb.eval_command(*s.dir);
  else if (s.if_stat)
    eval_shader_if(*s.if_stat, cb);
  else if (s.local_decl)
    cb.eval_shader_locdecl(*s.local_decl);
  else if (s.hlsl_compile_var)
    cb.eval_hlsl_compile(*s.hlsl_compile_var);
  else if (s.hlsl_decl_var)
    cb.eval_hlsl_decl(*s.hlsl_decl_var);
  else if (s.supports)
    cb.eval_supports(*s.supports);
  else if (s.external_block)
    cb.eval_external_block(*s.external_block);
  else if (s.imm_const_block)
    cb.eval(*s.imm_const_block);
  else if (s.boolean_decl)
    cb.eval_bool_decl(*s.boolean_decl);
  else if (s.error)
    cb.eval_error_stat(*s.error);
  else
  {
    // error
    G_ASSERT(0);
  }
}

static void eval_shader_stat(shader_stat *s, ShaderEvalCB &cb)
{
  if (s)
    eval_shader_stat(*s, cb);
}

static bool is_compute(hlsl_compile_class &hlsl_compile)
{
  return dd_strnicmp(hlsl_compile.profile->text, "target_cs", 9) == 0 || dd_strnicmp(hlsl_compile.profile->text, "cs_", 3) == 0;
}

void eval_shader(shader_decl &sh, ShaderEvalCB &cb)
{
  for (int i = 0; i < sh.stat.size(); ++i)
    eval_shader_stat(sh.stat[i], cb);
}

// ³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³//

template <bool USE_CB_FOR_NESTED_EXPRESSIONS>
static ShVarBool eval_shader_bool_not(not_expr &e, ShaderBoolEvalCB &cb)
{
  G_ASSERT(&e);
  G_ASSERT(e.value);
  ShVarBool v;
  if (e.value->expr)
    v = USE_CB_FOR_NESTED_EXPRESSIONS ? cb.eval_expr(*e.value->expr) : eval_shader_bool(*e.value->expr, cb);
  else
    v = cb.eval_bool_value(*e.value);
  if (e.is_not)
    v = !v;
  return v;
}

template <bool USE_CB_FOR_NESTED_EXPRESSIONS>
static ShVarBool eval_shader_bool_and(and_expr &e, ShaderBoolEvalCB &cb)
{
  if (e.value)
  {
    ShVarBool v = eval_shader_bool_not<USE_CB_FOR_NESTED_EXPRESSIONS>(*e.value, cb);
    return v;
  }

  ShVarBool a = eval_shader_bool_and<USE_CB_FOR_NESTED_EXPRESSIONS>(*e.a, cb);
#if defined(FAST_BOOLEAN_EVAL)
  if (a.isConst && !a.value)
  {
    return a;
  }
#endif

  ShVarBool b = eval_shader_bool_not<USE_CB_FOR_NESTED_EXPRESSIONS>(*e.b, cb);
  return a && b;
}

template <bool USE_CB_FOR_NESTED_EXPRESSIONS>
ShVarBool eval_shader_bool(bool_expr &e, ShaderBoolEvalCB &cb)
{
  G_ASSERT(&e);
  if (e.value)
  {
    ShVarBool v = eval_shader_bool_and<USE_CB_FOR_NESTED_EXPRESSIONS>(*e.value, cb);
    return v;
  }

  ShVarBool a = USE_CB_FOR_NESTED_EXPRESSIONS ? cb.eval_expr(*e.a) : eval_shader_bool(*e.a, cb);
#if defined(FAST_BOOLEAN_EVAL)
  if (a.isConst && a.value)
  {
    return a;
  }
#endif

  ShVarBool b = eval_shader_bool_and<USE_CB_FOR_NESTED_EXPRESSIONS>(*e.b, cb);
  return a || b;
}

template ShVarBool eval_shader_bool<true>(bool_expr &e, ShaderBoolEvalCB &cb);
template ShVarBool eval_shader_bool<false>(bool_expr &e, ShaderBoolEvalCB &cb);

// ³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³//

struct EvaluatedShaderVariant
{
  ShaderSemCode semcode;
  eastl::unique_ptr<ShaderSemCode::PassTab> pass;

  explicit EvaluatedShaderVariant(shc::TargetContext &ctx) : semcode{ctx}, pass{new ShaderSemCode::PassTab{}} { pass->pass.emplace(); }
};

// return true, if need render
static eastl::optional<EvaluatedShaderVariant> evalShaderVariant(shader_decl *sh, Parser &parser,
  const ShaderVariant::VariantInfo &variant, shc::ShaderContext &ctx)
{
  EvaluatedShaderVariant res{ctx.tgtCtx()};

  shc::VariantContext variantCtx = ctx.makeVariantContext(variant, res.semcode, res.pass.get());

  // evaluate shader
  parser.get_lexer().begin_shader();
  AssembleShaderEvalCB cb{variantCtx};
  try
  {
    eval_shader(*sh, cb);
    cb.end_eval(*sh);
  }
  catch (ShaderEvalCB::GsclStopProcessingException e)
  {}
  parser.get_lexer().end_shader();

  if (cb.dont_render)
  {
    // render not needed - skip shader code selection
    return eastl::nullopt;
  }

  return res;
}

static void shaderError(Parser &parser, const char *msg, Terminal *t)
{
  if (t)
    parser.get_lexer().set_error(t->file_start, t->line_start, t->col_start, msg);
  else
    parser.get_lexer().set_error(msg);
}

static void shaderWarn(Parser &parser, const char *msg, Terminal *t)
{
  G_ASSERT(t);
  parser.get_lexer().set_warning(t->file_start, t->line_start, t->col_start, msg);
}

static bool validate_cs(const ShaderClass &sclass, ShaderSemCode *ssc, int staticVariantCount)
{
  bool has_cs = false, has_non_cs = false, has_void = false;
  for (int m = 0; m < ssc->passes.size(); m++)
    if (!ssc->passes[m])
      has_void = true;
    else
    {
      if (ssc->passes[m]->pass)
      {
        if (ssc->passes[m]->pass->cs.size())
          has_cs = true;
        if (ssc->passes[m]->pass->vpr.size() || ssc->passes[m]->pass->fsh.size())
          has_non_cs = true;
        if (!ssc->passes[m]->pass->fsh.size() && !ssc->passes[m]->pass->vpr.size() && !ssc->passes[m]->pass->cs.size())
          has_void = true;
      }
    }

  if (!has_cs)
    return true;

  // validate compute shader case
  int stvar_cnt = 0;
  for (int m = 0; m < ssc->vars.size(); m++)
    if (!ssc->vars[m].dynamic)
      stvar_cnt++;

  if (stvar_cnt)
    sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders shall not have static variables (%d found)", sclass.name.c_str(), stvar_cnt);
  if (staticVariantCount > 1)
    sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders shall not have static variants (%d found)", sclass.name.c_str(),
      staticVariantCount);
  if (has_non_cs)
    sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders shall not mixed with other shaders", sclass.name.c_str());

  // strip stblk code
  if (ErrorCounter::curShader().err)
    return false;

  for (int m = 0; m < ssc->passes.size(); m++)
  {
    if (!ssc->passes[m])
      continue;
    const int *cod = ssc->passes[m]->pass->stblkcode.data(), *cod_e = cod + ssc->passes[m]->pass->stblkcode.size();
    int unexp_stcod = -1;

    for (const int *codp = cod; codp < cod_e && unexp_stcod < 0; codp++)
      switch (shaderopcode::getOp(*codp))
      {
        case SHCOD_IMM_REAL1:
          if (codp + 1 < cod_e)
            unexp_stcod = codp - cod;
          break;

        default: unexp_stcod = codp - cod;
      }

    if (unexp_stcod >= 0)
    {
      sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders contains unexpected STBLK at %d (code dumped to log)", sclass.name.c_str(),
        unexp_stcod);
      ShUtils::shcod_dump(dag::ConstSpan<int>(cod, cod_e - cod));
      break;
    }
    ssc->passes[m]->pass->stblkcode.reset();
  }

  return ErrorCounter::curShader().err == 0;
}

static void add_shader(shader_decl *sh, Parser &parser, Terminal *shname, shc::TargetContext &ctx)
{
  if (!shc::isShaderRequired(shname->text))
    return;

  sh_set_current_shader(shname->text);
  DEFER([] { sh_set_current_shader(nullptr); });

  shc::ShaderContext shContext = ctx.makeShaderContext(shname->text, ShaderBlockLevel::SHADER, shname);
  ShaderClass &sclass = shContext.compiledShader();

  if (ErrorCounter::curShader().err > 0)
  {
    sh_dump_warn_info();
    sh_debug(SHLOG_ERROR, "shader(%s): semantics not processed: syntax errors found!", shname->text);
    return;
  }

  sh_debug(SHLOG_INFO, "shader(%s): process semantics", shname->text);

  semantic::initialize_debug_mode(shContext);

  // gather static variant vars
  parser.get_lexer().begin_shader();
  GatherVarShaderEvalCB stVarCB{shContext};

  eval_shader(*sh, stVarCB);
  parser.get_lexer().end_shader();

  // set globals ShaderParser::curVsCode etc. for lifetime of this function
  class LocalSourceBlocks
  {
  public:
    LocalSourceBlocks()
    {
      for (HlslCompilationStage stage : HLSL_ALL_LIST)
        curHlslBlks.all[stage] = &blks.all[stage];
    }
    ~LocalSourceBlocks()
    {
      shc::await_all_jobs();
      curHlslBlks = {};
    }

    PerHlslStage<CodeSourceBlocks> blks{};
  };
  LocalSourceBlocks lsb;

  // parse code blocks
  if (!semantic::parse_hlsl_source_to_blocks(curHlslBlks, stVarCB, shContext))
  {
    sh_debug(SHLOG_ERROR, "cannot parse shader code in <%s>", shname->text);
    return;
  }

  const shc::TypeTables &types = shContext.typeTables();
  const IntervalList &intervals = shContext.intervals();

  // fill static variants table
  ShaderVariant::VariantTableSrc staticVariants{ctx};
  staticVariants.generateFromTypes(types.allStaticTypes, intervals, true);

  int staticVariantCount = staticVariants.getVarCount();
  G_ASSERT(staticVariantCount);

  sh_debug(SHLOG_INFO, "%d interval(s) found", staticVariants.getIntervals().getCount());
  sh_debug(SHLOG_INFO, "%d static variants", staticVariantCount);

  String stats;
  stats.resize(2048);
  get_memory_stats(stats, 2048);
  debug("%s", stats.str());

  struct VarUsageInfo
  {
    bool used;
    bool noWarnings;
    Terminal *terminal;

    void fromVar(const ShaderSemCode::Var &var)
    {
      terminal = (Terminal *)var.terminal;
      used = var.used;
      noWarnings = var.noWarnings;
    }
  };

  // const char * is ok cause we use ids from VarMap inside it's lifetime
  ska::flat_hash_map<const char *, VarUsageInfo> usageInfo{};
  auto addVarUsage = [&usageInfo, &ctx](const ShaderSemCode::Var &var) {
    auto [it, wasNew] = usageInfo.insert({get_semcode_var_name(var, ctx), VarUsageInfo{}});
    G_ASSERT(wasNew);
    it->second.fromVar(var);
  };

  if (shContext.isDebugModeEnabled())
  {
    const ShaderMessages &messages = shContext.messages();
    const auto &table = messages.strings;
    for (int i = 0; i < table.nameCount(); i++)
    {
      if (messages.isFilenameMessage(i))
        sclass.messages.emplace_back(table.getName(i));
      else
        sclass.messages.emplace_back(string_f("%s: %s", shname->text, table.getName(i)));
    }
  }

  Tab<eastl::unique_ptr<ShaderSemCode>> semcode{};
  Tab<int> shInitCodeLast{};

  // for each static variant:
  int i;
  bool has_first_static_variant = false;
  shc::prepareTestVariantShader(shname->text);
  shc::prepareTestVariant(&staticVariants.getTypes(), NULL);
  for (i = 0; i < staticVariantCount; ++i)
  {
    sclass.shInitCode.clear();

    ShaderVariant::VariantSrc *staticVariant = staticVariants.getVariant(i);
    if (!shc::isValidVariant(staticVariant, NULL))
    {
      staticVariant->codeId = shc::shouldMarkInvalidAsNull() ? -1 : -2;
      continue;
    }

    sh_set_current_variant(staticVariant);

    // empty class for storage
    eastl::unique_ptr<ShaderSemCode> ssc{};
    int render_stage_idx = -1;
    int passCount = 0;
    SemanticShaderPass *lastParsedPass = nullptr;

    usageInfo.clear();

    auto addVariantEvaluationResult = [&](EvaluatedShaderVariant &&evaluated_variant, int variant_id, int variant_cnt) {
      G_ASSERT(variant_id >= 0 && variant_id < variant_cnt);

      auto &[parsedSsc, parsedPass] = evaluated_variant;

      if (render_stage_idx < 0)
        render_stage_idx = (parsedSsc.flags & SC_STAGE_IDX_MASK);
      else if ((parsedSsc.flags & SC_STAGE_IDX_MASK) != render_stage_idx)
        sh_debug(SHLOG_WARNING, "shader(%s): different renderStageIdx used in dynvariants: %d and %d", shname->text, render_stage_idx,
          (parsedSsc.flags & SC_STAGE_IDX_MASK));

      if (!passCount)
      {
        for (const ShaderSemCode::Var &var : parsedSsc.vars)
          addVarUsage(var);
        ssc = eastl::make_unique<ShaderSemCode>(eastl::move(parsedSsc));
        ssc->passes.resize(variant_cnt);
      }
      else
      {
        // check for variable usage
        for (const ShaderSemCode::Var &var : parsedSsc.vars)
        {
          if (auto it = usageInfo.find(get_semcode_var_name(var, ctx)); it != usageInfo.end())
          {
            if (!it->second.used)
              it->second.used = var.used;
          }
          else
            addVarUsage(var);
        }

        // correct register count
        if (ssc->regsize != parsedSsc.regsize)
        {
          ssc->regsize = parsedSsc.regsize = (ssc->regsize > parsedSsc.regsize) ? ssc->regsize : parsedSsc.regsize;
        }

        if (ssc->passes[variant_id])
        {
          sh_debug(SHLOG_ERROR, "shader(%s): dynamic variant already exists", shname->text);
          return;
        }

        // check for equal static variants if this is not the first added ssc
        const char *eqDesc = ssc->equal(parsedSsc, false);
        if (passCount && eqDesc != nullptr)
        {
          sh_debug(SHLOG_ERROR, "shader(%s): illegal dynamic variants: different %s", shname->text, eqDesc);
          return;
        }
      }

      ssc->passes[variant_id] = eastl::move(parsedPass);
      lastParsedPass = &ssc->passes[variant_id]->pass.value();
      if (!passCount)
        ssc->initPassMap(variant_id);
      else
        ssc->mergeVars(eastl::move(parsedSsc.vars), eastl::move(parsedSsc.stvarmap), variant_id);

      ++passCount;
    };

    ShaderVariant::VariantTableSrc dynamicVariants{ctx};
    dynamicVariants.generateFromTypes(types.allDynamicTypes, intervals, false);

    if (types.allDynamicTypes.getCount())
    {
      // dynamic variants present

      int dynamicVariantCount = dynamicVariants.getVarCount();
      shc::prepareTestVariant(NULL, &dynamicVariants.getTypes());
      FastIntList invalidVariants;

      // for each dynamic variant:
      for (int d = 0; d < dynamicVariantCount; d++)
      {
        if (!shc::isValidVariant(staticVariant, dynamicVariants.getVariant(d)))
        {
          if (!shc::shouldMarkInvalidAsNull())
            invalidVariants.addInt(d);
          continue;
        }

        ShaderVariant::VariantSrc *dynamicVariant = dynamicVariants.getVariant(d);
        sh_set_current_dyn_variant(dynamicVariant);

        auto evaluatedVariantMaybe =
          evalShaderVariant(sh, parser, ShaderVariant::VariantInfo(*staticVariant, dynamicVariant), shContext);

        if (evaluatedVariantMaybe)
        {
          dynamicVariant->codeId = d;
          addVariantEvaluationResult(eastl::move(*evaluatedVariantMaybe), d, dynamicVariantCount);
        }

        sh_set_current_dyn_variant(nullptr);
      }

      if (invalidVariants.getList().size() == dynamicVariantCount)
        staticVariant->codeId = shc::shouldMarkInvalidAsNull() ? -1 : -2;
      else if (invalidVariants.getList().size())
      {
        for (int i = 0; i < invalidVariants.getList().size(); i++)
          dynamicVariants.getVariant(invalidVariants.getList()[i])->codeId = -2; // @TODO: use markInvalidAsNull here too?
      }
    }
    else
    {
      // no dynamic variants present
      auto parsedSscMaybe = evalShaderVariant(sh, parser, ShaderVariant::VariantInfo{*staticVariant, nullptr}, shContext);
      if (parsedSscMaybe)
        addVariantEvaluationResult(eastl::move(*parsedSscMaybe), 0, 1);
      else
        staticVariant->codeId = -1;
    }

    if (passCount)
    {
      // check for variable usage
      eastl::for_each(usageInfo.cbegin(), usageInfo.cend(), [&](const auto &pair) {
        auto [name, var] = pair;
        if (!var.used && !var.noWarnings)
        {
          if (strcmp("instancing_const_begin", name) != 0 && strcmp("instancing_const_end", name) != 0)
            shaderWarn(parser,
              "variable '" + String{name} + "' not used in static variant '" + staticVariant->getVarStringInfo() + "'", var.terminal);
        }
      });

      if ((ssc->flags & SC_STAGE_IDX_MASK) != render_stage_idx)
        sh_debug(SHLOG_WARNING, "shader(%s): staticvariant uses renderStageIdx=%d, while dynvariants uses %d", shContext.name(),
          (ssc->flags & SC_STAGE_IDX_MASK), render_stage_idx);

      // syncpoint for issued compile jobs
      shc::await_all_jobs(&sh_process_errors);
      sh_process_errors();
      ctx.bytecodeCache().resolvePendingPasses();

      if (!validate_cs(sclass, ssc.get(), staticVariantCount))
        break;

      // find the same shader code variant, add if not found
      int j;
      for (j = 0; j < semcode.size(); ++j)
        if (semcode[j]->equal(*ssc, true) == NULL)
          break;

      if (j >= semcode.size())
      {
        j = semcode.size();
        ShaderCode *sc = ssc->generateShaderCode(dynamicVariants);

        if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
        {
          if (passCount > 1)
          {
            shc::VariantContext variantCtx =
              shContext.makeVariantContext(ShaderVariant::VariantInfo{*staticVariant, nullptr}, *ssc, nullptr, true);

            auto &cppcode = variantCtx.cppStcode();

            StcodeBranchedBuildEvalCB cb{variantCtx};

            parser.get_lexer().begin_shader();
            eval_shader(*sh, cb);
            parser.get_lexer().end_shader();

            assembly::build_cpp_declarations_for_used_bool_vars(variantCtx);
            assembly::build_cpp_declarations_for_used_local_vars(variantCtx);

            // If the code is flat, just use base variant without dynamic elements
            if (!cppcode.cppStcode.hasCodeUnderConditions)
            {
              sc->branchedCppStcodeId = ctx.cppStcode().addCode(eastl::move(lastParsedPass->cppstcode.cppStcode),
                lastParsedPass->psOrCsConstRange, lastParsedPass->vsConstRange);
            }

            if (sc->branchedCppStcodeId == -1)
            {
              auto passRegsTable = build_pass_stcode_reg_table(*ssc);
              auto data = build_pass_reg_tables_for_branched_dynstcode(passRegsTable, cppcode.cppStcode);

              for (auto &&[pass, table] : data.passRegisterTables)
              {
                if (sc->branchedCppStcodeId == -1)
                  pass->target->branchedCppStcodeTableOffset = ctx.cppStcode().addRegisterTableWithIndex(eastl::move(table));
              }

              if (sc->branchedCppStcodeId == -1)
              {
                sc->branchedCppStcodeId =
                  ctx.cppStcode().addCode(eastl::move(cppcode.cppStcode), data.commonPsOrCsConstRange, data.commonVsConstRange);
              }
            }
          }
          else
          {
            sc->branchedCppStcodeId = ctx.cppStcode().addCode(eastl::move(lastParsedPass->cppstcode.cppStcode),
              lastParsedPass->psOrCsConstRange, lastParsedPass->vsConstRange);
          }
        }

        for (auto &pt : ssc->passes)
          if (pt && pt->pass)
            pt->pass->clearCppStcodeData();

        semcode.push_back(eastl::move(ssc));
        sclass.code.push_back(sc);
      }

      staticVariant->codeId = j;
      if (has_first_static_variant)
        if (shInitCodeLast.size() != sclass.shInitCode.size() || !mem_eq(shInitCodeLast, sclass.shInitCode.data()))
        {
          debug("shInitCode(%d):", sclass.shInitCode.size() / 2);
          for (int k = 0; k < sclass.shInitCode.size(); k += 2)
            debug("  %d <- %d", sclass.shInitCode[k], sclass.shInitCode[k + 1]);

          debug("shInitCodeLast(%d):", shInitCodeLast.size() / 2);
          for (int k = 0; k < shInitCodeLast.size(); k += 2)
            debug("  %d <- %d", shInitCodeLast[k], shInitCodeLast[k + 1]);

          sh_debug(SHLOG_ERROR,
            "shader(%s): init code for textures used in static branching"
            " differs between static variants",
            shContext.name());
        }
      shInitCodeLast = sclass.shInitCode;
      has_first_static_variant = true;
    }
  }

  if (shContext.isDebugModeEnabled())
    for (int i = 0; i < sclass.uniqueStrings.nameCount(); i++)
      sclass.messages.emplace_back(sclass.uniqueStrings.getName(i));

  shc::prepareTestVariantShader(NULL);
  if (has_first_static_variant)
    sclass.shInitCode = shInitCodeLast;
  else
    clear_and_shrink(sclass.assumedIntervals);

  sclass.sortStaticVarsByMode();

  staticVariants.fillVariantTable(sclass.staticVariants);

  sh_set_current_variant(NULL);
  sh_set_current_dyn_variant(NULL);

  sh_debug(SHLOG_INFO, "%d code variants", sclass.code.size());

  if (ErrorCounter::curShader().isEmpty())
  {
    sh_debug(SHLOG_INFO, "%d static/dynamic vars\n", sclass.stvar.size());
  }
  else
  {
    sh_debug(SHLOG_INFO, "%d static/dynamic vars", sclass.stvar.size());
    ErrorCounter::curShader().dumpInfo();
  }

  // If compiled successfully, move to heap and add to global shader classes list
  add_shader_class(shContext.releaseCompiledShader(), ctx);

  ErrorCounter::curShader().reset();
}


void add_shader(shader_decl *sh, Parser &parser, shc::TargetContext &ctx)
{
  if (!sh)
    return;

  for (int i = 0; i < sh->name.size(); ++i)
    add_shader(sh, parser, sh->name[i], ctx);
}

static void eval_block_stat(block_stat *s, auto &cb, Parser &parser);

static void eval_block_if(block_if &s, auto &cb, Parser &parser)
{
  G_ASSERT(s.expr);
  ShVarBool eTrue = cb.eval_expr(*s.expr);
  bool isTrue = eTrue.value, isFalse = !eTrue.value;
  if (!eTrue.isConst)
  {
    parser.get_lexer().set_error(s.expr->file_start, s.expr->line_start, s.expr->col_start,
      "not constant expression for if's in shader block");
    isTrue = isFalse = false;
  }
  if (isTrue)
  {
    for (int i = 0; i < s.true_stat.size(); ++i)
      eval_block_stat(s.true_stat[i], cb, parser);
  }
  if (isFalse)
  {
    if (s.false_stat.size() || s.else_if)
    {
      for (int i = 0; i < s.false_stat.size(); ++i)
        eval_block_stat(s.false_stat[i], cb, parser);
      if (s.else_if)
        eval_block_if(*s.else_if, cb, parser);
    }
  }
}

static void eval_block_stat(block_stat &s, auto &cb, Parser &parser)
{
  if (s.external_block)
    cb.eval_external_block(*s.external_block);
  else if (s.imm_const_block)
    cb.eval(*s.imm_const_block);
  else if (s.if_stat)
    eval_block_if(*s.if_stat, cb, parser);
  else if (s.supports)
    cb.eval_supports(*s.supports);
  else if (s.local_decl)
    cb.eval_shader_locdecl(*s.local_decl);
  else if (s.boolean_decl)
    cb.eval_bool_decl(*s.boolean_decl);
  else
  {
    G_ASSERT(0 && "not implemented");
  }
}

static void eval_block_stat(block_stat *s, auto &cb, Parser &parser)
{
  if (s)
    eval_block_stat(*s, cb, parser);
}

static void eval_block(block_decl &bl, auto &cb, Parser &parser)
{
  for (int i = 0; i < bl.stat.size(); ++i)
    eval_block_stat(bl.stat[i], cb, parser);
}

void add_block(block_decl *bl, Parser &parser, shc::TargetContext &ctx)
{
#define REPORT_ERR(msg, t) parser.get_lexer().set_error(t->file_start, t->line_start, t->col_start, msg)

  if (!bl)
    return;

  ShaderBlockLevel level = ShaderBlockLevel::UNDEFINED;
  if (strcmp(bl->block_scope->text, "global_const") == 0)
    level = ShaderBlockLevel::GLOBAL_CONST;
  else if (strcmp(bl->block_scope->text, "frame") == 0)
    level = ShaderBlockLevel::FRAME;
  else if (strcmp(bl->block_scope->text, "scene") == 0)
    level = ShaderBlockLevel::SCENE;
  else if (strcmp(bl->block_scope->text, "object") == 0)
    level = ShaderBlockLevel::OBJECT;
  else
  {
    REPORT_ERR("invalid block scope", bl->block_scope);
    return;
  }

  if (ctx.blocks().findBlock(bl->name->text))
  {
    REPORT_ERR("invalid redeclaration of block", bl->name);
    return;
  }

  IntervalList intervals;
  ShaderVariant::TypeTable nullType{ctx};
  nullType.setIntervalList(&intervals);
  ShaderVariant::VariantSrc stVariant(nullType);
  ShaderVariant::VariantInfo variant(stVariant, NULL);

  ShaderSemCode targetSsc{ctx};

  shc::ShaderContext shaderBlockCtx = ctx.makeShaderContext(bl->name->text, level, bl->name);
  shc::VariantContext variantCtx = shaderBlockCtx.makeVariantContext(variant, targetSsc);

  // evaluate shader
  AssembleShaderEvalCB cb{variantCtx};

  parser.get_lexer().begin_block();
  eval_block(*bl, cb, parser);
  parser.get_lexer().end_block();

  cb.compilePreshader();

  const auto &bytecode = variantCtx.stBytecode();

  Tab<int> stcode{};
  append_items(stcode, bytecode.stblkcode.size(), bytecode.stblkcode.data());
  append_items(stcode, bytecode.stcode.size(), bytecode.stcode.data());

  assembly::build_cpp_declarations_for_used_local_vars(variantCtx);

  DynamicStcodeRoutine collectedScript = eastl::move(variantCtx.cppStcode().cppStcode);
  collectedScript.merge(eastl::move((StcodeRoutine &)variantCtx.cppStcode().cppStblkcode));

  ShaderStateBlock *blk = new ShaderStateBlock(bl->name->text, level, eastl::move(variantCtx.namedConstTable()), make_span(stcode),
    &collectedScript, bytecode.regAllocator->requiredRegCount(), ctx);

  if (!ctx.blocks().registerBlock(blk))
  {
    REPORT_ERR("cannot add block", bl->name);
    delete blk;
    return;
  }
}

void add_hlsl(hlsl_global_decl_class &sh, Parser &parser, shc::TargetContext &ctx)
{
  if (!semantic::validate_hardcoded_regs_in_hlsl_block(sh.text))
    return;

  hlsl_mask_t hlsl_types = HLSL_FLAGS_ALL;
  if (sh.ident)
  {
    HlslCompilationStage stage = profile_to_hlsl_stage(sh.ident->text);
    if (stage == HLSL_INVALID)
      REPORT_ERR(String(0, "Unexpected scope %s", sh.ident->text).c_str(), sh.ident);

    hlsl_types = HLSL_ALL_FLAGS_LIST[stage];
  }

  if (hlsl_types & HLSL_FLAGS_VS)
    hlsl_types |= HLSL_FLAGS_DS | HLSL_FLAGS_GS | HLSL_FLAGS_HS;

  if (hlsl_types & HLSL_FLAGS_AS)
    hlsl_types |= HLSL_FLAGS_MS;

  ctx.globHlslSrc().forEach(hlsl_types, [&](String &src) { addSourceCode(src, sh.text, parser, ctx.globMessages(), true); });
}
#undef REPORT_ERR

void add_global_bool(ShaderTerminal::bool_decl &bool_var, Parser &parser, shc::TargetContext &ctx)
{
  ctx.globBoolVars().add(bool_var, parser);

  String hlsl_bool_var(0, "##bool %s\n", bool_var.name->text);
  ctx.globHlslSrc().forEach([&](String &src) { src.append(hlsl_bool_var); });
}
}; // namespace ShaderParser

static void build_condition_string(bool_expr &e, String &out);

static void build_condition_value(bool_value &e, String &out)
{
  const char *left = NULL, *op = NULL, *right = NULL;

  if (e.cmpop)
    switch (e.cmpop->op->num)
    {
      case SHADER_TOKENS::SHTOK_eq: op = "=="; break;

      case SHADER_TOKENS::SHTOK_noteq: op = "!="; break;
      case SHADER_TOKENS::SHTOK_greater: op = ">"; break;
      case SHADER_TOKENS::SHTOK_greatereq:
        op = ">=";
        ;
        break;
      case SHADER_TOKENS::SHTOK_smaller:
        op = "<";
        ;
        break;
      case SHADER_TOKENS::SHTOK_smallereq:
        op = "<=";
        ;
        break;

      default: debug("%p.num=%d", e.cmpop->op, e.cmpop->op->num);
    }

  if (e.two_sided)
    left = "two_sided";
  else if (e.real_two_sided)
    left = "real_two_sided";
  else if (e.shader)
  {
    left = "shader";
    right = e.shader->text;
  }
  else if (e.hw)
  {
    left = "hardware";
    op = ".";
    right = e.hw->var->text;
  }
  else if (e.interval_ident)
  {
    left = e.interval_ident->text;
    right = e.interval_value->text;
  }
  else if (e.texture_name)
  {
    left = e.texture_name->text;
    right = "NULL";
  }
  else if (e.bool_var)
  {
    left = e.bool_var->text;
  }
  else if (e.maybe)
  {
    left = "maybe(";
    op = e.maybe_bool_var->text;
    right = ")";
  }
  else if (e.true_value)
  {
    left = "true";
  }
  else if (e.false_value)
  {
    left = "false";
  }
  else
  {
    G_ASSERT(0);
  }

  if (!op)
    out.append(left);
  else
  {
    G_ASSERT(right);
    out.aprintf(64, "%s%s%s", left, op, right);
  }
}
static void build_condition_bool_not(not_expr &e, String &out)
{
  if (e.is_not)
    out.append("!");
  if (e.value->expr)
  {
    out.append("(");
    build_condition_string(*e.value->expr, out);
    out.append(")");
  }
  else
    build_condition_value(*e.value, out);
}
static void build_condition_bool_and(and_expr &e, String &out)
{
  if (e.value)
  {
    build_condition_bool_not(*e.value, out);
    return;
  }

  build_condition_bool_and(*e.a, out);
  out.append(" && ");
  build_condition_bool_not(*e.b, out);
}

static void build_condition_string(bool_expr &e, String &out)
{
  if (e.value)
  {
    build_condition_bool_and(*e.value, out);
    return;
  }

  build_condition_string(*e.a, out);
  out.append(" || ");
  build_condition_bool_and(*e.b, out);
}

void ShaderParser::build_bool_expr_string(bool_expr &e, String &out, bool clr_str)
{
  if (clr_str)
    out.clear();
  build_condition_string(e, out);
}

// report error, for plugging into generated code
void sh_report_error_cb(ShaderTerminal::shader_stat *s, GeneratedParser &parser)
{
  if (!s || !s->error_val)
    return;

  parser.get_lexer().set_error(s->error_val->file_start, s->error_val->line_start, s->error_val->col_start,
    "shader_stat is not valid!");
}
