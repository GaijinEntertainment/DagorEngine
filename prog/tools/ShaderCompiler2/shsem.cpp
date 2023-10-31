#include "shCompiler.h"
#include "shsem.h"
#include "shLog.h"
#include "assemblyShader.h"
#include "gatherVar.h"
#include "globVar.h"
#include "boolVar.h"
#include "codeBlocks.h"
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <generic/dag_tabUtils.h>
#include "linkShaders.h"
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>

using namespace ShaderParser;

// if defined, partial boolean expressions evaluation used
#define FAST_BOOLEAN_EVAL

static String hlsl_glob_vs, hlsl_glob_hs, hlsl_glob_ds, hlsl_glob_gs, hlsl_glob_ps, hlsl_glob_cs, hlsl_glob_ms, hlsl_glob_as;
extern bool optionalIntervalsAsBranches;

SCFastNameMap glob_string_table;

void ShaderParser::addSourceCode(String &text, Symbol *term, const char *src, ShaderSyntaxParser &p)
{
  const char *filename = p.get_lex_parser().get_input_stream()->get_filename(term->file_start);
  text.aprintf(32,
    "#line 1 \"precompiled\"\n"
    "#undef _FILE_\n"
    "#define _FILE_ %d\n"
    "#line %d \"%s\"\n"
    "%s",
    glob_string_table.addNameId(filename), term->line_start, filename, src);
}

