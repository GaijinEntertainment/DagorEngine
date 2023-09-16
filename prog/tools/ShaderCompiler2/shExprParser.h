/************************************************************************
  support for shader arithemtic expressions & local variables
/************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SHEXPRPARSER_H
#define __SHEXPRPARSER_H

#include "shExpr.h"


namespace ShaderTerminal
{
class ShaderSyntaxParser;
};
struct LocalVar;


namespace ShaderParser
{
//************************************************************************
//* forwards
//************************************************************************
class AssembleShaderEvalCB;

/*********************************
 *
 * class ExpressionParser
 *
 *********************************/
class ExpressionParser
{
public:
  // ctor/dtor
  ExpressionParser();
  ~ExpressionParser();

  static ExpressionParser &getStatic();

  // set owner
  void setOwner(AssembleShaderEvalCB *init_owner);
  void setParser(ShaderTerminal::ShaderSyntaxParser *_parser) { parser = _parser; };

  // parse local variable declaration
  LocalVar *parseLocalVarDecl(ShaderTerminal::local_var_decl &decl);

  // parse expression
  bool parseExpression(ShaderTerminal::arithmetic_expr &s, ComplexExpression *e);

  // parse expression - return true, if it's constant number
  bool parseConstExpression(ShaderTerminal::arithmetic_expr &s, Color4 &ret_value);

  // parse operand
  bool parseOperand(ShaderTerminal::arithmetic_operand &s, shexpr::OperandType op_type, ComplexExpression *e);

  // parse color
  bool parseColor(ShaderTerminal::arithmetic_color &s, ColorValueExpression *e);

  // show error
  void error(const char *msg, Terminal *t);

private:
  AssembleShaderEvalCB *owner;
  ShaderTerminal::ShaderSyntaxParser *parser;

  // parse expression - * and / operators or single expression
  bool parseSubExpression(ShaderTerminal::arithmetic_expr_md &s, ComplexExpression *e);

  // parse color mask
  shexpr::ColorChannel parseColorMask(char channel) const;

  void setVar(LocalVar *var, bool is_integer, ComplexExpression *rootExpr, ShaderTerminal::SHTOK_ident *decl_name);

  static ExpressionParser *self;
}; // class ExpressionParser
//
} // namespace ShaderParser


#endif //__SHEXPRPARSER_H
