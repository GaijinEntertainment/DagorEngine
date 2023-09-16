#include "codeBlocks.h"
#include "condParser.h"
#include "shsem.h"
#include "shLog.h"
#include "shCompiler.h"
#include "nameMap.h"
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <ctype.h>
#include <libTools/util/strUtil.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include "globVar.h"
#include "fast_isalnum.h"
#include "hash.h"

#if !_TARGET_PC_MACOSX
#include <windows.h>
#else
#define __iscsym(_c) fast_isalnum_or_(_c)
#endif

extern SCFastNameMap glob_string_table;
SCFastNameMap CodeSourceBlocks::incFiles;

typedef CodeSourceBlocks::Unconditional ublock_t;
typedef CodeSourceBlocks::ParserContext ctx_t;
typedef CodeSourceBlocks::Fragment fragment_t;

struct CodeSourceBlocks::ParserContext
{
  ParserContext(ShaderParser::ShaderBoolEvalCB &cb, const char *s) : stack(tmpmem), evalCb(cb), stage(s) {}

  Tab<CodeSourceBlocks::Fragment> &topProg() { return *stack.back().prog; }
  int topState() { return stack.back().state; }
  ExprValue topCondEval() { return stack.back().condEval; }
  int topHasTrue() { return stack.back().has1; }
  int topHasAny() { return stack.back().hasA; }
  int stackSize() { return stack.size(); }

  void push_back(Tab<CodeSourceBlocks::Fragment> *prog, int state, ExprValue cond, int has_1, int has_any)
  {
    Stack &s = stack.push_back();
    s.prog = prog;
    s.state = state;
    s.condEval = cond;
    s.has1 = has_1;
    s.hasA = has_any;
  }
  void pop_back()
  {
    if (stack.back().condEval == ExprValue::ConstFalse)
      clear_and_shrink(*stack.back().prog);
    stack.pop_back();
  }

public:
  ShaderParser::ShaderBoolEvalCB &evalCb;
  const char *stage = nullptr;

protected:
  struct Stack
  {
    Tab<fragment_t> *prog;
    ExprValue condEval = ExprValue::NonConst;
    signed char state, has1, hasA;
  };
  Tab<Stack> stack;
};


static SCFastNameMapExIns fileNames;
static SCFastNameMapEx codeBlocks;
static SCFastNameMapEx declIdents;
static String tmpSrc, tmpFileName;
static Tab<ublock_t *> tmpUncondProg(midmem);
static Tab<short> tmpDigest(midmem);


static void addCodeBlock(ublock_t &out_b, const char *code, int fname_id, int line)
{
  out_b.codeId = codeBlocks.addNameId(code);
  G_ASSERT(out_b.codeId < 32768 && "code piece count designed to be < 32K");
  out_b.fnameId = fname_id;
  out_b.line = line;
}
static void addDeclBlock(ublock_t &out_b, const char *ident, int val)
{
  out_b.codeId = -1;
  out_b.decl.identId = declIdents.addNameId(ident);
  out_b.decl.identValue = val;
}

static void removeComments(String &s)
{
  char *p = s, *pp;
  int comment_start = -1;

  pp = strchr(p, '/');
  while (pp)
  {
    if (comment_start != -1)
    {
      if (pp[-1] == '*')
      {
        // strip C comment
        char *end = pp + 1;
        bool can_remove = false;

        for (char *t = pp; t >= &s[comment_start]; t--)
          if (t[0] == '\n')
          {
            if (!can_remove)
              memset(t + 1, ' ', end - t - 1);
            else
            {
              erase_items(s, t + 1 - s.str(), end - t - 1);
              pp -= end - t - 1;
            }
            end = t;
            can_remove = true;
          }

        if (end - s.str() > comment_start)
        {
          if (!can_remove)
            memset(&s[comment_start], ' ', end - s.str() - comment_start);
          else
          {
            erase_items(s, comment_start, end - s.str() - comment_start);
            pp -= end - s.str() - comment_start;
          }
        }

        comment_start = -1;
      }
      p = pp + 1;
    }
    else if (pp[1] == '*')
    {
      // start C comment
      comment_start = pp - s.str();
      p = pp + 3;
    }
    else if (pp[1] == '/')
    {
      // strip C++ line comment
      int start = pp - s.str();
      pp = strchr(pp + 2, '\n');
      if (!pp)
        pp = s.str() + s.length();
      erase_items(s, start, pp - &s[start]);
      p = &s[start];
    }
    else
      p = pp + 1;

    pp = strchr(p, '/');
  }
}

