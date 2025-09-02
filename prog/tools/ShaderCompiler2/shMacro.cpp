// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shMacro.h"
#include "shlexterm.h"
#include <generic/dag_tabUtils.h>
#include "shlex.h"
#include "shsyntok.h"
#include "shLog.h"
#include "fast_isalnum.h"
#include <debug/dag_debug.h>
#include <EASTL/unique_ptr.h>

// #define DEBUG_SHADER_MACROS

#ifdef DEBUG_SHADER_MACROS
#define DBGLOG debug
#else
#define DBGLOG(...)
#endif

/*********************************
 *
 * struct ShaderMacro
 *
 *********************************/
// ctor/dtor
ShaderMacro::ShaderMacro() : textParts(midmem), variables(midmem), name(""), expandStr("") {}

// parse macro parameters. in = ([v1][, v2]..[, vn])
static bool process_macro_params(Lexer &lexer, Tab<String> &var_list)
{
  DBGLOG("ShaderMacro::processParams");

  clear_and_shrink(var_list);

  if (lexer.get_token() != SHADER_TOKENS::SHTOK_lpar)
  {
    lexer.error("expected '('");
    return false;
  }

  String varName;
  bool needVarName = false;
  bool needBreak = false;

  while (true)
  {
    int token = lexer.get_token();
    DBGLOG("token=%d %s", token, parser.get_lexeme());
    switch (token)
    {
      case SHADER_TOKENS::SHTOK_comma: needVarName = true; break;
      case SHADER_TOKENS::SHTOK_rpar: needBreak = true; break;
      case SHADER_TOKENS::SHTOK_ident:
      {
        varName = lexer.get_lexeme();

        // check variable
        if (tabutils::getIndex(var_list, varName) >= 0)
        {
          lexer.error(String(128, "duplicate macro parameter '%s'", varName.str()));
          return false;
        }

        DBGLOG("varName=%s", varName.str());
        var_list.push_back(varName);
        varName = "";
        needVarName = false;
      }
      break;
      default:
      {
        lexer.error(String(128, "illegal symbol '%s' in macro definition", lexer.get_lexeme()));
        return false;
      }
    }

    if (needBreak)
      break;
  }

  if (needVarName)
  {
    lexer.error("expected macro parameter");
    return false;
  }

  return true;
}

// parse macro declaration (return false if failed)
// in = macro <macro_id>([v1][, v2]..[, vn]) <macro_body> endmacro
bool ShaderMacroManager::parseMacroDefinition(Lexer &lexer, ShaderMacro &macro)
{
  if (lexer.get_token() != SHADER_TOKENS::SHTOK_ident)
  {
    lexer.error("expected macro name");
    return false;
  }

  macro.name = lexer.get_lexeme();
  DBGLOG("parse definition '%s'", name.str());

  macro.expandStr.printf(512, "expanding macro '%s' at file(%%s)line(%%d)pos(%%d),\n(declared at file(%s)line(%d)pos(%d))",
    macro.name.str(), lexer.__input_stream()->get_filename(lexer.get_cur_file()), lexer.get_cur_line(), lexer.get_cur_column());

  // gather parameters
  if (!process_macro_params(lexer, macro.variables))
    return false;
  int varCount = macro.variables.size();

  // get macro body & find all macro variable places
  String str;
  inMacroDefinition = true;
  while (true)
  {
    int token = lexer.get_token();
    if (token == SHADER_TOKENS::SHTOK_ident)
    {
      if (strcmp(macro.name, lexer.get_lexeme()) == 0)
      {
        lexer.error("macro recursion not allowed!");
        return false;
      }
    }

    if (token == SHADER_TOKENS::SHTOK_include || token == SHADER_TOKENS::SHTOK_include_optional)
    {
      lexer.error("include in macros not allowed!");
      return false;
    }

    switch (token)
    {
      case SHADER_TOKENS::SHTOK_endmacro:
      {
        // end macro - all ok, return
        inMacroDefinition = false;
        return true;
      }
      break;
      case SHADER_TOKENS::TerminalEOF:
      {
        // EOF occuried while macro definition is parsed - error!
        if (!lexer.__input_stream()->is_real_eof())
          lexer.clear_eof();
        else
        {
          lexer.error("expected 'endmacro' but EOF found!");
          inMacroDefinition = false;
          return false;
        }
      }
      break;
      default:
      {
        // all ok - check for parameter
        String varName(lexer.get_lexeme());

        int varIndex = tabutils::getIndex(macro.variables, varName);

        macro.textParts.push_back(ShaderMacro::TokenData((varIndex >= 0) ? varIndex : -1));
        ShaderMacro::TokenData &newData = macro.textParts.back();

        // store position data
        newData.info.lexeme = lexer.get_lexeme();
        newData.info.token = token;
        newData.info.file = lexer.get_cur_file();
        newData.info.line = lexer.get_cur_line();
        newData.info.column = lexer.get_cur_column();
      }
    }
  }

  return true;
}