void ShaderParser::addSourceCode(String &text, SHTOK_hlsl_text *src, ShaderSyntaxParser &p) { addSourceCode(text, src, src->text, p); }


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
    debug("  %s (%s)", v.getName(), ShUtils::shader_var_type_name(v.type));
  }

  debug("init code: %d tokens", initcode.size());
  for (int i = 0; i < initcode.size(); i += 2)
  {
    int vi = initcode[i];
    int c = initcode[i + 1];
    if (shaderopcode::getOp(c) == SHCOD_TEXTURE)
    {
      debug("  %s <- texture.%d", vars[vi].getName(), shaderopcode::getOp1p1(c));
    }
    else
      debug("  %s <- %s", vars[vi].getName(), ShUtils::shcod_tokname(shaderopcode::getOp(c)));
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

void ShaderSemCode::Pass::dump(ShaderSemCode &code)
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


bool ShaderSemCode::Pass::equal(Pass &p)
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

const char *ShaderSemCode::equal(ShaderSemCode &c, bool compare_passes)
{
  if (channel.size() != c.channel.size())
    return "channel count";
  if (!mem_eq(channel, c.channel.data()))
    return "channel data";

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

  if (regsize != c.regsize)
  {
    return "regsize";
  }

  if (initcode.size() != c.initcode.size())
    return "initcode count";
  if (!mem_eq(initcode, c.initcode.data()))
    return "initcode data";

  if (stvarmap.size() != c.stvarmap.size())
    return "stvarmap count";
  if (!mem_eq(stvarmap, c.stvarmap.data()))
    return "stvarmap data";

  if (!compare_passes)
    return NULL;

  if (passes.size() != c.passes.size())
    return "different number of dynamic variants";

  for (int di = 0; di < passes.size(); ++di)
  {
    ShaderSemCode::PassTab *thisTab = tabutils::getPtrVal(passes, di);
    ShaderSemCode::PassTab *otherTab = tabutils::getPtrVal(c.passes, di);

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
CodeSourceBlocks *curVsCode = NULL, *curHsCode = NULL, *curDsCode = NULL, *curGsCode = NULL, *curPsCode = NULL;
CodeSourceBlocks *curCsCode = NULL;
CodeSourceBlocks *curMsCode = nullptr;
CodeSourceBlocks *curAsCode = nullptr;
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
  else if (s.immediate_const_block)
    cb.eval(*s.immediate_const_block);
  else if (s.bool_decl)
    cb.eval_bool_decl(*s.bool_decl);
  else if (s.error)
    cb.eval_error_stat(*s.error);
  else
  {
    // error
    G_ASSERT(0);
    //    logerr("error\n");
  }
}

static void eval_shader_stat(shader_stat *s, ShaderEvalCB &cb)
{
  if (s)
    eval_shader_stat(*s, cb);
}

static void eval_optional_intervals_const(ShaderEvalCB &cb, bool compute)
{
  external_state_block external_block;
  external_block.scope = new SHTOK_ident;
  const IntervalList &intervals = ShaderGlobal::getIntervalList();
  for (uint32_t i = 0; i < intervals.getCount(); ++i)
  {
    const Interval *interval = intervals.getInterval(i);
    if (!interval->isOptional())
      continue;
    external_block.state_block_stat.emplace_back(new state_block_stat);
    auto &block = *external_block.state_block_stat.back();
    block.var = new external_variable;
    block.var->var = new external_var_name;
    block.var->var->name = new Terminal;
    block.var->var->name->text = str_dup(interval->getNameStr(), BaseParNamespace::symbolsmem);
    block.var->var->nameSpace = new SHTOK_type_ident;
    block.var->var->nameSpace->text = "@f1";
    block.var->val = new external_var_value_single;
    block.var->val->expr = new arithmetic_expr;
    block.var->val->expr->lhs = new arithmetic_expr_md;
    block.var->val->expr->lhs->lhs = new arithmetic_operand;
    block.var->val->expr->lhs->lhs->var_name = new SHTOK_ident;
    block.var->val->expr->lhs->lhs->var_name->text = str_dup(interval->getNameStr(), BaseParNamespace::symbolsmem);
  }
  if (compute)
  {
    external_block.scope->text = "cs";
    cb.eval_external_block(external_block);
  }
  else
  {
    external_block.scope->text = "vs";
    cb.eval_external_block(external_block);
    external_block.scope->text = "ps";
    cb.eval_external_block(external_block);
  }
}

static bool is_compute(hlsl_compile_class &hlsl_compile)
{
  return dd_stricmp(hlsl_compile.profile->text, "target_cs") == 0 || dd_stricmp(hlsl_compile.profile->text, "cs_5_0") == 0;
}

void eval_shader(shader_decl &sh, ShaderEvalCB &cb)
{
  if (optionalIntervalsAsBranches)
  {
    bool isCompute = false;
    for (int i = 0; i < sh.stat.size(); ++i)
    {
      if (sh.stat[i]->hlsl_compile_var)
        isCompute = is_compute(*sh.stat[i]->hlsl_compile_var);
    }

    eval_optional_intervals_const(cb, isCompute);
  }

  for (int i = 0; i < sh.stat.size(); ++i)
    eval_shader_stat(sh.stat[i], cb);
}

// ³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³//

static ShVarBool eval_shader_bool_not(not_expr &e, ShaderBoolEvalCB &cb)
{
  // debug("eval_shader_bool_not");
  G_ASSERT(&e);
  G_ASSERT(e.value);
  ShVarBool v;
  if (e.value->expr)
    v = eval_shader_bool(*e.value->expr, cb);
  else
    v = cb.eval_bool_value(*e.value);
  if (e.is_not)
    v = !v;
  // debug("eval_shader_bool_not returned %d %d", v.isConst, v.value);
  return v;
}

static ShVarBool eval_shader_bool_and(and_expr &e, ShaderBoolEvalCB &cb)
{
  // debug("eval_shader_bool_and");
  if (e.value)
  {
    ShVarBool v = eval_shader_bool_not(*e.value, cb);
    // debug("eval_shader_bool_and returned %d %d", v.isConst, v.value);
    return v;
  }
  // debug("const=%d v=%d", a.isConst, a.value);

  ShVarBool a = eval_shader_bool_and(*e.a, cb);
#if defined(FAST_BOOLEAN_EVAL)
  if (a.isConst && !a.value)
  {
    // debug("eval_shader_bool_and returned %d %d (truncated AND)", a.isConst, a.value);
    return a;
  }
#endif

  ShVarBool b = eval_shader_bool_not(*e.b, cb);

  // debug("eval_shader_bool_and returned %d %d", (a && b).isConst, (a && b).value);
  return a && b;
}

ShVarBool eval_shader_bool(bool_expr &e, ShaderBoolEvalCB &cb)
{
  // debug("eval_shader_bool");
  G_ASSERT(&e);
  if (e.value)
  {
    ShVarBool v = eval_shader_bool_and(*e.value, cb);
    // debug("eval_shader_bool returned %d %d", v.isConst, v.value);
    return v;
  }

  ShVarBool a = eval_shader_bool(*e.a, cb);
#if defined(FAST_BOOLEAN_EVAL)
  if (a.isConst && a.value)
  {
    // debug("eval_shader_bool returned %d %d (truncated OR)", a.isConst, a.value);
    return a;
  }
#endif

  ShVarBool b = eval_shader_bool_and(*e.b, cb);
  // debug("eval_shader_bool returned %d %d", (a || b).isConst, (a || b).value);
  return a || b;
}

// ³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³³//

extern ShHardwareOptions currentShaderOptions;

// return true, if need render
static bool evalShaderVariant(shader_decl *sh, int dyn_index, ShaderSemCode *ssc, ShaderClass *sclass, ShaderSyntaxParser &parser,
  Terminal *shname, const ShaderVariant::VariantInfo &variant, const ShaderVariant::TypeTable &allRefStaticVars)
{
  G_VERIFY(ssc->createPasses(dyn_index));

  // evaluate shader
  parser.get_lex_parser().begin_shader();
  AssembleShaderEvalCB cb(*ssc, parser, *sclass, shname, variant, currentShaderOptions, allRefStaticVars, true);
  try
  {
    eval_shader(*sh, cb);
    cb.end_eval(*sh);
  }
  catch (ShaderEvalCB::GsclStopProcessingException e)
  {
    // debug ( "variant processing stopped" );
  }
  parser.get_lex_parser().end_shader();

  if (cb.dont_render)
  {
    ssc->deletePasses(dyn_index);
    // debug("%d dont_render", i);
    //  render not needed - skip shader code selection
    return false;
  }

  return true;
}

static void shaderError(ShaderSyntaxParser &parser, const char *msg, Terminal *t)
{
  if (t)
    parser.get_lex_parser().set_error(t->file_start, t->line_start, t->col_start, msg);
  else
    parser.get_lex_parser().set_error(msg);
}

static void shaderWarn(ShaderSyntaxParser &parser, const char *msg, Terminal *t)
{
  G_ASSERT(t);
  parser.get_lex_parser().set_warning(t->file_start, t->line_start, t->col_start, msg);
}

struct VarUsageInfo
{
  String name;
  bool used;
  bool noWarnings;
  Terminal *terminal;

  void fromVar(const ShaderSemCode::Var &var)
  {
    name = var.getName();
    terminal = (Terminal *)var.terminal;
    used = var.used;
    noWarnings = var.noWarnings;
  }
};


// check for variable usage
static void checkVarsUsage(const Tab<VarUsageInfo> &info, ShaderSyntaxParser &parser, ShaderVariant::VariantSrc *staticVariant)
{
  for (int i = 0; i < info.size(); ++i)
  {
    const VarUsageInfo &var = info[i];
    if (!var.used && !var.noWarnings)
    {
      if (strcmp("instancing_const_begin", var.name) != 0 && strcmp("instancing_const_end", var.name) != 0)
        shaderWarn(parser, "variable '" + var.name + "' not used in static variant '" + staticVariant->getVarStringInfo() + "'",
          var.terminal);
    }
  }
}

static bool validate_cs(ShaderClass *sclass, ShaderSemCode *ssc, int staticVariantCount)
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
    sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders shall not have static variables (%d found)", sclass->name.c_str(), stvar_cnt);
  if (staticVariantCount > 1)
    sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders shall not have static variants (%d found)", sclass->name.c_str(),
      staticVariantCount);
  if (has_non_cs)
    sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders shall not mixed with other shaders", sclass->name.c_str());

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
      sh_debug(SHLOG_ERROR, "shader(%s): Compute shaders contains unexpected STBLK at %d (code dumped to log)", sclass->name.c_str(),
        unexp_stcod);
      ShUtils::shcod_dump(dag::ConstSpan<int>(cod, cod_e - cod));
      break;
    }
    ssc->passes[m]->pass->stblkcode.reset();
  }

  return ErrorCounter::curShader().err == 0;
}