static void removeExtraWhitespace(String &s)
{
  char *p = s.str() + s.length() - 1, *pe = p;
  bool remove_tail_lines = true, remove_tail = true;

  while (p > s.str())
  {
    if (p[0] == '\n')
    {
      if (remove_tail)
      {
        if (!remove_tail_lines)
          safe_erase_items(s, p + 1 - s.str(), pe - p - 1);
        else
          safe_erase_items(s, p - s.str(), pe - p);
      }

      remove_tail_lines = false;
      remove_tail = true;
      pe = p;
    }
    else if (remove_tail && p[0] != ' ' && p[0] != '\t' && p[0] != '\v')
    {
      safe_erase_items(s, p + 1 - s.str(), pe - p - 1);
      remove_tail = false;
    }

    p--;
  }
}

static char *skip_white_space(char *p)
{
  while (memchr(" \n\t\v\r", *p, 5))
    p++;
  return p;
}

static void inc_lines(const char *p, const char *end_incl, int &line_no)
{
  while (p <= end_incl)
  {
    if (*p == '\n')
      line_no++;
    p++;
  }
}

static void dump_code(dag::ConstSpan<CodeSourceBlocks::Fragment> prg, int indent)
{
  String str;
  for (int i = 0; i < prg.size(); i++)
    if (prg[i].cond)
    {
      ShaderParser::build_bool_expr_string(*prg[i].cond->onIf.expr, str);
      debug("%*s##if %s", indent, "", str.str());
      dump_code(prg[i].cond->onIf.onTrue, indent + 2);

      for (int j = 0; j < prg[i].cond->onElif.size(); j++)
      {
        ShaderParser::build_bool_expr_string(*prg[i].cond->onElif[j].expr, str);
        debug("%*s##elif %s", indent, "", str.str());
        dump_code(prg[i].cond->onElif[j].onTrue, indent + 2);
      }

      if (prg[i].cond->onElse.size())
      {
        debug("%*s##else", indent, "");
        dump_code(prg[i].cond->onElse, indent + 2);
      }
      debug("%*s##endif", indent, "");
    }
    else
      debug("%*scode %s,%d [\n%s\n%*s]", indent, "", fileNames.getName(prg[i].uncond.fnameId), prg[i].uncond.line,
        codeBlocks.getName(prg[i].uncond.codeId), indent, "");
}

extern bool hlslSavePPAsComments;
extern bool optionalIntervalsAsBranches;