extern int get_token(ShaderParser::TokenInfo &tok, Lexer &__this);

// parse macro parameters (real). in = ([v1][, v2]..[, vn])
static bool process_macro_variables(Lexer &lexer, Tab<ShaderMacro::TokenList> &var_list, int req_count, ShaderMacroManager &context)
{
  DBGLOG("ShaderMacro::processVariables '%s'", name.str());
  ShaderParser::TokenInfo tok;

  if (::get_token(tok, lexer) != SHADER_TOKENS::SHTOK_lpar)
  {
    lexer.error("expected '('");
    return false;
  }

  // String varName;
  bool needVarName = false;
  bool needBreak = false;
  bool isProcessed = false;
  int pairBalance = 1;

  ShaderMacro::TokenList newVar;
  while (true)
  {
    int token = ::get_token(tok, lexer);
    isProcessed = false;
    DBGLOG("token=%d %s", token, lexer->get_lexeme());

    switch (token)
    {
      case SHADER_TOKENS::SHTOK_comma:
        if (pairBalance == 1)
        {
          // var_list.push_back(varName);
          // varName = "";
          var_list.push_back(eastl::move(newVar));
          needVarName = true;
          isProcessed = true;
        }
        break;
      case SHADER_TOKENS::SHTOK_lpar: pairBalance++; break;
      case SHADER_TOKENS::SHTOK_rpar:
        pairBalance--;
        if (pairBalance == 0)
        {
          if (newVar.size())
          {
            var_list.push_back(eastl::move(newVar));
            isProcessed = true;
          }

          needBreak = true;
        }
        else if (pairBalance < 0)
        {
          lexer.error("'(' and ')' mismatch while parsing macro actial params");
          return false;
        }
        break;
    }

    if (needBreak)
      break;

    if (!isProcessed)
    {
      newVar.push_back(ShaderMacro::TokenData(-1));
      ShaderMacro::TokenData &newData = newVar.back();
      newData.info = tok;
      needVarName = false;
    }
  }

  if (needVarName)
  {
    lexer.error("expected macro parameter");
    return false;
  }

  if (req_count >= 0)
  {
    if (req_count != var_list.size())
    {
      lexer.error(String(128, "mismatch macro params - required %d but found %d", req_count, var_list.size()));
      return false;
    }
  }

  return true;
}