static void add_shader(shader_decl *sh, ShaderSyntaxParser &parser, Terminal *shname)
{
  if (!shc::isShaderRequired(shname->text))
    return;
  //  debug_flush(true);
  if (ErrorCounter::curShader().err > 0)
  {
    sh_dump_warn_info();
    sh_debug(SHLOG_ERROR, "shader(%s): semantics not processed: syntax errors found!", shname->text);
    return;
  }

  sh_debug(SHLOG_INFO, "shader(%s): process semantics", shname->text);

  // gather static variant vars
  parser.get_lex_parser().begin_shader();
  GatherVarShaderEvalCB stVarCB(parser, currentShaderOptions, shname, hlsl_glob_vs, hlsl_glob_hs, hlsl_glob_ds, hlsl_glob_gs,
    hlsl_glob_ps, hlsl_glob_cs, hlsl_glob_ms, hlsl_glob_as);

  ShaderClass *sclass = new ShaderClass(shname->text);

  eval_shader(*sh, stVarCB);
  parser.get_lex_parser().end_shader();

  // set globals ShaderParser::curVsCode etc. for lifetime of this function
  class LocalSourceBlocks
  {
  public:
    LocalSourceBlocks()
    {
      curVsCode = &vsBlk;
      curHsCode = &hsBlk;
      curDsCode = &dsBlk;
      curGsCode = &gsBlk;
      curPsCode = &psBlk;
      curCsCode = &csBlk;
      curMsCode = &msBlk;
      curAsCode = &asBlk;
    }
    ~LocalSourceBlocks()
    {
      if (shc::compileJobsCount > 1)
      {
        for (int i = 0; i < shc::compileJobsCount; i++)
          cpujobs::reset_job_queue(shc::compileJobsMgrBase + i, true);
        for (int i = 0; i < shc::compileJobsCount; i++)
          while (cpujobs::is_job_manager_busy(shc::compileJobsMgrBase + i))
            sleep_msec(10);
        cpujobs::release_done_jobs();
      }
      curVsCode = curHsCode = curDsCode = curGsCode = curPsCode = curCsCode = curMsCode = curAsCode = NULL;
    }

    CodeSourceBlocks vsBlk, hsBlk, dsBlk, gsBlk, psBlk, csBlk, msBlk, asBlk;
  };
  LocalSourceBlocks lsb;

  // parse code blocks
  if (!stVarCB.end_eval(lsb.vsBlk, lsb.hsBlk, lsb.dsBlk, lsb.gsBlk, lsb.psBlk, lsb.csBlk, lsb.msBlk, lsb.asBlk))
  {
    sh_debug(SHLOG_ERROR, "cannot parse shader code in <%s>", shname->text);
    return;
  }
  sh_set_current_shader(shname->text);

  Tab<eastl::unique_ptr<ShaderSemCode>> semcode(tmpmem_ptr());
  const IntervalList &intervals = stVarCB.intervals;

  // fill static variants table
  ShaderVariant::VariantTableSrc staticVariants;
  staticVariants.generateFromTypes(stVarCB.varTypes, intervals, &currentShaderOptions, true);

  int staticVariantCount = staticVariants.getVarCount();
  G_ASSERT(staticVariantCount);

  sh_debug(SHLOG_INFO, "%d interval(s) found", staticVariants.getIntervals().getCount());
  sh_debug(SHLOG_INFO, "%d static variants", staticVariantCount);

  String stats;
  stats.resize(2048);
  get_memory_stats(stats, 2048);
  debug("%s", stats.str());

  Tab<VarUsageInfo> usageInfo(tmpmem);
  Tab<int> shInitCodeLast(tmpmem);

  float debug_mode_value = -1.0f;
  shc::getAssumedValue(debug_mode_enabled_interval, shname->text, true, debug_mode_value);
  if (debug_mode_value > 0.0f)
  {
    for (int i = 0; i < stVarCB.get_messages().nameCount(); i++)
    {
      if (stVarCB.is_filename_message(i))
        sclass->messages.emplace_back(stVarCB.get_messages().getName(i));
      else
        sclass->messages.emplace_back(eastl::string::CtorSprintf{}, "%s: %s", shname->text, stVarCB.get_messages().getName(i));
    }
  }

  // for each static variant:
  int i;
  bool has_first_static_variant = false;
  shc::prepareTestVariantShader(shname->text);
  shc::prepareTestVariant(&staticVariants.getTypes(), NULL);
  for (i = 0; i < staticVariantCount; ++i)
  {
    sclass->shInitCode.clear();

    ShaderVariant::VariantSrc *staticVariant = staticVariants.getVariant(i);
    if (!shc::isValidVariant(staticVariant, NULL))
    {
      staticVariant->codeId = shc::shouldMarkInvalidAsNull() ? -1 : -2;
      continue;
    }

    sh_set_current_variant(staticVariant);

    // empty class for storade
    eastl::unique_ptr<ShaderSemCode> ssc;

    bool needRender = false;
    int render_stage_idx = -1;

    usageInfo.clear();

    ShaderVariant::VariantTableSrc dynamicVariants;
    dynamicVariants.generateFromTypes(stVarCB.dynVarTypes, intervals, NULL, false);

    if (stVarCB.dynVarTypes.getCount())
    {
      // dynamic variants present

      int dynamicVariantCount = dynamicVariants.getVarCount();
      shc::prepareTestVariant(NULL, &dynamicVariants.getTypes());
      FastIntList invalidVariants;

      // for each dynamic variant:
      for (int d = 0; d < dynamicVariantCount; d++)
      {
        // debug("Variant %d-%d", i, d);
        if (!shc::isValidVariant(staticVariant, dynamicVariants.getVariant(d)))
        {
          if (!shc::shouldMarkInvalidAsNull())
            invalidVariants.addInt(d);
          continue;
        }

        eastl::unique_ptr<ShaderSemCode> parsedSsc = eastl::make_unique<ShaderSemCode>();
        tabutils::safeResize(parsedSsc->passes, dynamicVariantCount);

        ShaderVariant::VariantSrc *dynamicVariant = dynamicVariants.getVariant(d);
        sh_set_current_dyn_variant(dynamicVariant);

        if (evalShaderVariant(sh, d, parsedSsc.get(), sclass, parser, shname,
              ShaderVariant::VariantInfo(*staticVariant, dynamicVariant), stVarCB.allRefStaticVars))
        {
          needRender = true;
          dynamicVariant->codeId = d;
          if (render_stage_idx < 0)
            render_stage_idx = (parsedSsc->flags & SC_STAGE_IDX_MASK);
          else if ((parsedSsc->flags & SC_STAGE_IDX_MASK) != render_stage_idx)
            sh_debug(SHLOG_WARNING, "shader(%s): different renderStageIdx used in dynvariants: %d and %d", shname->text,
              render_stage_idx, (parsedSsc->flags & SC_STAGE_IDX_MASK));
        }
        else
        {
          sh_set_current_dyn_variant(NULL);
          continue;
        }

        if (!ssc)
        {
          // first dynamic variant - store it
          ssc = eastl::move(parsedSsc);

          usageInfo.resize(ssc->vars.size());
          for (int v = 0; v < usageInfo.size(); v++)
            usageInfo[v].fromVar(ssc->vars[v]);
        }
        else
        {
          // correct & check for variable usage
          if (parsedSsc->vars.size() != ssc->vars.size())
          {
            sh_debug(SHLOG_ERROR, "shader(%s): illegal dynamic variants: different variable count", shname->text);
          }
          else
          {
            for (int v = 0; v < usageInfo.size(); ++v)
            {
              ShaderSemCode::Var &var = parsedSsc->vars[v];
              VarUsageInfo &uv = usageInfo[v];

              if (!uv.used)
                uv.used = var.used;
            }
          }

          // correct register count
          if (ssc->regsize != parsedSsc->regsize)
          {
            ssc->regsize = parsedSsc->regsize = (ssc->regsize > parsedSsc->regsize) ? ssc->regsize : parsedSsc->regsize;
          }

          // check for equal static variants
          const char *eqDesc = ssc->equal(*parsedSsc, false);
          if (eqDesc != NULL)
          {
            sh_debug(SHLOG_ERROR, "shader(%s): illegal dynamic variants: different %s", shname->text, eqDesc);
            sh_set_current_dyn_variant(NULL);
            continue;
          }

          if (ssc->passes[d])
          {
            sh_debug(SHLOG_ERROR, "shader(%s): dynamic variant already exists", shname->text);
            sh_set_current_dyn_variant(NULL);
            continue;
          }

          ssc->passes[d] = parsedSsc->passes[d];
          parsedSsc->passes[d] = NULL;

          parsedSsc.reset();
        }
        sh_set_current_dyn_variant(NULL);
      }

      if (invalidVariants.getList().size() == dynamicVariantCount)
        staticVariant->codeId = shc::shouldMarkInvalidAsNull() ? -1 : -2;
      else if (invalidVariants.getList().size())
      {
        if (!ssc)
        {
          ssc = eastl::make_unique<ShaderSemCode>();
          tabutils::safeResize(ssc->passes, dynamicVariantCount);
        }
        for (int i = 0; i < invalidVariants.getList().size(); i++)
          dynamicVariants.getVariant(invalidVariants.getList()[i])->codeId = -2;
      }
    }
    else
    {
      // no dynamic variants present
      ssc = eastl::make_unique<ShaderSemCode>();
      tabutils::safeResize(ssc->passes, 1);
      needRender = evalShaderVariant(sh, 0, ssc.get(), sclass, parser, shname, ShaderVariant::VariantInfo(*staticVariant, NULL),
        stVarCB.allRefStaticVars);
      render_stage_idx = (ssc->flags & SC_STAGE_IDX_MASK);

      if (!needRender)
        staticVariant->codeId = -1;

      usageInfo.resize(ssc->vars.size());
      for (int v = 0; v < usageInfo.size(); v++)
        usageInfo[v].fromVar(ssc->vars[v]);
    }

    if (needRender)
    {
      checkVarsUsage(usageInfo, parser, staticVariant);
      if ((ssc->flags & SC_STAGE_IDX_MASK) != render_stage_idx)
        sh_debug(SHLOG_WARNING, "shader(%s): staticvariant uses renderStageIdx=%d, while dynvariants uses %d", sclass->name.c_str(),
          (ssc->flags & SC_STAGE_IDX_MASK), render_stage_idx);

      // synpoint for issued compile jobs
      if (shc::compileJobsCount > 1)
      {
        for (int i = 0; i < shc::compileJobsCount; i++)
          while (cpujobs::is_job_manager_busy(shc::compileJobsMgrBase + i))
          {
            sleep_msec(1);
            cpujobs::release_done_jobs();
            sh_process_errors();
          }
        cpujobs::release_done_jobs();
      }
      sh_process_errors();
      resolve_pending_shaders_from_cache();

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
        semcode.push_back(eastl::move(ssc));
        sclass->code.push_back(sc);
        sclass->assumedIntervals.reserve(stVarCB.assumedIntervals.size());
        for (auto &[name, value] : stVarCB.assumedIntervals)
        {
          auto &inter = sclass->assumedIntervals.emplace_back();
          inter.name = name;
          inter.value = value;
        }
        // debug("new code variant #%d", j);
      }

      staticVariant->codeId = j;
      if (has_first_static_variant)
        if (shInitCodeLast.size() != sclass->shInitCode.size() || !mem_eq(shInitCodeLast, sclass->shInitCode.data()))
        {
          debug("shInitCode(%d):", sclass->shInitCode.size() / 2);
          for (int k = 0; k < sclass->shInitCode.size(); k += 2)
            debug("  %d <- %d", sclass->shInitCode[k], sclass->shInitCode[k + 1]);

          debug("shInitCodeLast(%d):", shInitCodeLast.size() / 2);
          for (int k = 0; k < shInitCodeLast.size(); k += 2)
            debug("  %d <- %d", shInitCodeLast[k], shInitCodeLast[k + 1]);

          sh_debug(SHLOG_ERROR,
            "shader(%s): init code for textures used in static branching"
            " differs between static variants",
            sclass->name.c_str());
        }
      shInitCodeLast = sclass->shInitCode;
      has_first_static_variant = true;
    }
  }

  if (debug_mode_value > 0.0f)
  {
    for (int i = 0; i < sclass->uniqueStrings.nameCount(); i++)
    {
      sclass->messages.emplace_back(sclass->uniqueStrings.getName(i));
    }
  }

  shc::prepareTestVariantShader(NULL);
  if (has_first_static_variant)
    sclass->shInitCode = shInitCodeLast;

  staticVariants.fillVariantTable(sclass->staticVariants);

  sh_set_current_variant(NULL);
  sh_set_current_dyn_variant(NULL);

  sh_debug(SHLOG_INFO, "%d code variants", sclass->code.size());

  if (ErrorCounter::curShader().isEmpty())
  {
    sh_debug(SHLOG_INFO, "%d static/dynamic vars\n", sclass->stvar.size());
  }
  else
  {
    sh_debug(SHLOG_INFO, "%d static/dynamic vars", sclass->stvar.size());
    ErrorCounter::curShader().dumpInfo();
  }


  // add to global shader classes list
  add_shader_class(sclass);

  ErrorCounter::curShader().reset();
  sh_set_current_shader(NULL);
}