bool CodeSourceBlocks::parseSourceCode(const char *stage, const char *src, ShaderParser::ShaderBoolEvalCB &cb)
{
  tmpSrc = src;
  removeComments(tmpSrc);
  removeExtraWhitespace(tmpSrc);
  // debug("----- stripped code:\n%s---", tmpSrc.str());
  String start_pp, end_pp;

  // preprocessor source code to get code fragments
  char *p = tmpSrc, *src_end = tmpSrc.str() + tmpSrc.length();
  char *code_start = p, *code_end = p;
  int fname_id = -1, line_no = -1;

  ctx_t ctx(cb, stage);
  ctx.push_back(&blocks, -1, ExprValue::ConstTrue, 1, 0);

  // debug("\n----- parsing code:");
  while (p && p < src_end)
  {
    char *line_start = p;
    p = skip_white_space(p);
    if (p[0] != '#')
    {
      if (p[0] == '\0')
        break;
      p = code_end = strchr(p, '\n');
    }
    else if (p[1] != '#') // single # - standard HLSL preprocessor
    {
      char *pp_start = p;
      p = strchr(p, '\n');

      char *dtext = skip_white_space(pp_start + 1);
      if (memcmp(dtext, "line ", 5) == 0)
      {
        // process #line directive
        if (code_start < code_end)
          ppSrcCode(code_start, code_end - code_start, fname_id, line_no, ctx, NULL, NULL);

        char *t = skip_white_space(dtext + 4);
        int new_line_no = atoi(t);
        while (isdigit(*t))
          t++;

        char *t2 = t;
        t = strchr(t, '\"');
        if (t && t < p)
        {
          t2 = t ? strchr(t + 1, '\"') : NULL;
          if (!t2 || t2 > p)
          {
          err_bad_line:
            inc_lines(code_start, pp_start, line_no);
            sh_debug(SHLOG_ERROR, "bad #line directive at %s,%d:\n    %.*s", fileNames.getName(fname_id), line_no, p - pp_start,
              pp_start);
            return false;
          }

          tmpFileName.printf(260, "%.*s", t2 - t - 1, t + 1);

          char fullName[260];
#if _TARGET_PC_MACOSX
          realpath(tmpFileName, fullName);
#else
          GetFullPathName(tmpFileName, 260, fullName, NULL);
          dd_simplify_fname_c(fullName);
#endif
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2



#endif

          fname_id = fileNames.addNameId(fullName);
        }
        else
          while (t2 < p)
            if (fast_isspace(*t2))
              t2++;
            else
              goto err_bad_line;

        line_no = new_line_no;

        if (p[0] == '\n')
          p++;
        code_start = code_end = p;
      }
      else if (memcmp(dtext, "include ", 8) == 0)
      {
        char *fn_line = skip_white_space(dtext + 8);
        Tab<char> incl_code;
        char fn_delim = fn_line[0];
        if (fn_delim == '\"')
          fn_line++;
        else if (fn_delim == '<')
          fn_line++, fn_delim = '>';
        else
          fn_delim = 0;
        const char *fn_e = fn_delim ? (char *)memchr(fn_line, fn_delim, p - fn_line) : p;
        if (!fn_e)
        {
          sh_debug(SHLOG_ERROR, "bad pp include at %s,%d:\n    %.*s\n", fileNames.getName(fname_id), line_no, p - dtext + 1,
            dtext - 1);
          return false;
        }
        String fn(0, "%.*s", fn_e - fn_line, fn_line);
        for (int i = fn.size() - 2; i > 0; i--)
          if (memchr(" \n\t\v\r", fn[i], 5))
            erase_items(fn, i, 1);
        int pp_line = line_no;
        inc_lines(code_start, pp_start, pp_line);
        if (!ppDoInclude(fn, incl_code, fileNames.getName(fname_id), pp_line))
        {
          sh_debug(SHLOG_ERROR, "failed to resolve include at %s,%d:\n    %.*s\n", fileNames.getName(fname_id), line_no,
            fn_e - fn_line, fn_line);
          return false;
        }
        memcpy(dtext - 1, "//   inc", 8);
        char *old_p = tmpSrc.str();
        tmpSrc.insert(p - old_p, incl_code.data(), incl_code.size());
        size_t diff = tmpSrc.str() - old_p;
        p += diff;
        src_end += diff + incl_code.size();
        code_start += diff;
        code_end += diff;

        if (p[0] == '\n')
          p++;
        code_end = p;
      }
      code_end = p;
    }
    else // double ## - shader compiler's preprocessor
    {
      char *pp_start = p + 1;

      while (p && p < src_end)
      {
        p = strchr(p, '\n');
        if (p && p[-1] == '\\')
          p = strchr(p + 1, '\n');
        else
        {
          if (!p)
            p = src_end;
          break;
        }
      }

      char *dtext = skip_white_space(pp_start + 1);
      if (ppCheckDirective(dtext, ctx))
      {
        // directive approved as part of preprocessor
        if (hlslSavePPAsComments)
        {
          while (line_start[0] == '\n')
            line_start++;
          char *line_end = strchr(dtext, '\n');
          if (!line_end)
            line_end = dtext + strlen(dtext);
          end_pp.printf(128, "%.*s", line_end - line_start, line_start);
          if (code_start < code_end)
            ppSrcCode(code_start, code_end - code_start, fname_id, line_no, ctx, start_pp.str(), end_pp.str());
          start_pp = end_pp;
        }
        else if (code_start < code_end)
          ppSrcCode(code_start, code_end - code_start, fname_id, line_no, ctx, NULL, NULL);
        int pp_line = line_no;
        inc_lines(code_start, pp_start, pp_line);

        if (!ppDirective(pp_start, p - pp_start, dtext, fname_id, pp_line, ctx))
        {
          sh_debug(SHLOG_ERROR, "bad pp directive at %s,%d:\n    %.*s\n", fileNames.getName(fname_id), pp_line, p - pp_start,
            pp_start);
          return false;
        }


        // inc_lines(code_start, p, line_no);
        line_no = pp_line;
        inc_lines(pp_start + 1, p, line_no);

        if (p[0] == '\n')
          p++;
        code_start = code_end = p;
      }
      else
      {
        // directive is treated as part of code
        code_end = p;
        if (code_end[0] != '\n')
          code_end++;
      }
    }
  }

  if (code_start < code_end)
    ppSrcCode(code_start, code_end - code_start, fname_id, line_no, ctx, NULL, NULL);

  // debug("\n----- parsed code:");
  // dump_code(blocks, 0);
  return true;
}


