// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shMacro.h"
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
 * class ShaderMacro
 *
 *********************************/
// ctor/dtor
ShaderMacro::ShaderMacro() : textParts(midmem), variables(midmem), name(""), expandStr("") {}

ShaderMacro::~ShaderMacro() {}


// parse macro declaration (return false if failed)
bool in_macro_definition = false;

bool ShaderMacro::parseDefinition(ShaderLexParser *parser)
{
  // DBGLOG("'%s' %d", parser->get_lexeme(), parser->get_token());
  if (parser->get_token() != SHADER_TOKENS::SHTOK_ident)
  {
    parser->error("expected macro name");
    return false;
  }

  name = parser->get_lexeme();
  DBGLOG("parse definition '%s'", name.str());

  expandStr.printf(512, "expanding macro '%s' at file(%%s)line(%%d)pos(%%d),\n(declared at file(%s)line(%d)pos(%d))", name.str(),
    parser->__input_stream()->get_filename(parser->get_cur_file()), parser->get_cur_line(), parser->get_cur_column());

  // gather parameters
  // Tab<String> varList(tmpmem);
  if (!processParams(parser, variables))
    return false;
  int varCount = variables.size();

  // get macro body & find all macro variable places
  String str;
  in_macro_definition = true;
  while (true)
  {
    int token = parser->get_token();
    if (token == SHADER_TOKENS::SHTOK_ident)
    {
      if (strcmp(name, parser->get_lexeme()) == 0)
      {
        parser->error("macro recursion not allowed!");
        return false;
      }
    }

    if (token == SHADER_TOKENS::SHTOK_include || token == SHADER_TOKENS::SHTOK_include_optional)
    {
      parser->error("include in macros not allowed!");
      return false;
    }

    switch (token)
    {
      case SHADER_TOKENS::SHTOK_endmacro:
      {
        // end macro - all ok, return
        // processToken("", str, varList);
        in_macro_definition = false;
        return true;
      }
      break;
      case SHADER_TOKENS::TerminalEOF:
      {
        // EOF occuried while macro definition is parsed - error!

        // DBGLOG("!!!eof");
        if (!parser->__input_stream()->is_real_eof())
        {
          // DBGLOG("!!!eof not real");
          parser->clear_eof();
        }
        else
        {
          parser->error("expected 'endmacro' but EOF found!");
          in_macro_definition = false;
          return false;
        }
      }
      break;
      default:
      {
        // all ok - check for parameter
        // processToken(parser->get_lexeme(), str, varList);
        String varName(parser->get_lexeme());

        int varIndex = tabutils::getIndex(variables, varName);

        textParts.push_back(TokenData((varIndex >= 0) ? varIndex : -1));
        TokenData &newData = textParts.back();

        // store position data
        newData.info.lexeme = parser->get_lexeme();
        newData.info.token = token;
        newData.info.file = parser->get_cur_file();
        newData.info.line = parser->get_cur_line();
        newData.info.column = parser->get_cur_column();
      }
    }
  }

  return true;
}


// parse macro parameters. in = ([v1][, v2]..[, vn])
bool ShaderMacro::processParams(ShaderLexParser *parser, Tab<String> &var_list)
{
  DBGLOG("ShaderMacro::processParams");

  clear_and_shrink(var_list);

  if (parser->get_token() != SHADER_TOKENS::SHTOK_lpar)
  {
    parser->error("expected '('");
    return false;
  }

  String varName;
  bool needVarName = false;
  bool needBreak = false;

  while (true)
  {
    int token = parser->get_token();
    DBGLOG("token=%d %s", token, parser->get_lexeme());
    switch (token)
    {
      case SHADER_TOKENS::SHTOK_comma: needVarName = true; break;
      case SHADER_TOKENS::SHTOK_rpar: needBreak = true; break;
      case SHADER_TOKENS::SHTOK_ident:
      {
        varName = parser->get_lexeme();

        // check variable
        if (tabutils::getIndex(var_list, varName) >= 0)
        {
          parser->error(String(128, "duplicate macro parameter '%s'", varName.str()));
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
        parser->error(String(128, "illegal symbol '%s' in macro definition", parser->get_lexeme()));
        return false;
      }
    }

    if (needBreak)
      break;
  }

  if (needVarName)
  {
    parser->error("expected macro parameter");
    return false;
  }

  return true;
}

extern int get_token(ShaderParser::TokenInfo &tok, ShaderLexParser *__this);
// parse macro parameters (real). in = ([v1][, v2]..[, vn])
bool ShaderMacro::processVariables(ShaderLexParser *parser, Tab<TokenList> &var_list, int req_count)
{
  DBGLOG("ShaderMacro::processVariables '%s'", name.str());
  ShaderParser::TokenInfo tok;

  if (::get_token(tok, parser) != SHADER_TOKENS::SHTOK_lpar)
  {
    parser->error("expected '('");
    return false;
  }

  // String varName;
  bool needVarName = false;
  bool needBreak = false;
  bool isProcessed = false;
  int pairBalance = 1;

  TokenList newVar;
  while (true)
  {
    int token = ::get_token(tok, parser);
    isProcessed = false;
    DBGLOG("token=%d %s", token, parser->get_lexeme());

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
          parser->error("'(' and ')' mismatch while parsing macro actial params");
          return false;
        }
        break;
    }

    if (needBreak)
      break;

    if (!isProcessed)
    {
      newVar.push_back(TokenData(-1));
      TokenData &newData = newVar.back();
      newData.info = tok;
      /*newData.info.lexeme = parser->get_lexeme();
      newData.info.token = token;
      newData.info.file = parser->get_cur_file();
      newData.info.line = parser->get_cur_line();
      newData.info.column = parser->get_cur_column();*/

      needVarName = false;
    }
  }

  if (needVarName)
  {
    parser->error("expected macro parameter");
    return false;
  }

  if (req_count >= 0)
  {
    if (req_count != var_list.size())
    {
      parser->error(String(128, "mismatch macro params - required %d but found %d", req_count, var_list.size()));
      return false;
    }
  }

  return true;
}