void add_shader(shader_decl *sh, ShaderSyntaxParser &parser)
{
  if (!sh)
    return;

  for (int i = 0; i < sh->name.size(); ++i)
    add_shader(sh, parser, sh->name[i]);
}

static Tab<block_stat *> curBlockStat;
static bool cacheBlockStat = false;
static void eval_block_stat(block_stat *s, AssembleShaderEvalCB &cb);

static void eval_block_if(block_if &s, AssembleShaderEvalCB &cb)
{
  G_ASSERT(s.expr);
  ShVarBool eTrue = ShaderParser::eval_shader_bool(*s.expr, cb);
  bool isTrue = eTrue.value, isFalse = !eTrue.value;
  if (!eTrue.isConst)
  {
    cb.parser.get_lex_parser().set_error(s.expr->file_start, s.expr->line_start, s.expr->col_start,
      "not constant expression for if's in shader block");
    isTrue = isFalse = true;
  }
  if (isTrue)
  {
    for (int i = 0; i < s.true_stat.size(); ++i)
      eval_block_stat(s.true_stat[i], cb);
  }
  if (isFalse)
  {
    if (s.false_stat.size() || s.else_if)
    {
      for (int i = 0; i < s.false_stat.size(); ++i)
        eval_block_stat(s.false_stat[i], cb);
      if (s.else_if)
        eval_block_if(*s.else_if, cb);
    }
  }
}