void CodeSourceBlocks::ppSrcCode(char *s, int len, int fnameId, int line, ctx_t &ctx, char *start_comment, char *end_comment)
{
  if (ctx.topCondEval() == ExprValue::ConstFalse)
    return;

  char *p = s + len - 1;
  while (len > 0 && *p == '\n')
    p--, len--;
  while (len > 0 && *s == '\n')
    s++, len--, line++;

  Tab<CodeSourceBlocks::Fragment> &prg = ctx.topProg();

  char c = s[len];
  s[len] = '\0';

  String st;
  if (start_comment)
    st.printf(128 + len, "//%s\n%s\n//%s", start_comment, s, end_comment);
  addCodeBlock(prg.push_back().uncond, start_comment ? st.str() : s, fnameId, line);
  s[len] = c;

  // debug("code %s,%d [\n%.*s\n]", fileNames.getName(fnameId), line, len, s);
}

bool CodeSourceBlocks::ppCheckDirective(char *dtext, ctx_t &ctx)
{
  if (memcmp(dtext, "pragma ", 7) == 0 || memcmp(dtext, "define ", 7) == 0 || memcmp(dtext, "undef ", 6) == 0)
    return false;
  return true;
}

static bool is_optional_val(bool_value &value)
{
  if (!value.interval_ident)
    return false;
  int intervalName_id = IntervalValue::getIntervalNameId(value.interval_ident->text);
  ShaderVariant::ExtType intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(intervalName_id);
  const Interval *interv = ShaderGlobal::getIntervalList().getInterval(intervalIndex);
  return interv ? interv->isOptional() : false;
}

static bool is_optional_expr_not(not_expr &expr) { return is_optional_val(*expr.value); }

static bool is_optional_expr_and(and_expr &expr)
{
  if (expr.value)
    return is_optional_expr_not(*expr.value);
  return is_optional_expr_and(*expr.a) && is_optional_expr_not(*expr.b);
}

static bool is_optional_expr(bool_expr &expr)
{
  if (expr.value)
    return is_optional_expr_and(*expr.value);
  return is_optional_expr(*expr.a) && is_optional_expr_and(*expr.b);
}

static String interval_val_to_branch_condition(bool_value &value)
{
  if (!value.interval_ident)
    return String("(false)");
  const char *intervalName = value.interval_ident->text;
  int intervalNameId = IntervalValue::getIntervalNameId(intervalName);
  int intervalValueId = IntervalValue::getIntervalNameId(value.interval_value->text);
  ShaderVariant::ExtType intervalIndex = ShaderGlobal::getIntervalList().getIntervalIndex(intervalNameId);
  const Interval *interv = ShaderGlobal::getIntervalList().getInterval(intervalIndex);
  int valueIdx = -1;
  for (int i = 0; i < interv->getValueCount(); ++i)
    if (IntervalValue::getIntervalNameId(interv->getValueName(i)) == intervalValueId)
      valueIdx = i;
  RealValueRange range = interv->getValueRange(valueIdx);
  switch (value.cmpop->op->num)
  {
    case SHADER_TOKENS::SHTOK_eq:
      if (valueIdx == 0)
        return String(0, "(%s < %f)", intervalName, range.getMax());
      else if (valueIdx + 1 == interv->getValueCount())
        return String(0, "(%s >= %f)", intervalName, range.getMin());
      else
        return String(0, "(%s >= %f && %s < %f)", intervalName, range.getMin(), intervalName, range.getMax());
    case SHADER_TOKENS::SHTOK_noteq:
      if (valueIdx == 0)
        return String(0, "(%s >= %f)", intervalName, range.getMax());
      else if (valueIdx + 1 == interv->getValueCount())
        return String(0, "(%s < %f)", intervalName, range.getMin());
      else
        return String(0, "(%s < %f || %s >= %f)", intervalName, range.getMin(), intervalName, range.getMax());
    case SHADER_TOKENS::SHTOK_smaller: return String(0, "(%s < %f)", intervalName, range.getMin());
    case SHADER_TOKENS::SHTOK_greater: return String(0, "(%s >= %f)", intervalName, range.getMax());
    case SHADER_TOKENS::SHTOK_smallereq: return String(0, "(%s < %f)", intervalName, range.getMax());
    case SHADER_TOKENS::SHTOK_greatereq: return String(0, "(%s >= %f)", intervalName, range.getMin());
    default: return String("(false)");
  }
}

static String interval_expr_not_to_branch_condition(not_expr &expr)
{
  if (expr.is_not)
    return "(!" + interval_val_to_branch_condition(*expr.value) + ")";
  return interval_val_to_branch_condition(*expr.value);
}

static String interval_expr_and_to_branch_condition(and_expr &expr)
{
  if (expr.value)
    return interval_expr_not_to_branch_condition(*expr.value);
  return "(" + interval_expr_and_to_branch_condition(*expr.a) + " && " + interval_expr_not_to_branch_condition(*expr.b) + ")";
}