// parse params, make full code text & include code in input file - return false if failed
static bool expand_macro(Lexer &lexer, ShaderMacro::TokenList &code, ShaderMacro &macro, ShaderMacroManager &context)
{
  DBGLOG("ShaderMacro::expandMacro '%s'", name.str());

  // gather parameters
  Tab<ShaderMacro::TokenList> varList;
  if (!process_macro_variables(lexer, varList, macro.variables.size(), context))
    return false;

  // if macro is empty, do nothing
  if (!macro.textParts.size())
    return true;
  code.reserve(macro.textParts.size() + varList.size() + 4 * macro.variables.size());
  int code_start = code.size();
  Tab<String> varStrings(tmpmem);
  varStrings.resize(macro.variables.size());
  for (int i = 0; i < macro.variables.size(); i++)
  {
    const ShaderMacro::TokenList &varTokens = varList[i];
    for (int j = 0; j < varTokens.size(); j++)
      varStrings[i].aprintf(256, "%s", varTokens[j].info.lexeme.str());
  }
  // replace parameters with it's values
  const int partsCount = macro.textParts.size();
  for (int i = 0; i < partsCount; i++)
  {
    const bool nextTokenIsMacroConcatOp = i + 1 < partsCount && macro.textParts[i + 1].info.token == SHADER_TOKENS::SHTOK_macro_concat;
    if (nextTokenIsMacroConcatOp)
    {
      if (i + 2 >= partsCount)
      {
        lexer.error("Invalid macro concatenation expression. Right arg is missing.");
        return false;
      }

      const ShaderMacro::TokenData &leftToken = macro.textParts[i];
      const ShaderMacro::TokenData &rightToken = macro.textParts[i + 2];

      const char *invalidArgError =
        "Invalid macro concatenation expression. Arguments must be macro params with only single text value";

      if (leftToken.paramIndex < 0 || rightToken.paramIndex < 0)
      {
        lexer.error(invalidArgError);
        return false;
      }

      const auto getTokenReplacement = [&](const int text_idx) {
        const std::initializer_list<int> invalidTokensForConcat = {SHADER_TOKENS::SHTOK_hlsl_text, SHADER_TOKENS::SHTOK_beg,
          SHADER_TOKENS::SHTOK_end, SHADER_TOKENS::SHTOK_lpar, SHADER_TOKENS::SHTOK_rpar, SHADER_TOKENS::SHTOK_lbrk,
          SHADER_TOKENS::SHTOK_rbrk, SHADER_TOKENS::SHTOK_comma, SHADER_TOKENS::SHTOK_dot, SHADER_TOKENS::SHTOK_semi,
          SHADER_TOKENS::SHTOK_colon, SHADER_TOKENS::SHTOK_assign, SHADER_TOKENS::SHTOK_eq, SHADER_TOKENS::SHTOK_noteq,
          SHADER_TOKENS::SHTOK_not, SHADER_TOKENS::SHTOK_or, SHADER_TOKENS::SHTOK_and, SHADER_TOKENS::SHTOK_plus,
          SHADER_TOKENS::SHTOK_minus, SHADER_TOKENS::SHTOK_mul, SHADER_TOKENS::SHTOK_div, SHADER_TOKENS::SHTOK_smaller,
          SHADER_TOKENS::SHTOK_greater, SHADER_TOKENS::SHTOK_smallereq, SHADER_TOKENS::SHTOK_greatereq};

        const ShaderMacro::TokenList &replacementTokens = varList[macro.textParts[text_idx].paramIndex];
        const bool concatArgIsValid =
          replacementTokens.size() == 1 && !item_is_in(replacementTokens[0].info.token, invalidTokensForConcat);
        return concatArgIsValid ? &replacementTokens[0] : nullptr;
      };

      const ShaderMacro::TokenData *leftReplacement = getTokenReplacement(i);
      const ShaderMacro::TokenData *rightReplacement = getTokenReplacement(i + 2);
      if (!leftReplacement || !rightReplacement)
      {
        lexer.error(invalidArgError);
        return false;
      }

      ShaderMacro::TokenData newToken = leftToken;
      newToken.info.token = SHADER_TOKENS::SHTOK_ident;
      newToken.info.lexeme.printf(256, "%s%s", leftReplacement->info.lexeme.c_str(), rightReplacement->info.lexeme.c_str());

      code.push_back(eastl::move(newToken));
      i = i + 2;
      continue;
    }

    if (macro.textParts[i].paramIndex != -1)
    {
      ShaderMacro::TokenList &varTokens = varList[macro.textParts[i].paramIndex];
      for (int j = 0; j < varTokens.size(); j++)
      {
        // override place info
        varTokens[j].info.file = macro.textParts[i].info.file;
        varTokens[j].info.line = macro.textParts[i].info.line;
        varTokens[j].info.column = macro.textParts[i].info.column;

        code.push_back(varTokens[j]);
      }
    }
    else
    {
      if (macro.textParts[i].info.token == SHADER_TOKENS::SHTOK_hlsl_text)
      {
        // replace macro parameters within hlsl
        ShaderMacro::TokenData token;
        bool replaced = false;
        for (int vi = 0; vi < macro.variables.size(); vi++)
        {
          char *start = replaced ? token.info.lexeme.str() : macro.textParts[i].info.lexeme.str();
          char *gofrom = start;

          String newtext;
          for (bool found = false;;)
          {
            char *replaceAt = strstr(gofrom, macro.variables[vi].str());
            if (!replaceAt)
            {
              if (found)
              {
                newtext.aprintf(256, "%s", start);
                token.info.file = macro.textParts[i].info.file;
                token.info.line = macro.textParts[i].info.line;
                token.info.column = macro.textParts[i].info.column;
                token.info.lexeme = newtext;
                token.info.token = macro.textParts[i].info.token;
                token.info.isMacro = macro.textParts[i].info.isMacro;
                replaced = true;
              }
              break;
            }
            else
            {
              int varLength = macro.variables[vi].length();
              if (replaceAt > start)
                if (isalpha(replaceAt[-1]) || replaceAt[-1] == '#' || replaceAt[-1] == '_')
                {
                  gofrom = replaceAt + varLength;
                  continue;
                }
              if (fast_isalnum_or_(replaceAt[varLength]))
              {
                gofrom = replaceAt + varLength + 1;
                continue;
              }
              found = true;
              replaceAt[0] = 0;
              newtext.aprintf(256, "%s", start);
              newtext.insert(newtext.size() - 1, varStrings[vi].str(), varStrings[vi].length());
              replaceAt[0] = macro.variables[vi].str()[0];
              gofrom = start = replaceAt + varLength;
            }
          }
        }
        if (replaced)
        {
          code.push_back(token);
        }
        else
          code.push_back(macro.textParts[i]);
      }
      else
        code.push_back(macro.textParts[i]);
    }
  }

#if defined(DEBUG_SHADER_MACROS)
  String outs("");
  for (int i = 0; i < code.size(); i++)
  {
    outs += "|";
    outs += code[i].info.lexeme;
  }

  DBGLOG("code generated:\n-----------------\n%s\n-----------------", outs.str());
#endif

  return true;
}

