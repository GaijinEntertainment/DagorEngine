// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  macro support for shaders
/************************************************************************/

#include "parser/bparser.h"
#include "parser/base_par.h"
#include "nameMap.h"
#include <memory/dag_memBase.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_string.h>
#include <generic/dag_tabUtils.h>
#include <debug/dag_debug.h>
#include <EASTL/unique_ptr.h>

//************************************************************************
//* forwards
//************************************************************************
class Lexer;
class ShaderMacroManager;

namespace ShaderParser
{
struct TokenInfo
{
  int file;
  int line;
  int column;
  int token;
  String lexeme;
  bool isMacro;
  BaseParNamespace::Symbol::MacroCallStack macroCallStack;

  TokenInfo() : file(0), line(0), column(0), token(-1), lexeme(""), isMacro(false) {}
};
} // namespace ShaderParser

/*********************************
 *
 * struct ShaderMacro
 *
 *********************************/
struct ShaderMacro
{
  struct TokenData
  {
    int paramIndex;
    ShaderParser::TokenInfo info;

    inline TokenData(int param_index = -1) : paramIndex(param_index) {}
  };

  typedef Tab<TokenData> TokenList;

  String name;
  String expandStr;

  TokenList textParts; // text table
  Tab<String> variables;

  // ctor
  ShaderMacro();

  // retrun macro name
  inline const String &getName() const { return name; };

  // retern desc
  inline const String &getDesc() const { return expandStr; };
}; // class ShaderMacro
//


/*********************************
 *
 * class ShaderMacroManager
 *
 *********************************/
class ShaderMacroManager
{
  friend class ShaderMacro;

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

  Tab<eastl::unique_ptr<ShaderMacro>> objList{};
  FastNameMap objNameMap{};
  Tab<MacroCode *> codeStack{};

  // parse macro declaration (return false if failed)
  bool inMacroDefinition = false;

public:
  // parse new macro definition & add new macros; return false if failed
  bool parseDefinition(Lexer &parser, bool optional);

  // check for macros & parse it if nessesary
  void tryExpandMacro(Lexer &parser, ShaderParser::TokenInfo &token);

  // return macro description (name, file, line, column)
  const String &getMacroDesc(const char *name) const;

  // return next token or false, if finished
  bool getToken(ShaderParser::TokenInfo &out_info);

  // return current macro decription for error message
  const String &getCurrentMacroDesc(Lexer &lexer) const;

private:
  // find macro by it's name
  int findMacro(const char *name) const;

  // parse macro declaration (return false if failed)
  // in = macro <macro_id>([v1][, v2]..[, vn]) <macro_body> endmacro
  bool parseMacroDefinition(Lexer &lexer, ShaderMacro &macro);
};