static void eval_block_stat(block_stat &s, AssembleShaderEvalCB &cb)
{
  if (s.state)
    cacheBlockStat ? (void)curBlockStat.push_back(&s) : cb.eval_state(*s.state);
  else if (s.external_block)
    cacheBlockStat ? (void)curBlockStat.push_back(&s) : cb.eval_external_block(*s.external_block);
  else if (s.immediate_const_block)
    cacheBlockStat ? (void)curBlockStat.push_back(&s) : cb.eval(*s.immediate_const_block);
  else if (s.if_stat)
    eval_block_if(*s.if_stat, cb);
  else if (s.supports)
    cb.eval_supports(*s.supports);
  else if (s.local_decl)
    cacheBlockStat ? (void)curBlockStat.push_back(&s) : cb.eval_shader_locdecl(*s.local_decl);
  else
  {
    G_ASSERT(0 && "not implemented");
  }
}

static void eval_block_stat(block_stat *s, AssembleShaderEvalCB &cb)
{
  if (s)
    eval_block_stat(*s, cb);
}

void eval_block(block_decl &bl, AssembleShaderEvalCB &cb)
{
  for (int i = 0; i < bl.stat.size(); ++i)
    eval_block_stat(bl.stat[i], cb);
}

void add_block(block_decl *bl, ShaderSyntaxParser &parser)
{
#define REPORT_ERR(msg, t) parser.get_lex_parser().set_error(t->file_start, t->line_start, t->col_start, msg)

  if (!bl)
    return;

  int level = -1;
  if (strcmp(bl->block_scope->text, "global_const") == 0)
    level = ShaderStateBlock::LEV_GLOBAL_CONST;
  else if (strcmp(bl->block_scope->text, "frame") == 0)
    level = ShaderStateBlock::LEV_FRAME;
  else if (strcmp(bl->block_scope->text, "scene") == 0)
    level = ShaderStateBlock::LEV_SCENE;
  else if (strcmp(bl->block_scope->text, "object") == 0)
    level = ShaderStateBlock::LEV_OBJECT;
  else
  {
    REPORT_ERR("invalid block scope", bl->block_scope);
    return;
  }

  if (ShaderStateBlock::findBlock(bl->name->text))
  {
    REPORT_ERR("invalid redeclaration of block", bl->name);
    return;
  }

  IntervalList intervals;
  ShaderVariant::TypeTable nullType;
  nullType.setIntervalList(&intervals);
  ShaderVariant::VariantSrc stVariant(nullType);
  ShaderVariant::VariantInfo variant(stVariant, NULL);
  ShaderClass sclass("not-shader");

  ShaderSemCode ssc;
  ssc.passes.push_back(NULL);

  G_VERIFY(ssc.createPasses(0));

  // evaluate shader
  AssembleShaderEvalCB cb(ssc, parser, sclass, bl->name, variant, currentShaderOptions, nullType, false);
  cb.shBlockLev = level;

  curBlockStat.clear();
  cacheBlockStat = true;
  eval_block(*bl, cb);

  static Tab<int> stcode(tmpmem);

  stcode.clear();
  cacheBlockStat = false;
  int cnt0 = cb.curpass->get_alt_curstcode(false).size(), cnt1 = cb.curpass->get_alt_curstcode(true).size();
  int max_reg = 0;
  if (cnt0 + cnt1)
  {
    stcode.resize(1 + cnt0 + cnt1);
    stcode[0] = shaderopcode::makeOp1(SHCOD_BLK_ICODE_LEN, cnt0 + cnt1);
    mem_copy_to(cb.curpass->get_alt_curstcode(false), &stcode[1]);
    mem_copy_to(cb.curpass->get_alt_curstcode(true), &stcode[1 + cnt0]);
  }
  cb.curpass->reset_code();
  ssc.locVars.clear();

  for (int i = 0; i < curBlockStat.size(); ++i)
    eval_block_stat(curBlockStat[i], cb);
  int st_idx = stcode.size();
  append_items(stcode, cb.curpass->get_alt_curstcode(false).size(), cb.curpass->get_alt_curstcode(false).data());
  append_items(stcode, cb.curpass->get_alt_curstcode(true).size(), cb.curpass->get_alt_curstcode(true).data());

  cb.curpass->reset_code();

  ShaderStateBlock *blk =
    new ShaderStateBlock(bl->name->text, level, cb.shConst, make_span(stcode), cb.maxregsize, cb.supportsStaticCbuf);

  if (!ShaderStateBlock::registerBlock(blk))
  {
    REPORT_ERR("cannot add block", bl->name);
    delete blk;
    return;
  }
}