static String interval_expr_to_branch_condition(bool_expr &expr)
{
  if (expr.value)
    return interval_expr_and_to_branch_condition(*expr.value);
  return "(" + interval_expr_to_branch_condition(*expr.a) + " || " + interval_expr_and_to_branch_condition(*expr.b) + ")";
}

static bool pp_is_optional(CodeSourceBlocks::Fragment &fragment, ShaderParser::ShaderBoolEvalCB &eval_cb)
{
  if (!is_optional_expr(*fragment.cond->onIf.expr))
    return false;
  for (uint32_t i = 0; i < fragment.cond->onElif.size(); ++i)
    if (!is_optional_expr(*fragment.cond->onElif[i].expr))
      return false;
  return true;
}

static ublock_t::Decl get_first_decl(fragment_t &frag)
{
  if (!frag.cond)
    return frag.decl();
  return get_first_decl(frag.cond->onIf.onTrue[0]);
}

static void eval_optional_to_branching(Tab<CodeSourceBlocks::Fragment> &prog, ShaderParser::ShaderBoolEvalCB &eval_cb)
{
  fragment_t &frag = prog.back();
  Tab<fragment_t> blocks;

  // If statement
  ublock_t::Decl ifDecl = get_first_decl(frag);

  fragment_t &ifStatement = blocks.push_back();
  addCodeBlock(ifStatement.uncond, String(0, "BRANCH\nif %s\n{\n", interval_expr_to_branch_condition(*frag.cond->onIf.expr).c_str()),
    ifDecl.identId, ifDecl.identValue);

  for (auto &onIf : frag.cond->onIf.onTrue)
    blocks.push_back(eastl::move(onIf));

  fragment_t closeStatement;
  addCodeBlock(closeStatement.uncond, "\n}\n", ifDecl.identId, ifDecl.identValue);
  blocks.push_back(eastl::move(closeStatement));

  // Elif statements
  for (auto &onElif : frag.cond->onElif)
  {
    ublock_t::Decl elifDecl = get_first_decl(onElif.onTrue[0]);

    fragment_t &elifStatement = blocks.push_back();
    addCodeBlock(elifStatement.uncond, String(0, "else if %s\n{\n", interval_expr_to_branch_condition(*onElif.expr).c_str()),
      elifDecl.identId, elifDecl.identValue);

    for (auto &onElif2 : onElif.onTrue)
      blocks.push_back(eastl::move(onElif2));

    fragment_t closeStatement;
    addCodeBlock(closeStatement.uncond, "\n}\n", elifDecl.identId, elifDecl.identValue);
    blocks.push_back(eastl::move(closeStatement));
  }

  // Else statement
  if (frag.cond->onElse.size())
  {
    fragment_t &elseStatement = blocks.push_back();
    addCodeBlock(elseStatement.uncond, "else\n{\n", 0, 0);

    for (auto &onElse : frag.cond->onElse)
      blocks.push_back(eastl::move(onElse));

    fragment_t closeStatement;
    addCodeBlock(closeStatement.uncond, "\n}\n", 0, 0);
    blocks.push_back(eastl::move(closeStatement));
  }

  prog.pop_back();
  for (uint32_t i = 0; i < blocks.size(); ++i)
    prog.push_back(eastl::move(blocks[i]));
}

