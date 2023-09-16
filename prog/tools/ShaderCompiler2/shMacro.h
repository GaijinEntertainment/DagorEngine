/************************************************************************
  macro support for shaders
/************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SHMACRO_H
#define __SHMACRO_H

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_string.h>
#include "parser/bparser.h"
#include "parser/base_par.h"

//************************************************************************
//* forwards
//************************************************************************
class ShaderLexParser;

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

  inline TokenInfo() : file(0), line(0), column(0), token(-1), lexeme(""), isMacro(false){};
};
} // namespace ShaderParser

/*********************************
 *
 * class ShaderMacro
 *
 *********************************/
class ShaderMacro
{
public:
  struct TokenData
  {
    int paramIndex;
    ShaderParser::TokenInfo info;

    inline TokenData(int param_index = -1) : paramIndex(param_index) {}
  };

  typedef Tab<TokenData> TokenList;

  // ctor/dtor
  ShaderMacro();
  ~ShaderMacro();

  // parse macro declaration (return false if failed)
  // in = macro <macro_id>([v1][, v2]..[, vn]) <macro_body> endmacro
  bool parseDefinition(ShaderLexParser *parser);

  // retrun macro name
  inline const String &getName() const { return name; };

  // parse params, make full code text & include code in input file - return false if failed
  // in = ([v1][, v2]..[, vn])  out = <macro_body> with replaced variables with it values v1..vN
  bool expandMacro(ShaderLexParser *parser, TokenList &code);

  // retern desc
  inline const String &getDesc() const { return expandStr; };

private:
  String name;
  String expandStr;

  // int varCount;         // variable count
  TokenList textParts; // text table
  Tab<String> variables;

  // parse macro parameters (definition). in = ([v1][, v2]..[, vn])
  bool processParams(ShaderLexParser *parser, Tab<String> &var_list);

  // parse macro parameters (real). in = ([v1][, v2]..[, vn])
  bool processVariables(ShaderLexParser *parser, Tab<TokenList> &var_list, int req_count);

  // process token. in = [any_token]
  // void processToken(const String& cur_token, String& collected_string, Tab<String>& var_list);
}; // class ShaderMacro
//


/*********************************
 *
 * namespase ShaderMacroManager
 *
 *********************************/
namespace ShaderMacroManager
{

// clear all macro table
void init();

void clearMacros();

// parse new macro definition & add new macros; return false if failed
bool parseDefinition(ShaderLexParser *parser, bool optional);

// check for macros & parse it if nessesary; return false if failed
bool expandMacro(ShaderLexParser *parser, ShaderParser::TokenInfo &token);

// return macro description (name, file, line, column)
const String &getMacroDesc(const char *name);

// return next token or false, if finished
bool getToken(ShaderParser::TokenInfo &out_info);

// return current macro decription for error message
const String &getCurrentMacroDesc(ShaderLexParser *parser);

void realize();

}; // namespace ShaderMacroManager
//

#endif //__SHMACRO_H