void add_hlsl(hlsl_global_decl_class &sh, ShaderSyntaxParser &parser)
{
  uint32_t hlsl_types = HLSL_ALL;
  if (sh.ident)
  {
    hlsl_types = 0;
    if (strstr(sh.ident->text, "ps") != 0)
      hlsl_types |= HLSL_PS;
    else if (strstr(sh.ident->text, "vs") != 0)
      hlsl_types |= HLSL_VS;
    else if (strstr(sh.ident->text, "cs") != 0)
      hlsl_types |= HLSL_CS;
    else if (strstr(sh.ident->text, "ds") != 0)
      hlsl_types |= HLSL_DS;
    else if (strstr(sh.ident->text, "hs") != 0)
      hlsl_types |= HLSL_HS;
    else if (strstr(sh.ident->text, "gs") != 0)
      hlsl_types |= HLSL_GS;
    else if (strstr(sh.ident->text, "ms") != 0)
      hlsl_types |= HLSL_MS;
    else if (strstr(sh.ident->text, "as") != 0)
      hlsl_types |= HLSL_AS;
    else
      REPORT_ERR(String(0, "Unexpected scope %s", sh.ident->text).c_str(), sh.ident);
  }

  if (hlsl_types & HLSL_VS)
    hlsl_types |= HLSL_DS | HLSL_GS | HLSL_HS;

  if (hlsl_types & HLSL_AS)
    hlsl_types |= HLSL_MS;

  if (hlsl_types & HLSL_PS)
    addSourceCode(hlsl_glob_ps, sh.text, parser);
  if (hlsl_types & HLSL_CS)
    addSourceCode(hlsl_glob_cs, sh.text, parser);
  if (hlsl_types & HLSL_VS)
    addSourceCode(hlsl_glob_vs, sh.text, parser);
  if (hlsl_types & HLSL_GS)
    addSourceCode(hlsl_glob_gs, sh.text, parser);
  if (hlsl_types & HLSL_DS)
    addSourceCode(hlsl_glob_ds, sh.text, parser);
  if (hlsl_types & HLSL_HS)
    addSourceCode(hlsl_glob_hs, sh.text, parser);
  if (hlsl_types & HLSL_MS)
    addSourceCode(hlsl_glob_ms, sh.text, parser);
  if (hlsl_types & HLSL_AS)
    addSourceCode(hlsl_glob_as, sh.text, parser);
}
#undef REPORT_ERR