// parse new macro definition & add new macros; return false if failed
bool ShaderMacroManager::parseDefinition(Lexer &lexer, bool optional)
{
  eastl::unique_ptr<ShaderMacro> macro = eastl::make_unique<ShaderMacro>();

  if (!parseMacroDefinition(lexer, *macro))
    return false;

  int macroNid = objNameMap.addNameId(macro->getName());
  if (macroNid < objList.size())
  {
    if (optional)
      return true;

    eastl::string message(eastl::string::CtorSprintf{}, "macro '%s' already declared in ", macro->getName().str());
    message += lexer.get_symbol_location(macroNid, SymbolType::MACRO);
    lexer.error(message.c_str());
    return false;
  }

  G_ASSERT(macroNid == int(objList.size()));
  objList.push_back(eastl::move(macro));
  BaseParNamespace::Terminal curSymbol(lexer.get_cur_file(), lexer.get_cur_line(), lexer.get_cur_column(), lexer.get_cur_file(),
    lexer.get_cur_line(), lexer.get_cur_column());
  curSymbol.text = objList.back()->getName();
  lexer.register_symbol(macroNid, SymbolType::MACRO, &curSymbol);

  return true;
}

// check for macros & parse it if nessesary; return false if failed
void ShaderMacroManager::tryExpandMacro(Lexer &lexer, ShaderParser::TokenInfo &tok)
{
  if (inMacroDefinition)
    return;

  // check for identifier
  if (tok.token != SHADER_TOKENS::SHTOK_ident)
    return;
  int index = findMacro(tok.lexeme);
  if (index < 0)
  {
    return;
  }

  tok.token = -1;
  ShaderMacro *macro = objList[index].get();

  ShaderMacro::TokenList code(tmpmem);
  if (!expand_macro(lexer, code, *macro, *this))
    return;


  codeStack.push_back(new MacroCode(macro, code));
  codeStack.back()->caller_file = tok.file;
  codeStack.back()->caller_line = tok.line;
}


// find macro by it's name
int ShaderMacroManager::findMacro(const char *name) const
{
  if (DAGOR_UNLIKELY(!name || !*name))
    return -1;

  return objNameMap.getNameId(name);
}

// return macro description (name, file, line, column)
const String &ShaderMacroManager::getMacroDesc(const char *name) const
{
  int index = findMacro(name);
  if (!tabutils::isCorrectIndex(objList, index))
  {
    static const String EMPTY("<unregistered macro>");
    return EMPTY;
  }

  return objList[index]->getDesc();
}

// return next token or false, if finished
bool ShaderMacroManager::getToken(ShaderParser::TokenInfo &out_info)
{
  if (codeStack.size())
  {
    while (codeStack.size())
    {
      MacroCode *code = codeStack.back();

      if (code->getToken(out_info))
      {
        out_info.macroCallStack.clear();
        for (auto *c : codeStack)
        {
          out_info.macroCallStack.emplace_back();
          out_info.macroCallStack.back().name = c->macro->getName().c_str();
          out_info.macroCallStack.back().file = c->caller_file;
          out_info.macroCallStack.back().line = c->caller_line;
        }
        return true;
      }

      delete code;
      codeStack.pop_back();
    }
  }

  return false;
}

const String &ShaderMacroManager::getCurrentMacroDesc(Lexer &lexer) const
{
  if (!codeStack.size())
  {
    static const String EMPTY("");
    return EMPTY;
  }

  int file, line, col;

  if (codeStack.size() > 1)
  {
    // macro from macro
    MacroCode *code = codeStack.back();
    G_ASSERT(tabutils::isCorrectIndex(code->code, code->curToken));
    ShaderMacro::TokenData &tok = code->code[code->curToken];
    file = tok.info.file;
    line = tok.info.line;
    col = tok.info.column;
  }
  else
  {
    // macrlexom normal file
    file = lexer.get_cur_file();
    line = lexer.get_cur_line();
    col = lexer.get_cur_column();
  }

  static String fmtStr;
  fmtStr.printf(512, (codeStack.back())->macro->getDesc(), lexer.__input_stream()->get_filename(file), line, col);
  return fmtStr;
}


ShaderMacroManager::MacroCode::MacroCode(ShaderMacro *m, ShaderMacro::TokenList &init_code) : macro(m), code(init_code), curToken(-1)
{}

// return next token or false, if finished
bool ShaderMacroManager::MacroCode::getToken(ShaderParser::TokenInfo &out_info)
{
  curToken++;

  if (!tabutils::isCorrectIndex(code, curToken))
    return false;
  out_info = code[curToken].info;
  return true;
}