// parse params, make full code text & include code in input file - return false if failed
bool ShaderMacro::expandMacro(ShaderLexParser *parser, TokenList &code)
{
  DBGLOG("ShaderMacro::expandMacro '%s'", name.str());

  // gather parameters
  Tab<TokenList> varList;
  if (!processVariables(parser, varList, variables.size()))
    return false;

  // if macro is empty, do nothing
  if (!textParts.size())
    return true;
  code.reserve(textParts.size() + varList.size() + 4 * variables.size());
  int code_start = code.size();
  Tab<String> varStrings(tmpmem);
  varStrings.resize(variables.size());
  for (int i = 0; i < variables.size(); i++)
  {
    const TokenList &varTokens = varList[i];
    for (int j = 0; j < varTokens.size(); j++)
      varStrings[i].aprintf(256, "%s", varTokens[j].info.lexeme.str());
    /*if (varTokens.size() != 1)
      continue;
    TokenData token[2];
    token[0].info = varTokens[0].info;
    token[1].info = varTokens[0].info;
    token[0].info.token = SHADER_TOKENS::SHTOK_hlsl;
    token[1].info.token = SHADER_TOKENS::SHTOK_hlsl_text;
    token[1].info.lexeme.printf(128, "\n#define param_%s %s\n", variables[i].str(), varTokens[0].info.lexeme.str());
    code.append(2, token);*/
  }
  // replace parameters with it's values
  for (int i = 0; i < textParts.size(); i++)
  {
    if (textParts[i].paramIndex != -1)
    {
      TokenList &varTokens = varList[textParts[i].paramIndex];
      for (int j = 0; j < varTokens.size(); j++)
      {
        // override place info
        varTokens[j].info.file = textParts[i].info.file;
        varTokens[j].info.line = textParts[i].info.line;
        varTokens[j].info.column = textParts[i].info.column;

        code.push_back(varTokens[j]);
      }
    }
    else
    {
      if (textParts[i].info.token == SHADER_TOKENS::SHTOK_hlsl_text)
      {
        // replace macro parameters within hlsl
        TokenData token;
        bool replaced = false;
        for (int vi = 0; vi < variables.size(); vi++)
        {
          char *start = replaced ? token.info.lexeme.str() : textParts[i].info.lexeme.str();
          char *gofrom = start;

          String newtext;
          for (bool found = false;;)
          {
            char *replaceAt = strstr(gofrom, variables[vi].str());
            if (!replaceAt)
            {
              if (found)
              {
                newtext.aprintf(256, "%s", start);
                token.info.file = textParts[i].info.file;
                token.info.line = textParts[i].info.line;
                token.info.column = textParts[i].info.column;
                token.info.lexeme = newtext;
                token.info.token = textParts[i].info.token;
                token.info.isMacro = textParts[i].info.isMacro;
                replaced = true;
              }
              break;
            }
            else
            {
              int varLength = variables[vi].length();
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
              replaceAt[0] = variables[vi].str()[0];
              gofrom = start = replaceAt + varLength;
            }
          }
        }
        if (replaced)
        {
          code.push_back(token);
        }
        else
          code.push_back(textParts[i]);
      }
      else
        code.push_back(textParts[i]);
    }
  }
  /*for (int i = 0, cnt = 0; i < variables.size(); i++,cnt++)
  {
    const TokenList& varTokens = varList[i];
    if (varTokens.size() != 1)
      continue;
    TokenData token[2];
    token[0] = code[code_start+cnt*2];
    token[1] = code[code_start+cnt*2+1];
    token[1].info.lexeme.printf(128, "\n#undef param_%s\n", variables[i].str());
    code.append(2, token);
  }*/

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

// class ShaderMacro
//


/*********************************
 *
 * namespase ShaderMacroManager
 *
 *********************************/
namespace ShaderMacroManager
{
struct MacroCode
{
  ShaderMacro *macro;
  ShaderMacro::TokenList code;
  int curToken;
  int caller_file = 0;
  int caller_line = 0;

  MacroCode(ShaderMacro *m, ShaderMacro::TokenList &init_code);

  // return next token or false, if finished
  bool getToken(ShaderParser::TokenInfo &out_info);
};

static Tab<eastl::unique_ptr<ShaderMacro>> objList(midmem_ptr());
static Tab<MacroCode *> codeStack(midmem);

// find macro by it's name
int findMacro(const char *name);

// init compiler. shaders must be inited before.
void init() { clearMacros(); }

// clear all macro table
void clearMacros()
{
  objList.clear();
  tabutils::deleteAll(codeStack);
}


// parse new macro definition & add new macros; return false if failed
bool parseDefinition(ShaderLexParser *parser, bool optional)
{
  eastl::unique_ptr<ShaderMacro> macro = eastl::make_unique<ShaderMacro>();

  if (!macro->parseDefinition(parser))
    return false;

  int oldMacroIndex = findMacro(macro->getName());
  if (oldMacroIndex >= 0)
  {
    if (optional)
      return true;

    eastl::string message(eastl::string::CtorSprintf{}, "macro '%s' already declared in ", macro->getName().str());
    message += parser->get_symbol_location(oldMacroIndex, SymbolType::MACRO);
    parser->error(message.c_str());
    return false;
  }

  int newMacroIndex = (int)objList.size();
  objList.push_back(eastl::move(macro));
  BaseParNamespace::Terminal curSymbol(parser->get_cur_file(), parser->get_cur_line(), parser->get_cur_column(),
    parser->get_cur_file(), parser->get_cur_line(), parser->get_cur_column());
  curSymbol.text = objList.back()->getName();
  parser->register_symbol(newMacroIndex, SymbolType::MACRO, &curSymbol);

  return true;
}


// check for macros & parse it if nessesary; return false if failed
bool expandMacro(ShaderLexParser *parser, ShaderParser::TokenInfo &tok)
{
  // check for identifier
  if (tok.token != SHADER_TOKENS::SHTOK_ident)
    return true;
  int index = findMacro(tok.lexeme);
  if (index < 0)
  {
    return true;
  }


  tok.token = -1;
  ShaderMacro *macro = objList[index].get();

  ShaderMacro::TokenList code(tmpmem);
  if (!macro->expandMacro(parser, code))
    return false;

  codeStack.push_back(new MacroCode(macro, code));
  codeStack.back()->caller_file = tok.file;
  codeStack.back()->caller_line = tok.line;

  return true;
}


// find macro by it's name
int findMacro(const char *name)
{
  if (!name || !*name)
    return -1;

  for (int i = 0; i < objList.size(); i++)
  {
    if (objList[i] && strcmp(objList[i]->getName(), name) == 0)
    {
      return i;
    }
  }

  return -1;
}

// return macro description (name, file, line, column)
const String &getMacroDesc(const char *name)
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
bool getToken(ShaderParser::TokenInfo &out_info)
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


const String &getCurrentMacroDesc(ShaderLexParser *parser)
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
    // macro from normal file
    file = parser->get_cur_file();
    line = parser->get_cur_line();
    col = parser->get_cur_column();
  }

  static String fmtStr;
  fmtStr.printf(512, (codeStack.back())->macro->getDesc(), parser->__input_stream()->get_filename(file), line, col);
  return fmtStr;
}


MacroCode::MacroCode(ShaderMacro *m, ShaderMacro::TokenList &init_code) : macro(m), code(init_code), curToken(-1) {}


// return next token or false, if finished
bool MacroCode::getToken(ShaderParser::TokenInfo &out_info)
{
  curToken++;

  if (!tabutils::isCorrectIndex(code, curToken))
    return false;
  out_info = code[curToken].info;
  return true;
}
void realize() { clearMacros(); }

} // namespace ShaderMacroManager


// class ShaderMacroManager
//