void add_global_bool(ShaderTerminal::bool_decl &bool_var, ShaderTerminal::ShaderSyntaxParser &parser)
{
  BoolVar::add(nullptr, bool_var, parser);

  String hlsl_bool_var(0, "##bool %s\n", bool_var.name->text);
  hlsl_glob_ps.append(hlsl_bool_var);
  hlsl_glob_cs.append(hlsl_bool_var);
  hlsl_glob_vs.append(hlsl_bool_var);
  hlsl_glob_gs.append(hlsl_bool_var);
  hlsl_glob_ds.append(hlsl_bool_var);
  hlsl_glob_hs.append(hlsl_bool_var);
  hlsl_glob_ms.append(hlsl_bool_var);
  hlsl_glob_as.append(hlsl_bool_var);
}

void reset()
{
  glob_string_table.clear();
  clear_and_shrink(hlsl_glob_vs);
  clear_and_shrink(hlsl_glob_hs);
  clear_and_shrink(hlsl_glob_ds);
  clear_and_shrink(hlsl_glob_gs);
  clear_and_shrink(hlsl_glob_ps);
  clear_and_shrink(hlsl_glob_cs);
  clear_and_shrink(hlsl_glob_ms);
  clear_and_shrink(hlsl_glob_as);
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

      default: debug("%p.num=%d", e.cmpop->op, e.cmpop->op->num); // G_ASSERT(0);
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

// report error
void sh_report_error(ShaderTerminal::shader_stat *s, ShaderTerminal::ShaderSyntaxParser &parser)
{
  if (!s || !s->error_val)
    return;

  parser.get_lex_parser().set_error(s->error_val->file_start, s->error_val->line_start, s->error_val->col_start,
    "shader_stat is not valid!");
}