bool CodeSourceBlocks::ppDirective(char *s, int len, char *dtext, int fnameId, int line, ctx_t &ctx)
{
  // debug("preproc [#%.*s]", s+len-dtext, dtext);
  if (strncmp(dtext, "if ", 3) == 0)
  {
    int condLen = s + len - dtext - 3;
    char *condStr = dtext + 3;

    Tab<CodeSourceBlocks::Fragment> &prg = ctx.topProg();
    CodeSourceBlocks::Fragment &f = prg.push_back();

    f.cond.reset(new Condition);
    f.cond->onIf.expr = parse_pp_condition(ctx.stage, condStr, condLen, fileNames.getName(fnameId), line);
    if (!f.cond->onIf.expr)
      return false;

    ExprValue cond_eval = ctx.topCondEval();
    if (cond_eval != ExprValue::ConstFalse)
    {
      ShVarBool e = ctx.evalCb.eval_expr(*f.cond->onIf.expr);
      if (e.isConst)
        cond_eval = e.value ? ExprValue::ConstTrue : ExprValue::ConstFalse;
      else
        cond_eval = ExprValue::NonConst;
    }

    f.cond->onIf.constExprVal = cond_eval;
    ctx.push_back(&f.cond->onIf.onTrue, 0, cond_eval, cond_eval == ExprValue::ConstTrue, cond_eval == ExprValue::NonConst);
    return true;
  }
  else if (strncmp(dtext, "elif ", 5) == 0)
  {
    if (ctx.stackSize() < 2 || ctx.topState() != 0)
    {
      debug("non-paired ##elif");
      return false;
    }
    int has_true = ctx.topHasTrue(), has_any = ctx.topHasAny();
    ctx.pop_back();

    int condLen = s + len - dtext - 5;
    char *condStr = dtext + 5;

    Tab<CodeSourceBlocks::Fragment> &prg = ctx.topProg();
    G_ASSERT(prg.size() > 0);
    Condition *cond = prg.back().cond.get();
    G_ASSERT(cond);

    CodeSourceBlocks::CondIf &f = cond->onElif.push_back();
    f.expr = parse_pp_condition(ctx.stage, condStr, condLen, fileNames.getName(fnameId), line);
    if (!f.expr)
      return false;

    ExprValue cond_eval = ExprValue::ConstFalse;
    if (!has_true)
    {
      cond_eval = ctx.topCondEval();
      if (cond_eval != ExprValue::ConstFalse)
      {
        ShVarBool e = ctx.evalCb.eval_expr(*f.expr);
        if (e.isConst)
          cond_eval = e.value ? ExprValue::ConstTrue : ExprValue::ConstFalse;
        else
          cond_eval = ExprValue::NonConst;

        has_true |= (cond_eval == ExprValue::ConstTrue);
        has_any |= (cond_eval == ExprValue::NonConst);
      }
    }

    f.constExprVal = cond_eval;
    ctx.push_back(&f.onTrue, 0, cond_eval, has_true, has_any);
    return true;
  }
  else if (strncmp(dtext, "else", 4) == 0)
  {
    if (skip_white_space(dtext + 4) < s + len)
      return false;

    if (ctx.stackSize() < 2 || ctx.topState() != 0)
    {
      debug("non-paired ##else");
      return false;
    }
    int has_true = ctx.topHasTrue(), has_any = ctx.topHasAny();
    ctx.pop_back();

    Tab<CodeSourceBlocks::Fragment> &prg = ctx.topProg();
    G_ASSERT(prg.size() > 0);
    Condition *cond = prg.back().cond.get();
    G_ASSERT(cond);

    ExprValue cond_eval = ctx.topCondEval();
    if (cond_eval != ExprValue::ConstFalse)
      cond_eval = has_true ? ExprValue::ConstFalse : (has_any ? ExprValue::NonConst : ExprValue::ConstTrue);
    ctx.push_back(&cond->onElse, 1, cond_eval, has_true, 0);
    return true;
  }
  else if (strncmp(dtext, "endif", 5) == 0 && fast_isspace(dtext[5]))
  {
    if (ctx.stackSize() < 2 || ctx.topState() < 0)
    {
      debug("non-paired ##endif");
      return false;
    }
    ctx.pop_back();

    Tab<CodeSourceBlocks::Fragment> &prg = ctx.topProg();
    G_ASSERT(prg.size() > 0);
    if (optionalIntervalsAsBranches && pp_is_optional(prg.back(), ctx.evalCb))
    {
      eval_optional_to_branching(prg, ctx.evalCb);
      return true;
    }
    Condition *cond = prg.back().cond.get();
    G_ASSERT(cond);

    if (cond->onIf.constExprVal == ExprValue::ConstTrue)
      clear_and_shrink(cond->onElif);
    else
    {
      for (int i = 0; i < cond->onElif.size(); i++)
        if (cond->onElif[i].constExprVal == ExprValue::ConstTrue)
        {
          cond->onElif.resize(i + 1);
          break;
        }
        else if (cond->onElif[i].constExprVal == ExprValue::ConstFalse)
        {
          erase_items(cond->onElif, i, 1);
          i--;
        }
    }

    if (!cond->onElse.size())
    {
      bool has_elif = false;
      for (int i = cond->onElif.size() - 1; i >= 0; i--)
        if (cond->onElif[i].onTrue.size())
        {
          has_elif = true;
          break;
        }
        else
          erase_items(cond->onElif, i, 1);

      if (!cond->onIf.onTrue.size() && !has_elif)
      {
        // debug("removed empty #if/#else/#endif block");
        prg.pop_back();
      }
    }
    return true;
  }
  else if (strncmp(dtext, "bool ", 5) == 0)
  {
    char *p = skip_white_space(dtext + 5);
    if (p > s + len)
    {
      debug("ident expected in ##bool");
      return false;
    }
    char *pe = p;
    while (pe < s + len && __iscsym(*pe))
      pe++;

    String ident(p, pe - p);

    Tab<CodeSourceBlocks::Fragment> &prg = ctx.topProg();
    CodeSourceBlocks::Fragment &f = prg.push_back();

    f.uncond.codeId = -2;
    f.uncond.declBool.name = mangle_bool_var(ctx.stage, ident);
    f.uncond.declBool.expr = parse_pp_condition(nullptr, ident, ident.length(), fileNames.getName(fnameId), line);
    if (!f.uncond.declBool.expr)
      return false;
    ctx.evalCb.decl_bool_alias(f.uncond.declBool.name, *f.uncond.declBool.expr);
    return true;
  }
  else if (strncmp(dtext, "declare ", 8) == 0)
  {
    char *p = skip_white_space(dtext + 8);
    if (p > s + len)
    {
      debug("ident expected in ##declare");
      return false;
    }
    char *pe = p;
    while (pe < s + len && __iscsym(*pe))
      pe++;
    char c = *pe;
    *pe = '\0';
    addDeclBlock(ctx.topProg().push_back().uncond, p, ctx.evalCb.eval_interval_value(p));
    *pe = c;
    declCount++;
    return true;
  }
  else if (strncmp(dtext, "assert", 6) == 0)
  {
    char *par = eastl::find(dtext + 6, s + len, '(');
    if (par >= s + len)
      return false;
    char *p = par + 1;
    char *begin_expr = skip_white_space(p);
    char *begin_quote = eastl::find(begin_expr, s + len, '\"');
    bool have_message = begin_quote < s + len;
    char *end_quote = have_message ? eastl::find(begin_quote + 1, s + len, '\"') : begin_quote;
    if (have_message != (end_quote < s + len))
      return false;

    eastl::string_view expr(begin_expr, have_message ? begin_quote - begin_expr : s + len - begin_expr);
    char *end_expr = begin_expr + expr.rfind(have_message ? ',' : ')');

    String msg(0, "Assert failed in %s:%i\n\"%.*s\"%s%.*s\n", fileNames.getName(fnameId), line, end_expr - begin_expr, begin_expr,
      have_message ? "\n\n" : "", have_message ? end_quote - begin_quote - 1 : 0, begin_quote + 1);
    int msg_id = ctx.evalCb.add_message(msg);
    if (msg_id < 0)
      return true;

    debug("Added message:\n%s", msg);

    char *tail = have_message ? end_quote + 1 : end_expr;
    String code(0, "_assert(%.*s, %i%.*s", end_expr - begin_expr, begin_expr, msg_id, dtext + len - tail, tail);
    addCodeBlock(ctx.topProg().push_back().uncond, code, fnameId, line);
    return true;
  }
  return false;
}
bool CodeSourceBlocks::ppDoInclude(const char *incl_fn, Tab<char> &out_text, const char *src_fn, int src_ln)
{
  char buf[DAGOR_MAX_PATH];
  String fn(0, "%s/%s", dd_get_fname_location(buf, src_fn), incl_fn);
  dd_simplify_fname_c(fn);

  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    crd.open(incl_fn, DF_READ);
  if (!crd.fileHandle)
    if (const char *f = shc::getSrcRootFolder())
      crd.open(String(0, "%s/%s", f, incl_fn, DF_READ), DF_READ);
  if (!crd.fileHandle)
    crd.open(shc::search_include_with_pathes(incl_fn), DF_READ);
  if (!crd.fileHandle)
    return false;

  String s(0,
    "\n#undef _FILE_\n"
    "#define _FILE_ %d\n"
    "#line 1 \"%s\"\n",
    glob_string_table.addNameId(incl_fn), incl_fn);
  append_items(out_text, s.size() - 1, s.data());

  Tab<char> fcont;
  int file_sz = df_length(crd.fileHandle);
  fcont.resize(file_sz + 1);
  crd.read(fcont.data(), file_sz);
  fcont[file_sz] = 0;

  const char *ssp = fcont.data(), *sp = strchr(ssp, '#');
  while (sp)
  {
    if (sp[1] == '#')
    {
      sp++;
      if (strncmp(sp, "#include", 8) != 0)
      {
        const char *p = sp - 1;
        while (p > fcont.data() && (*p == ' ' || *p == '\t'))
          p--;
        if (*p == '\n' || *p == '\r' || p <= fcont.data())
        {
          append_items(out_text, sp - ssp, ssp);
          out_text.push_back('#');
          ssp = sp;
        }
      }
    }
    sp = strchr(sp + 1, '#');
  }
  append_items(out_text, fcont.data() + fcont.size() - 1 - ssp, ssp);

  s.printf(0,
    "\n#undef _FILE_\n"
    "#define _FILE_ %d\n"
    "#line %d \"%s\"\n",
    glob_string_table.addNameId(src_fn), src_ln, src_fn);
  append_items(out_text, s.size() - 1, s.data());

  G_ASSERT(is_main_thread());
  CodeSourceBlocks::incFiles.addNameId(crd.getTargetName());
  return true;
}


static void distill_code(dag::Span<CodeSourceBlocks::Fragment> p, ShaderParser::ShaderBoolEvalCB &c)
{
  for (int i = 0; i < p.size(); i++)
    if (p[i].cond)
    {
      if (p[i].cond->onIf.constExprVal != CodeSourceBlocks::ExprValue::ConstFalse)
        if (p[i].cond->onIf.constExprVal == CodeSourceBlocks::ExprValue::ConstTrue ||
            ShaderParser::eval_shader_bool(*p[i].cond->onIf.expr, c).value)
        {
          distill_code(make_span(p[i].cond->onIf.onTrue), c);
          continue;
        }

      bool on_elif = false;
      for (int j = 0; j < p[i].cond->onElif.size(); j++)
        if (p[i].cond->onElif[j].constExprVal == CodeSourceBlocks::ExprValue::ConstTrue ||
            ShaderParser::eval_shader_bool(*p[i].cond->onElif[j].expr, c).value)
        {
          distill_code(make_span(p[i].cond->onElif[j].onTrue), c);
          on_elif = true;
          break;
        }

      if (!on_elif)
        distill_code(make_span(p[i].cond->onElse), c);
    }
    else
    {
      if (p[i].isDecl())
        p[i].decl().identValue = c.eval_interval_value(declIdents.getName(p[i].decl().identId));
      else if (p[i].isDeclBool())
      {
        c.decl_bool_alias(p[i].declBool().name, *p[i].declBool().expr);
        continue;
      }
      tmpUncondProg.push_back(&p[i].uncond);
    }
}

dag::ConstSpan<ublock_t *> CodeSourceBlocks::getPreprocessedCode(ShaderParser::ShaderBoolEvalCB &cb)
{
  tmpUncondProg.clear();
  distill_code(make_span(blocks), cb);
  return tmpUncondProg;
}

CryptoHash CodeSourceBlocks::getCodeDigest(dag::ConstSpan<ublock_t *> code) const
{
  CryptoHasher hasher;
  static const int BUF_SZ = 1024;
  short buf[BUF_SZ];
  short *p = buf;

  for (int i = 0, e = code.size(); i < e; i++)
  {
    if (code[i]->isDecl())
    {
      *p = code[i]->decl.identId | 0x8000;
      p++;
      *p = code[i]->decl.identValue;
    }
    else
      *p = code[i]->codeId;
    p++;
    if (p - buf >= BUF_SZ - 1)
    {
      hasher.update(buf, sizeof(*p) * (p - buf));
      p = buf;
    }
  }
  hasher.update(buf, sizeof(*p) * (p - buf));

  return hasher.hash();
}

dag::ConstSpan<char> CodeSourceBlocks::buildSourceCode(dag::ConstSpan<ublock_t *> code)
{
  tmpSrc.resize(max(max(65536u, code.size() * 32), data_size(tmpSrc)));
  int printedSz = 0, szLeft = tmpSrc.size();
  for (int i = 0, e = code.size(); i < e; i++)
  {
    int sz = 0;
    if (code[i]->codeId >= 0)
    {
      sz = _snprintf(tmpSrc.data() + printedSz, szLeft, "#line %d \"%s\"\n%s\n", (int)code[i]->line,
        fileNames.getName(code[i]->fnameId), codeBlocks.getName(code[i]->codeId));
    }
    else if (code[i]->isDecl())
    {
      sz = _snprintf(tmpSrc.data() + printedSz, szLeft, "#define %s %d\n", declIdents.getName(code[i]->decl.identId),
        (int)code[i]->decl.identValue);
    }
    if (szLeft <= sz || sz == -1)
    {
      --i;
      tmpSrc.resize(tmpSrc.size() * 2);
      szLeft = tmpSrc.size() - printedSz;
    }
    else
    {
      szLeft -= sz;
      printedSz += sz;
    }
  }
  tmpSrc.resize(printedSz + 1);
  return tmpSrc;
}


void CodeSourceBlocks::resetCompilation()
{
  fileNames.reset();
  codeBlocks.reset();
  declIdents.reset();
  clear_and_shrink(tmpSrc);
  clear_and_shrink(tmpDigest);
  clear_and_shrink(tmpUncondProg);
}
